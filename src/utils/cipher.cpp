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

#include "cipher.h"

#include <iomanip>
#include <sstream>

#include <openssl/evp.h>
#include <openssl/md5.h>

namespace tmwserv
{
namespace utils
{


/**
 * Default constructor.
 */
Cipher::Cipher(void)
    throw()
{
    // NOOP
}


/**
 * Destructor.
 */
Cipher::~Cipher(void)
    throw()
{
    // NOOP
}


/**
 * Encode using the MD5 digest algorithm.
 */
std::string
Cipher::md5(const std::string& str)
{
    unsigned char md[MD5_DIGEST_LENGTH];

    EVP_Digest(
        (unsigned char*) str.c_str(),
        (unsigned long) str.length(),
        md,
        NULL,
        EVP_md5(),
        NULL
    );

    return toHex(md, MD5_DIGEST_LENGTH);
}


/**
 * Convert a string into hexadecimal.
 */
std::string
Cipher::toHex(const unsigned char* str,
              const unsigned short length)
{
    using namespace std;

    ostringstream os;

    for (int i = 0; i < length; ++i) {
       os << setfill('0') << setw(2) << hex << (int) str[i];
    }

    return os.str();
}


} // namespace utils
} // namespace tmwserv
