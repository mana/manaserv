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

#ifndef _TMW_SERVER_MESSAGEIN_
#define _TMW_SERVER_MESSAGEIN_

#include "packet.h"
#include <string>

/**
 * A helper class to 
 */
class MessageIn
{
    public:
        /**
         * Constructor.
         */
        MessageIn(Packet *p);

        char readByte();              /**< Reads a byte. */
        int readShort();              /**< Reads a short. */
        int readLong();               /**< Reads a long. */

        /**
         * Reads a string. If a length is not given (-1), it is assumed
         * that the length of the string is stored in a short at the
         * start of the string.
         */
        std::string readString(int length = -1);

    private:
        Packet *packet;
        unsigned int pos;
};

#endif
