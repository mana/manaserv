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
#include "common/resourcemanager.hpp"
#include "common/transaction.hpp"
#include "net/connectionhandler.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/functors.h"
#include "utils/logger.h"
#include "utils/stringfilter.h"
#include "utils/tokencollector.hpp"
#include "utils/tokendispenser.hpp"
#include "utils/sha256.h"
#include "utils/string.hpp"
#include "utils/xml.hpp"

static void addUpdateHost(MessageOut *msg)
{
    std::string updateHost = Configuration::getValue("net_defaultUpdateHost",
                                                     "");
    msg->writeString(updateHost);

    /*
     * This is for developing/testing an experimental new resource manager that
     * downloads only the files it needs on demand.
     */
    std::string dataUrl = Configuration::getValue("net_clientDataUrl", "");
    msg->writeString(dataUrl);
}

// List of attributes that the client can send at account creation.
static std::vector< int > initAttr;

// Character's starting points
static int startPoints, attributesMinimum, attributesMaximum = 0;

/*
 * Map attribute ids to values that they need to be initialised to at account
 * creation.
 * The pair contains two elements of the same value (the default) so that the
 * iterators can be used to copy a range.
 */
static std::map< unsigned int, std::pair< double, double> > defAttr;

class AccountHandler : public ConnectionHandler
{
public:
    /**
     * Constructor.
     */
    AccountHandler(const std::string &attrFile);

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

    static void sendCharacterData(AccountClient &client, int slot,
                              const Character &ch);

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

AccountHandler::AccountHandler(const std::string &attrFile):
    mTokenCollector(this)
{
    // In case of reloading...
    initAttr.clear();

    std::string absPathFile = ResourceManager::resolve(attrFile);
    if (absPathFile.empty())
    {
        LOG_FATAL("Account handler: Could not find " << attrFile << "!");
        exit(EXIT_XML_NOT_FOUND);
    }

    XML::Document doc(absPathFile, int());
    xmlNodePtr node = doc.rootNode();
    if (!node || !xmlStrEqual(node->name, BAD_CAST "attributes"))
    {
        LOG_FATAL("Account handler: " << attrFile << ": "
                  << " is not a valid database file!");
        exit(EXIT_XML_BAD_PARAMETER);
    }

    for_each_xml_child_node(attributenode, node)
    {
        if (xmlStrEqual(attributenode->name, BAD_CAST "attribute"))
        {
            int id = XML::getProperty(attributenode, "id", 0);
            if (!id)
            {
                LOG_WARN("Account handler: " << attrFile << ": "
                         << "An invalid attribute id value (0) has been found "
                         << "and will be ignored.");
                continue;
            }

            if (XML::getBoolProperty(attributenode, "modifiable", false))
                initAttr.push_back(id);

            // Store as string initially to check
            // that the property is defined.
            std::string defStr = XML::getProperty(attributenode, "default", "");
            if (!defStr.empty())
            {
                double val = string_to<double>()(defStr);
                defAttr.insert(std::make_pair(id,std::make_pair(val, val)));
            }
        }
        else if (xmlStrEqual(attributenode->name, BAD_CAST "points"))
        {
            startPoints = XML::getProperty(attributenode, "start", 0);
            attributesMinimum = XML::getProperty(attributenode, "minimum", 0);
            attributesMaximum = XML::getProperty(attributenode, "maximum", 0);

            // Stops if not all the values are given.
            if (!startPoints || !attributesMinimum || !attributesMaximum)
            {
                LOG_FATAL("Account handler: " << attrFile << ": "
                          << " The characters starting points "
                          << "are incomplete or not set!");
                exit(EXIT_XML_BAD_PARAMETER);
            }
        }
    } // End for each XML nodes

    // Sanity checks on attributes.
    if (initAttr.empty())
    {
        LOG_FATAL("Account handler: " << attrFile << ": "
                  << "No modifiable attributes found!");
        exit(EXIT_XML_BAD_PARAMETER);
    }

    // Sanity checks on starting points.
    int modifiableAttributeCount = (int) initAttr.size();
    if (modifiableAttributeCount * attributesMaximum < startPoints ||
        modifiableAttributeCount * attributesMinimum > startPoints)
    {
        LOG_FATAL("Account handler: " << attrFile << ": "
                  << "Character's point values make "
                  << "the character's creation impossible!");
        exit(EXIT_XML_BAD_PARAMETER);
    }

    LOG_DEBUG("Character start points: " << startPoints << " (Min: "
              << attributesMinimum << ", Max: " << attributesMaximum << ")");
}

bool AccountClientHandler::initialize(const std::string &configFile, int port,
                                      const std::string &host)
{
    accountHandler = new AccountHandler(configFile);
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

void AccountHandler::sendCharacterData(AccountClient &client, int slot,
                              const Character &ch)
{
    MessageOut charInfo(APMSG_CHAR_INFO);
    charInfo.writeInt8(slot);
    charInfo.writeString(ch.getName());
    charInfo.writeInt8(ch.getGender());
    charInfo.writeInt8(ch.getHairStyle());
    charInfo.writeInt8(ch.getHairColor());
    charInfo.writeInt16(ch.getLevel());
    charInfo.writeInt16(ch.getCharacterPoints());
    charInfo.writeInt16(ch.getCorrectionPoints());

    for (AttributeMap::const_iterator it = ch.mAttributes.begin(),
                                      it_end = ch.mAttributes.end();
        it != it_end;
        ++it)
    {
        // {id, base value in 256ths, modified value in 256ths }*
        charInfo.writeInt32(it->first);
        charInfo.writeInt32((int) (it->second.first * 256));
        charInfo.writeInt32((int) (it->second.second * 256));
    }

    client.send(charInfo);
}

void AccountHandler::handleLoginMessage(AccountClient &client, MessageIn &msg)
{
    MessageOut reply(APMSG_LOGIN_RESPONSE);

    if (client.status != CLIENT_LOGIN)
    {
        reply.writeInt8(ERRMSG_FAILURE);
        client.send(reply);
        return;
    }

    const int clientVersion = msg.readInt32();

    if (clientVersion < Configuration::getValue("net_clientVersion", 0))
    {
        reply.writeInt8(LOGIN_INVALID_VERSION);
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
            reply.writeInt8(LOGIN_INVALID_TIME);
            client.send(reply);
            return;
        }
    }
    mLastLoginAttemptForIP[address] = now;

    const std::string username = msg.readString();
    const std::string password = msg.readString();

    if (stringFilter->findDoubleQuotes(username))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        return;
    }

