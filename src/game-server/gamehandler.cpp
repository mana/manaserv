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

#include "game-server/accountconnection.hpp"
#include "game-server/buysell.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/npc.hpp"
#include "game-server/state.hpp"
#include "game-server/trade.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/logger.h"
#include "utils/tokendispenser.hpp"

GameHandler::GameHandler():
    mTokenCollector(this)
{
}

bool GameHandler::startListen(enet_uint16 port)
{
    LOG_INFO("Game handler started:");
    return ConnectionHandler::startListen(port);
}

NetComputer *GameHandler::computerConnected(ENetPeer *peer)
{
    return new GameClient(peer);
}

void GameHandler::computerDisconnected(NetComputer *comp)
{
    GameClient &computer = *static_cast< GameClient * >(comp);

    if (computer.status == CLIENT_QUEUED)
    {
        mTokenCollector.deletePendingClient(&computer);
    }
    else if (Character *ch = computer.character)
    {
        accountHandler->sendCharacterData(ch);
        GameState::remove(ch);
        delete ch;
    }
    delete &computer;
}

void GameHandler::kill(Character *ch)
{
    GameClient *client = ch->getClient();
    assert(client != NULL);
    client->character = NULL;
    client->status = CLIENT_LOGIN;
}

void GameHandler::prepareServerChange(Character *ch)
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
            msg.writeString(token, MAGIC_TOKEN_LENGTH);
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
}

static MovingObject *findBeingNear(Object *p, int id)
{
    MapComposite *map = p->getMap();
    Point const &ppos = p->getPosition();
    // TODO: use a less arbitrary value.
    for (MovingObjectIterator i(map->getAroundPointIterator(ppos, 48)); i; ++i)
    {
        MovingObject *o = *i;
        if (o->getPublicID() != id) continue;
        return ppos.inRangeOf(o->getPosition(), 48) ? o : NULL;
    }
    return NULL;
}

static Character *findCharacterNear(Object *p, int id)
{
    MapComposite *map = p->getMap();
    Point const &ppos = p->getPosition();
    // TODO: use a less arbitrary value.
    for (CharacterIterator i(map->getAroundPointIterator(ppos, 48)); i; ++i)
    {
        Character *o = *i;
        if (o->getPublicID() != id) continue;
        return ppos.inRangeOf(o->getPosition(), 48) ? o : NULL;
    }
    return NULL;
}

