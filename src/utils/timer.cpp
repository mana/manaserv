/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
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

#include "timer.h"

#include <time.h>
#include <sys/time.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace utils
{

Timer::Timer(unsigned int ms, bool createActive)
{
    active = createActive;
    interval = ms;
    lastpulse = getTimeInMillisec();
}

void Timer::sleep()
{
    if (!active) return;
    uint64_t now = getTimeInMillisec();
    if (now - lastpulse >= interval) return;
#ifndef _WIN32
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = (interval - (now - lastpulse)) * (1000 * 1000);
    nanosleep(&req, 0);
#else
    Sleep(interval - (now - lastpulse));
#endif
}

int Timer::poll()
{
    int elapsed = 0;
    if (active)
    {
        uint64_t now = getTimeInMillisec();
        if (now > lastpulse)
        {
            elapsed = (now - lastpulse) / interval;
            lastpulse += interval * elapsed;
        }
        else
        {
            // Time has made a jump to the past. This should be a rare
            // occurence, so just reset lastpulse to prevent problems.
            lastpulse = now;
        }
    };
    return elapsed;
}

void Timer::start()
{
    active = true;
    lastpulse = getTimeInMillisec();
}

void Timer::stop()
{
    active = false;
}

void Timer::changeInterval(unsigned int newinterval)
{
    interval = newinterval;
}

uint64_t Timer::getTimeInMillisec()
{
    uint64_t timeInMillisec;
    timeval time;

    gettimeofday(&time, 0);
    timeInMillisec = (uint64_t)time.tv_sec * 1000 + time.tv_usec / 1000;
    return timeInMillisec;
}

} // ::utils