    const unsigned maxClients =
            (unsigned) Configuration::getValue("net_maxClients", 1000);

    if (getClientCount() >= maxClients)
    {
        reply.writeInt8(ERRMSG_SERVER_FULL);
        client.send(reply);
        return;
    }

    // Check if the account exists
    Account *acc = storage->getAccount(username);

    if (!acc || acc->getPassword() != sha256(password))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        delete acc;
        return;
    }

    if (acc->getLevel() == AL_BANNED)
    {
        reply.writeInt8(LOGIN_BANNED);
        client.send(reply);
        delete acc;
        return;
    }

    // The client successfully logged in...

    // Set lastLogin date of the account.
    time_t login;
    time(&login);
    acc->setLastLogin(login);
    storage->updateLastLogin(acc);

    // Associate account with connection.
    client.setAccount(acc);
    client.status = CLIENT_CONNECTED;

    reply.writeInt8(ERRMSG_OK);
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
        reply.writeInt8(ERRMSG_NO_LOGIN);
    }
    else if (client.status == CLIENT_CONNECTED)
    {
        client.unsetAccount();
        client.status = CLIENT_LOGIN;
        reply.writeInt8(ERRMSG_OK);
    }
    else if (client.status == CLIENT_QUEUED)
    {
        // Delete it from the pendingClient list
        mTokenCollector.deletePendingClient(&client);
        client.status = CLIENT_LOGIN;
        reply.writeInt8(ERRMSG_OK);
    }
    client.send(reply);
}

