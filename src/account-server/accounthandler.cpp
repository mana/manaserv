/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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

#include "account-server/accounthandler.hpp"

#include "protocol.h"
#include "point.h"
#include "account-server/account.hpp"
#include "account-server/accountclient.hpp"
#include "account-server/character.hpp"
#include "account-server/storage.hpp"
#include "account-server/serverhandler.hpp"
#include "chat-server/chathandler.hpp"
#include "common/configuration.hpp"
#include "common/transaction.hpp"
#include "net/connectionhandler.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/logger.h"
#include "utils/stringfilter.h"
#include "utils/tokencollector.hpp"
#include "utils/tokendispenser.hpp"
#include "utils/sha256.h"

static void addUpdateHost(MessageOut *msg)
{
    std::string updateHost = Configuration::getValue("defaultUpdateHost", "");
    msg->writeString(updateHost);
}

class AccountHandler : public ConnectionHandler
{
public:
    /**
     * Constructor.
     */
    AccountHandler();

    /**
     * Called by the token collector in order to associate a client to its
     * account ID.
     */
    void tokenMatched(AccountClient *client, int accountID);

    /**
     * Called by the token collector when a client was not acknowledged for
     * some time and should be disconnected.
     */
    void deletePendingClient(AccountClient *client);

    /**
     * Called by the token collector.
     */
    void deletePendingConnect(int) {}

    /**
     * Token collector for connecting a client coming from a game server
     * without having to provide username and password a second time.
     */
    TokenCollector<AccountHandler, AccountClient *, int> mTokenCollector;

protected:
    /**
     * Processes account related messages.
     */
    void processMessage(NetComputer *client, MessageIn &message);

    NetComputer *computerConnected(ENetPeer *peer);

    void computerDisconnected(NetComputer *comp);

private:
    void handleLoginMessage(AccountClient &client, MessageIn &msg);
    void handleLogoutMessage(AccountClient &client);
    void handleReconnectMessage(AccountClient &client, MessageIn &msg);
    void handleRegisterMessage(AccountClient &client, MessageIn &msg);
    void handleUnregisterMessage(AccountClient &client, MessageIn &msg);
    void handleRequestRegisterInfoMessage(AccountClient &client, MessageIn &msg);
    void handleEmailChangeMessage(AccountClient &client, MessageIn &msg);
    void handlePasswordChangeMessage(AccountClient &client, MessageIn &msg);
    void handleCharacterCreateMessage(AccountClient &client, MessageIn &msg);
    void handleCharacterSelectMessage(AccountClient &client, MessageIn &msg);
    void handleCharacterDeleteMessage(AccountClient &client, MessageIn &msg);

    typedef std::map<int, time_t> IPsToTime;
    IPsToTime mLastLoginAttemptForIP;
};

static AccountHandler *accountHandler;

AccountHandler::AccountHandler():
    mTokenCollector(this)
{
}

bool AccountClientHandler::initialize(int port, const std::string &host)
{
    accountHandler = new AccountHandler;
    LOG_INFO("Account handler started:");

    return accountHandler->startListen(port, host);
}

void AccountClientHandler::deinitialize()
{
    accountHandler->stopListen();
    delete accountHandler;
}

void AccountClientHandler::process()
{
    accountHandler->process(50);
}

void AccountClientHandler::prepareReconnect(const std::string &token, int id)
{
    accountHandler->mTokenCollector.addPendingConnect(token, id);
}

NetComputer *AccountHandler::computerConnected(ENetPeer *peer)
{
    return new AccountClient(peer);
}

void AccountHandler::computerDisconnected(NetComputer *comp)
{
    AccountClient *client = static_cast< AccountClient * >(comp);

    if (client->status == CLIENT_QUEUED)
        // Delete it from the pendingClient list
        mTokenCollector.deletePendingClient(client);

    delete client; // ~AccountClient unsets the account
}

