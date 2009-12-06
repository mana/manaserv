/*
 *  The Mana Server
 *  Copyright (C) 2006  The Mana World Development Team
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

#ifndef SERVER_MAPCOMPOSITE_HPP
#define SERVER_MAPCOMPOSITE_HPP

#include <string>
#include <vector>

class Actor;
class Being;
class Character;
class Map;
class MapComposite;
class Point;
class Rectangle;
class Script;
class Thing;

struct MapContent;
struct MapZone;

enum PvPRules
{
    PVP_NONE,   // no PvP on this map
    PVP_FREE    // unrestricted PvP on this map
    // [space for additional PvP modes]
};

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
    const MapContent *map;

    ZoneIterator(const MapRegion &, const MapContent *);
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

    CharacterIterator(const ZoneIterator &);
    void operator++();
    Character *operator*() const { return current; }
    operator bool() const { return iterator; }
};

/**
 * Iterates through the Beings of a region.
 */
struct BeingIterator
{
    ZoneIterator iterator;
    unsigned short pos;
    Being *current;

    BeingIterator(const ZoneIterator &);
    void operator++();
    Being *operator*() const { return current; }
    operator bool() const { return iterator; }
};

/**
 * Iterates through the non-moving Actors of a region.
 */
struct FixedActorIterator
{
    ZoneIterator iterator;
    unsigned short pos;
    Actor *current;

    FixedActorIterator(const ZoneIterator &);
    void operator++();
    Actor *operator*() const { return current; }
    operator bool() const { return iterator; }
};

/**
 * Iterates through the Actors of a region.
 */
struct ActorIterator
{
    ZoneIterator iterator;
    unsigned short pos;
    Actor *current;

    ActorIterator(const ZoneIterator &);
    void operator++();
    Actor *operator*() const { return current; }
    operator bool() const { return iterator; }
};

/**
 * Part of a map.
 */
struct MapZone
{
    unsigned short nbCharacters, nbMovingObjects;
    /**
     * Objects present in this zone.
     * Characters are stored first, then the remaining MovingObjects, then the
     * remaining Objects.
     */
    std::vector< Actor * > objects;

    /**
     * Destinations of the objects that left this zone.
     * This is necessary in order to have an accurate iterator around moving
     * objects.
     */
    MapRegion destinations;

    MapZone(): nbCharacters(0), nbMovingObjects(0) {}
    void insert(Actor *);
    void remove(Actor *);
};

/**
 * Pool of public IDs for MovingObjects on a map. By maintaining public ID
 * availability using bits, it can locate an available public ID fast while
 * using minimal memory access.
 */
struct ObjectBucket
{
    static int const int_bitsize = sizeof(unsigned) * 8;
    unsigned bitmap[256 / int_bitsize]; /**< Bitmap of free locations. */
    short free;                         /**< Number of empty places. */
    short next_object;                  /**< Next object to look at. */
    Actor *objects[256];

    ObjectBucket();
    int allocate();
    void deallocate(int);
};

/**
 * Entities on a map.
 */
struct MapContent
{
    MapContent(Map *);
    ~MapContent();

    /**
     * Allocates a unique ID for an actor on this map.
     */
    bool allocate(Actor *);

    /**
     * Deallocates an ID.
     */
    void deallocate(Actor *);

    /**
     * Fills a region of zones within the range of a point.
     */
    void fillRegion(MapRegion &, const Point &, int) const;

    /**
     * Fills a region of zones inside a rectangle.
     */
    void fillRegion(MapRegion &, const Rectangle &) const;

    /**
     * Gets zone at given position.
     */
    MapZone &getZone(const Point &pos) const;

    /**
     * Things (items, characters, monsters, etc) located on the map.
     */
    std::vector< Thing * > things;

    /**
     * Buckets of MovingObjects located on the map, referenced by ID.
     */
    ObjectBucket *buckets[256];

    int last_bucket; /**< Last bucket acted upon. */

    /**
     * Partition of the Objects, depending on their position on the map.
     */
    MapZone *zones;

    unsigned short mapWidth;  /**< Width with respect to zones. */
    unsigned short mapHeight; /**< Height with respect to zones. */
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
        MapComposite(int id, const std::string &name);

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
        const std::string &getName() const
        { return mName; }

        /**
         * Inserts a thing on the map. Sets its public ID if relevant.
         */
        bool insert(Thing *);

        /**
         * Removes a thing from the map.
         */
        void remove(Thing *);

        /**
         * Updates zones of every moving beings.
         */
        void update();

        /**
         * Gets the PvP rules on the map.
         */
        PvPRules getPvP() const { return mPvPRules; }

        /**
         * Gets an iterator on the objects of the whole map.
         */
        ZoneIterator getWholeMapIterator() const
        { return ZoneIterator(MapRegion(), mContent); }

        /**
         * Gets an iterator on the objects inside a given rectangle.
         */
        ZoneIterator getInsideRectangleIterator(const Rectangle &) const;

        /**
         * Gets an iterator on the objects around a given point.
         */
        ZoneIterator getAroundPointIterator(const Point &, int radius) const;

        /**
         * Gets an iterator on the objects around a given actor.
         */
        ZoneIterator getAroundActorIterator(Actor *, int radius) const;

        /**
         * Gets an iterator on the objects around the old and new positions of
         * a character (including the ones that were but are now elsewhere).
         */
        ZoneIterator getAroundBeingIterator(Being *, int radius) const;

        /**
         * Gets everything related to the map.
         */
        const std::vector< Thing * > &getEverything() const;

    private:
        MapComposite(const MapComposite &);

        Map *mMap;            /**< Actual map. */
        MapContent *mContent; /**< Entities on the map. */
        Script *mScript;      /**< Script associated to this map. */
        std::string mName;    /**< Name of the map. */
        unsigned short mID;   /**< ID of the map. */

        PvPRules mPvPRules;
};

#endif
