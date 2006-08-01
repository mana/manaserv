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

#include "messageout.h"

#include <string>

#include <enet/enet.h>

#include "packet.h"

MessageOut::MessageOut():
    mPacket(0),
    mData(0),
    mDataSize(0),
    mPos(0)
{
}

MessageOut::MessageOut(short id):
    mPacket(0),
    mData(0),
    mDataSize(0),
    mPos(0)
{
    writeShort(id);
}

MessageOut::~MessageOut()
{
    if (mPacket) {
        delete mPacket;
    }

    if (mData) {
        free(mData);
    }
}

void
MessageOut::expand(size_t bytes)
{
    mData = (char*)realloc(mData, bytes);
    mDataSize = bytes;
}

void
MessageOut::writeByte(char value)
{
    expand(mPos + 1);
    mData[mPos] = value;
    mPos += 1;
}

void MessageOut::writeShort(short value)
{
    expand(mPos + 2);
    uint16_t t = ENET_HOST_TO_NET_16(value);
    memcpy(mData + mPos, &t, 2);
    mPos += 2;
}

void
MessageOut::writeLong(long value)
{
    expand(mPos + 4);
    uint32_t t = ENET_HOST_TO_NET_32(value);
    memcpy(mData + mPos, &t, 4);
    mPos += 4;
}

void
MessageOut::writeString(const std::string &string, int length)
{
    int stringLength = string.length();
    if (length < 0)
    {
        // Write the length at the start if not fixed
        writeShort(stringLength);
        length = stringLength;
    }
    else if (length < stringLength)
    {
        // Make sure the length of the string is no longer than specified
        stringLength = length;
    }
    expand(mPos + length);

    // Write the actual string
    memcpy(mData + mPos, string.c_str(), stringLength);

    // Pad remaining space with zeros
    if (length > stringLength)
    {
        memset(mData + mPos + stringLength, '\0', length - stringLength);
    }
    mPos += length;
}

const Packet*
MessageOut::getPacket()
{
    if (!mPacket)
    {
        mPacket = new Packet(mData, mDataSize);
    }

    return mPacket;
}

char*
MessageOut::getData()
{
    return mData;
}

unsigned int
MessageOut::getDataSize()
{
    return mDataSize;
}