void AccountHandler::handleReconnectMessage(AccountClient &client,
                                            MessageIn &msg)
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

void AccountHandler::handleRegisterMessage(AccountClient &client,
                                           MessageIn &msg)
{
    int clientVersion = msg.readInt32();
    std::string username = msg.readString();
    std::string password = msg.readString();
    std::string email = msg.readString();
    std::string captcha = msg.readString();
    int minClientVersion = Configuration::getValue("net_clientVersion", 0);
    unsigned minNameLength = Configuration::getValue("account_minNameLength", 4);
    unsigned maxNameLength = Configuration::getValue("account_maxNameLength", 15);

    MessageOut reply(APMSG_REGISTER_RESPONSE);

    if (client.status != CLIENT_LOGIN)
    {
        reply.writeInt8(ERRMSG_FAILURE);
    }
    else if (!Configuration::getBoolValue("account_allowRegister", true))
    {
        reply.writeInt8(ERRMSG_FAILURE);
    }
    else if (clientVersion < minClientVersion)
    {
        reply.writeInt8(REGISTER_INVALID_VERSION);
    }
    else if (stringFilter->findDoubleQuotes(username))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (stringFilter->findDoubleQuotes(email))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (username.length() < minNameLength ||
            username.length() > maxNameLength)
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (stringFilter->findDoubleQuotes(password))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (!stringFilter->isEmailValid(email))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    // Checking if the Name is slang's free.
    else if (!stringFilter->filterContent(username))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    // Check whether the account already exists.
    else if (storage->doesUserNameExist(username))
    {
        reply.writeInt8(REGISTER_EXISTS_USERNAME);
    }
    // Find out whether the email is already in use.
    else if (storage->doesEmailAddressExist(sha256(email)))
    {
        reply.writeInt8(REGISTER_EXISTS_EMAIL);
    }
    else if (!checkCaptcha(client, captcha))
    {
        reply.writeInt8(REGISTER_CAPTCHA_WRONG);
    }
    else
    {
        Account *acc = new Account;
        acc->setName(username);
        acc->setPassword(sha256(password));
        // We hash email server-side for additional privacy
        // we ask for it again when we need it and verify it
        // through comparing it with the hash.
        acc->setEmail(sha256(email));
        acc->setLevel(AL_PLAYER);

        // Set the date and time of the account registration, and the last login
        time_t regdate;
        time(&regdate);
        acc->setRegistrationDate(regdate);
        acc->setLastLogin(regdate);

        storage->addAccount(acc);
        reply.writeInt8(ERRMSG_OK);
        addUpdateHost(&reply);

        // Associate account with connection
        client.setAccount(acc);
        client.status = CLIENT_CONNECTED;
    }

    client.send(reply);
}

void AccountHandler::handleUnregisterMessage(AccountClient &client,
                                             MessageIn &msg)
{
    LOG_DEBUG("AccountHandler::handleUnregisterMessage");

    MessageOut reply(APMSG_UNREGISTER_RESPONSE);

    if (client.status != CLIENT_CONNECTED)
    {
        reply.writeInt8(ERRMSG_FAILURE);
        client.send(reply);
        return;
    }

    std::string username = msg.readString();
    std::string password = msg.readString();

    if (stringFilter->findDoubleQuotes(username))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        return;
    }

    // See whether the account exists
    Account *acc = storage->getAccount(username);

    if (!acc || acc->getPassword() != sha256(password))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        delete acc;
        return;
    }

    // Delete account and associated characters
    LOG_INFO("Unregistered \"" << username
             << "\", AccountID: " << acc->getID());
    storage->delAccount(acc);
    reply.writeInt8(ERRMSG_OK);

    client.send(reply);
}

