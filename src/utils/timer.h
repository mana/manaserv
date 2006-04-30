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

#include <sys/time.h>

/* I need a 64-bit unsigned integer */
#ifdef _MSC_VER
   typedef __uint64 uint64_t // when using MSVC use its internal type
#else
   #include <stdint.h> // on other compilers use the C99 official header
#endif

#ifdef _WIN32
    #include "wingettimeofday.h"
#endif

namespace tmwserv
{
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
        * returns the number of elapsed tics since last call
        */
        int poll();

        /**
        * activates the timer
        */
        void start();

        /**
        * deactivates the timer
        */
        void stop();

        /**
        * changes the interval between two pulses
        */
        void changeInterval (unsigned int newinterval);

    private:
        /**
        * calls gettimeofday() and converts it into milliseconds
        */       
        uint64_t getTimeInMillisec();

        /**
        * interval between two pulses
        */
        unsigned int interval;

        /**
        * the time the last pulse occured
        */
        uint64_t lastpulse;

        /**
        * activity status of the timer
        */
        bool active;
};

} // ::utils
} // ::tmwserv

#endif
