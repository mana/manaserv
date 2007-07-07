/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#include <cassert>

#include "defines.h"
#include "point.h"
#include "game-server/accountconnection.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/item.hpp"
#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/state.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"

typedef std::map< Object *, DelayedEvent > DelayedEvents;

/**
 * List of delayed events.
 */
static DelayedEvents delayedEvents;

/**
 * Updates object states on the map.
 */
static void updateMap(MapComposite *map)
{
    // 1. update object status.
    std::vector< Thing * > const &things = map->getEverything();
    for (std::vector< Thing * >::const_iterator i = things.begin(),
         i_end = things.end(); i != i_end; ++i)
    {
        (*i)->update();
    }

    // 2. perform attacks.
    for (MovingObjectIterator i(map->getWholeMapIterator()); i; ++i)
    {
        MovingObject *o = *i;
        if (o->getUpdateFlags() & UPDATEFLAG_ATTACK)
        {
            static_cast< Being * >(o)->performAttack(map);
        }
    }

    // 3. move objects around and update zones.
    for (MovingObjectIterator i(map->getWholeMapIterator()); i; ++i)
    {
        (*i)->move();
    }

    // 4. remove dead beings.
    for (MovingObjectIterator i(map->getWholeMapIterator()); i; ++i)
    {
        if ((*i)->getUpdateFlags() & UPDATEFLAG_REMOVE)
        {
            DelayedEvent e = { EVENT_REMOVE};
            GameState::enqueueEvent((*i), e);
        }
    }

    // 5. update the map itself.
    map->update();
}

/**
 * Informs a player of what happened around the character.
 */