static void sendCharacterData(AccountClient &client, int slot,
                              const Character &ch)
{
    MessageOut charInfo(APMSG_CHAR_INFO);
    charInfo.writeByte(slot);
    charInfo.writeString(ch.getName());
    charInfo.writeByte(ch.getGender());
    charInfo.writeByte(ch.getHairStyle());
    charInfo.writeByte(ch.getHairColor());
    charInfo.writeShort(ch.getLevel());
    charInfo.writeShort(ch.getCharacterPoints());
    charInfo.writeShort(ch.getCorrectionPoints());
    charInfo.writeLong(ch.getPossessions().money);

    for (int j = CHAR_ATTR_BEGIN; j < CHAR_ATTR_END; ++j)
    {
        charInfo.writeShort(ch.getAttribute(j));
    }

    client.send(charInfo);
}

void AccountHandler::handleLoginMessage(AccountClient &client, MessageIn &msg)
{
    MessageOut reply(APMSG_LOGIN_RESPONSE);

    if (client.status != CLIENT_LOGIN)
    {
        reply.writeByte(ERRMSG_FAILURE);
        client.send(reply);
        return;
    }

    const int clientVersion = msg.readLong();

    if (clientVersion < Configuration::getValue("clientVersion", 0))
    {
        reply.writeByte(LOGIN_INVALID_VERSION);
        client.send(reply);
        return;
    }

    // Check whether the last login attempt for this IP is still too fresh
    const int address = client.getIP();
    const time_t now = time(NULL);
    IPsToTime::const_iterator it = mLastLoginAttemptForIP.find(address);
    if (it != mLastLoginAttemptForIP.end())
    {
        const time_t lastAttempt = it->second;
        if (now < lastAttempt + 1)
        {
            reply.writeByte(LOGIN_INVALID_TIME);
            client.send(reply);
            return;
        }
    }
    mLastLoginAttemptForIP[address] = now;

    const std::string username = msg.readString();
    const std::string password = msg.readString();

    if (stringFilter->findDoubleQuotes(username))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        return;
    }

    const unsigned maxClients =
            (unsigned) Configuration::getValue("net_maxClients", 1000);

    if (getClientCount() >= maxClients)
    {
        reply.writeByte(ERRMSG_SERVER_FULL);
        client.send(reply);
        return;
    }

    // Check if the account exists
    Account *acc = storage->getAccount(username);

    if (!acc || acc->getPassword() != sha256(password))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        delete acc;
        return;
    }

    if (acc->getLevel() == AL_BANNED)
    {
        reply.writeByte(LOGIN_BANNED);
        client.send(reply);
        delete acc;
        return;
    }

    // The client succesfully logged in

    // set lastLogin date of the account
    time_t login;
    time(&login);
    acc->setLastLogin(login);
    storage->updateLastLogin(acc);

    // Associate account with connection
    client.setAccount(acc);
    client.status = CLIENT_CONNECTED;

    reply.writeByte(ERRMSG_OK);
    addUpdateHost(&reply);
    client.send(reply); // Acknowledge login

    // Return information about available characters
    Characters &chars = acc->getCharacters();

    // Send characters list
    for (unsigned int i = 0; i < chars.size(); i++)
    {
        sendCharacterData(client, i, *chars[i]);
    }
}

void AccountHandler::handleLogoutMessage(AccountClient &client)
{
    MessageOut reply(APMSG_LOGOUT_RESPONSE);

    if (client.status == CLIENT_LOGIN)
    {
        reply.writeByte(ERRMSG_NO_LOGIN);
    }
    else if (client.status == CLIENT_CONNECTED)
    {
        client.unsetAccount();
        client.status = CLIENT_LOGIN;
        reply.writeByte(ERRMSG_OK);
    }
    else if (client.status == CLIENT_QUEUED)
    {
        // Delete it from the pendingClient list
        mTokenCollector.deletePendingClient(&client);
        client.status = CLIENT_LOGIN;
        reply.writeByte(ERRMSG_OK);
    }
    client.send(reply);
}

void AccountHandler::handleReconnectMessage(AccountClient &client, MessageIn &msg)
{
    if (client.status != CLIENT_LOGIN)
    {
        LOG_DEBUG("Account tried to reconnect, but was already logged in "
                  "or queued.");
        return;
    }

    std::string magic_token = msg.readString(MAGIC_TOKEN_LENGTH);
    client.status = CLIENT_QUEUED; // Before the addPendingClient
    mTokenCollector.addPendingClient(magic_token, &client);
}

