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

#include "utils/stringfilter.h"

#include "defines.h"
#include "common/configuration.hpp"
#include "utils/logger.h"

namespace utils
{

StringFilter::StringFilter():
    mInitialized(false)
{
    mSlangs.clear(); // Does this make any sense here?
    loadSlangFilterList();
}

StringFilter::~StringFilter()
{
    writeSlangFilterList();
    mSlangs.clear(); // Does this make any sense here?
}

bool StringFilter::loadSlangFilterList()
{
    mInitialized = false;

    std::string slangsList = Configuration::getValue("SlangsList", "");
    if (slangsList != "") {
        std::istringstream iss(slangsList);
        std::string tmp;
        while (getline(iss, tmp, ',')) {
            mSlangs.push_back(tmp);
        }
        mInitialized = true;
    }

    return mInitialized;
}

void StringFilter::writeSlangFilterList()
{
    // Write the list to config
    std::string slangsList = "";
    for (SlangIterator i = mSlangs.begin(); i != mSlangs.end(); )
    {
        slangsList += *i;
        ++i;
        if (i != mSlangs.end()) slangsList += ",";
    }
    //mConfig->setValue("SlangsList", slangsList);
}

bool StringFilter::filterContent(const std::string& text)
{
    if (!mInitialized) {
        LOG_DEBUG("Slangs List is not initialized.");
        return true;
    }

    bool isContentClean = true;
    std::string upperCaseText = text;

    std::transform(text.begin(), text.end(), upperCaseText.begin(),
            (int(*)(int))std::toupper);

    for (SlangIterator i = mSlangs.begin(); i != mSlangs.end(); ++i)
    {
        // We look for slangs into the sentence.
        std::string upperCaseSlang = *i;
        std::transform(upperCaseSlang.begin(), upperCaseSlang.end(),
                upperCaseSlang.begin(), (int(*)(int))std::toupper);

        if (upperCaseText.compare(upperCaseSlang)) {
            isContentClean = false;
            break;
        }
    }

    return isContentClean;
}

bool StringFilter::isEmailValid(const std::string& email)
{
    // Testing Email validity
    if ((email.length() < MIN_EMAIL_LENGTH) ||
            (email.length() > MAX_EMAIL_LENGTH))
    {
        return false;
    }

    std::string::size_type atpos = email.find_first_of('@');

    // TODO Find some nice regex for this...
    return (atpos != std::string::npos) &&
        (email.find_first_of('.', atpos) != std::string::npos) &&
        (email.find_first_of(' ') == std::string::npos);
}

bool StringFilter::findDoubleQuotes(const std::string& text)
{
    return (text.find('"', 0) != std::string::npos);
}

} // ::utils
