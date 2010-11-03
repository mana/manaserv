/*
 *  The Mana Server
 *  Copyright (C) 2006-2010  The Mana World Development Team
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

#include "game-server/accountconnection.hpp"

#include "common/configuration.hpp"
#include "game-server/character.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/postman.hpp"
#include "game-server/quest.hpp"
#include "game-server/state.hpp"
#include "net/messagein.hpp"
#include "serialize/characterdata.hpp"
#include "utils/logger.h"
#include "utils/tokendispenser.hpp"
#include "utils/tokencollector.hpp"

AccountConnection::AccountConnection():
    mSyncBuffer(0)
{
}

AccountConnection::~AccountConnection()
{
    delete mSyncBuffer;
}

bool AccountConnection::start(int gameServerPort)
{
    const std::string accountServerAddress =
        Configuration::getValue("net_accountServerAddress", "localhost");
    const int accountServerPort =
        Configuration::getValue("net_accountServerPort", DEFAULT_SERVER_PORT) + 1;

    if (!Connection::start(accountServerAddress, accountServerPort))
    {
        LOG_INFO("Unable to create a connection to an account server.");
        return false;
    }

    LOG_INFO("Connection established to the account server.");

    const std::string gameServerAddress =
        Configuration::getValue("net_gameServerAddress", "localhost");
    const std::string password =
        Configuration::getValue("net_password", "changeMe");

    // Register with the account server and send the list of maps we handle
    MessageOut msg(GAMSG_REGISTER);
    msg.writeString(gameServerAddress);
    msg.writeInt16(gameServerPort);
    msg.writeString(password);
    msg.writeInt32(itemManager->getDatabaseVersion());
    const MapManager::Maps &m = MapManager::getMaps();
    for (MapManager::Maps::const_iterator i = m.begin(), i_end = m.end();
            i != i_end; ++i)
    {
        msg.writeInt16(i->first);
    }
    send(msg);

    // initialize sync buffer
    mSyncBuffer = new MessageOut(GAMSG_PLAYER_SYNC);
    mSyncMessages = 0;

    return true;
}

void AccountConnection::sendCharacterData(Character *p)
{
    MessageOut msg(GAMSG_PLAYER_DATA);
    msg.writeInt32(p->getDatabaseID());
    serializeCharacterData(*p, msg);
    send(msg);
}

void AccountConnection::processMessage(MessageIn &msg)
{
    switch (msg.getId())
    {
        case AGMSG_REGISTER_RESPONSE:
        {
            if (msg.readInt16() != DATA_VERSION_OK)
            {
                LOG_ERROR("Item database is outdated! Please update to "
                          "prevent inconsistencies");
                stop();  // Disconnect gracefully from account server.
                // Stop gameserver to prevent inconsistencies.
                exit(EXIT_DB_EXCEPTION);
            }
            else
            {
                LOG_DEBUG("Local item database is "
                          "in sync with account server.");
            }
            if (msg.readInt16() != PASSWORD_OK)
            {
                LOG_ERROR("This game server sent a invalid password");
                stop();
                exit(EXIT_BAD_CONFIG_PARAMETER);
            }
        } break;

        case AGMSG_PLAYER_ENTER:
        {
            std::string token = msg.readString(MAGIC_TOKEN_LENGTH);
            Character *ptr = new Character(msg);
            gameHandler->addPendingCharacter(token, ptr);
        } break;

        case AGMSG_ACTIVE_MAP:
        {
            int id = msg.readInt16();
            MapManager::raiseActive(id);
        } break;

        case AGMSG_REDIRECT_RESPONSE:
        {
            int id = msg.readInt32();
            std::string token = msg.readString(MAGIC_TOKEN_LENGTH);
            std::string address = msg.readString();
            int port = msg.readInt16();
            gameHandler->completeServerChange(id, token, address, port);
        } break;

        case AGMSG_GET_QUEST_RESPONSE:
        {
            int id = msg.readInt32();
            std::string name = msg.readString();
            std::string value = msg.readString();
            recoveredQuestVar(id, name, value);
        } break;

        case CGMSG_CHANGED_PARTY:
        {
            // Character DB id
            int charid = msg.readInt32();
            // Party id, 0 for none
            int partyid = msg.readInt32();
            gameHandler->updateCharacter(charid, partyid);
        } break;

        case CGMSG_POST_RESPONSE:
        {
            // get the character
            Character *character = postMan->getCharacter(msg.readInt32());

            // check character is still valid
            if (!character)
            {
                break;
            }

            std::string sender = msg.readString();
            std::string letter = msg.readString();

            postMan->gotPost(character, sender, letter);

        } break;

        case CGMSG_STORE_POST_RESPONSE:
        {
            // get character
            Character *character = postMan->getCharacter(msg.readInt32());

            // check character is valid
            if (!character)
            {
                break;
            }

            // TODO: Get NPC to tell character if the sending of post
            // was successful or not

        } break;

        default:
            LOG_WARN("Invalid message type");
            break;
    }
}

void AccountConnection::playerReconnectAccount(int id,
                                               const std::string &magic_token)
{
    LOG_DEBUG("Send GAMSG_PLAYER_RECONNECT.");
    MessageOut msg(GAMSG_PLAYER_RECONNECT);
    msg.writeInt32(id);
    msg.writeString(magic_token, MAGIC_TOKEN_LENGTH);
    send(msg);
}

void AccountConnection::requestQuestVar(Character *ch, const std::string &name)
{
    MessageOut msg(GAMSG_GET_QUEST);
    msg.writeInt32(ch->getDatabaseID());
    msg.writeString(name);
    send(msg);
}

void AccountConnection::updateQuestVar(Character *ch, const std::string &name,
                                       const std::string &value)
{
    MessageOut msg(GAMSG_SET_QUEST);
    msg.writeInt32(ch->getDatabaseID());
    msg.writeString(name);
    msg.writeString(value);
    send(msg);
}

void AccountConnection::banCharacter(Character *ch, int duration)
{
    MessageOut msg(GAMSG_BAN_PLAYER);
    msg.writeInt32(ch->getDatabaseID());
    msg.writeInt16(duration);
    send(msg);
}

void AccountConnection::sendStatistics()
{
    MessageOut msg(GAMSG_STATISTICS);
    const MapManager::Maps &maps = MapManager::getMaps();
    for (MapManager::Maps::const_iterator i = maps.begin(),
         i_end = maps.end(); i != i_end; ++i)
    {
        MapComposite *m = i->second;
        if (!m->isActive()) continue;
        msg.writeInt16(i->first);
        int nbThings = 0, nbMonsters = 0;
        typedef std::vector< Thing * > Things;
        const Things &things = m->getEverything();
        std::vector< int > players;
        for (Things::const_iterator j = things.begin(),
             j_end = things.end(); j != j_end; ++j)
        {
            Thing *t = *j;
            switch (t->getType())
            {
                case OBJECT_CHARACTER:
                    players.push_back
                        (static_cast< Character * >(t)->getDatabaseID());
                    break;
                case OBJECT_MONSTER:
                    ++nbMonsters;
                    break;
                default:
                    ++nbThings;
            }
        }
        msg.writeInt16(nbThings);
        msg.writeInt16(nbMonsters);
        msg.writeInt16(players.size());
        for (std::vector< int >::const_iterator j = players.begin(),
             j_end = players.end(); j != j_end; ++j)
        {
            msg.writeInt32(*j);
        }
    }
    send(msg);
}

void AccountConnection::sendPost(Character *c, MessageIn &msg)
{
    // send message to account server with id of sending player,
    // the id of receiving player, the letter receiver and contents, and attachments
    LOG_DEBUG("Sending GCMSG_STORE_POST.");
    MessageOut out(GCMSG_STORE_POST);
    out.writeInt32(c->getDatabaseID());
    out.writeString(msg.readString()); // name of receiver
    out.writeString(msg.readString()); // content of letter
    while (msg.getUnreadLength()) // attachments
    {
        // write the item id and amount for each attachment
        out.writeInt32(msg.readInt16());
        out.writeInt32(msg.readInt16());
    }
    send(out);
}

void AccountConnection::getPost(Character *c)
{
    // let the postman know to expect some post for this character
    postMan->addCharacter(c);

    // send message to account server with id of retrieving player
    LOG_DEBUG("Sending GCMSG_REQUEST_POST");
    MessageOut out(GCMSG_REQUEST_POST);
    out.writeInt32(c->getDatabaseID());
    send(out);
}

void AccountConnection::changeAccountLevel(Character *c, int level)
{
    MessageOut msg(GAMSG_CHANGE_ACCOUNT_LEVEL);
    msg.writeInt32(c->getDatabaseID());
    msg.writeInt16(level);
    send(msg);
}

void AccountConnection::syncChanges(bool force)
{
    if (mSyncMessages == 0)
        return;

    // send buffer if:
    //    a.) forced by any process
    //    b.) every 10 seconds
    //    c.) buffer reaches size of 1kb
    //    d.) buffer holds more then 20 messages
    if (force ||
        mSyncMessages > SYNC_BUFFER_LIMIT ||
        mSyncBuffer->getLength() > SYNC_BUFFER_SIZE )
    {
        LOG_DEBUG("Sending GAMSG_PLAYER_SYNC with "
                << mSyncMessages << " messages." );

        // attach end-of-buffer flag
        mSyncBuffer->writeInt8(SYNC_END_OF_BUFFER);
        send(*mSyncBuffer);
        delete (mSyncBuffer);

        mSyncBuffer = new MessageOut(GAMSG_PLAYER_SYNC);
        mSyncMessages = 0;
    }
    else
    {
        LOG_DEBUG("No changes to sync with account server.");
    }
}

void AccountConnection::updateCharacterPoints(int charId, int charPoints,
                                              int corrPoints)
{
    mSyncMessages++;
    mSyncBuffer->writeInt8(SYNC_CHARACTER_POINTS);
    mSyncBuffer->writeInt32(charId);
    mSyncBuffer->writeInt32(charPoints);
    mSyncBuffer->writeInt32(corrPoints);
    syncChanges();
}

void AccountConnection::updateAttributes(int charId, int attrId, double base,
                              double mod)
{
    ++mSyncMessages;
    mSyncBuffer->writeInt8(SYNC_CHARACTER_ATTRIBUTE);
    mSyncBuffer->writeInt32(charId);
    mSyncBuffer->writeInt32(attrId);
    mSyncBuffer->writeDouble(base);
    mSyncBuffer->writeDouble(mod);
    syncChanges();
}

void AccountConnection::updateExperience(int charId, int skillId,
                                         int skillValue)
{
    mSyncMessages++;
    mSyncBuffer->writeInt8(SYNC_CHARACTER_SKILL);
    mSyncBuffer->writeInt32(charId);
    mSyncBuffer->writeInt8(skillId);
    mSyncBuffer->writeInt32(skillValue);
    syncChanges();
}

void AccountConnection::updateOnlineStatus(int charId, bool online)
{
    mSyncMessages++;
    mSyncBuffer->writeInt8(SYNC_ONLINE_STATUS);
    mSyncBuffer->writeInt32(charId);
    mSyncBuffer->writeInt8(online ? 0x01 : 0x00);
    syncChanges();
}

void AccountConnection::sendTransaction(int id, int action, const std::string &message)
{
    MessageOut msg(GAMSG_TRANSACTION);
    msg.writeInt32(id);
    msg.writeInt32(action);
    msg.writeString(message);
    send(msg);
}
