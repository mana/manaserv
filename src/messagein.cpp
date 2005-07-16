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
#include <SDL_net.h>

MessageIn::MessageIn(Packet *p)
{
    pos = 0;
    packet = p;
}

MessageIn::~MessageIn()
{
    delete packet; // To be removed if the packet is deleted elsewhere.
}

char MessageIn::readByte()
{
    if (packet) // if Packet exists
    {
        if ( pos < packet->length ) // if there is enough to read
        {
            return packet->data[pos++];
        }
        else
        {
            pos = packet->length - 1;
            return 0;
        }
    }
    return -1;
}

short MessageIn::readShort()
{
    if (packet) // if Packet exists
    {
        if ( (pos + sizeof(short)) <= packet->length )
        {
            pos += sizeof(short);
            return (short) SDLNet_Read16(&(packet->data[pos - sizeof(short)]));
        }
        else
        {
            // We do nothing. And keep what's left to be read by a function 
            // that requires less length.
        }
    }
    return -1;
}

long MessageIn::readLong()
{
    if (packet) // if Packet exists
    {
        if ( (pos + sizeof(long)) <= packet->length )
        {
            pos += sizeof(long);
            return (long) SDLNet_Read32(&(packet->data[pos - sizeof(long)]));
        }
        else
        {
            // We do nothing. And keep what's left to be read by a function 
            // that requires less length.
        }
    }
    return -1;
}

std::string MessageIn::readString(int length)
{
    int stringLength = 0;
    std::string readString = "";

    if (packet) {
	// get string length
	if (length < 0) {
	    stringLength = (short) packet->data[pos];
	    pos += sizeof(short);
	} else {
	    stringLength = length;
	}

	// make sure the string isn't erroneus
	if (pos + length > packet->length) {
	    return "";
	}

	// read the string
	char *tmpString = new char[stringLength + 1];
	memcpy(tmpString, (void*)&packet->data[pos], stringLength);
	tmpString[stringLength] = 0;
	pos += stringLength;

	readString = tmpString;
	delete tmpString;
    } else {
	return "";
    }

    return readString;
}
