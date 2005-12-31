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
#include "logger.h"

namespace tmwserv
{
namespace utils
{

SlangsFilter::SlangsFilter(Configuration *config)
    : mInitialized(false),
      mConfig(config)
{
    mSlangs.clear();
    loadFilterList();
}

SlangsFilter::~SlangsFilter()
{
    writeFilterList();
    mSlangs.clear();
}

bool SlangsFilter::loadFilterList()
{
    mInitialized = false;
    std::string slangsList = mConfig->getValue("SlangsList", "");
    if ( slangsList != "")
    {
        // Getting the words from the list.
        unsigned int i = 0; // this is the latest comma position keeper
        for (unsigned int j = 0; j < slangsList.length(); j++)
        {
            if (slangsList[j] == ',')
            {
                if (i == 0)
                    mSlangs.push_back(slangsList.substr(i, j-i));
                else
                    mSlangs.push_back(slangsList.substr(i+1, j-i-1));

                i = j;
            }
        }
        // Getting the last word
        mSlangs.push_back(slangsList.substr(i+1, slangsList.length() - 1));
        mInitialized = true;
        return true;
    }
    return false;
}

void SlangsFilter::writeFilterList()
{
    // Write the list to config
    std::string slangsList = "";
    for (std::list<std::string>::iterator i = mSlangs.begin(); i != mSlangs.end(); )
    {
        slangsList += *i;
        ++i;
        if (i != mSlangs.end()) slangsList += ",";
    }
    //mConfig->setValue("SlangsList", slangsList);
}

bool SlangsFilter::filterContent(const std::string& text)
{
    if (mInitialized)
    {
        bool isContentClean = true;

        for (std::list<std::string>::iterator i = mSlangs.begin(); i != mSlangs.end(); )
        {
            // We look for slangs into the sentence.
            std::string upcasedText = text;
            std::string upcasedSlang = *i;
            std::transform(upcasedText.begin(), upcasedText.end(), upcasedText.begin(),
                (int(*)(int))std::toupper);
            std::transform(upcasedSlang.begin(), upcasedSlang.end(), upcasedSlang.begin(),
                (int(*)(int))std::toupper);

            for ( unsigned int j = 0; j < text.length(); j++)
            {
                if ( upcasedText.substr(j, upcasedSlang.length()) == upcasedSlang )
                {
                    isContentClean = false;
                    break;
                }
            }
            if (!isContentClean) break;
            ++i;
        }
        return isContentClean;
    }
    else
    {
        return true;
        LOG_INFO("Slangs List is not initialized.", 2)
    }
}

} // ::utils
} // ::tmwserv
