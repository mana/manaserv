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
 *
 *  $Id$
 */

#include "netcomputer.h"

#include "chatchannelmanager.h"
#include "packet.h"
#include "state.h"

NetComputer::NetComputer(ConnectionHandler *handler, TCPsocket sock):
    handler(handler),
    socket(sock),
    mAccountPtr(NULL),
    mCharacterPtr(NULL)
{
}

NetComputer::~NetComputer()
{
    unsetAccount();
}

void NetComputer::disconnect(const std::string &reason)
{
    // Somehow notify the netsession listener about the disconnect after
    // sending this computer a disconnect message containing the reason.
}

void NetComputer::send(const Packet *p)
{
    SDLNet_TCP_Send(socket, p->data, p->length);
}

void NetComputer::setAccount(tmwserv::AccountPtr acc)
{
    mAccountPtr = acc;
}

void NetComputer::setCharacter(tmwserv::BeingPtr ch)
{
    tmwserv::State &state = tmwserv::State::instance();
    if (mCharacterPtr.get() != NULL)
    {
        // Remove being from the world.
        unsetCharacter();
    }
    mCharacterPtr = ch;
    state.addBeing(mCharacterPtr, mCharacterPtr->getMapId());
}

void NetComputer::unsetAccount()
{
    unsetCharacter();
    mAccountPtr = tmwserv::AccountPtr(NULL);
}

void NetComputer::unsetCharacter()
{
    // remove being from world
    tmwserv::State &state = tmwserv::State::instance();
    state.removeBeing(mCharacterPtr);
    chatChannelManager->removeUserFromEveryChannels(mCharacterPtr);
    mCharacterPtr = tmwserv::BeingPtr(NULL);
}

