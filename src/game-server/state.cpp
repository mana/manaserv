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
#include "map.h"
#include "mapcomposite.h"
#include "point.h"
#include "game-server/gamehandler.hpp"
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
        addObject(ObjectPtr(being));
    }
}

State::~State()
{
    for (std::map< unsigned, MapComposite * >::iterator i = maps.begin(),
         i_end = maps.end(); i != i_end; ++i)
    {
        delete i->second;
    }
}

void
State::update()
{
    /*
     * Update game state (update AI, etc.)
     */
    for (std::map< unsigned, MapComposite * >::iterator m = maps.begin(),
         m_end = maps.end(); m != m_end; ++m)
    {
        MapComposite *map = m->second;

        for (ObjectIterator o(map->getWholeMapIterator()); o; ++o)
        {
            (*o)->update();
            if ((*o)->getType() == OBJECT_PLAYER || (*o)->getType() == OBJECT_MONSTER)
            {
                static_cast< Being * >(*o)->clearHitsTaken();
            }
        }

        for (MovingObjectIterator o(map->getWholeMapIterator()); o; ++o)
        {
            if ((*o)->getUpdateFlags() & ATTACK)
            {
                static_cast< Being * >(*o)->performAttack(m->second);
            }
        }

        for (MovingObjectIterator o(map->getWholeMapIterator()); o; ++o)
        {
            (*o)->move();
        }

        map->update();

        /*
         * Inform clients about changes in the game state
         */
        for (PlayerIterator p(map->getWholeMapIterator()); p; ++p)
        {
            MessageOut moveMsg(GPMSG_BEINGS_MOVE);
            MessageOut damageMsg(GPMSG_BEINGS_DAMAGE);

            for (MovingObjectIterator o(map->getAroundPlayerIterator(*p)); o; ++o)
            {

                Point os = (*o)->getOldPosition();
                Point on = (*o)->getPosition();

                int flags = 0;

                // Send attack messages
                if (    (*o)->getUpdateFlags() & ATTACK
                    &&  (*o)->getPublicID() != (*p)->getPublicID()
                    &&  (*p)->getPosition().inRangeOf(on)
                    )
                {
                    MessageOut AttackMsg (GPMSG_BEING_ATTACK);
                    AttackMsg.writeShort((*o)->getPublicID());

                    LOG_DEBUG("Sending attack packet from " << (*o)->getPublicID()
                              << " to " << (*p)->getPublicID(), 0);

                    gameHandler->sendTo(*p, AttackMsg);
                }

                // Send damage messages

                if ((*o)->getType() == OBJECT_PLAYER || (*o)->getType() == OBJECT_MONSTER)
                {
                    Being *victim = static_cast< Being * >(*o);
                    Hits const &hits = victim->getHitsTaken();
                    for (Hits::const_iterator i = hits.begin(),
                         i_end = hits.end(); i != i_end; ++i)
                    {
                        damageMsg.writeShort(victim->getPublicID());
                        damageMsg.writeShort((*i));
                    }
                }

                // Send move messages

                /* Check whether this player and this moving object were around
                 * the last time and whether they will be around the next time.
                 */
                bool wereInRange = (*p)->getOldPosition().inRangeOf(os) &&
                    !(((*p)->getUpdateFlags() | (*o)->getUpdateFlags()) & NEW_ON_MAP);
                bool willBeInRange = (*p)->getPosition().inRangeOf(on);
                if (!wereInRange)
                {
                    // o was outside p's range.
                    if (!willBeInRange)
                    {
                        // Nothing to report: o will not be inside p's range.
                        continue;
                    }
                    flags |= MOVING_DESTINATION;

                    int type = (*o)->getType();
                    MessageOut enterMsg(GPMSG_BEING_ENTER);
                    enterMsg.writeByte(type);
                    enterMsg.writeShort((*o)->getPublicID());
                    switch (type) {
                    case OBJECT_PLAYER:
                    {
                        Player *q = static_cast< Player * >(*o);
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
                    gameHandler->sendTo(*p, enterMsg);
                }
                else if (!willBeInRange)
                {
                    // o is no longer visible from p.
                    MessageOut leaveMsg(GPMSG_BEING_LEAVE);
                    leaveMsg.writeShort((*o)->getPublicID());
                    gameHandler->sendTo(*p, leaveMsg);
                    continue;
                }
                else if (os.x == on.x && os.y == on.y)
                {
                    // o does not move, nothing to report.
                    continue;
                }

                /* At this point, either o has entered p's range, either o is
                   moving inside p's range. Report o's movements. */

                Point od = (*o)->getDestination();
                if (on.x != od.x || on.y != od.y)
                {
                    flags |= MOVING_POSITION;
                    if ((*o)->getUpdateFlags() & NEW_DESTINATION)
                    {
                        flags |= MOVING_DESTINATION;
                    }
                }
                else
                {
                    // no need to synchronize on the very last step
                    flags |= MOVING_DESTINATION;
                }

                moveMsg.writeShort((*o)->getPublicID());
                moveMsg.writeByte(flags);
                if (flags & MOVING_POSITION)
                {
                    moveMsg.writeCoordinates(on.x / 32, on.y / 32);
                }
                if (flags & MOVING_DESTINATION)
                {
                    moveMsg.writeShort(od.x);
                    moveMsg.writeShort(od.y);
                }
            }

            // Don't send a packet if nothing happened in p's range.
            if (moveMsg.getLength() > 2)
                gameHandler->sendTo(*p, moveMsg);

            if (damageMsg.getLength() > 2)
                gameHandler->sendTo(*p, damageMsg);
        }

        for (ObjectIterator o(map->getWholeMapIterator()); o; ++o)
        {
            (*o)->clearUpdateFlags();
        }
    }
}

void
State::addObject(ObjectPtr objectPtr)
{
    unsigned mapId = objectPtr->getMapId();
    MapComposite *map = loadMap(mapId);
    if (!map || !map->insert(objectPtr))
    {
        // TODO: Deal with failure to place Object on the map.
        return;
    }
    objectPtr->raiseUpdateFlags(NEW_ON_MAP);
    if (objectPtr->getType() != OBJECT_PLAYER) return;
    Player *playerPtr = static_cast< Player * >(objectPtr.get());

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

void
State::removeObject(ObjectPtr objectPtr)
{
    unsigned mapId = objectPtr->getMapId();
    std::map< unsigned, MapComposite * >::iterator m = maps.find(mapId);
    if (m == maps.end()) return;
    MapComposite *map = m->second;

    int type = objectPtr->getType();
    if (type == OBJECT_MONSTER || type == OBJECT_PLAYER || type == OBJECT_NPC)
    {
        MovingObject *obj = static_cast< MovingObject * >(objectPtr.get());
        MessageOut msg(GPMSG_BEING_LEAVE);
        msg.writeShort(obj->getPublicID());
        Point objectPos = obj->getPosition();

        for (PlayerIterator p(map->getAroundObjectIterator(obj)); p; ++p)
        {
            if (*p != obj && objectPos.inRangeOf((*p)->getPosition()))
            {
                gameHandler->sendTo(*p, msg);
            }
        }
    }

    map->remove(objectPtr);
}

MapComposite *State::loadMap(unsigned mapId)
{
    std::map< unsigned, MapComposite * >::iterator m = maps.find(mapId);
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

    std::map< unsigned, MapComposite * >::iterator m = maps.find(obj->getMapId());
    if (m == maps.end()) return;
    MapComposite *map = m->second;
    Point speakerPosition = obj->getPosition();

    for (PlayerIterator i(map->getAroundObjectIterator(obj)); i; ++i)
    {
        if (speakerPosition.inRangeOf((*i)->getPosition()))
        {
            gameHandler->sendTo(*i, msg);
        }
    }
}