void GameHandler::processMessage(NetComputer *comp, MessageIn &message)
{
    GameClient &computer = *static_cast< GameClient * >(comp);
    MessageOut result;

    if (computer.status == CLIENT_LOGIN)
    {
        if (message.getId() != PGMSG_CONNECT) return;

        std::string magic_token = message.readString(MAGIC_TOKEN_LENGTH);
        computer.status = CLIENT_QUEUED; // Before the addPendingClient
        mTokenCollector.addPendingClient(magic_token, &computer);
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
            GameState::sayAround(computer.character, say);
        } break;

        case PGMSG_NPC_TALK:
        case PGMSG_NPC_TALK_NEXT:
        case PGMSG_NPC_SELECT:
        {
            int id = message.readShort();
            MovingObject *o = findBeingNear(computer.character, id);
            if (!o || o->getType() != OBJECT_NPC) break;

            NPC *q = static_cast< NPC * >(o);
            if (message.getId() == PGMSG_NPC_SELECT)
            {
                q->select(computer.character, message.readByte());
            }
            else
            {
                q->prompt(computer.character, message.getId() == PGMSG_NPC_TALK);
            }
        } break;

        case PGMSG_PICKUP:
        {
            int x = message.readShort();
            int y = message.readShort();
            Point ppos = computer.character->getPosition();

            // TODO: use a less arbitrary value.
            if (std::abs(x - ppos.x) + std::abs(y - ppos.y) < 48)
            {
                MapComposite *map = computer.character->getMap();
                Point ipos(x, y);
                for (FixedObjectIterator i(map->getAroundPointIterator(ipos, 0)); i; ++i)
                {
                    Object *o = *i;
                    Point opos = o->getPosition();
                    if (o->getType() == OBJECT_ITEM && opos.x == x && opos.y == y)
                    {
                        Item *item = static_cast< Item * >(o);
                        ItemClass *ic = item->getItemClass();
                        Inventory(computer.character)
                            .insert(ic->getDatabaseID(), item->getAmount());
                        GameState::remove(item);
                        break;
                    }
                }
            }
        } break;

        case PGMSG_DROP:
        {
            int slot = message.readByte();
            int amount = message.readByte();
            Inventory inv(computer.character);
            if (ItemClass *ic = ItemManager::getItem(inv.getItem(slot)))
            {
                int nb = inv.removeFromSlot(slot, amount);
                Item *item = new Item(ic, amount - nb);
                item->setMap(computer.character->getMap());
                item->setPosition(computer.character->getPosition());
                GameState::insert(item);
            }
        } break;

        case PGMSG_WALK:
        {
            int x = message.readShort();
            int y = message.readShort();
            Point dst(x, y);
            computer.character->setDestination(dst);

            // no response should be required
        } break;

        case PGMSG_EQUIP:
        {
            int slot = message.readByte();
            Inventory(computer.character).equip(slot);
        } break;

        case PGMSG_UNEQUIP:
        {
            int slot = message.readByte();
            if (slot >= 0 && slot < EQUIP_PROJECTILE_SLOT)
            {
                Inventory(computer.character).unequip(slot);
            }
        } break;

        case PGMSG_MOVE_ITEM:
        {
            int slot1 = message.readByte();
            int slot2 = message.readByte();
            int amount = message.readByte();
            Inventory(computer.character).move(slot1, slot2, amount);
        } break;

        case PGMSG_ATTACK:
        {
            LOG_DEBUG("Character " << computer.character->getPublicID()
                      << " attacks");
            computer.character->setDirection(message.readByte());
            computer.character->setAction(Being::ATTACK);
        } break;

        case PGMSG_ACTION_CHANGE:
        {
            Being::Action action = (Being::Action)message.readByte();
            Being::Action current = (Being::Action)computer.character->getAction();

            switch (action)
            {
                case Being::STAND:
                {
                    if (current == Being::SIT)
                        computer.character->setAction(Being::STAND);
                } break;
                case Being::SIT:
                {
                    if (current == Being::STAND)
                        computer.character->setAction(Being::SIT);
                } break;
                default:
                    break;
            }

        } break;

        case PGMSG_DISCONNECT:
        {
            bool reconnectAccount = (bool) message.readByte();

            result.writeShort(GPMSG_DISCONNECT_RESPONSE);
            result.writeByte(ERRMSG_OK); // It is, when control reaches here

            if (reconnectAccount)
            {
                std::string magic_token(utils::getMagicToken());
                result.writeString(magic_token, MAGIC_TOKEN_LENGTH);
                // No accountserver data, the client should remember that
                accountHandler->playerReconnectAccount(
                                   computer.character->getDatabaseID(),
                                   magic_token);
            }
            // TODO: implement a delayed remove
            GameState::remove(computer.character);

            accountHandler->sendCharacterData(computer.character);

            // Done with the character
            delete computer.character;
            computer.character = NULL;
            computer.status = CLIENT_LOGIN;
        } break;

        case PGMSG_TRADE_REQUEST:
        {
            int id = message.readShort();

            if (Trade *t = computer.character->getTrading())
            {
                if (t->request(computer.character, id)) break;
            }

            Character *q = findCharacterNear(computer.character, id);
            if (!q || q->isBusy())
            {
                result.writeShort(GPMSG_TRADE_CANCEL);
                break;
            }

            new Trade(computer.character, q);
        } break;

        case PGMSG_TRADE_CANCEL:
        case PGMSG_TRADE_ACCEPT:
        case PGMSG_TRADE_ADD_ITEM:
        {
            Trade *t = computer.character->getTrading();
            if (!t) break;

            switch (message.getId())
            {
                case PGMSG_TRADE_CANCEL:
                    t->cancel(computer.character);
                    break;
                case PGMSG_TRADE_ACCEPT :
                    t->accept(computer.character);
                    break;
                case PGMSG_TRADE_ADD_ITEM:
                    int slot = message.readByte();
                    t->addItem(computer.character, slot, message.readByte());
                    break;
            }
        } break;

        case PGMSG_NPC_BUYSELL:
        {
            BuySell *t = computer.character->getBuySell();
            if (!t) break;
            int id = message.readShort();
            int amount = message.readShort();
            t->perform(id, amount);
        } break;

// The following messages should be handled by the chat server, not the game server.
#if 0

        case PGMSG_GUILD_CREATE:
        {
            std::string name = message.readString();
            int characterId = computer.character->getDatabaseID();
            messageMap[characterId] = computer.character;
            accountHandler->playerCreateGuild(characterId, name);
        } break;
            
        case PGMSG_GUILD_INVITE:
        {
            short guildId = message.readShort();
            std::string member = message.readString();
            int characterId = computer.character->getDatabaseID();
            messageMap[characterId] = computer.character;
            accountHandler->playerInviteToGuild(characterId, guildId, member);
        } break;
            
        case PGMSG_GUILD_ACCEPT:
        {
            std::string guildName = message.readString();
            int characterId = computer.character->getDatabaseID();
            messageMap[characterId] = computer.character;
            accountHandler->playerAcceptInvite(characterId, guildName);
        } break;
            
        case PGMSG_GUILD_GET_MEMBERS:
        {
            short guildId = message.readShort();
            int characterId = computer.character->getDatabaseID();
            messageMap[characterId] = computer.character;
            accountHandler->getGuildMembers(characterId, guildId);
        } break;
            
        case PGMSG_GUILD_QUIT:
        {
            short guildId = message.readShort();
            int characterId = computer.character->getDatabaseID();
            messageMap[characterId] = computer.character;
            accountHandler->quitGuild(characterId, guildId);
        } break;
#endif

        default:
            LOG_WARN("Invalid message type");
            result.writeShort(XXMSG_INVALID);
            break;
    }

    if (result.getLength() > 0)
        computer.send(result);
}

void GameHandler::sendTo(Character *beingPtr, MessageOut &msg)
{
    GameClient *client = beingPtr->getClient();
    assert(client && client->status == CLIENT_CONNECTED);
    client->send(msg);
}

void
GameHandler::tokenMatched(GameClient* computer, Character* character)
{
    computer->character = character;
    computer->status = CLIENT_CONNECTED;

    character->setClient(computer);

    MessageOut result(GPMSG_CONNECT_RESPONSE);
    result.writeByte(ERRMSG_OK);
    computer->send(result);

    GameState::insert(character);

    Inventory(character).sendFull();
}

void
GameHandler::deletePendingClient(GameClient* computer)
{
    // Something might have changed since it was inserted
    if (computer->status != CLIENT_QUEUED) return;

    MessageOut msg(GPMSG_CONNECTION_TIMEDOUT);

    // The computer will be deleted when the disconnect event is processed
    computer->disconnect(msg);
}

void
GameHandler::deletePendingConnect(Character* character)
{
    delete character;
}
