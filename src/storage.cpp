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

#include "storage.h"

#include "dalstorage.h"

// initialize the static attributes.
Storage* Storage::mInstance = 0;
std::string Storage::mName("");
std::string Storage::mUser("");
std::string Storage::mPassword("");


/**
 * Constructor.
 */
Storage::Storage(void)
    throw()
{
    // NOOP
}


/**
 * Destructor.
 */
Storage::~Storage(void)
    throw()
{
    // NOOP
}


/**
 * Create an instance of Storage.
 */
Storage&
Storage::instance(const std::string& name)
{
    if (mInstance == 0) {
        mInstance = new DALStorage();

        // set the name of the storage.
        mName = name;
    }

    return (*mInstance);
}


/**
 * Delete the instance.
 */
void
Storage::destroy(void)
{
    if (mInstance != 0) {
        delete mInstance;
        mInstance = 0;
    }

    // reset the attributes.
    mName = "";
    mUser = "";
    mPassword = "";
}


/**
 * Check if the storage is open.
 */
bool
Storage::isOpen(void) const
{
    return mIsOpen;
}


/**
 * Get the storage name.
 */
const std::string&
Storage::getName(void) const
{
    return mName;
}


/**
 * Set a user name for the storage.
 */
void
Storage::setUser(const std::string& userName)
{
    mUser = userName;
}


/**
 * Get the user name.
 */
const std::string&
Storage::getUser(void) const
{
    return mUser;
}


/**
 * Set a user password for the storage.
 */
void
Storage::setPassword(const std::string& password)
{
    mPassword = password;
}


/**
 * Get the user password.
 */
const std::string&
Storage::getPassword(void) const
{
    return mPassword;
}
