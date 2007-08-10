/*
 *  The Mana World Server
 *  Copyright 2006 The Mana World Development Team
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

#ifndef _TMW_SERVER_MAPCOMPOSITE_
#define _TMW_SERVER_MAPCOMPOSITE_

#include <string>
#include <vector>

class Map;
class MapComposite;
class MovingObject;
class Object;
class Character;
class Point;
class Rectangle;
class Script;
class Thing;

struct MapContent;
struct MapZone;

/**
 * Ordered sets of zones of a map.
 */
typedef std::vector< unsigned > MapRegion;

/**
 * Iterates through the zones of a region of the map.
 */
struct ZoneIterator
{
    MapRegion region; /**< Zones to visit. Empty means the entire map. */
    unsigned pos;
    MapZone *current;
    MapContent const *map;

    ZoneIterator(MapRegion const &, MapContent const *);
    void operator++();
    MapZone *operator*() const { return current; }
    operator bool() const { return current; }
};

/**
 * Iterates through the Characters of a region.
 */
struct CharacterIterator
{
    ZoneIterator iterator;
    unsigned short pos;
    Character *current;

    CharacterIterator(ZoneIterator const &);
    void operator++();
    Character *operator*() const { return current; }
    operator bool() const { return iterator; }
};

/**
 * Iterates through the MovingObjects of a region.
 */
struct MovingObjectIterator
{
    ZoneIterator iterator;
    unsigned short pos;
    MovingObject *current;

    MovingObjectIterator(ZoneIterator const &);
    void operator++();
    MovingObject *operator*() const { return current; }
    operator bool() const { return iterator; }
};

/**
 * Iterates through the non-moving Objects of a region.
 */
struct FixedObjectIterator
{
    ZoneIterator iterator;
    unsigned short pos;
    Object *current;

    FixedObjectIterator(ZoneIterator const &);
    void operator++();
    Object *operator*() const { return current; }
    operator bool() const { return iterator; }
};

/**
 * Iterates through the Objects of a region.
 */
struct ObjectIterator
{
    ZoneIterator iterator;
    unsigned short pos;
    Object *current;

    ObjectIterator(ZoneIterator const &);
    void operator++();
    Object *operator*() const { return current; }
    operator bool() const { return iterator; }
};

/**
 * Combined map/entity structure.
 */
class MapComposite
{
    public:
        /**
         * Constructor.
         */
        MapComposite(int id, std::string const &name);

        /**
         * Destructor.
         */
        ~MapComposite();

        /**
         * Sets the underlying pathfinding map.
         * Can be done only once.
         */
        void setMap(Map *);

        /**
         * Gets the underlying pathfinding map.
         */
        Map *getMap() const
        { return mMap; }

        /**
         * Sets the associated script.
         */
        void setScript(Script *s)
        { mScript = s; }

        /**
         * Gets the associated script.
         */
        Script *getScript() const
        { return mScript; }

        /**
         * Returns whether the map is active on this server or not.
         */
        bool isActive() const
        { return mMap; }

        /**
         * Gets the game ID of this map.
         */
        int getID() const
        { return mID; }

        /**
         * Gets the name of this map.
         */
        std::string const &getName() const
        { return mName; }

        /**
         * Inserts an object on the map. Sets its public ID if relevant.
         */
        bool insert(Thing *);

        /**
         * Removes an object from the map.
         */
        void remove(Thing *);

        /**
         * Updates zones of every moving beings.
         */
        void update();

        /**
         * Gets an iterator on the objects of the whole map.
         */
        ZoneIterator getWholeMapIterator() const
        { return ZoneIterator(MapRegion(), mContent); }

        /**
         * Gets an iterator on the objects inside a given rectangle.
         */
        ZoneIterator getInsideRectangleIterator(Rectangle const &) const;

        /**
         * Gets an iterator on the objects around a given point.
         */
        ZoneIterator getAroundPointIterator(Point const &, int radius) const;

        /**
         * Gets an iterator on the objects around a given object.
         */
        ZoneIterator getAroundObjectIterator(Object *, int radius) const;

        /**
         * Gets an iterator on the objects around the old and new positions of
         * a character (including the ones that were but are now elsewhere).
         */
        ZoneIterator getAroundCharacterIterator(MovingObject *, int radius) const;

        /**
         * Gets everything related to the map.
         */
        std::vector< Thing * > const &getEverything() const;

    private:
        MapComposite(MapComposite const &);

        Map *mMap;            /**< Actual map. */
        MapContent *mContent; /**< Entities on the map. */
        Script *mScript;      /**< Script associated to this map. */
        std::string mName;    /**< Name of the map. */
        unsigned short mID;   /**< ID of the map. */
};

#endif
