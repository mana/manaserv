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

#include "gamehandler.h"
#include "map.h"
#include "mapmanager.h"
#include "messageout.h"
#include "point.h"
#include "storage.h"

#include "utils/logger.h"

State::State()
{
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
        typedef std::vector< utils::CountedPtr<MovingObject> > Movings;
        Movings movings;

        for (Objects::iterator o = m->second.objects.begin(),
             o_end = m->second.objects.end(); o != o_end; ++o)
        {
            (*o)->update();
            int t = (*o)->getType();
            if (t == OBJECT_NPC || t == OBJECT_PLAYER || t == OBJECT_MONSTER) {
                utils::CountedPtr<MovingObject> ptr(*o);
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
                Point os = (*o)->getXY();
                Point on = (*o)->getNextPosition();

                /* Check whether this player and this moving object were around
                 * the last time and whether they will be around the next time.
                 */
                bool wereInRange = (*p)->getXY().inRangeOf(os) &&
                    !(*p)->isNew() && !(*o)->isNew();
                bool willBeInRange = (*p)->getNextPosition().inRangeOf(on);

                if (!wereInRange)
                {
                    // o was outside p's range.
                    if (!willBeInRange)
                    {
                        // Nothing to report: o will not be inside p's range.
                        continue;
                    }

                    int type = (*o)->getType();
                    MessageOut msg2(GPMSG_BEING_ENTER);
                    msg2.writeByte(type);
                    msg2.writeLong((*o)->getID());
                    switch (type) {
                    case OBJECT_PLAYER:
                    {
                        PlayerPtr q(*o);
                        msg2.writeString(q->getName());
                        msg2.writeByte(q->getHairStyle());
                        msg2.writeByte(q->getHairColor());
                        msg2.writeByte(q->getGender());
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
                    msg2.writeByte((*o)->getType());
                    msg2.writeLong((*o)->getID());
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
                msg.writeLong((*o)->getID());
                msg.writeShort(on.x);
                msg.writeShort(on.y);
                msg.writeShort(od.x);
                msg.writeShort(od.y);
            }

            // Don't send a packet if nothing happed in p's range.
            if (msg.getLength() > 2)
                gameHandler->sendTo(*p, msg);
        }

        for (Movings::iterator o = movings.begin(),
             o_end = movings.end(); o != o_end; ++o)
        {
            (*o)->setXY((*o)->getNextPosition());
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
    maps[mapId].players.push_back(PlayerPtr(objectPtr));
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
    msg.writeByte(OBJECT_PLAYER);
    msg.writeLong(objectPtr->getID());

    Point objectPosition = objectPtr->getXY();
    Players &players = maps[mapId].players;
    Players::iterator p_end = players.end(), j = p_end;
    for (Players::iterator p = players.begin(); p != p_end; ++p)
    {
        if (p->get() == objectPtr.get())
        {
            j = p;
        }
        else if (objectPosition.inRangeOf((*p)->getXY()))
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
    mapChangeMessage.writeShort(playerPtr->getX());
    mapChangeMessage.writeShort(playerPtr->getY());
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
