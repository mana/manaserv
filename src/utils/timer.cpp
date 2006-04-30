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
 */

#include "timer.h"

namespace tmwserv
{
namespace utils
{

Timer::Timer(signed int ms, bool createActive)
{
    active = createActive;
    interval = (ms * CLOCKS_PER_SEC) / 1000;
    nextpulse = clock() + interval;
};

bool Timer::poll()
{
    if (!active) return false;

    if (nextpulse < clock())
    {
        nextpulse += interval;
        return true;
    }
    else {
        return false;
    };

};

void Timer::start()
{
    active = true;
    nextpulse = clock() + interval;
};

void Timer::stop()
{
    active = false;
};

void Timer::changeInterval(signed int ms)
{
    signed int newinterval = ms * CLOCKS_PER_SEC / 1000;
    nextpulse = nextpulse - interval + newinterval;
    interval = newinterval;
};


} // ::utils
} // ::tmwserv