bool checkCaptcha(AccountClient &client, std::string captcha)
{
    // TODO
    return true;
}

void AccountHandler::handleRegisterMessage(AccountClient &client, MessageIn &msg)
{
    int clientVersion = msg.readLong();
    std::string username = msg.readString();
    std::string password = msg.readString();
    std::string email = msg.readString();
    std::string captcha = msg.readString();
    std::string allowed = Configuration::getValue("account_allowRegister", "1");
    int minClientVersion = Configuration::getValue("clientVersion", 0);
    unsigned minNameLength = Configuration::getValue("account_minNameLength", 4);
    unsigned maxNameLength = Configuration::getValue("account_maxNameLength", 15);

    MessageOut reply(APMSG_REGISTER_RESPONSE);

    if (client.status != CLIENT_LOGIN)
    {
        reply.writeByte(ERRMSG_FAILURE);
    }
    else if (allowed == "0" or allowed == "false")
    {
        reply.writeByte(ERRMSG_FAILURE);
    }
    else if (clientVersion < minClientVersion)
    {
        reply.writeByte(REGISTER_INVALID_VERSION);
    }
    else if (stringFilter->findDoubleQuotes(username))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (stringFilter->findDoubleQuotes(email))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (username.length() < minNameLength ||
            username.length() > maxNameLength)
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (stringFilter->findDoubleQuotes(password))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (!stringFilter->isEmailValid(email))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    // Checking if the Name is slang's free.
    else if (!stringFilter->filterContent(username))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    // Check whether the account already exists.
    else if (storage->doesUserNameExist(username))
    {
        reply.writeByte(REGISTER_EXISTS_USERNAME);
    }
    // Find out whether the email is already in use.
    else if (storage->doesEmailAddressExist(sha256(email)))
    {
        reply.writeByte(REGISTER_EXISTS_EMAIL);
    }
    else if (!checkCaptcha(client, captcha))
    {
        reply.writeByte(REGISTER_CAPTCHA_WRONG);
    }
    else
    {
        Account *acc = new Account;
        acc->setName(username);
        acc->setPassword(sha256(password));
        // We hash email server-side for additional privacy
        // we ask for it again when we need it and verify it
        // through comparing it with the hash
        acc->setEmail(sha256(email));
        acc->setLevel(AL_PLAYER);

        // set the date and time of the account registration, and the last login
        time_t regdate;
        time(&regdate);
        acc->setRegistrationDate(regdate);
        acc->setLastLogin(regdate);

        storage->addAccount(acc);
        reply.writeByte(ERRMSG_OK);
        addUpdateHost(&reply);

        // Associate account with connection
        client.setAccount(acc);
        client.status = CLIENT_CONNECTED;
    }

    client.send(reply);
}

void AccountHandler::handleUnregisterMessage(AccountClient &client, MessageIn &msg)
{
    LOG_DEBUG("AccountHandler::handleUnregisterMessage");
    std::string username = msg.readString();
    std::string password = msg.readString();

    MessageOut reply(APMSG_UNREGISTER_RESPONSE);

    if (client.status != CLIENT_CONNECTED)
    {
        reply.writeByte(ERRMSG_FAILURE);
        client.send(reply);
        return;
    }

    if (stringFilter->findDoubleQuotes(username))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        return;
    }

    // See if the account exists
    Account *acc = storage->getAccount(username);

    if (!acc || acc->getPassword() != password)
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        delete acc;
        return;
    }

    // Delete account and associated characters
    LOG_INFO("Unregistered \"" << username
             << "\", AccountID: " << acc->getID());
    storage->delAccount(acc);
    reply.writeByte(ERRMSG_OK);

    client.send(reply);
}

void AccountHandler::handleRequestRegisterInfoMessage(AccountClient &client, MessageIn &msg)
{
    LOG_INFO("AccountHandler::handleRequestRegisterInfoMessage");
    MessageOut reply(APMSG_REGISTER_INFO_RESPONSE);
    std::string allowed = Configuration::getValue("account_allowRegister", "1");
    if (allowed == "0" or allowed == "false")
    {
        reply.writeByte(false);
        reply.writeString(Configuration::getValue(
            "account_denyRegisterReason", ""));
    } else {
        reply.writeByte(true);
        reply.writeByte(Configuration::getValue("account_minNameLength", 4));
        reply.writeByte(Configuration::getValue("account_maxNameLength", 16));
        reply.writeString("http://www.server.example/captcha.png");
        reply.writeString("<instructions for solving captcha>");
    }
    client.send(reply);
}

