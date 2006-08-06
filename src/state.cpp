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
    for (std::map<unsigned int, MapComposite>::iterator m = maps.begin(),
         m_end = maps.end(); m != m_end; ++m) {

        typedef std::vector< utils::CountedPtr<MovingObject> > Movings;
        Movings movings;

        for (Objects::iterator o = m->second.objects.begin(),
             o_end = m->second.objects.end(); o != o_end; ++o) {
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
             p_end = players.end(); p != p_end; ++p) {
            std::pair<unsigned, unsigned> ps = (*p)->getXY();
            std::pair<unsigned, unsigned> pn = (*p)->getNextPosition();
            MessageOut msg;
            msg.writeShort(GPMSG_BEINGS_MOVE);

            for (Movings::iterator o = movings.begin(),
                 o_end = movings.end(); o != o_end; ++o) {
                std::pair<unsigned, unsigned> os = (*o)->getXY();
                std::pair<unsigned, unsigned> on = (*o)->getNextPosition();

                bool were = areAround(ps.first, ps.second, os.first, os.second);
                bool will = areAround(pn.first, pn.second, on.first, on.second);
                bool has_moved = os.first != on.first || os.second != on.second;
                if (!(will && has_moved) && !(were != will))
                    continue;

                std::pair<unsigned, unsigned> od = (*o)->getDestination();
                msg.writeLong((*o)->getID());
                msg.writeShort(on.first);
                msg.writeShort(on.second);
                msg.writeShort(od.first);
                msg.writeShort(od.second);
            }

            if (msg.getDataSize() > 2)
                gameHandler->sendTo(*p, msg);
        }

        for (Movings::iterator o = movings.begin(),
             o_end = movings.end(); o != o_end; ++o) {
            std::pair<unsigned, unsigned> pos = (*o)->getNextPosition();
            (*o)->setXY(pos.first, pos.second);
        }
    }
}

void State::addObject(ObjectPtr objectPtr) {
    unsigned mapId = objectPtr->getMapId();
    if (!loadMap(mapId)) return;
    maps[mapId].objects.push_back(objectPtr);
    if (objectPtr->getType() != OBJECT_PLAYER) return;
    PlayerPtr playerPtr(objectPtr);
    Players &players = maps[mapId].players;
    players.push_back(playerPtr);
    MessageOut msg;
    msg.writeShort(GPMSG_BEING_ENTER);
    msg.writeByte(OBJECT_PLAYER);
    msg.writeLong(playerPtr->getID());
    msg.writeString(playerPtr->getName());
    msg.writeByte(playerPtr->getHairStyle());
    msg.writeByte(playerPtr->getHairColor());
    msg.writeByte(playerPtr->getGender());
    for (Players::iterator p = players.begin(),
         p_end = players.end(); p != p_end; ++p) {
        gameHandler->sendTo(*p, msg);
    }
}

void State::removeObject(ObjectPtr objectPtr) {
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
    PlayerPtr playerPtr(objectPtr);
    Players &players = maps[mapId].players;
    MessageOut msg;
    msg.writeShort(GPMSG_BEING_LEAVE);
    msg.writeByte(OBJECT_PLAYER);
    msg.writeLong(playerPtr->getID());
    Players::iterator p_end = players.end(), j = p_end;
    for (Players::iterator p = players.begin(); p != p_end; ++p) {
        if (p->get() == playerPtr.get())
            j = p;
        else
            gameHandler->sendTo(*p, msg);
    }
    if (j != players.end()) players.erase(j);
}

void State::informPlayer(PlayerPtr playerPtr) {
    unsigned mapId = playerPtr->getMapId();
    std::map<unsigned, MapComposite>::iterator m = maps.find(mapId);
    if (m == maps.end()) return;
    Players &players = m->second.players;
    for (Players::iterator p = players.begin(),
         p_end = players.end(); p != p_end; ++p) {
        MessageOut msg;
        msg.writeShort(GPMSG_BEING_ENTER);
        msg.writeByte(OBJECT_PLAYER);
        msg.writeLong((*p)->getID());
        msg.writeString((*p)->getName());
        msg.writeByte((*p)->getHairStyle());
        msg.writeByte((*p)->getHairColor());
        msg.writeByte((*p)->getGender());
        gameHandler->sendTo(playerPtr, msg);
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
