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
#include <iostream>
#include "messageout.h"
#include "utils/logger.h"

namespace tmwserv
{

void State::update(ConnectionHandler &connectionHandler)
{
    // update game state (update AI, etc.)
    for (std::map<std::string, MapComposite>::iterator i = maps.begin();
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
    for (std::map<std::string, MapComposite>::iterator i = maps.begin();
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
                    msg.writeLong((int)b2->get());   // id
                    msg.writeLong(b2->get()->getX());// x
                    msg.writeLong(b2->get()->getY());// y

                    connectionHandler.sendTo(b->get(), msg);
                }
            }
        }
    }
}

void State::addBeing(Being *being, const std::string &map) {
    if (!mapExists(map))
        return;

    if (!beingExists(being))
        maps[map].beings.push_back(tmwserv::BeingPtr(being));
}

void State::removeBeing(Being *being) {
    for (std::map<std::string, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Beings::iterator b = i->second.beings.begin();
             b != i->second.beings.end();
             b++) {
            if (b->get() == being) {
                i->second.beings.erase(b);
                return;
            }
        }
    }
}

bool State::mapExists(const std::string &map) {
    std::map<std::string, MapComposite>::iterator i = maps.find(map);
    if (i == maps.end())
        return false;
    return true;
}

bool State::beingExists(Being *being) {
    for (std::map<std::string, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Beings::iterator b = i->second.beings.begin();
             b != i->second.beings.end();
             b++) {
            if (b->get() == being)
                return true;
        }
    }
    return false;
}

void State::loadMap(const std::string &map) {
    // load map
    maps[map] = MapComposite();
}

void State::addObject(Object *object, const std::string &map) {
    //
}

void State::removeObject(Object *object) {
    //
}

bool State::objectExists(Object *object) {
    for (std::map<std::string, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (std::vector<Object*>::iterator b = i->second.objects.begin();
             b != i->second.objects.end();
             b++) {
            if (*b == object)
                return true;
        }
    }
    return false;
}

const std::string State::findPlayer(Being *being) {
    for (std::map<std::string, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (Beings::iterator b = i->second.beings.begin();
             b != i->second.beings.end();
             b++) {
            if (b->get() == being)
                return i->first;
        }
    }
    return "";
}

const std::string State::findObject(Object *object) {
    for (std::map<std::string, MapComposite>::iterator i = maps.begin();
         i != maps.end();
         i++) {
        for (std::vector<Object*>::iterator b = i->second.objects.begin();
             b != i->second.objects.end();
             b++) {
            if (*b == object)
                return i->first;
        }
    }
    return "";
}

} // namespace tmwserv
