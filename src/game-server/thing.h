/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef THING_H
#define THING_H

#include "manaserv_protocol.h"
using namespace ManaServ;

#include <set>

class EventListener;
class MapComposite;

/**
 * Base class for in-game objects. Knows only its type and the map it resides
 * on. Provides listeners.
 */
class Thing
{
    public:
        /**
         * Constructor.
         */
        Thing(ThingType type, MapComposite *map = NULL)
          : mMap(map),
            mType(type)
        {}

        virtual ~Thing();

        /**
         * Gets type of this thing.
         *
         * @return the type of this thing.
         */
        ThingType getType() const
        { return mType; }

        /**
         * Returns whether this thing is visible on the map or not. (Actor)
         */
        bool isVisible() const
        { return mType != OBJECT_OTHER; }

        /**
         * Returns whether this thing can move on the map or not. (Actor)
         */
        bool canMove() const
        { return mType == OBJECT_CHARACTER || mType == OBJECT_MONSTER ||
                 mType == OBJECT_NPC; }

        /**
         * Returns whether this thing can fight or not. (Being)
         */
        bool canFight() const
        { return mType == OBJECT_CHARACTER || mType == OBJECT_MONSTER; }

        /**
         * Updates the internal status.
         */
        virtual void update() = 0;

        /**
         * Gets the map this thing is located on.
         */
        MapComposite *getMap() const
        { return mMap; }

        /**
         * Sets the map this thing is located on.
         */
        virtual void setMap(MapComposite *map)
        { mMap = map; }

        /**
         * Adds a new listener.
         */
        void addListener(const EventListener *);

        /**
         * Removes an existing listener.
         */
        void removeListener(const EventListener *);

        /**
         * Calls all the "inserted" listeners.
         */
        virtual void inserted();

        /**
         * Calls all the "removed" listeners.
         */
        virtual void removed();

    protected:
        typedef std::set< const EventListener * > Listeners;
        Listeners mListeners;   /**< List of event listeners. */

    private:
        MapComposite *mMap;     /**< Map the thing is on */
        ThingType mType;        /**< Type of this thing. */
};

#endif // THING_H
