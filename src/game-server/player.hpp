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

#ifndef _TMWSERV_PLAYER_H_
#define _TMWSERV_PLAYER_H_

#include <string>
#include <vector>

#include "playerdata.hpp"
#include "game-server/being.hpp"

class GameClient;

/**
 * Actions for a player being.
 */
enum
{
    PLAYER_STAND = 0,
    PLAYER_SIT,
    PLAYER_ATTACK
};

/**
 * Stores the data of a remote client.
 */
class Player : public Being, public PlayerData
{
    public:

        Player(std::string const &name, int id = -1)
          : Being(OBJECT_PLAYER, 65535),
            PlayerData(name, id),
            mClient(NULL),
            mAction(PLAYER_STAND)
        {}

        /**
         * Updates the internal status.
         */
        void update();

        /**
         * Sets next action.
         **/
        void setAction(int s)
        { mAction = s; }

        /**
         * Gets next action.
         **/
        int getAction() const
        { return mAction; }

        /**
         * Gets client computer.
         */
        GameClient *getClient() const
        { return mClient; }

        /**
         * Sets client computer.
         */
        void setClient(GameClient *c)
        { mClient = c; }

    private:
        Player(Player const &);
        Player &operator=(Player const &);

        GameClient *mClient;   /**< Client computer. */
        unsigned char mAction; /**< Player state. */
};

#endif // _TMWSERV_PLAYER_H_
