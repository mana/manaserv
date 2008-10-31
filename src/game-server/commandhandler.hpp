/*
 *  The Mana World
 *  Copyright 2008 The Mana World Development Team
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

#ifndef _TMW_SERVER_COMMANDHANDLER_
#define _TMW_SERVER_COMMANDHANDLER_

#include <string>

class Character;

/**
 * A class to parse and handle user commands
 */
class CommandHandler
{
    public:
        /**
         * Constructor
         */
        CommandHandler() {}

        /**
         * Destructor
         */
        ~CommandHandler() {}

        /**
         * Parse and handle the given command.
         */
        void handleCommand(Character *player, const std::string &command);

    private:
        void handleHelp(Character *player, std::string &args);
        void handleWarp(Character *player, std::string &args);
        void handleItem(Character *player, std::string &args);
        void handleDrop(Character *player, std::string &args);
        void handleMoney(Character *player, std::string &args);
        void handleSpawn(Character *player, std::string &args);
        void handleGoto(Character *player, std::string &args);
        void handleRecall(Character *player, std::string &args);
        void handleReload(Character *player);
        void handleBan(Character *player, std::string &args);
        void handleLevel(Character *player, std::string &args);
        void handleAttribute(Character *player, std::string &args);

        void errorMsg(const std::string error, Character *player);
        std::string getArgument(std::string &args);
        Character* getPlayer(const std::string &player);
};

#endif //_TMW_COMMANDHANDLER_H
