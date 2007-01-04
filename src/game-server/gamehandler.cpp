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

#include <cassert>
#include <map>

#include "game-server/gamehandler.hpp"
#include "game-server/item.hpp"
#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/state.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/logger.h"

enum
{
    CLIENT_LOGIN = 0,
    CLIENT_CONNECTED,
    CLIENT_CHANGE_SERVER
};

struct GameClient: NetComputer
{
    GameClient(ENetPeer *peer)
      : NetComputer(peer), character(NULL), status(CLIENT_LOGIN) {}
    Player *character;
    int status;
};

struct GamePendingLogin
{
    Player *character;
    int timeout;
};

typedef std::map< std::string, GamePendingLogin > GamePendingLogins;
typedef std::map< std::string, GameClient * > GamePendingClients;

/**
 * The pending logins represent clients who were given a magic token by the
 * account server but who have not yet logged in to the game server.
 */
static GamePendingLogins pendingLogins;

/**
 * The pending clients represent clients who tried to login to the game server,
 * but for which no magic token is available yet. This can happen when the
 * communication between the account server and client went faster than the
 * communication between the account server and the game server.
 */
static GamePendingClients pendingClients;

/**
 * Links a client to a character.
 */
static void linkCharacter(GameClient *computer, Player *ch)
{
    computer->character = ch;
    computer->status = CLIENT_CONNECTED;
    ch->setClient(computer);
    gameState->insert(ch);
    MessageOut result;
    result.writeShort(GPMSG_CONNECT_RESPONSE);
    result.writeByte(ERRMSG_OK);
    computer->send(result);
}

/**
 * Notification that a particular token has been given to allow a certain
 * player to enter the game.
 */
void registerGameClient(std::string const &token, Player *ch)
{
    GamePendingClients::iterator i = pendingClients.find(token);
    if (i != pendingClients.end())
    {
        linkCharacter(i->second, ch);
        pendingClients.erase(i);
    }
    else
    {
        GamePendingLogin p;
        p.character = ch;
        p.timeout = 300; // world ticks
        pendingLogins.insert(std::make_pair(token, p));
    }
}

bool GameHandler::startListen(enet_uint16 port)
{
    LOG_INFO("Game handler started:", 0);
    return ConnectionHandler::startListen(port);
}

NetComputer *GameHandler::computerConnected(ENetPeer *peer)
{
    return new GameClient(peer);
}

void GameHandler::computerDisconnected(NetComputer *computer)
{
    for (GamePendingClients::iterator i = pendingClients.begin(),
         i_end = pendingClients.end(); i != i_end; ++i)
    {
        if (i->second == computer)
        {
            pendingClients.erase(i);
            break;
        }
    }
    if (Player *ch = static_cast< GameClient * >(computer)->character)
    {
        gameState->remove(ch);
        delete ch;
    }
    delete computer;
}

void GameHandler::kill(Player *ch)
{
    GameClient *client = ch->getClient();
    assert(client != NULL);
    client->character = NULL;
    client->status = CLIENT_LOGIN;
}

void GameHandler::prepareServerChange(Player *ch)
{
    GameClient *client = ch->getClient();
    assert(client != NULL);
    client->status = CLIENT_CHANGE_SERVER;
}

void GameHandler::completeServerChange(int id, std::string const &token,
                                       std::string const &address, int port)
{
    for (NetComputers::const_iterator i = clients.begin(),
         i_end = clients.end(); i != i_end; ++i)
    {
        GameClient *c = static_cast< GameClient * >(*i);
        if (c->status == CLIENT_CHANGE_SERVER &&
            c->character->getDatabaseID() == id)
        {
            MessageOut msg(GPMSG_PLAYER_SERVER_CHANGE);
            msg.writeString(token, 32);
            msg.writeString(address);
            msg.writeShort(port);
            c->send(msg);
            delete c->character;
            c->character = NULL;
            c->status = CLIENT_LOGIN;
            return;
        }
    }
}

void GameHandler::process()
{
    ConnectionHandler::process();

    // Removes characters that have been left unconnected for too long.
    GamePendingLogins::iterator i = pendingLogins.begin();
    while (i != pendingLogins.end())
    {
        if (--i->second.timeout <= 0)
        {
            delete i->second.character;
            pendingLogins.erase(i++);
        }
        else
        {
            ++i;
        }
    }
}

void GameHandler::processMessage(NetComputer *comp, MessageIn &message)
{
    GameClient &computer = *static_cast< GameClient * >(comp);
    MessageOut result;

    if (computer.status == CLIENT_LOGIN)
    {
        if (message.getId() != PGMSG_CONNECT) return;
        std::string magic_token = message.readString(32);
        GamePendingLogins::iterator i = pendingLogins.find(magic_token);
        if (i == pendingLogins.end())
        {
            for (GamePendingClients::iterator j = pendingClients.begin(),
                 j_end = pendingClients.end(); j != j_end; ++j)
            {
                if (j->second == &computer) return;
            }
            pendingClients.insert(std::make_pair(magic_token, &computer));
            return;
        }
        linkCharacter(&computer, i->second.character);
        pendingLogins.erase(i);
        return;
    }
    else if (computer.status != CLIENT_CONNECTED)
    {
        return;
    }

    switch (message.getId())
    {
        case PGMSG_SAY:
        {
            std::string say = message.readString();
            gameState->sayAround(computer.character, say);
        } break;

        case PGMSG_PICKUP:
        {
            int x = message.readShort();
            int y = message.readShort();
            Point ppos = computer.character->getPosition();

            // TODO: use a less arbitrary value.
            if (std::abs(x - ppos.x) + std::abs(y - ppos.y) < 48)
            {
                int mapId = computer.character->getMapId();
                MapComposite *map = gameState->getMap(mapId);
                Point ipos = { x, y };
                for (FixedObjectIterator i(map->getAroundPointIterator(ipos, 0)); i; ++i)
                {
                    Object *o = *i;
                    Point opos = o->getPosition();
                    if (o->getType() == OBJECT_ITEM && opos.x == x && opos.y == y)
                    {
                        result.writeShort(GPMSG_INVENTORY);
                        ItemClass *item = static_cast< Item * >(o)->getItemClass();
                        Inventory(computer.character, result).insert(item->getDatabaseID(), 1);
                        gameState->remove(o);
                        break;
                    }
                }
            }
        } break;

        case PGMSG_WALK:
        {
            int x = message.readShort();
            int y = message.readShort();
            Point dst = {x, y};
            computer.character->setDestination(dst);

            // no response should be required
        } break;

        case PGMSG_EQUIP:
        {
            int slot = message.readByte();
            result.writeShort(GPMSG_INVENTORY);
            Inventory(computer.character, result).equip(slot);
        } break;

        case PGMSG_ATTACK:
        {
            LOG_DEBUG("Player " << computer.character->getPublicID()
                      << " attacks", 0);
            computer.character->setDirection(message.readByte());
            computer.character->setAttacking(true);
        } break;

        default:
            LOG_WARN("Invalid message type", 0);
            result.writeShort(XXMSG_INVALID);
            break;
    }

    if (result.getLength() > 0)
        computer.send(result);
}

void GameHandler::sendTo(Player *beingPtr, MessageOut &msg)
{
    GameClient *client = beingPtr->getClient();
    assert(client && client->status == CLIENT_CONNECTED);
    client->send(msg);
}
