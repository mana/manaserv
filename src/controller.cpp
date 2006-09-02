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

#include "controller.h"

#include "utils/logger.h"

Controller::Controller():
    mCountDown(0)
{
}

void Controller::possess(BeingPtr being)
{
    unPossess();

    mBeing = being;

    if (mBeing.get())
        mBeing->possessedBy(this);
}

void Controller::unPossess()
{
    if (mBeing.get())
        mBeing->possessedBy(NULL);

    mBeing = BeingPtr();
}

void Controller::update()
{
    /* Temporary "AI" behaviour that is purely artificial and not at all
     * intelligent.
     */
    if (mCountDown == 0)
    {
        if (mBeing.get())
        {
            Point randomPos = { rand() % 320 + 720,
                                rand() % 320 + 840 };

            LOG_INFO("Setting new random destination " << randomPos.x << ","
                    << randomPos.y << " for being " << mBeing->getPublicID(), 2);
            mBeing->setDestination(randomPos);
        }

        mCountDown = 10 + rand() % 10;
    }
    else
    {
        mCountDown--;
    }
}

