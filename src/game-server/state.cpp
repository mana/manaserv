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

#include "controller.h"
#include "defines.h"
#include "map.h"
#include "point.h"
#include "game-server/accountconnection.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/state.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"

State::State()
{
    // Create 10 maggots for testing purposes
    for (int i = 0; i < 10; i++)
    {
        Being *being = new Controlled(OBJECT_MONSTER);
        being->setSpeed(150);
        being->setMapId(1);
        Point pos = { 720, 900 };
        being->setPosition(pos);
        DelayedEvent e = { EVENT_INSERT };
        enqueueEvent(being, e);
    }
}

State::~State()
{
    for (Maps::iterator i = maps.begin(), i_end = maps.end(); i != i_end; ++i)
    {
        delete i->second;
    }
}

void State::updateMap(MapComposite *map)
{
    // 1. update object status.
    for (ObjectIterator i(map->getWholeMapIterator()); i; ++i)
    {
        (*i)->update();
    }

    // 2. perform attacks.
    for (MovingObjectIterator i(map->getWholeMapIterator()); i; ++i)
    {
        MovingObject *o = *i;
        if (o->getUpdateFlags() & ATTACK)
        {
            static_cast< Being * >(o)->performAttack(map);
        }
    }

    // 3. move objects around and update zones.
    for (MovingObjectIterator i(map->getWholeMapIterator()); i; ++i)
    {
        (*i)->move();
    }
    map->update();

    // 4. Just for fun. Should be replaced by a real trigger system.
    for (MovingObjectIterator i(map->getWholeMapIterator()); i; ++i)
    {
        Object *o = *i;
        if (o->getType() != OBJECT_PLAYER) continue;
        Point pos = o->getPosition();
        int x = pos.x / 32, y = pos.y / 32;
        int m = 0;
        switch (o->getMapId())
        {
            case 1:
                if (x >= 56 && x <= 60 && y == 12)
                { m = 3; x = 44; y = 80; }
                break;
            case 3:
                if (x >= 42 && x <= 46 && y == 88)
                { m = 1; x = 58; y = 17; }
                break;
        }
        if (m != 0)
        {
            DelayedEvent e = { EVENT_WARP, m, x * 32 + 16, y * 32 + 16 };
            enqueueEvent(o, e);
        }
    }
}

