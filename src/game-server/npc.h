/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMESERVER_NPC_H
#define GAMESERVER_NPC_H

#include "game-server/being.h"
#include "scripting/script.h"

class Character;

/**
 * Class describing a non-player character.
 */
class NPC : public Being
{
    public:
        NPC(const std::string &name, int id);

        ~NPC();

        /**
         * Sets the function that should be called when this NPC is talked to.
         */
        void setTalkCallback(Script::Ref function);

        /**
         * Sets the function that should be called each update.
         */
        void setUpdateCallback(Script::Ref function);

        /**
         * Calls the update callback, if any.
         */
        void update();

        /**
         * Sets whether the NPC is enabled.
         */
        void setEnabled(bool enabled);

        /**
         * Prompts NPC.
         */
        void prompt(Character *, bool restart);

        /**
         * Selects an NPC proposition.
         */
        void select(Character *, int index);

        /**
         * The player has choosen an integer.
         */
        void integerReceived(Character *ch, int value);

        /**
         * The player has entered an string.
         */
        void stringReceived(Character *ch, const std::string &value);

        /**
         * Gets NPC ID.
         */
        int getNPC() const
        { return mID; }

    protected:
        /**
         * Gets the way a monster blocks pathfinding for other objects
         */
        virtual BlockType getBlockType() const
        { return BLOCKTYPE_CHARACTER; } // blocks like a player character

    private:
        unsigned short mID; /**< ID of the NPC. */
        bool mEnabled;      /**< Whether NPC is enabled */

        Script::Ref mTalkCallback;
        Script::Ref mUpdateCallback;
};

#endif // GAMESERVER_NPC_H
