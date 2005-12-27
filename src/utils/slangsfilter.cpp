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

#include "slangsfilter.h"

namespace tmwserv
{
namespace utils
{

/**
 * The slang table. Don't be surprised. We need to know about bad words in order
 * to keep them out of the players' conversations.
*/
std::string slangs[] = {
"fuck","shit","slut","whore","bitch",
"END" // This is the terminator, don't remove it.
};

bool filterContent(std::string text)
{
    bool good = true;
    unsigned int i = 0;
    while ( !(slangs[i] == "END") )
    {
        for (unsigned int j = 0; j <= text.length(); j++)
        {
            // We look for slangs into the sentence.
            std::string upcasedText = text;
            std::string upcasedSlang = slangs[i];
            std::transform(upcasedText.begin(), upcasedText.end(), upcasedText.begin(),
                (int(*)(int))std::toupper);
            std::transform(upcasedSlang.begin(), upcasedSlang.end(), upcasedSlang.begin(),
                (int(*)(int))std::toupper);
            if ( upcasedText.substr(j, upcasedSlang.length()) == upcasedSlang )
            {
                good = false;
            }
        }
        // Next slang
        i++;
    }

    return good;
}

} // ::utils
} // ::tmwserv