void State::informPlayer(MapComposite *map, Player *p)
{
    MessageOut moveMsg(GPMSG_BEINGS_MOVE);
    MessageOut damageMsg(GPMSG_BEINGS_DAMAGE);
    Point pold = p->getOldPosition(), ppos = p->getPosition();
    int pid = p->getPublicID(), pflags = p->getUpdateFlags();

    for (MovingObjectIterator i(map->getAroundPlayerIterator(p, AROUND_AREA)); i; ++i)
    {
        MovingObject *o = *i;

        Point oold = o->getOldPosition(), opos = o->getPosition();
        int otype = o->getType();
        int oid = o->getPublicID(), oflags = o->getUpdateFlags();
        int flags = 0;

        // Send attack messages.
        if ((oflags & ATTACK) && oid != pid && ppos.inRangeOf(opos))
        {
            MessageOut AttackMsg(GPMSG_BEING_ATTACK);
            AttackMsg.writeShort(oid);
            gameHandler->sendTo(p, AttackMsg);
        }

        // Send damage messages.
        if (otype == OBJECT_PLAYER || otype == OBJECT_MONSTER)
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

        /* Check whether this player and this moving object were around
           the last time and whether they will be around the next time. */
        bool wereInRange = pold.inRangeOf(oold) &&
                           !((pflags | oflags) & NEW_ON_MAP);
        bool willBeInRange = ppos.inRangeOf(opos);

        // Send enter/leaver messages.
        if (!wereInRange)
        {
            // o was outside p's range.
            if (!willBeInRange)
            {
                // Nothing to report: o will not be inside p's range.
                continue;
            }
            flags |= MOVING_DESTINATION;

            MessageOut enterMsg(GPMSG_BEING_ENTER);
            enterMsg.writeByte(otype);
            enterMsg.writeShort(oid);
            switch (otype) {
                case OBJECT_PLAYER:
                {
                    Player *q = static_cast< Player * >(o);
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
        }
        else if (!willBeInRange)
        {
            // o is no longer visible from p.
            MessageOut leaveMsg(GPMSG_BEING_LEAVE);
            leaveMsg.writeShort(oid);
            gameHandler->sendTo(p, leaveMsg);
            continue;
        }
        else if (oold.x == opos.x && oold.y == opos.y)
        {
            // o does not move, nothing to report.
            continue;
        }

        /* At this point, either o has entered p's range, either o is
           moving inside p's range. Report o's movements. */

        Point odst = o->getDestination();
        if (opos.x != odst.x || opos.y != odst.y)
        {
            flags |= MOVING_POSITION;
            if (oflags & NEW_DESTINATION)
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
}

void State::update()
{
    // Update game state (update AI, etc.)
    for (Maps::iterator m = maps.begin(), m_end = maps.end(); m != m_end; ++m)
    {
        MapComposite *map = m->second;
        updateMap(map);

        for (PlayerIterator p(map->getWholeMapIterator()); p; ++p)
        {
            informPlayer(map, *p);
        }

        for (ObjectIterator i(map->getWholeMapIterator()); i; ++i)
        {
            Object *o = *i;
            o->clearUpdateFlags();
            int type = o->getType();
            if (type == OBJECT_PLAYER || type == OBJECT_MONSTER)
            {
                static_cast< Being * >(o)->clearHitsTaken();
            }
        }
    }

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
                removeObject(o);
                if (o->getType() == OBJECT_PLAYER)
                {
                    gameHandler->kill(static_cast< Player * >(o));
                }
                delete o;
            } break;

            case EVENT_INSERT:
            {
                insertObject(o);
            } break;

            case EVENT_WARP:
            {
                removeObject(o);
                o->setMapId(e.map);
                Point pos = { e.x, e.y };
                o->setPosition(pos);
                if (mapManager->isActive(e.map))
                {
                    insertObject(o);
                }
                else
                {
                    assert(o->getType() == OBJECT_PLAYER);
                    Player *p = static_cast< Player * >(o);
                    accountHandler->sendPlayerData(p);
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

void State::insertObject(Object *objectPtr)
{
    int mapId = objectPtr->getMapId();
    MapComposite *map = loadMap(mapId);
    if (!map || !map->insert(objectPtr))
    {
        // TODO: Deal with failure to place Object on the map.
        return;
    }
    objectPtr->raiseUpdateFlags(NEW_ON_MAP);
    if (objectPtr->getType() != OBJECT_PLAYER) return;
    Player *playerPtr = static_cast< Player * >(objectPtr);

    /* Since the player doesn't know yet where on the world he is after
     * connecting to the map server, we send him an initial change map message.
     */
    MessageOut mapChangeMessage(GPMSG_PLAYER_MAP_CHANGE);
    mapChangeMessage.writeString(mapManager->getMapName(mapId));
    Point pos = playerPtr->getPosition();
    mapChangeMessage.writeShort(pos.x);
    mapChangeMessage.writeShort(pos.y);
    mapChangeMessage.writeByte(0);
    gameHandler->sendTo(playerPtr, mapChangeMessage);
}

void State::removeObject(Object *objectPtr)
{
    int mapId = objectPtr->getMapId();
    Maps::iterator m = maps.find(mapId);
    if (m == maps.end()) return;
    MapComposite *map = m->second;

    int type = objectPtr->getType();
    if (type == OBJECT_MONSTER || type == OBJECT_PLAYER || type == OBJECT_NPC)
    {
        MovingObject *obj = static_cast< MovingObject * >(objectPtr);
        MessageOut msg(GPMSG_BEING_LEAVE);
        msg.writeShort(obj->getPublicID());
        Point objectPos = obj->getPosition();

        for (PlayerIterator p(map->getAroundObjectIterator(obj, AROUND_AREA)); p; ++p)
        {
            if (*p != obj && objectPos.inRangeOf((*p)->getPosition()))
            {
                gameHandler->sendTo(*p, msg);
            }
        }
    }

    map->remove(objectPtr);
}

void State::enqueueEvent(Object *ptr, DelayedEvent const &e)
{
    std::pair< DelayedEvents::iterator, bool > p =
        delayedEvents.insert(std::make_pair(ptr, e));
    // Delete events take precedence over other events.
    if (!p.second && e.type == EVENT_REMOVE)
    {
        p.first->second.type = EVENT_REMOVE;
    }
}

MapComposite *State::loadMap(int mapId)
{
    Maps::iterator m = maps.find(mapId);
    if (m != maps.end()) return m->second;
    Map *map = mapManager->getMap(mapId);
    assert(map);
    MapComposite *tmp = new MapComposite(map);
    maps[mapId] = tmp;

    // will need to load extra map related resources here also

    return tmp;
}

void State::sayAround(Object *obj, std::string text)
{
    unsigned short id = 65535;
    int type = obj->getType();
    if (type == OBJECT_PLAYER || type == OBJECT_NPC || type == OBJECT_MONSTER)
    {
        id = static_cast< MovingObject * >(obj)->getPublicID();
    }
    MessageOut msg(GPMSG_SAY);
    msg.writeShort(id);
    msg.writeString(text);

    Maps::iterator m = maps.find(obj->getMapId());
    if (m == maps.end()) return;
    MapComposite *map = m->second;
    Point speakerPosition = obj->getPosition();

    for (PlayerIterator i(map->getAroundObjectIterator(obj, AROUND_AREA)); i; ++i)
    {
        if (speakerPosition.inRangeOf((*i)->getPosition()))
        {
            gameHandler->sendTo(*i, msg);
        }
    }
}
