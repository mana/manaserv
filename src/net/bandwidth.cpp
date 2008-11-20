/*
 *  The Mana World Server
 *  Copyright 2008 The Mana World Development Team
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

#include "bandwidth.hpp"

#include "netcomputer.hpp"

BandwidthMonitor::BandwidthMonitor():
    mAmountServerOutput(0),
    mAmountServerInput(0),
    mAmountClientOutput(0),
    mAmountClientInput(0)
{
}

void BandwidthMonitor::increaseInterServerOutput(int size)
{
    mAmountServerOutput += size;
}

void BandwidthMonitor::increaseInterServerInput(int size)
{
    mAmountServerInput += size;
}

void BandwidthMonitor::increaseClientOutput(NetComputer *nc, int size)
{
    mAmountClientOutput += size;
    // look for an existing client stored
    ClientBandwidth::iterator itr = mClientBandwidth.find(nc);

    // if there isnt one, create one
    if (itr == mClientBandwidth.end())
    {
        std::pair<ClientBandwidth::iterator, bool> retItr;
        retItr = mClientBandwidth.insert(std::pair<NetComputer*, std::pair<int, int> >(nc, std::pair<int, int>(0, 0)));
        itr = retItr.first;
    }

    itr->second.first += size;

}

void BandwidthMonitor::increaseClientInput(NetComputer *nc, int size)
{
    mAmountClientInput += size;

    // look for an existing client stored
    ClientBandwidth::iterator itr = mClientBandwidth.find(nc);

    // if there isnt one, create it
    if (itr == mClientBandwidth.end())
    {
        std::pair<ClientBandwidth::iterator, bool> retItr;
        retItr = mClientBandwidth.insert(std::pair<NetComputer*, std::pair<int, int> >(nc, std::pair<int, int>(0, 0)));
        itr = retItr.first;
    }

    itr->second.second += size;
}

