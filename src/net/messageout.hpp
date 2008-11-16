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
 */

#ifndef _TMWSERV_MESSAGEOUT_H_
#define _TMWSERV_MESSAGEOUT_H_

#include <iosfwd>

/**
 * Used for building an outgoing message.
 */
class MessageOut
{
    public:
        /**
         * Constructor.
         */
        MessageOut();

        /**
         * Constructor that takes a message ID.
         */
        MessageOut(int id);

        /**
         * Destructor.
         */
        ~MessageOut();

        /**
         * Clears current message.
         */
        void clear();

        void writeByte(int value);     /**< Writes an integer on one byte. */

        void writeShort(int value);    /**< Writes an integer on two bytes. */

        void writeLong(int value);     /**< Writes an integer on four bytes. */

        /**
         * Writes a 3-byte block containing tile-based coordinates.
         */
        void writeCoordinates(int x, int y);

        /**
         * Writes a string. If a fixed length is not given (-1), it is stored
         * as a short at the start of the string.
         */
        void
        writeString(const std::string &string, int length = -1);

        /**
         * Returns the content of the message.
         */
        char*
        getData() const { return mData; }

        /**
         * Returns the length of the data.
         */
        unsigned int
        getLength() const { return mPos; }

    private:
        /**
         * Ensures the capacity of the data buffer is large enough to hold the
         * given amount of bytes.
         */
        void
        expand(size_t size);

        char *mData;                         /**< Data building up. */
        unsigned int mPos;                   /**< Position in the data. */
        unsigned int mDataSize;              /**< Allocated datasize. */

        /**
         * Streams message ID and length to the given output stream.
         */
        friend std::ostream& operator <<(std::ostream &os,
                                         const MessageOut &msg);
};

#endif //_TMWSERV_MESSAGEOUT_H_
