/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <enet/enet.h>

#include "net/messageout.hpp"

/** Initial amount of bytes allocated for the messageout data buffer. */
const unsigned int INITIAL_DATA_CAPACITY = 16;

/** Factor by which the messageout data buffer is increased when too small. */
const unsigned int CAPACITY_GROW_FACTOR = 2;

MessageOut::MessageOut():
    mPos(0)
{
    mData = (char*) malloc(INITIAL_DATA_CAPACITY);
    mDataSize = INITIAL_DATA_CAPACITY;
}

MessageOut::MessageOut(int id):
    mPos(0)
{
    mData = (char*) malloc(INITIAL_DATA_CAPACITY);
    mDataSize = INITIAL_DATA_CAPACITY;

    writeShort(id);
}

MessageOut::~MessageOut()
{
    free(mData);
}

void MessageOut::clear()
{
    mData = (char *) realloc(mData, INITIAL_DATA_CAPACITY);
    mDataSize = INITIAL_DATA_CAPACITY;
    mPos = 0;
}

void
MessageOut::expand(size_t bytes)
{
    if (bytes > mDataSize)
    {
        do
        {
            mDataSize *= CAPACITY_GROW_FACTOR;
        }
        while (bytes > mDataSize);

        mData = (char*) realloc(mData, mDataSize);
    }
}

void MessageOut::writeByte(int value)
{
    expand(mPos + 1);
    mData[mPos] = value;
    mPos += 1;
}

void MessageOut::writeShort(int value)
{
    expand(mPos + 2);
    uint16_t t = ENET_HOST_TO_NET_16(value);
    memcpy(mData + mPos, &t, 2);
    mPos += 2;
}

void MessageOut::writeLong(int value)
{
    expand(mPos + 4);
    uint32_t t = ENET_HOST_TO_NET_32(value);
    memcpy(mData + mPos, &t, 4);
    mPos += 4;
}

void MessageOut::writeCoordinates(int x, int y)
{
    expand(mPos + 3);
    char *p = mData + mPos;
    p[0] = x & 0x00FF;
    p[1] = ((x & 0x0700) >> 8) | ((y & 0x001F) << 3);
    p[2] = (y & 0x07E0) >> 5;
    mPos += 3;
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

    if (length > stringLength)
    {
        // Pad remaining space with zeros
        memset(mData + mPos + stringLength, '\0', length - stringLength);
    }
    mPos += length;
}

std::ostream&
operator <<(std::ostream &os, const MessageOut &msg)
{
    if (msg.getLength() >= 2)
    {
        unsigned short id = ENET_NET_TO_HOST_16(*(short*) msg.mData);
        os << std::setw(6) << std::hex << std::showbase << std::internal
           << std::setfill('0') << id
           << std::dec << " (" << msg.getLength() << " B)";
    }
    else
    {
        os << "Unknown"
           << std::dec << " (" << msg.getLength() << " B)";
    }
    return os;
}