void AccountHandler::handleEmailChangeMessage(AccountClient &client, MessageIn &msg)
{
    MessageOut reply(APMSG_EMAIL_CHANGE_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeByte(ERRMSG_NO_LOGIN);
        client.send(reply);
        return;
    }

    const std::string email = msg.readString();
    const std::string emailHash = sha256(email);

    if (!stringFilter->isEmailValid(email))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (stringFilter->findDoubleQuotes(email))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (storage->doesEmailAddressExist(emailHash))
    {
        reply.writeByte(ERRMSG_EMAIL_ALREADY_EXISTS);
    }
    else
    {
        acc->setEmail(emailHash);
        // Keep the database up to date otherwise we will go out of sync
        storage->flush(acc);
        reply.writeByte(ERRMSG_OK);
    }
    client.send(reply);
}

void AccountHandler::handlePasswordChangeMessage(AccountClient &client, MessageIn &msg)
{
    std::string oldPassword = sha256(msg.readString());
    std::string newPassword = sha256(msg.readString());

    MessageOut reply(APMSG_PASSWORD_CHANGE_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeByte(ERRMSG_NO_LOGIN);
    }
    else if (stringFilter->findDoubleQuotes(newPassword))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (oldPassword != acc->getPassword())
    {
        reply.writeByte(ERRMSG_FAILURE);
    }
    else
    {
        acc->setPassword(newPassword);
        // Keep the database up to date otherwise we will go out of sync
        storage->flush(acc);
        reply.writeByte(ERRMSG_OK);
    }

    client.send(reply);
}

void AccountHandler::handleCharacterCreateMessage(AccountClient &client, MessageIn &msg)
{
    std::string name = msg.readString();
    int hairStyle = msg.readByte();
    int hairColor = msg.readByte();
    int gender = msg.readByte();
    int numHairStyles = Configuration::getValue("char_numHairStyles", 17);
    int numHairColors = Configuration::getValue("char_numHairColors", 11);
    int numGenders = Configuration::getValue("char_numGenders", 2);
    unsigned minNameLength = Configuration::getValue("char_minNameLength", 4);
    unsigned maxNameLength = Configuration::getValue("char_maxNameLength", 25);
    unsigned maxCharacters = Configuration::getValue("char_maxCharacters", 3);
    unsigned startingPoints = Configuration::getValue("char_startingPoints", 60);


    MessageOut reply(APMSG_CHAR_CREATE_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeByte(ERRMSG_NO_LOGIN);
    }
    else if (!stringFilter->filterContent(name))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (stringFilter->findDoubleQuotes(name))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (hairStyle > numHairStyles)
    {
        reply.writeByte(CREATE_INVALID_HAIRSTYLE);
    }
    else if (hairColor > numHairColors)
    {
        reply.writeByte(CREATE_INVALID_HAIRCOLOR);
    }
    else if (gender > numGenders)
    {
        reply.writeByte(CREATE_INVALID_GENDER);
    }
    else if ((name.length() < minNameLength) ||
             (name.length() > maxNameLength))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else
    {
        if (storage->doesCharacterNameExist(name))
        {
            reply.writeByte(CREATE_EXISTS_NAME);
            client.send(reply);
            return;
        }

        // An account shouldn't have more than MAX_OF_CHARACTERS characters.
        Characters &chars = acc->getCharacters();
        if (chars.size() >= maxCharacters)
        {
            reply.writeByte(CREATE_TOO_MUCH_CHARACTERS);
            client.send(reply);
            return;
        }

        // LATER_ON: Add race, face and maybe special attributes.

        // Customization of character's attributes...
        int attributes[CHAR_ATTR_NB];
        for (int i = 0; i < CHAR_ATTR_NB; ++i)
            attributes[i] = msg.readShort();

        unsigned int totalAttributes = 0;
        bool validNonZeroAttributes = true;
        for (int i = 0; i < CHAR_ATTR_NB; ++i)
        {
            // For good total attributes check.
            totalAttributes += attributes[i];

            // For checking if all stats are at least > 0
            if (attributes[i] <= 0) validNonZeroAttributes = false;
        }

        if (totalAttributes > startingPoints)
        {
            reply.writeByte(CREATE_ATTRIBUTES_TOO_HIGH);
        }
        else if (totalAttributes < startingPoints)
        {
            reply.writeByte(CREATE_ATTRIBUTES_TOO_LOW);
        }
        else if (!validNonZeroAttributes)
        {
            reply.writeByte(CREATE_ATTRIBUTES_EQUAL_TO_ZERO);
        }
        else
        {
            Character *newCharacter = new Character(name);
            for (int i = CHAR_ATTR_BEGIN; i < CHAR_ATTR_END; ++i)
                newCharacter->setAttribute(i, attributes[i - CHAR_ATTR_BEGIN]);
            newCharacter->setAccount(acc);
            newCharacter->setLevel(1);
            newCharacter->setCharacterPoints(0);
            newCharacter->setCorrectionPoints(0);
            newCharacter->setGender(gender);
            newCharacter->setHairStyle(hairStyle);
            newCharacter->setHairColor(hairColor);
            newCharacter->setMapId(Configuration::getValue("char_startMap", 1));
            Point startingPos(Configuration::getValue("char_startX", 1024),
                              Configuration::getValue("char_startY", 1024));
            newCharacter->setPosition(startingPos);
            acc->addCharacter(newCharacter);

            LOG_INFO("Character " << name << " was created for "
                     << acc->getName() << "'s account.");

            storage->flush(acc); // flush changes

            // log transaction
            Transaction trans;
            trans.mCharacterId = newCharacter->getDatabaseID();
            trans.mAction = TRANS_CHAR_CREATE;
            trans.mMessage = acc->getName() + " created character ";
            trans.mMessage.append("called " + name);
            storage->addTransaction(trans);

            reply.writeByte(ERRMSG_OK);
            client.send(reply);

            // Send new characters infos back to client
            int slot = chars.size() - 1;
            sendCharacterData(client, slot, *chars[slot]);
            return;
        }
    }

    client.send(reply);
}

