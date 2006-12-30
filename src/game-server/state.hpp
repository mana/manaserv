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

#ifndef _TMW_SERVER_STATE_
#define _TMW_SERVER_STATE_

#include <map>
#include <string>

class MapComposite;
class Object;

enum
{
    EVENT_REMOVE = 0,
    EVENT_INSERT,
    EVENT_WARP
};

struct DelayedEvent
{
    unsigned short type, map, x, y;
};

/**
 * State class contains all information/procedures associated with the game
 * world's state.
 */
class State
{
        typedef std::map< int, MapComposite * > Maps;
        typedef std::map< Object *, DelayedEvent > DelayedEvents;

        /**
         * List of maps.
         */
        Maps maps;

        /**
         * List of delayed events.
         */
        DelayedEvents delayedEvents;

        /**
         * Updates object states on the map.
         */
        void updateMap(MapComposite *);

        /**
         * Informs a player of what happened around.
         */
        void informPlayer(MapComposite *, Player *);

     public:
        State();
        ~State();

        /**
         * Updates game state (contains core server logic).
         */
        void update();

        /**
         * Loads map into game world.
         */
        MapComposite *loadMap(int mapId);

        /**
         * Inserts an object on the map.
         */
        void insertObject(Object *);

        /**
         * Removes an object from the map.
         */
        void removeObject(Object *);

        /**
         * Enqueues an event. It will be executed at end of update.
         */
        void enqueueEvent(Object *, DelayedEvent const &);

        /**
         * Says something around an object.
         */
        void sayAround(Object *, std::string text);
};

extern State *gameState;

#endif
