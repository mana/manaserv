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

#ifndef _TMWSERV_TIMER_H_
#define _TMWSERV_TIMER_H_

/* I need a 64-bit unsigned integer */
#ifdef _MSC_VER
   typedef __uint64 uint64_t // when using MSVC use its internal type
#else
   #include <stdint.h> // on other compilers use the C99 official header
#endif

namespace utils
{

/**
 * This class is for timing purpose as a replacement for SDL_TIMER
 */
class Timer
{
    public:
        /**
         * Constructor.
         */
        Timer(unsigned int ms, bool createActive = true);

        /**
         * Returns the number of elapsed ticks since last call.
         */
        int poll();

        /**
         * Sleeps till the next tick occurs.
         */
        void sleep();

        /**
         * Activates the timer.
         */
        void start();

        /**
         * Deactivates the timer.
         */
        void stop();

        /**
         * Changes the interval between two pulses.
         */
        void changeInterval (unsigned int newinterval);

    private:
        /**
         * Calls gettimeofday() and converts it into milliseconds.
         */
        uint64_t getTimeInMillisec();

        /**
         * Interval between two pulses.
         */
        unsigned int interval;

        /**
         * The time the last pulse occured.
         */
        uint64_t lastpulse;

        /**
         * Activity status of the timer.
         */
        bool active;
};

} // ::utils

#endif
