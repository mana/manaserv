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

#include "messagein.h"

#include <string>

#include <enet/enet.h>

#include "packet.h"

MessageIn::MessageIn(Packet *packet):
    mPacket(packet),
    mPos(0)
{
    // Read the message ID
    mId = readShort();
}

MessageIn::~MessageIn()
{
    delete mPacket; // To be removed if the packet is deleted elsewhere.
}

char MessageIn::readByte()
{
    char value = -1;
    if (mPacket)
    {
        if (mPos < mPacket->length)
        {
            value = mPacket->data[mPos];
        }
        mPos += 1;
    }
    return value;
}

short MessageIn::readShort()
{
    short value = -1;
    if (mPacket)
    {
        if (mPos + 2 <= mPacket->length)
        {
            uint16_t t;
            memcpy(&t, mPacket->data + mPos, 2);
            value = ENET_NET_TO_HOST_16(t);
        }
        mPos += 2;
    }
    return value;
}

long MessageIn::readLong()
{
    long value = -1;
    if (mPacket)
    {
        if (mPos + 4 <= mPacket->length)
        {
            uint32_t t;
            memcpy(&t, mPacket->data + mPos, 4);
            value = ENET_NET_TO_HOST_32(t);
        }
        mPos += 4;
    }
    return value;
}

std::string MessageIn::readString(int length)
{
    if (mPacket)
    {
        // Get string length
        if (length < 0) {
            length = readShort();
        }

        // Make sure the string isn't erroneus
        if (length < 0 || mPos + length > mPacket->length) {
            mPos = mPacket->length + 1;
            return "";
        }

        // Read the string
        char const *stringBeg = mPacket->data + mPos,
                   *stringEnd = (char const *)memchr(stringBeg, '\0', length);
        std::string readString(stringBeg, stringEnd ? stringEnd - stringBeg : length);
        mPos += length;
        return readString;
    }

    return "";
}
