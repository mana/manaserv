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

#ifndef _TMW_SERVER_NETCOMPUTER_

// Forward declaration
class NetSession;

/**
 * This class represents a known computer on the network. For example a
 * connected client or a server we're connected to.
 */
class NetComputer
{
    public:
        /**
         * Constructor.
         */
        NetComputer(NetSession *session);

        /**
         * Returns the netsession that the computer is attached to.
         */
        NetSession *getSession();

        /**
         * Returns <code>true</code> if this computer is disconnected.
         */
        bool isDisconnected();

        /**
         * Disconnects the computer from the server.
         */
        void disconnect(const std::string &reason);

        /**
         * Sends a packet to this computer.
         * 
         * Note: When we'd want to allow communication through UDP, we could
         *  introduce the reliable argument, which would could a UDP message
         *  to be sent when set to false.
         */
        void send(Packet *p);
        //void send(Packet *p, bool reliable = true);
};

#endif
