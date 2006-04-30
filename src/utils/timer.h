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

#include <time.h>

namespace tmwserv
{
namespace utils
{

/**
 * This class is for timing purpose as a replacement for SDL_TIMER
 * connections from and connecting to other computers.
 */

class Timer
{
    public:
        /**
        * Constructor.
        */
        Timer(signed int ms, bool createActive = true);

        /**
        * checks if the desired time has passed
        * returns true if yes and false if not
        */
        bool poll();

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
        void changeInterval (signed int ms);

    private:
        /**
        * interval between two pulses
        */
        signed int interval;   

        /**
        * time for next pulse
        */
        signed int nextpulse;

        /**
        * activity status of the timer
        */
        bool active;
};

} // ::utils
} // ::tmwserv

#endif
