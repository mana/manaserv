/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */


#ifndef _TMWSERV_CIPHER_H_
#define _TMWSERV_CIPHER_H_


#include <string>

#include "singleton.h"


namespace tmwserv
{
namespace utils
{


/**
 * A helper class for the encoding of strings using different algorithms.
 *
 * Notes:
 *     - this class implements the Meyer's singleton design pattern.
 *     - this class uses the OpenSSL's crypto library.
 */
class Cipher: public Singleton<Cipher>
{
    // friend so that Singleton can call the constructor.
    friend class Singleton<Cipher>;


    public:
        /**
         * Encode using the MD5 digest algorithm.
         *
         * @param str the string to encode.
         *
         * @return the MD5 digest hash string.
         */
        std::string
        md5(const std::string& str);


        /**
         * Add encryption algorithms here.
         */


    private:
        /**
         * Default constructor.
         */
        Cipher(void)
            throw();


        /**
         * Destructor.
         */
        ~Cipher(void)
            throw();


        /**
         * Copy constructor.
         */
        Cipher(const Cipher& rhs);


        /**
         * Assignment operator.
         */
        Cipher&
        operator=(const Cipher& rhs);


        /**
         * Convert a string into hexadecimal.
         *
         * @param str the string to convert.
         * @param length the string length.
         *
         * @return the hexadecimal string.
         */
        std::string
        toHex(const unsigned char* str,
              const unsigned short length);
};


} // namespace utils
} // namespace tmwserv


#endif // _TMWSERV_CIPHER_H_
