/*
 *  The Mana Server
 *  Copyright (C) 2010  The Mana Development Team
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

#include "attack.h"

void Attacks::add(const Attack &attack)
{
    mAttacks.push_back(attack);
    // Slow, but safe.
    mAttacks.sort();
}

void Attacks::clear()
{
    mAttacks.clear();
}

void Attacks::stop()
{
    for (std::list<Attack>::iterator it = mAttacks.begin();
         it != mAttacks.end(); ++it)
    {
        it->halt();
    }
    mActive = false;
}

void Attacks::start()
{
    for (std::list<Attack>::iterator it = mAttacks.begin();
         it != mAttacks.end(); ++it)
    {
        // If the attack is inactive, we hard reset it.
        if (!it->getTimer())
            it->reset();
        else
            it->softReset();
    }
    mActive = true;
}

void Attacks::tick(std::list<Attack> *ret)
{
    for (std::list<Attack>::iterator it = mAttacks.begin();
         it != mAttacks.end(); ++it)
    {
        if (it->tick())
        {
            if (mActive)
                it->reset();
            else
                it->halt();
        }

        if (ret && it->isReady())
        {
            ret->push_back(*it);
        }
    }
}
