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

#ifndef MESSAGEIN_H
#define MESSAGEIN_H

#include <iosfwd>

class Packet;

/**
 * Used for parsing an incoming message.
 */
class MessageIn
{
    public:
        /**
         * Constructor.
         */
        MessageIn(const char *data, int length);

        /**
         * Returns the message ID.
         */
        int
        getId() const { return mId; }

        /**
         * Returns the total length of this message.
         */
        int
        getLength() const { return mLength; }

        int readByte();             /**< Reads a byte. */
        int readShort();            /**< Reads a short. */
        int readLong();             /**< Reads a long. */

        /**
         * Reads a string. If a length is not given (-1), it is assumed
         * that the length of the string is stored in a short at the
         * start of the string.
         */
        std::string
        readString(int length = -1);

        /**
         * Returns the length of unread data.
         */
        int
        getUnreadLength() const { return mLength - mPos; }

        /**
         * Returns
         */

    private:
        const char *mData;            /**< Packet data */
        unsigned short mLength;       /**< Length of data in bytes */
        unsigned short mId;           /**< The message ID. */

        /**
         * Actual position in the packet. From 0 to packet->length. A value
         * bigger than packet->length means EOP was reached when reading it.
         */
        unsigned short mPos;

        /**
         * Streams message ID and length to the given output stream.
         */
        friend std::ostream& operator <<(std::ostream &os,
                                         const MessageIn &msg);
};

#endif
