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

#include "state.h"

#include "gamehandler.h"
#include "map.h"
#include "mapmanager.h"
#include "messageout.h"

#include "utils/logger.h"

namespace tmwserv
{

State::State() throw() {
}

State::~State() throw() {
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        delete i->second.map;
    }
}

void State::update()
{
    // update game state (update AI, etc.)
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Beings::iterator b = i->second.beings.begin();
             b != i->second.beings.end();
             b++) {
            b->get()->update();
        }
    }

    // notify clients about changes in the game world (only on their maps)
    // NOTE: Should only send differences in the game state.
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        //
        for (Beings::iterator b = i->second.beings.begin();
             b != i->second.beings.end();
             b++) {
            // send info about other players
            for (Beings::iterator b2 = i->second.beings.begin();
                 b2 != i->second.beings.end();
                 b2++) {
                if (b != b2) {
                    MessageOut msg;
                    msg.writeShort(SMSG_NEW_BEING); // 
                    msg.writeLong(OBJECT_PLAYER);    // type
                    msg.writeString(b2->get()->getName()); // Name
                    msg.writeLong(b2->get()->getX());// x
                    msg.writeLong(b2->get()->getY());// y

                    gameHandler->sendTo(*b, msg);
                }
            }
        }
    }
}

void State::addBeing(BeingPtr beingPtr, const unsigned int mapId) {
    if (!beingExists(beingPtr)) {
        if (!mapExists(mapId))
            if (!loadMap(mapId))
                return;
        maps[mapId].beings.push_back(beingPtr);
    }
}

void State::removeBeing(BeingPtr beingPtr) {
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Beings::iterator b = i->second.beings.begin();
             b != i->second.beings.end();
             b++) {
            if (b->get() == beingPtr.get()) {
                i->second.beings.erase(b);
                return;
            }
        }
    }
}

bool State::mapExists(const unsigned int mapId) {
    std::map<unsigned int, MapComposite>::iterator i = maps.find(mapId);
    if (i == maps.end())
        return false;
    return true;
}

bool State::beingExists(BeingPtr beingPtr) {
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Beings::iterator b = i->second.beings.begin();
             b != i->second.beings.end();
             b++) {
            if (b->get() == beingPtr.get())
                return true;
        }
    }
    return false;
}

bool State::loadMap(const unsigned int mapId) {
    Map *tmp = MapManager::instance().loadMap(mapId);
    if (!tmp)
    {
        return false;
    }

    maps[mapId] = MapComposite();
    maps[mapId].map = tmp;

    // will need to load extra map related resources here also

    return true; // We let true for testing on beings
}

void State::addObject(ObjectPtr objectPtr, const unsigned int mapId) {
    if (!objectExists(objectPtr)) {
        if (!mapExists(mapId))
            if (!loadMap(mapId))
                return;
        maps[mapId].objects.push_back(objectPtr);
    }
}

void State::removeObject(ObjectPtr objectPtr) {
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Objects::iterator b = i->second.objects.begin();
             b != i->second.objects.end();
             b++) {
            if (b->get() == objectPtr.get()) {
                i->second.objects.erase(b);
                return;
            }
        }
    }
}

bool State::objectExists(const ObjectPtr objectPtr) {
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Objects::iterator b = i->second.objects.begin();
             b != i->second.objects.end();
             b++) {
            if (b->get() == objectPtr.get())
                return true;
        }
    }
    return false;
}

const unsigned int State::findPlayer(BeingPtr beingPtr) {
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Beings::iterator b = i->second.beings.begin();
             b != i->second.beings.end();
             b++) {
            if (b->get() == beingPtr.get())
                return i->first;
        }
    }
    return 0;
}

const unsigned int State::findObject(ObjectPtr objectPtr) {
    for (std::map<unsigned int, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Objects::iterator b = i->second.objects.begin();
             b != i->second.objects.end();
             b++) {
            if (b->get() == objectPtr.get())
                return i->first;
        }
    }
    return 0;
}

} // namespace tmwserv
