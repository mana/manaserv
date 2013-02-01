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

#ifndef ACTOR_H
#define ACTOR_H

#include "game-server/map.h"
#include "game-server/entity.h"
#include "utils/point.h"

/**
 * Flags that are raised as necessary. They trigger messages that are sent to
 * the clients.
 */
enum
{
    UPDATEFLAG_NEW_ON_MAP = 1,
    UPDATEFLAG_NEW_DESTINATION = 2,
    UPDATEFLAG_ATTACK = 4,
    UPDATEFLAG_ACTIONCHANGE = 8,
    UPDATEFLAG_LOOKSCHANGE = 16,
    UPDATEFLAG_DIRCHANGE = 32,
    UPDATEFLAG_HEALTHCHANGE = 64,
    UPDATEFLAG_EMOTE = 128
};

/**
 * Generic client-visible object. Keeps track of position, size and what to
 * update clients about.
 */
class Actor : public Entity
{
    public:
        Actor(EntityType type)
          : Entity(type),
            mMoveTime(0),
            mUpdateFlags(0),
            mPublicID(65535),
            mSize(0),
            mWalkMask(0)
        {}

        ~Actor();

        /**
         * Sets the coordinates. Also updates the walkmap of the map the actor
         * is on.
         *
         * @param p the coordinates.
         */
        void setPosition(const Point &p);

        /**
         * Gets the coordinates.
         *
         * @return the coordinates.
         */
        const Point &getPosition() const
        { return mPos; }

        /**
         * Gets what changed in the actor.
         */
        int getUpdateFlags() const
        { return mUpdateFlags; }

        /**
         * Sets some changes in the actor.
         */
        void raiseUpdateFlags(int n)
        { mUpdateFlags |= n; }

        /**
         * Clears changes in the actor.
         */
        void clearUpdateFlags()
        { mUpdateFlags = 0; }

        /**
         * Sets actor bounding circle radius.
         */
        void setSize(int s) { mSize = s; }
        int getSize() const { return mSize; }

        /**
         * Get public ID.
         *
         * @return the public ID, 65535 if none yet.
         */
        int getPublicID() const
        { return mPublicID; }

        /**
         * Set public ID. The actor shall not have any public ID yet.
         */
        void setPublicID(int id)
        { mPublicID = id; }

        bool isPublicIdValid() const
        { return (mPublicID > 0 && mPublicID != 65535); }

        void setWalkMask(unsigned char mask)
        { mWalkMask = mask; }

        /**
         * Gets the way the actor blocks pathfinding for other actors.
         */
        unsigned char getWalkMask() const
        { return mWalkMask; }

        /**
         * Overridden in order to update the walkmap.
         */
        virtual void setMap(MapComposite *map);

    protected:
        /**
         * Gets the way the actor blocks pathfinding for other actors.
         */
        virtual BlockType getBlockType() const
        { return BLOCKTYPE_NONE; }

        /** Delay until move to next tile in miliseconds. */
        unsigned short mMoveTime;

    private:
        char mUpdateFlags;          /**< Changes in actor status. */

        /** Actor ID sent to clients (unique with respect to the map). */
        unsigned short mPublicID;

        Point mPos;                 /**< Coordinates. */
        unsigned char mSize;        /**< Radius of bounding circle. */

        unsigned char mWalkMask;
};

#endif // ACTOR_H