void AccountHandler::handleRequestRegisterInfoMessage(AccountClient &client,
                                                      MessageIn &msg)
{
    LOG_INFO("AccountHandler::handleRequestRegisterInfoMessage");
    MessageOut reply(APMSG_REGISTER_INFO_RESPONSE);
    if (!Configuration::getBoolValue("account_allowRegister", true))
    {
        reply.writeInt8(false);
        reply.writeString(Configuration::getValue(
                                             "account_denyRegisterReason", ""));
    }
    else
    {
        reply.writeInt8(true);
        reply.writeInt8(Configuration::getValue("account_minNameLength", 4));
        reply.writeInt8(Configuration::getValue("account_maxNameLength", 16));
        reply.writeString("http://www.server.example/captcha.png");
        reply.writeString("<instructions for solving captcha>");
    }
    client.send(reply);
}

void AccountHandler::handleEmailChangeMessage(AccountClient &client,
                                              MessageIn &msg)
{
    MessageOut reply(APMSG_EMAIL_CHANGE_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeInt8(ERRMSG_NO_LOGIN);
        client.send(reply);
        return;
    }

    const std::string email = msg.readString();
    const std::string emailHash = sha256(email);

    if (!stringFilter->isEmailValid(email))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (stringFilter->findDoubleQuotes(email))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (storage->doesEmailAddressExist(emailHash))
    {
        reply.writeInt8(ERRMSG_EMAIL_ALREADY_EXISTS);
    }
    else
    {
        acc->setEmail(emailHash);
        // Keep the database up to date otherwise we will go out of sync
        storage->flush(acc);
        reply.writeInt8(ERRMSG_OK);
    }
    client.send(reply);
}

void AccountHandler::handlePasswordChangeMessage(AccountClient &client,
                                                 MessageIn &msg)
{
    std::string oldPassword = sha256(msg.readString());
    std::string newPassword = sha256(msg.readString());

    MessageOut reply(APMSG_PASSWORD_CHANGE_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeInt8(ERRMSG_NO_LOGIN);
    }
    else if (stringFilter->findDoubleQuotes(newPassword))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (oldPassword != acc->getPassword())
    {
        reply.writeInt8(ERRMSG_FAILURE);
    }
    else
    {
        acc->setPassword(newPassword);
        // Keep the database up to date otherwise we will go out of sync
        storage->flush(acc);
        reply.writeInt8(ERRMSG_OK);
    }

    client.send(reply);
}

void AccountHandler::handleCharacterCreateMessage(AccountClient &client,
                                                  MessageIn &msg)
{
    std::string name = msg.readString();
    int hairStyle = msg.readInt8();
    int hairColor = msg.readInt8();
    int gender = msg.readInt8();
    int numHairStyles = Configuration::getValue("char_numHairStyles", 17);
    int numHairColors = Configuration::getValue("char_numHairColors", 11);
    int numGenders = Configuration::getValue("char_numGenders", 2);
    unsigned int minNameLength = Configuration::getValue("char_minNameLength", 4);
    unsigned int maxNameLength = Configuration::getValue("char_maxNameLength", 25);
    unsigned int maxCharacters = Configuration::getValue("account_maxCharacters", 3);

    MessageOut reply(APMSG_CHAR_CREATE_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeInt8(ERRMSG_NO_LOGIN);
    }
    else if (!stringFilter->filterContent(name))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (stringFilter->findDoubleQuotes(name))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else if (hairStyle > numHairStyles)
    {
        reply.writeInt8(CREATE_INVALID_HAIRSTYLE);
    }
    else if (hairColor > numHairColors)
    {
        reply.writeInt8(CREATE_INVALID_HAIRCOLOR);
    }
    else if (gender > numGenders)
    {
        reply.writeInt8(CREATE_INVALID_GENDER);
    }
    else if ((name.length() < minNameLength) ||
             (name.length() > maxNameLength))
    {
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
    }
    else
    {
        if (storage->doesCharacterNameExist(name))
        {
            reply.writeInt8(CREATE_EXISTS_NAME);
            client.send(reply);
            return;
        }

        // An account shouldn't have more than MAX_OF_CHARACTERS characters.
        Characters &chars = acc->getCharacters();
        if (chars.size() >= maxCharacters)
        {
            reply.writeInt8(CREATE_TOO_MUCH_CHARACTERS);
            client.send(reply);
            return;
        }

        // TODO: Add race, face and maybe special attributes.

        // Customization of character's attributes...
        std::vector<int> attributes = std::vector<int>(initAttr.size(), 0);
        for (unsigned int i = 0; i < initAttr.size(); ++i)
            attributes[i] = msg.readInt16();

        int totalAttributes = 0;
        for (unsigned int i = 0; i < initAttr.size(); ++i)
        {
            // For good total attributes check.
            totalAttributes += attributes.at(i);

            // For checking if all stats are >= min and <= max.
            if (attributes.at(i) < attributesMinimum
                || attributes.at(i) > attributesMaximum)
            {
                reply.writeInt8(CREATE_ATTRIBUTES_OUT_OF_RANGE);
                client.send(reply);
                return;
            }
        }

        if (totalAttributes > startPoints)
        {
            reply.writeInt8(CREATE_ATTRIBUTES_TOO_HIGH);
        }
        else if (totalAttributes < startPoints)
        {
            reply.writeInt8(CREATE_ATTRIBUTES_TOO_LOW);
        }
        else
        {
            Character *newCharacter = new Character(name);
            for (unsigned int i = 0; i < initAttr.size(); ++i)
                newCharacter->mAttributes.insert(std::make_pair(
                        (unsigned int) (initAttr.at(i)),
                        std::make_pair((double) (attributes[i]),
                                       (double) (attributes[i]))));
            newCharacter->mAttributes.insert(defAttr.begin(), defAttr.end());
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

            reply.writeInt8(ERRMSG_OK);
            client.send(reply);

            // Send new characters infos back to client
            int slot = chars.size() - 1;
            sendCharacterData(client, slot, *chars[slot]);
            return;
        }
    }

    client.send(reply);
}

