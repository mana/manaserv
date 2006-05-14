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

#include <time.h>
#include "timer.h"

namespace tmwserv
{
namespace utils
{

Timer::Timer(unsigned int ms, bool createActive)
{
    active = createActive;
    interval = ms;
    lastpulse = getTimeInMillisec();
};

void Timer::sleep()
{
    if (!active) return;
    uint64_t now = getTimeInMillisec();
    if (now - lastpulse >= interval) return;
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = (interval - (now - lastpulse)) * (1000 * 1000);
    nanosleep(&req, 0);
}

int Timer::poll()
{
    int elapsed = 0;
    if (active)
    {
        elapsed = (getTimeInMillisec() - lastpulse) / interval;
        lastpulse += interval * elapsed;
    };
    return elapsed;
};

void Timer::start()
{
    active = true;
    lastpulse = getTimeInMillisec();
};

void Timer::stop()
{
    active = false;
};

void Timer::changeInterval(unsigned int newinterval)
{
    interval = newinterval;
};

uint64_t Timer::getTimeInMillisec()
{
    uint64_t timeInMillisec;
    timeval time;

    gettimeofday(&time, 0);
    timeInMillisec = (uint64_t)time.tv_sec * 1000 + time.tv_usec / 1000;
    return timeInMillisec;
};

} // ::utils
} // ::tmwserv
