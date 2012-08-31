/*
 *  The Mana Server
 *  Copyright (C) 2006-2010  The Mana World Development Team
 *  Copyright (C) 2010-2011  The Mana Development Team
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

#ifndef SERVER_MAPCOMPOSITE_H
#define SERVER_MAPCOMPOSITE_H

#include <string>
#include <vector>
#include <map>

#include "scripting/script.h"

class Actor;
class Being;
class Character;
class Map;
class MapComposite;
class Point;
class Rectangle;
class Entity;

struct MapContent;
struct MapZone;

enum PvPRules
{
    PVP_NONE = 0,   // no PvP on this map
    PVP_FREE        // unrestricted PvP on this map
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
     * Entities (items, characters, monsters, etc) located on the map.
     */
    std::vector< Entity * > entities;

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
        MapComposite(int id, const std::string &name);
        ~MapComposite();

        bool readMap();

        /**
         * Loads the map and initializes the map content. Should only be called
         * once!
         *
         * @return <code>true</code> when succesful, <code>false</code> when
         *         an error occurred.
         */
        bool activate();

        /**
         * Gets the underlying pathfinding map.
         */
        Map *getMap() const
        { return mMap; }

        /**
         * Returns whether the map is active on this server or not.
         */
        bool isActive() const
        { return mActive; }

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
        bool insert(Entity *);

        /**
         * Removes a thing from the map.
         */
        void remove(Entity *);

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
        const std::vector< Entity * > &getEverything() const;

        /**
         * Gets the cached value of a map-bound script variable
         */
        std::string getVariable(const std::string &key) const;

        /**
         * Changes a script variable and notifies the database server
         * about the change
         */
        void setVariable(const std::string &key, const std::string &value);

        /**
         * Changes a script variable without notifying the database server
         * about the change
         */
        void setVariableFromDbserver (const std::string &key,
                                      const std::string &value)
        { mScriptVariables[key] = value; }

        /**
         * Sets callback for map variable
         */
        void setMapVariableCallback(const std::string &key, Script *script)
        { script->assignCallback(mMapVariableCallbacks[key]); }

        /**
         * Sets callback for global variable
         */
        void setWorldVariableCallback(const std::string &key, Script *script)
        { script->assignCallback(mWorldVariableCallbacks[key]); }

        void callWorldVariableCallback(const std::string &key,
                                       const std::string &value);

        static void setInitializeCallback(Script *script)
        { script->assignCallback(mInitializeCallback); }

        static void setUpdateCallback(Script *script)
        { script->assignCallback(mUpdateCallback); }

    private:
        MapComposite(const MapComposite &);

        void initializeContent();
        void callMapVariableCallback(const std::string &key,
                                     const std::string &value);

        bool mActive;         /**< Status of map. */
        Map *mMap;            /**< Actual map. */
        MapContent *mContent; /**< Entities on the map. */
        std::string mName;    /**< Name of the map. */
        unsigned short mID;   /**< ID of the map. */
        /** Cached persistent variables */
        std::map<std::string, std::string> mScriptVariables;
        PvPRules mPvPRules;
        std::map<const std::string, Script::Ref> mMapVariableCallbacks;
        std::map<const std::string, Script::Ref> mWorldVariableCallbacks;

        static Script::Ref mInitializeCallback;
        static Script::Ref mUpdateCallback;
};

#endif