void AccountHandler::handleCharacterSelectMessage(AccountClient &client,
                                                  MessageIn &msg)
{
    MessageOut reply(APMSG_CHAR_SELECT_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeInt8(ERRMSG_NO_LOGIN);
        client.send(reply);
        return; // not logged in
    }

    unsigned charNum = msg.readInt8();
    Characters &chars = acc->getCharacters();

    // Character ID = 0 to Number of Characters - 1.
    if (charNum >= chars.size())
    {
        // invalid char selection
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
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
        reply.writeInt8(ERRMSG_FAILURE);
        client.send(reply);
        return;
    }

    reply.writeInt8(ERRMSG_OK);

    LOG_DEBUG(selectedChar->getName() << " is trying to enter the servers.");

    std::string magic_token(utils::getMagicToken());
    reply.writeString(magic_token, MAGIC_TOKEN_LENGTH);
    reply.writeString(address);
    reply.writeInt16(port);

    // TODO: get correct address and port for the chat server
    reply.writeString(Configuration::getValue("net_accountServerAddress",
                                              "localhost"));
    reply.writeInt16(Configuration::getValue("net_accountServerPort",
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

void AccountHandler::handleCharacterDeleteMessage(AccountClient &client,
                                                  MessageIn &msg)
{
    MessageOut reply(APMSG_CHAR_DELETE_RESPONSE);

    Account *acc = client.getAccount();
    if (!acc)
    {
        reply.writeInt8(ERRMSG_NO_LOGIN);
        client.send(reply);
        return; // not logged in
    }

    unsigned charNum = msg.readInt8();
    Characters &chars = acc->getCharacters();

    // Character ID = 0 to Number of Characters - 1.
    if (charNum >= chars.size())
    {
        // invalid char selection
        reply.writeInt8(ERRMSG_INVALID_ARGUMENT);
        client.send(reply);
        return; // not logged in
    }

    LOG_INFO("Character deleted:" << chars[charNum]->getName());

    acc->delCharacter(charNum);
    storage->flush(acc);

    reply.writeInt8(ERRMSG_OK);
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

    reply.writeInt8(ERRMSG_OK);
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
    msg.writeInt8(ERRMSG_TIME_OUT);
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
