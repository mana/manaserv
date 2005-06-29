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


#include "object.h"


namespace tmwserv
{


/**
 * Default constructor.
 */
Object::Object(void)
        : mX(0),
          mY(0)
{
    mStats.health = 0;
    mStats.attack = 0;
    mStats.defense = 0;
    mStats.magic = 0;
    mStats.accuracy = 0;
    mStats.speed = 0;
}


/**
 * Destructor.
 */
Object::~Object(void)
    throw()
{
    // NOOP
}


/**
 * Set the x coordinate.
 */
void
Object::setX(unsigned int x)
{
    mX = x;
}


/**
 * Get the x coordinate.
 */
unsigned int
Object::getX(void) const
{
    return mX;
}


/**
 * Set the y coordinate.
 */
void
Object::setY(unsigned int y)
{
    mY = y;
}


/**
 * Get the y coordinate.
 */
unsigned int
Object::getY(void) const
{
    return mY;
}


/**
 * Set the statistics.
 */
void
Object::setStatistics(const Statistics& stats)
{
    mStats = stats;
}


/**
 * Get the statistics.
 */
Statistics&
Object::getStatistics(void)
{
    return mStats;
}


} // namespace tmwserv