static void informPlayer(MapComposite *map, Character *p)
{
    MessageOut moveMsg(GPMSG_BEINGS_MOVE);
    MessageOut damageMsg(GPMSG_BEINGS_DAMAGE);
    Point pold = p->getOldPosition(), ppos = p->getPosition();
    int pid = p->getPublicID(), pflags = p->getUpdateFlags();

    // Inform client about activities of other beings near its character
    for (MovingObjectIterator i(map->getAroundCharacterIterator(p, AROUND_AREA)); i; ++i)
    {
        MovingObject *o = *i;

        Point oold = o->getOldPosition(), opos = o->getPosition();
        int otype = o->getType();
        int oid = o->getPublicID(), oflags = o->getUpdateFlags();
        int flags = 0;
        bool willBeInRange = ppos.inRangeOf(opos, AROUND_AREA);

        if (willBeInRange)
        {
            // Send attack messages.
            if ((oflags & UPDATEFLAG_ATTACK) && oid != pid)
            {
                MessageOut AttackMsg(GPMSG_BEING_ATTACK);
                AttackMsg.writeShort(oid);
                gameHandler->sendTo(p, AttackMsg);
            }

            // Send state change messages.
            if ((oflags & UPDATEFLAG_ACTIONCHANGE))
            {
                MessageOut ActionMsg(GPMSG_BEING_ACTION_CHANGE);
                ActionMsg.writeShort(oid);
                ActionMsg.writeByte(static_cast< Being * >(o)->getAction());
                gameHandler->sendTo(p, ActionMsg);
            }

            // Send leave messages of dead beings
            if ((oflags & UPDATEFLAG_REMOVE))
            {
                MessageOut leaveMsg(GPMSG_BEING_LEAVE);
                leaveMsg.writeShort(oid);
                gameHandler->sendTo(p, leaveMsg);
            }

            // Send damage messages.
            if (o->canFight())
            {
                Being *victim = static_cast< Being * >(o);
                Hits const &hits = victim->getHitsTaken();
                for (Hits::const_iterator j = hits.begin(),
                     j_end = hits.end(); j != j_end; ++j)
                {
                    damageMsg.writeShort(oid);
                    damageMsg.writeShort(*j);
                }
            }
        }

        // Check if this character and this moving object were around.
        bool wereInRange = pold.inRangeOf(oold, AROUND_AREA) &&
                           !((pflags | oflags) & UPDATEFLAG_NEW_ON_MAP);

        // Send enter/leaver messages.
        if (!wereInRange)
        {
            // o was outside p's range.
            if (!willBeInRange)
            {
                // Nothing to report: o will not be inside p's range.
                continue;
            }
            flags |= MOVING_POSITION;

            MessageOut enterMsg(GPMSG_BEING_ENTER);
            enterMsg.writeByte(otype);
            enterMsg.writeShort(oid);
            enterMsg.writeByte(static_cast< Being *>(o)->getAction());
            enterMsg.writeShort(opos.x);
            enterMsg.writeShort(opos.y);
            switch (otype) {
                case OBJECT_CHARACTER:
                {
                    Character *q = static_cast< Character * >(o);
                    enterMsg.writeString(q->getName());
                    enterMsg.writeByte(q->getHairStyle());
                    enterMsg.writeByte(q->getHairColor());
                    enterMsg.writeByte(q->getGender());
                } break;
                case OBJECT_MONSTER:
                {
                    enterMsg.writeShort(0); // TODO: The monster ID
                } break;
                default:
                    assert(false); // TODO
            }
            gameHandler->sendTo(p, enterMsg);
            continue;
        }
        else if (!willBeInRange)
        {
            // o is no longer visible from p.
            MessageOut leaveMsg(GPMSG_BEING_LEAVE);
            leaveMsg.writeShort(oid);
            gameHandler->sendTo(p, leaveMsg);
            continue;
        }
        else if (oold == opos)
        {
            // o does not move, nothing to report.
            continue;
        }

        /* At this point, either o has entered p's range, either o is
           moving inside p's range. Report o's movements. */

        Point odst = o->getDestination();
        if (opos != odst)
        {
            flags |= MOVING_POSITION;
            if (oflags & UPDATEFLAG_NEW_DESTINATION)
            {
                flags |= MOVING_DESTINATION;
            }
        }
        else
        {
            // No need to synchronize on the very last step.
            flags |= MOVING_DESTINATION;
        }

        // Send move messages.
        moveMsg.writeShort(oid);
        moveMsg.writeByte(flags);
        if (flags & MOVING_POSITION)
        {
            moveMsg.writeCoordinates(opos.x / 32, opos.y / 32);
        }
        if (flags & MOVING_DESTINATION)
        {
            moveMsg.writeShort(odst.x);
            moveMsg.writeShort(odst.y);
        }
    }

    // Do not send a packet if nothing happened in p's range.
    if (moveMsg.getLength() > 2)
        gameHandler->sendTo(p, moveMsg);

    if (damageMsg.getLength() > 2)
        gameHandler->sendTo(p, damageMsg);

    // Inform client about attribute changes of its character
    MessageOut attributeUpdateMsg(GPMSG_PLAYER_ATTRIBUTE_UPDATE);
    p->writeAttributeUpdateMessage(attributeUpdateMsg);
    if (attributeUpdateMsg.getLength() > 2)
        gameHandler->sendTo(p, attributeUpdateMsg);

    // Inform client about items on the ground around its character
    MessageOut itemMsg(GPMSG_ITEMS);
    for (FixedObjectIterator i(map->getAroundCharacterIterator(p, AROUND_AREA)); i; ++i)
    {
        assert((*i)->getType() == OBJECT_ITEM);
        Item *o = static_cast< Item * >(*i);
        Point opos = o->getPosition();
        int oflags = o->getUpdateFlags();
        bool willBeInRange = ppos.inRangeOf(opos, AROUND_AREA);
        bool wereInRange = pold.inRangeOf(opos, AROUND_AREA) &&
                           !((pflags | oflags) & UPDATEFLAG_NEW_ON_MAP);

        if (willBeInRange ^ wereInRange)
        {
            if (oflags & UPDATEFLAG_NEW_ON_MAP)
            {
                MessageOut appearMsg(GPMSG_ITEM_APPEAR);
                appearMsg.writeShort(o->getItemClass()->getDatabaseID());
                appearMsg.writeShort(opos.x);
                appearMsg.writeShort(opos.y);
                gameHandler->sendTo(p, appearMsg);
            }
            else
            {
                itemMsg.writeShort(willBeInRange ? o->getItemClass()->getDatabaseID() : 0);
                itemMsg.writeShort(opos.x);
                itemMsg.writeShort(opos.y);
            }
        }
    }

    // Do not send a packet if nothing happened in p's range.
    if (itemMsg.getLength() > 2)
        gameHandler->sendTo(p, itemMsg);
}

#ifndef NDEBUG
static bool dbgLockObjects;
#endif

