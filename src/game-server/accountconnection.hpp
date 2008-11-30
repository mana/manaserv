/*
 *  The Mana World
 *  Copyright 2006 The Mana World Development Team
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
 */

#ifndef _TMW_ACCOUNTCONNECTION_H_
#define _TMW_ACCOUNTCONNECTION_H_

#include "net/messageout.hpp"
#include "net/connection.hpp"

class Character;

/** \fn void AccountConnection::syncChanges(bool force = false)
 *
 *  The gameserver holds a buffer with all changes made by a character. The
 *  changes are added at the time they occur. When the buffer reaches one of
 *  the following limits, the buffer is sent to the account server and applied
 *  to the database.
 *
 *  The sync buffer is sent when:
 *  - forced by any process (param force = true)
 *  - every 10 seconds
 *  - buffer reaches size of 1kb (defined in #SYNC_BUFFER_SIZE)
 *  - buffer holds more then 20 messages (defined in #SYNC_BUFFER_LIMIT)
 */
#define SYNC_BUFFER_SIZE    1024   /**< maximum size of sync buffer in bytes. */
#define SYNC_BUFFER_LIMIT     20   /**< maximum number of messages in sync buffer. */

/**
 * A connection to the account server.
 */
class AccountConnection : public Connection
{
    public:

        /**
        * Destructor
        */
        ~AccountConnection();

        /**
         * Initializes a connection to the account server described in the
         * configuration file. Registers the maps known by MapManager.
         */
        bool start();

        /**
         * Sends data of a given character.
         */
        void sendCharacterData(Character *);

        /**
         * Prepares the account server for a reconnecting player
         */
        void playerReconnectAccount(int id, std::string const &magic_token);

        /**
         * Requests the value of a quest variable from the database.
         */
        void requestQuestVar(Character *, std::string const &);

        /**
         * Pushes a new quest value to the database.
         */
        void updateQuestVar(Character *, std::string const &name,
                            std::string const &value);

        /**
         * Sends ban message.
         */
        void banCharacter(Character *, int);

        /**
         * Gathers statistics and sends them.
         */
        void sendStatistics();

        /**
         * Send letter
         */
        void sendPost(Character *, MessageIn &);

        /**
         * Get post
         */
        void getPost(Character *);

        /**
         * Change Account Level
         */
        void changeAccountLevel(Character *, int);

        /**
         * Sends all changed player data to the account server to minimize
         * dataloss due to failure of one server component.
         *
         * @param force Send changes even if buffer hasn't reached its size
         *              or message limit. (used to send in timed schedules)
         */
        void syncChanges(bool force = false);

        /**
         * Write a modification message about character points to the sync buffer.
         *
         * @param CharId      ID of the character
         * @param CharPoints  Number of character points left for the character
         * @param CorrPoints  Number of correction points left for the character
         * @param AttribId    ID of the modified attribute
         * @param AttribValue New value of the modified attribute
         */
        void updateCharacterPoints(const int CharId, const int CharPoints,
                                   const int CorrPoints, const int AttribId,
                                   const int AttribValue);


        /**
         * Write a modification message about character skills to the sync buffer.
         * @param CharId      ID of the character
         * @param SkillId     ID of the skill
         * @param SkillValue  new skill points
         */
        void updateExperience(const int CharId, const int SkillId,
                              const int SkillValue);

    protected:
        /**
         * Processes server messages.
         */
        virtual void processMessage(MessageIn &);

    private:

        MessageOut* mSyncBuffer;     /**< Message buffer to store sync data. */
        int mSyncMessages;           /**< Number of messages in the sync buffer. */

};

extern AccountConnection *accountHandler;

#endif