void AccountHandler::handleCharacterSelectMessage(AccountClient &client, MessageIn &msg)
{
    MessageOut reply(APMSG_CHAR_SELECT_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeByte(ERRMSG_NO_LOGIN);
        client.send(reply);
        return; // not logged in
    }

    unsigned charNum = msg.readByte();
    Characters &chars = acc->getCharacters();

    // Character ID = 0 to Number of Characters - 1.
    if (charNum >= chars.size())
    {
        // invalid char selection
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        return;
    }

    Character *selectedChar = chars[charNum];

    std::string address;
    int port;
    if (!GameServerHandler::getGameServerFromMap
            (selectedChar->getMapId(), address, port))
    {
        LOG_ERROR("Character Selection: No game server for the map.");
        reply.writeByte(ERRMSG_FAILURE);
        client.send(reply);
        return;
    }

    reply.writeByte(ERRMSG_OK);

    LOG_DEBUG(selectedChar->getName() << " is trying to enter the servers.");

    std::string magic_token(utils::getMagicToken());
    reply.writeString(magic_token, MAGIC_TOKEN_LENGTH);
    reply.writeString(address);
    reply.writeShort(port);

    // TODO: get correct address and port for the chat server
    reply.writeString(Configuration::getValue("net_accountServerAddress",
                                              "localhost"));
    reply.writeShort(Configuration::getValue("net_accountServerPort",
                                             DEFAULT_SERVER_PORT) + 2);

    GameServerHandler::registerClient(magic_token, selectedChar);
    registerChatClient(magic_token, selectedChar->getName(), acc->getLevel());

    client.send(reply);

    // log transaction
    Transaction trans;
    trans.mCharacterId = selectedChar->getDatabaseID();
    trans.mAction = TRANS_CHAR_SELECTED;
    trans.mMessage = "";
    storage->addTransaction(trans);
}

