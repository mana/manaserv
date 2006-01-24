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
 *
 *  $Id$
 */

#ifndef _TMWSERV_CHATHANDLER_H_
#define _TMWSERV_CHATHANDLER_H_

#include "messagehandler.h"
#include "netcomputer.h"
#include "messagein.h"

/**
 * Manages all chat related
 */
class ChatHandler : public MessageHandler
{
 public:
    /**
     * Receives chat related messages.
     */
    void receiveMessage(NetComputer &computer, MessageIn &message);

 private:
    /**
     * Deals with command messages
     */
    void handleCommand(NetComputer &computer, const std::string& command);

    /**
    * Tells the player to be more polite.
    */
    void warnPlayerAboutBadWords(NetComputer &computer);

    /**
    * Announce a message to every being in the default channel.
    */
    void announce(NetComputer &computer, const std::string& text);

    /**
    * Display a message to every player around one's player
    * in the default channel.
    * The tile area has been set to 10 for now.
    */
    void sayAround(NetComputer &computer, const std::string& text);

    /**
    * Say something private to a player.
    */
    void sayToPlayer(NetComputer &computer, const std::string& playerName, const std::string& text);

    /**
    * Say something in a specific channel.
    */
    void sayInChannel(NetComputer &computer, short channel, const std::string& text);
};

#endif
