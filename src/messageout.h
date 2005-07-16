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

#ifndef _TMW_SERVER_MESSAGEOUT_
#define _TMW_SERVER_MESSAGEOUT_

#include <string>

#include "packet.h"

class MessageOut
{
    public:
        /**
         * Constructor.
         */
        MessageOut();

        /**
         * Destructor.
         */
        ~MessageOut();

        void writeByte(char value);          /**< Reads a byte. */
        void writeShort(unsigned short value);/**< Reads a short. */
        void writeLong(unsigned long value); /**< Reads a long. */

        /**
         * Writes a string. If a fixed length is not given (-1), it is stored
         * as a short at the start of the string.
         */
        void writeString(const std::string &string, int length = -1);

        /**
         * Returns an instance of Packet derived from the written data. Use for
         * sending the packet.
         */
        const Packet *getPacket();

    private:
        Packet *packet;                      /**< Created packet. */
};

#endif
