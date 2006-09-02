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

#include "state.h"

#include "controller.h"
#include "gamehandler.h"
#include "map.h"
#include "mapmanager.h"
#include "messageout.h"
#include "point.h"
#include "storage.h"

#include "utils/logger.h"

State::State()
{
    // Create 10 maggots for testing purposes
    for (int i = 0; i < 10; i++)
    {
        // TODO: Unique object numbering
        BeingPtr being = BeingPtr(new Being(OBJECT_MONSTER, 1000 + i));
        being->setSpeed(150);
        being->setMapId(1);
        Point pos = { 720, 900 };
        being->setPosition(pos);
        Controller *controller = new Controller();
        controller->possess(being);
        addObject(ObjectPtr(being));
    }
}

State::~State()
{
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++)
    {
        delete i->second.map;
    }
}

void
State::update()
{
    // update game state (update AI, etc.)
    for (std::map<unsigned int, MapComposite>::iterator m = maps.begin(),
         m_end = maps.end(); m != m_end; ++m)
    {
        typedef std::vector<MovingObjectPtr> Movings;
        Movings movings;

        for (Objects::iterator o = m->second.objects.begin(),
             o_end = m->second.objects.end(); o != o_end; ++o)
        {
            (*o)->update();
            int t = (*o)->getType();
            if (t == OBJECT_NPC || t == OBJECT_PLAYER || t == OBJECT_MONSTER) {
                MovingObjectPtr ptr(*o);
                ptr->move();
                movings.push_back(ptr);
            }
        }

        Players &players = m->second.players;
        for (Players::iterator p = players.begin(),
             p_end = players.end(); p != p_end; ++p)
        {
            MessageOut msg(GPMSG_BEINGS_MOVE);

            for (Movings::iterator o = movings.begin(),
                 o_end = movings.end(); o != o_end; ++o)
            {
                Point os = (*o)->getOldPosition();
                Point on = (*o)->getPosition();

                /* Check whether this player and this moving object were around
                 * the last time and whether they will be around the next time.
                 */
                bool wereInRange = (*p)->getOldPosition().inRangeOf(os) &&
                                   !(*p)->isNew() && !(*o)->isNew();
                bool willBeInRange = (*p)->getPosition().inRangeOf(on);

                int flags = 0;

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
                    MessageOut msg2(GPMSG_BEING_ENTER);
                    msg2.writeByte(type);
                    msg2.writeShort((*o)->getPublicID());
                    switch (type) {
                    case OBJECT_PLAYER:
                    {
                        PlayerPtr q(*o);
                        msg2.writeString(q->getName());
                        msg2.writeByte(q->getHairStyle());
                        msg2.writeByte(q->getHairColor());
                        msg2.writeByte(q->getGender());
                    } break;
                    case OBJECT_MONSTER:
                    {
                        msg2.writeShort(0); // TODO: The monster ID
                    } break;
                    default:
                        assert(false); // TODO
                    }
                    gameHandler->sendTo(*p, msg2);
                }
                else if (!willBeInRange)
                {
                    // o is no longer visible from p.
                    MessageOut msg2(GPMSG_BEING_LEAVE);
                    msg2.writeShort((*o)->getPublicID());
                    gameHandler->sendTo(*p, msg2);
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
                }
                else
                {
                    // no need to synchronize on the very last step
                    flags |= MOVING_DESTINATION;
                }

                // TODO: updates destination only on changes.
                flags |= MOVING_DESTINATION;

                if (!(flags & (MOVING_POSITION | MOVING_DESTINATION)))
                {
                    continue;
                }

                msg.writeShort((*o)->getPublicID());
                msg.writeByte(flags);
                if (flags & MOVING_POSITION)
                {
                    msg.writeCoordinates(on.x / 32, on.y / 32);
                }
                if (flags & MOVING_DESTINATION)
                {
                    msg.writeShort(od.x);
                    msg.writeShort(od.y);
                }
            }

            // Don't send a packet if nothing happened in p's range.
            if (msg.getLength() > 2)
                gameHandler->sendTo(*p, msg);
        }

        for (Movings::iterator o = movings.begin(),
             o_end = movings.end(); o != o_end; ++o)
        {
            (*o)->setNew(false);
        }
    }
}

void
State::addObject(ObjectPtr objectPtr)
{
    unsigned mapId = objectPtr->getMapId();
    if (!loadMap(mapId)) return;
    maps[mapId].objects.push_back(objectPtr);
    objectPtr->setNew(true);
    if (objectPtr->getType() != OBJECT_PLAYER) return;
    PlayerPtr ptr(objectPtr);
    // TODO: Unique object numbering
    ptr->setPublicID(ptr->getDatabaseID());
    maps[mapId].players.push_back(ptr);
}

void
State::removeObject(ObjectPtr objectPtr)
{
    unsigned mapId = objectPtr->getMapId();
    std::map<unsigned, MapComposite>::iterator m = maps.find(mapId);
    if (m == maps.end()) return;
    Objects &objects = m->second.objects;
    for (Objects::iterator o = objects.begin(),
         o_end = objects.end(); o != o_end; ++o) {
        if (o->get() == objectPtr.get()) {
            objects.erase(o);
            break;
        }
    }
    if (objectPtr->getType() != OBJECT_PLAYER) return;

    MessageOut msg(GPMSG_BEING_LEAVE);
    msg.writeShort(PlayerPtr(objectPtr)->getPublicID());

    Point objectPosition = objectPtr->getPosition();
    Players &players = maps[mapId].players;
    Players::iterator p_end = players.end(), j = p_end;
    for (Players::iterator p = players.begin(); p != p_end; ++p)
    {
        if (p->get() == objectPtr.get())
        {
            j = p;
        }
        else if (objectPosition.inRangeOf((*p)->getPosition()))
        {
            gameHandler->sendTo(*p, msg);
        }
    }

    if (j != p_end)
    {
        players.erase(j);
    }
}

void
State::informPlayer(PlayerPtr playerPtr)
{
    unsigned mapId = playerPtr->getMapId();
    std::map<unsigned, MapComposite>::iterator m = maps.find(mapId);
    if (m == maps.end()) return;

    /* Since the player doesn't know yet where on the world he is after
     * connecting to the map server, we send him an initial change map message.
     */
    Storage &store = Storage::instance("tmw");
    MessageOut mapChangeMessage(GPMSG_PLAYER_MAP_CHANGE);
    mapChangeMessage.writeString(store.getMapNameFromId(mapId));
    Point pos = playerPtr->getPosition();
    mapChangeMessage.writeShort(pos.x);
    mapChangeMessage.writeShort(pos.y);
    mapChangeMessage.writeByte(0);
    gameHandler->sendTo(playerPtr, mapChangeMessage);
}

bool
State::loadMap(const unsigned int mapId)
{
    if (maps.find(mapId) != maps.end()) return true;
    Map *tmp = MapManager::instance().loadMap(mapId);
    if (!tmp) return false;
    maps[mapId].map = tmp;

    // will need to load extra map related resources here also

    return true; // We let true for testing on beings
}
