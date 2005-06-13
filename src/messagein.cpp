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

/**
 * Constructor.
 */
MessageIn::MessageIn(Packet *p)
{
    pos = 0;
    packet = p;
}

/**
 * Destructor.
 */
MessageIn::~MessageIn()
{
    delete packet; // To be removed if the packet is deleted elsewhere.
}

/**< Reads a byte. */
char MessageIn::readByte()
{
    if (packet) // if Packet exists
    {
        if ( pos < (packet->length - 1) ) // if there is enough to read
        {
            pos++;
            return packet->data[pos - 1];
        }
        else
        {
            pos = packet->length - 1;
            return 0;
        }
    }
    return -1;
}

/**< Reads a short. */
short MessageIn::readShort()
{
    if (packet) // if Packet exists
    {
        if ( (pos + sizeof(short)) <= (packet->length - 1) )
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

/**< Reads a long. */
long MessageIn::readLong()
{
    if (packet) // if Packet exists
    {
        if ( (pos + sizeof(long)) <= (packet->length - 1) )
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

/**
 * Reads a string. If a length is not given (-1), it is assumed
 * that the length of the string is stored in a short at the
 * start of the string.
 */
std::string MessageIn::readString(int length)
{
    if (packet) // If there a good packet
    {
        int stringLength = 0;
        std::string readString = "";

        if ( length != -1 ) // if the length isn't specified, read it in the packet
        {
            // If the length of the packet is sufficient for us to read the length of the string
            if ( (pos + sizeof(short)) <= (packet->length - 1) )
            {
                // We first read the length of the string given by the short before it.
                stringLength = (short)SDLNet_Read16(&(packet->data[pos]));
                pos += sizeof(short);
            }
            else
            {
                // Place us to the end as the size is a short
                pos = packet->length - 1;
            }
        }
        else // if the length is specified
        {
                stringLength = length;
        }

        if ( stringLength > 0 )
        {
            if ( (pos + stringLength) <= (packet->length - 1) ) // If there's enough length to copy
            {
                char tempChar[stringLength+1];
                memcpy(&tempChar, packet->data, stringLength);
                readString = tempChar; // We first copy the entire char array
                // And then, we copy from the pos
                // to the stringLength.
                readString = readString.substr(pos,stringLength);
                pos += stringLength;
            }
            else // if there isn't, copy till the end
            {
                char tempChar[packet->length +1];
                memcpy(&tempChar, packet->data, packet->length - 1);
                readString = tempChar; // We first copy the entire char array
                // And then, we copy from the pos
                // to the length - 1 of the string.
                readString = readString.substr(pos,packet->length - 1 - pos);
                pos = packet->length - 1;
            }
        }
        else
        {
            // The length of the string is zero ?
        }
    } // End if there is a packet
    return "";
}