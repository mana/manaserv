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

State::State() {
}

State::~State() {
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
}

void State::addBeing(BeingPtr beingPtr) {
    unsigned mapId = beingPtr->getMapId();
    if (!loadMap(mapId)) return;
    Beings &beings = maps[mapId].beings;
    beings.push_back(beingPtr);
    MessageOut msg;
    msg.writeShort(GPMSG_BEING_ENTER);
    msg.writeByte(OBJECT_PLAYER);
    msg.writeLong(0); // ID
    msg.writeString(beingPtr->getName());
    msg.writeByte(beingPtr->getHairStyle());
    msg.writeByte(beingPtr->getHairColor());
    msg.writeByte(beingPtr->getGender());
    for (Beings::iterator b = beings.begin(), end = beings.end();
         b != end; ++b) {
        gameHandler->sendTo(*b, msg);
    }
}

void State::removeBeing(BeingPtr beingPtr) {
    unsigned mapId = beingPtr->getMapId();
    std::map<unsigned, MapComposite>::iterator i = maps.find(mapId);
    if (i == maps.end()) return;
    Beings &beings = i->second.beings;
    MessageOut msg;
    msg.writeShort(GPMSG_BEING_LEAVE);
    msg.writeByte(OBJECT_PLAYER);
    msg.writeLong(0); // ID
    Beings::iterator j = beings.end();
    for (Beings::iterator b = beings.begin(), end = beings.end();
         b != end; ++b) {
        if (b->get() == beingPtr.get())
            j = b;
        else
            gameHandler->sendTo(*b, msg);
    }
    if (j != beings.end()) beings.erase(j);
}

void State::informBeing(BeingPtr beingPtr) {
    unsigned mapId = beingPtr->getMapId();
    std::map<unsigned, MapComposite>::iterator i = maps.find(mapId);
    if (i == maps.end()) return;
    Beings &beings = i->second.beings;
    for (Beings::iterator b = beings.begin(), end = beings.end();
         b != end; ++b) {
        MessageOut msg;
        msg.writeShort(GPMSG_BEING_ENTER);
        msg.writeByte(OBJECT_PLAYER);
        msg.writeLong(0); // ID
        msg.writeString((*b)->getName());
        msg.writeByte((*b)->getHairStyle());
        msg.writeByte((*b)->getHairColor());
        msg.writeByte((*b)->getGender());
        gameHandler->sendTo(beingPtr, msg);
    }
}

bool State::loadMap(const unsigned int mapId) {
    if (maps.find(mapId) != maps.end()) return true;
    Map *tmp = MapManager::instance().loadMap(mapId);
    if (!tmp) return false;
    maps[mapId].map = tmp;

    // will need to load extra map related resources here also

    return true; // We let true for testing on beings
}

void State::addObject(ObjectPtr objectPtr) {
    unsigned mapId = objectPtr->getMapId();
    if (!loadMap(mapId)) return;
    maps[mapId].objects.push_back(objectPtr);
}

void State::removeObject(ObjectPtr objectPtr) {
    unsigned mapId = objectPtr->getMapId();
    std::map<unsigned, MapComposite>::iterator i = maps.find(mapId);
    if (i == maps.end()) return;
    for (Objects::iterator b = i->second.objects.begin(), end = i->second.objects.end();
         b != end; ++b) {
        if (b->get() == objectPtr.get()) {
            i->second.objects.erase(b);
            return;
        }
    }
}

} // namespace tmwserv
