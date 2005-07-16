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
#include <iostream>
#include <cstdlib>

MessageOut::MessageOut():
    packet(0)
{
    packet = new Packet(NULL, 0);
}

MessageOut::~MessageOut()
{
    if (packet) {
        delete packet;
    }
}

void MessageOut::writeByte(char value)
{
    packet->expand(sizeof(char));
    packet->data[packet->length] = value;
    packet->length += sizeof(char);
}

void MessageOut::writeShort(short value)
{
    packet->expand(sizeof(short));
    memcpy(&packet->data[packet->length], (void*)&value, sizeof(short));
    packet->length += sizeof(short);
}

void MessageOut::writeLong(long value)
{
    packet->expand(sizeof(long));
    memcpy(&packet->data[packet->length], (void*)&value, sizeof(long));
    packet->length += sizeof(long);
}

void MessageOut::writeString(const std::string &string, int length)
{
    if (length < 0)
	length = string.length();

    packet->expand(length + sizeof(unsigned short));

    // length prefix
    memcpy(&packet->data[packet->length], (void*)&length, sizeof(unsigned short));
    // actual string
    memcpy(&packet->data[packet->length + sizeof(unsigned short)],
	   (void*)string.c_str(), length);

    packet->length += length + sizeof(unsigned short);
}

const Packet *MessageOut::getPacket()
{
    return packet;
}
