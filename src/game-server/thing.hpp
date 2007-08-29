/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */

#ifndef _TMWSERV_THING_H_
#define _TMWSERV_THING_H_

#include <set>

class EventListener;
class MapComposite;

/**
 * Object type enumeration.
 */
enum
{
    OBJECT_ITEM = 0,  /**< A simple item. */
    OBJECT_ACTOR,     /**< An item that toggle map/quest actions (doors,
                           switchs, ...) and can speak (map panels). */
    OBJECT_NPC,       /**< Non-Playable-Character is an actor capable of
                           movement and maybe actions. */
    OBJECT_MONSTER,   /**< A monster (moving actor with AI. Should be able to
                           toggle map/quest actions, too). */
    OBJECT_CHARACTER, /**< A normal being. */
    OBJECT_OTHER      /**< Server-only object. */
};

/**
 * Base class for in-game objects. Knows only its type and the map is resides
 * on. Provides listeners.
 */
class Thing
{
    public:
        /**
         * Constructor.
         */
        Thing(int type, MapComposite *map = NULL)
          : mMap(map),
            mType(type)
        {}

        /**
         * Destructor.
         */
        virtual ~Thing();

        /**
         * Gets type of this thing.
         *
         * @return the type of this thing.
         */
        int getType() const
        { return mType; }

        /**
         * Returns whether this thing is visible on the map or not. (Object)
         */
        bool isVisible() const
        { return mType != OBJECT_OTHER; }

        /**
         * Returns whether this thing can move on the map or not. (MovingObject)
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
        virtual void
        update() = 0;

        /**
         * Gets the map this thing is located on.
         */
        MapComposite *getMap() const
        { return mMap; }

        /**
         * Sets the map this thing is located on.
         */
        void setMap(MapComposite *map)
        { mMap = map; }

        /**
         * Adds a new listener.
         */
        void addListener(EventListener const *);

        /**
         * Removes an existing listener.
         */
        void removeListener(EventListener const *);

        /**
         * Calls all the "inserted" listeners.
         */
        virtual void inserted();

        /**
         * Calls all the "removed" listeners.
         */
        virtual void removed();

    protected:
        typedef std::set< EventListener const * > Listeners;
        Listeners mListeners; /**< List of event listeners. */

    private:
        MapComposite *mMap;     /**< Map the thing is on */
        char mType;             /**< Type of this thing. */
};

#endif // _TMWSERV_THING_H_
