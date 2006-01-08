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


#ifndef _TMWSERV_OBJECT_H_
#define _TMWSERV_OBJECT_H_


#include <utility>
#include <string>


namespace tmwserv
{


/**
 * Structure type for the statistics.
 *
 * Notes:
 *     - the structure can be used to define stats modifiers (i.e. how an
 *       object would modify the stats of a being when it is equipped) or
 *       computed stats of a being.
 *     - the attributes are not unsigned to allow negative values.
 */
struct Statistics
{
    int health;
    int attack;
    int defense;
    int magic;
    int accuracy;
    int speed;
};


/**
 * Generic in-game object definition.
 * Base class for in-game objects.
 */
class Object
{
    public:
        /**
         * Default constructor.
         */
        Object(void);


        /**
         * Destructor.
         */
        virtual
        ~Object(void)
            throw();


        /**
         * Set the x coordinate.
         *
         * @param x the new x coordinate.
         */
        void
        setX(unsigned int x);


        /**
         * Get the x coordinate.
         *
         * @return the x coordinate.
         */
        unsigned int
        getX(void) const;


        /**
         * Set the y coordinate.
         *
         * @param y the new y coordinate.
         */
        void
        setY(unsigned int y);


        /**
         * Get the y coordinate.
         *
         * @return the y coordinate.
         */
        unsigned int
        getY(void) const;


        /**
         * Set the coordinates.
         *
         * @param x the x coordinate.
         * @param y the y coordinate.
         */
        void
        setXY(unsigned int x, unsigned int y);


        /**
         * Get the coordinates.
         *
         * @return the coordinates as a pair.
         */
        std::pair<unsigned int, unsigned int>
        getXY(void) const;


        /**
         * Set the statistics.
         *
         * @param stats the statistics.
         */
        void
        setStatistics(const Statistics& stats);


        /**
         * Get the statistics.
         *
         * @return the statistics.
         */
        Statistics&
        getStatistics(void);


        /**
         * Update the internal status.
         */
        virtual void
        update(void) = 0;

        /**
         * Get map name where being is
         *
         * @return Name of map being is located.
         */
        const unsigned int
        getMapId();

        /**
         * Set map being is located
         */
        void
        setMap(const unsigned int mapId);

    protected:
        Statistics mStats; /**< stats modifiers or computed stats */
        bool mNeedUpdate;  /**< update() must be invoked if true */


    private:
        unsigned int mX; /**< x coordinate */
        unsigned int mY; /**< y coordinate */

        unsigned int mMapId;  /**< id of the map being is on */
};


} // namespace tmwserv


#endif // _TMWSERV_OBJECT_H_