void AccountHandler::handleCharacterDeleteMessage(AccountClient &client, MessageIn &msg)
{
    MessageOut reply(APMSG_CHAR_DELETE_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeByte(ERRMSG_NO_LOGIN);
        client.send(reply);
        return; // not logged in
    }

    unsigned charNum = msg.readByte();
    Characters &chars = acc->getCharacters();

    // Character ID = 0 to Number of Characters - 1.
    if (charNum >= chars.size())
    {
        // invalid char selection
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        return; // not logged in
    }

    LOG_INFO("Character deleted:" << chars[charNum]->getName());

    acc->delCharacter(charNum);
    storage->flush(acc);

    reply.writeByte(ERRMSG_OK);
    client.send(reply);

    // log transaction
    Transaction trans;
    trans.mCharacterId = chars[charNum]->getDatabaseID();
    trans.mAction = TRANS_CHAR_DELETED;
    trans.mMessage = chars[charNum]->getName() + " deleted by ";
    trans.mMessage.append(acc->getName());
    storage->addTransaction(trans);
}

void AccountHandler::tokenMatched(AccountClient *client, int accountID)
{
    MessageOut reply(APMSG_RECONNECT_RESPONSE);

    // Associate account with connection.
    Account *acc = storage->getAccount(accountID);
    client->setAccount(acc);
    client->status = CLIENT_CONNECTED;

    reply.writeByte(ERRMSG_OK);
    client->send(reply);

    // Return information about available characters
    Characters &chars = acc->getCharacters();

    // Send characters list
    for (unsigned int i = 0; i < chars.size(); i++)
    {
        sendCharacterData(*client, i, *chars[i]);
    }
}

void AccountHandler::deletePendingClient(AccountClient *client)
{
    MessageOut msg(APMSG_RECONNECT_RESPONSE);
    msg.writeByte(ERRMSG_TIME_OUT);
    client->disconnect(msg);
    // The client will be deleted when the disconnect event is processed
}

void AccountHandler::processMessage(NetComputer *comp, MessageIn &message)
{
    AccountClient &client = *static_cast< AccountClient * >(comp);

    switch (message.getId())
    {
        case PAMSG_LOGIN:
            LOG_DEBUG("Received msg ... PAMSG_LOGIN");
            handleLoginMessage(client, message);
            break;

        case PAMSG_LOGOUT:
            LOG_DEBUG("Received msg ... PAMSG_LOGOUT");
            handleLogoutMessage(client);
            break;

        case PAMSG_RECONNECT:
            LOG_DEBUG("Received msg ... PAMSG_RECONNECT");
            handleReconnectMessage(client, message);
            break;

        case PAMSG_REGISTER:
            LOG_DEBUG("Received msg ... PAMSG_REGISTER");
            handleRegisterMessage(client, message);
            break;

        case PAMSG_UNREGISTER:
            LOG_DEBUG("Received msg ... PAMSG_UNREGISTER");
            handleUnregisterMessage(client, message);
            break;

        case PAMSG_REQUEST_REGISTER_INFO :
            LOG_DEBUG("Received msg ... REQUEST_REGISTER_INFO");
            handleRequestRegisterInfoMessage(client, message);
            break;


        case PAMSG_EMAIL_CHANGE:
            LOG_DEBUG("Received msg ... PAMSG_EMAIL_CHANGE");
            handleEmailChangeMessage(client, message);
            break;

        case PAMSG_PASSWORD_CHANGE:
            LOG_DEBUG("Received msg ... PAMSG_PASSWORD_CHANGE");
            handlePasswordChangeMessage(client, message);
            break;

        case PAMSG_CHAR_CREATE:
            LOG_DEBUG("Received msg ... PAMSG_CHAR_CREATE");
            handleCharacterCreateMessage(client, message);
            break;

        case PAMSG_CHAR_SELECT:
            LOG_DEBUG("Received msg ... PAMSG_CHAR_SELECT");
            handleCharacterSelectMessage(client, message);
            break;

        case PAMSG_CHAR_DELETE:
            LOG_DEBUG("Received msg ... PAMSG_CHAR_DELETE");
            handleCharacterDeleteMessage(client, message);
            break;

        default:
            LOG_WARN("AccountHandler::processMessage, Invalid message type "
                     << message.getId());
            MessageOut result(XXMSG_INVALID);
            client.send(result);
            break;
    }
}