void GameState::update()
{
#   ifndef NDEBUG
    dbgLockObjects = true;
#   endif

    // Update game state (update AI, etc.)
    MapManager::Maps const &maps = MapManager::getMaps();
    for (MapManager::Maps::const_iterator m = maps.begin(), m_end = maps.end(); m != m_end; ++m)
    {
        MapComposite *map = m->second;
        if (!map->isActive())
        {
            continue;
        }

        updateMap(map);

        for (CharacterIterator p(map->getWholeMapIterator()); p; ++p)
        {
            informPlayer(map, *p);
        }

        for (ObjectIterator i(map->getWholeMapIterator()); i; ++i)
        {
            Object *o = *i;
            o->clearUpdateFlags();
            if (o->canFight())
            {
                static_cast< Being * >(o)->clearHitsTaken();
            }
        }
    }

#   ifndef NDEBUG
    dbgLockObjects = false;
#   endif

    // Take care of events that were delayed because of their side effects.
    for (DelayedEvents::iterator i = delayedEvents.begin(),
         i_end = delayedEvents.end(); i != i_end; ++i)
    {
        DelayedEvent const &e = i->second;
        Object *o = i->first;
        switch (e.type)
        {
            case EVENT_REMOVE:
            {
                remove(o);
                if (o->getType() == OBJECT_CHARACTER)
                {
                    gameHandler->kill(static_cast< Character * >(o));
                }
                delete o;
            } break;

            case EVENT_INSERT:
            {
                insert(o);
            } break;

            case EVENT_WARP:
            {
                remove(o);
                Point pos(e.x, e.y);
                o->setMap(e.map);
                o->setPosition(pos);

                assert(o->getType() == OBJECT_CHARACTER);
                Character *p = static_cast< Character * >(o);
                p->clearDestination();
                /* Force update of persistent data on map change, so that
                   characters can respawn at the start of the map after a death or
                   a disconnection. */
                accountHandler->sendCharacterData(p);

                if (e.map->getMap())
                {
                    insert(o);
                }
                else
                {
                    MessageOut msg(GAMSG_REDIRECT);
                    msg.writeLong(p->getDatabaseID());
                    accountHandler->send(msg);
                    gameHandler->prepareServerChange(p);
                }
            } break;
        }
    }
    delayedEvents.clear();
}

void GameState::insert(Thing *ptr)
{
    assert(!dbgLockObjects);
    MapComposite *map = ptr->getMap();
    if (!map || !map->insert(ptr))
    {
        // TODO: Deal with failure to place Thing on the map.
        return;
    }

    if (ptr->isVisible())
    {
        Object *obj = static_cast< Object * >(ptr);
        obj->raiseUpdateFlags(UPDATEFLAG_NEW_ON_MAP);
        if (obj->getType() != OBJECT_CHARACTER) return;

        /* Since the character doesn't know yet where on the world he is after
           connecting to the map server, we send him an initial change map message. */
        MessageOut mapChangeMessage(GPMSG_PLAYER_MAP_CHANGE);
        mapChangeMessage.writeString(map->getName());
        Point pos = obj->getPosition();
        mapChangeMessage.writeShort(pos.x);
        mapChangeMessage.writeShort(pos.y);
        gameHandler->sendTo(static_cast< Character * >(obj), mapChangeMessage);
    }
}

void GameState::remove(Thing *ptr)
{
    assert(!dbgLockObjects);
    MapComposite *map = ptr->getMap();

    if (ptr->canMove())
    {
        MovingObject *obj = static_cast< MovingObject * >(ptr);
        MessageOut msg(GPMSG_BEING_LEAVE);
        msg.writeShort(obj->getPublicID());
        Point objectPos = obj->getPosition();

        for (CharacterIterator p(map->getAroundObjectIterator(obj, AROUND_AREA)); p; ++p)
        {
            if (*p != obj && objectPos.inRangeOf((*p)->getPosition(), AROUND_AREA))
            {
                gameHandler->sendTo(*p, msg);
            }
        }
    }
    else if (ptr->getType() == OBJECT_ITEM)
    {
        Item *obj = static_cast< Item * >(ptr);
        Point pos = obj->getPosition();
        MessageOut msg(GPMSG_ITEMS);
        msg.writeShort(0);
        msg.writeShort(pos.x);
        msg.writeShort(pos.y);

        for (CharacterIterator p(map->getAroundObjectIterator(obj, AROUND_AREA)); p; ++p)
        {
            if (pos.inRangeOf((*p)->getPosition(), AROUND_AREA))
            {
                gameHandler->sendTo(*p, msg);
            }
        }
    }

    map->remove(ptr);
}

void GameState::enqueueEvent(Object *ptr, DelayedEvent const &e)
{
    std::pair< DelayedEvents::iterator, bool > p =
        delayedEvents.insert(std::make_pair(ptr, e));
    // Delete events take precedence over other events.
    if (!p.second && e.type == EVENT_REMOVE)
    {
        p.first->second.type = EVENT_REMOVE;
    }
}

void GameState::sayAround(Object *obj, std::string const &text)
{
    MessageOut msg(GPMSG_SAY);
    msg.writeShort(!obj->canMove() ? 65535 :
                   static_cast< MovingObject * >(obj)->getPublicID());
    msg.writeString(text);

    Point speakerPosition = obj->getPosition();

    for (CharacterIterator i(obj->getMap()->getAroundObjectIterator(obj, AROUND_AREA)); i; ++i)
    {
        if (speakerPosition.inRangeOf((*i)->getPosition(), AROUND_AREA))
        {
            gameHandler->sendTo(*i, msg);
        }
    }
}
