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

const unsigned int TILES_TO_BE_NEAR = 7;

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
        ch->disconnected();
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
    ch->setClient(NULL);
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
            c->character->disconnected();
            delete c->character;
            c->character = NULL;
            c->status = CLIENT_LOGIN;
            return;
        }
    }
}

void GameHandler::updateCharacter(int charid, int partyid)
{
    for (NetComputers::const_iterator i = clients.begin(),
         i_end = clients.end(); i != i_end; ++i)
    {
        GameClient *c = static_cast< GameClient * >(*i);
        if (c->character->getDatabaseID() == charid)
        {
            c->character->setParty(partyid);
        }
    }
}

static MovingObject *findBeingNear(Object *p, int id)
{
    MapComposite *map = p->getMap();
    Point const &ppos = p->getPosition();
    // See map.hpp for tiles constants
    for (MovingObjectIterator i(map->getAroundPointIterator(ppos,
        DEFAULT_TILE_WIDTH * TILES_TO_BE_NEAR)); i; ++i)
    {
        MovingObject *o = *i;
        if (o->getPublicID() != id) continue;
        return ppos.inRangeOf(o->getPosition(), DEFAULT_TILE_WIDTH *
        TILES_TO_BE_NEAR) ? o : NULL;
    }
    return NULL;
}

static Character *findCharacterNear(Object *p, int id)
{
    MapComposite *map = p->getMap();
    Point const &ppos = p->getPosition();
    // See map.hpp for tiles constants
    for (CharacterIterator i(map->getAroundPointIterator(ppos,
        DEFAULT_TILE_WIDTH * TILES_TO_BE_NEAR)); i; ++i)
    {
        Character *o = *i;
        if (o->getPublicID() != id) continue;
        return ppos.inRangeOf(o->getPosition(), DEFAULT_TILE_WIDTH *
        TILES_TO_BE_NEAR) ? o : NULL;
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
            if (say.empty()) break;

            if (say[0] == '/')
            {
                // Handle special command.
                extern void runCommand(Character *, std::string const &);
                runCommand(computer.character, say);
                break;
            }
            GameState::sayAround(computer.character, say);
        } break;

        case PGMSG_NPC_TALK:
        case PGMSG_NPC_TALK_NEXT:
        case PGMSG_NPC_SELECT:
        {
            int id = message.readShort();
            MovingObject *o = findBeingNear(computer.character, id);
            if (!o || o->getType() != OBJECT_NPC)
            {
                sendError(comp, id, "Not close enough to NPC\n");
                break;
            }

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

        case PGMSG_USE_ITEM:
        {
            int slot = message.readByte();
            Inventory inv(computer.character);
            if (ItemClass *ic = ItemManager::getItem(inv.getItem(slot)))
            {
                if (ic->use(computer.character))
                {
                    inv.removeFromSlot(slot, 1);
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
                if (!GameState::insert(item))
                {
                    // The map is full. Put back into inventory.
                    inv.insert(ic->getDatabaseID(), amount - nb);
                    delete item;
                }
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
            computer.character->disconnected();
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
        case PGMSG_TRADE_SET_MONEY:
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
                case PGMSG_TRADE_SET_MONEY:
                    t->setMoney(computer.character, message.readLong());
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

        case PGMSG_RAISE_ATTRIBUTE:
        {
            int attribute = message.readByte();
            AttribmodResponseCode retCode;
            retCode = computer.character->useCharacterPoint(attribute);
            result.writeShort(GPMSG_RAISE_ATTRIBUTE_RESPONSE);
            result.writeByte(retCode);
            result.writeByte(attribute);
        } break;

        case PGMSG_LOWER_ATTRIBUTE:
        {
            int attribute = message.readByte();
            AttribmodResponseCode retCode;
            retCode = computer.character->useCorrectionPoint(attribute);
            result.writeShort(GPMSG_LOWER_ATTRIBUTE_RESPONSE);
            result.writeByte(retCode);
            result.writeByte(attribute);
        } break;

        case PGMSG_RESPAWN:
        {
            computer.character->respawn(); // plausibility check is done by character class
        } break;

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

void GameHandler::addPendingCharacter(std::string const &token, Character *ch)
{
    /* First, check if the character is already on the map. This may happen if
       a client just lost its connection, and logged to the account server
       again, yet the game server has not yet detected the lost connection. */

    int id = ch->getDatabaseID();
    for (NetComputers::const_iterator i = clients.begin(),
         i_end = clients.end(); i != i_end; ++i)
    {
        GameClient *c = static_cast< GameClient * >(*i);
        Character *old_ch = c->character;
        if (old_ch && old_ch->getDatabaseID() == id)
        {
            if (c->status != CLIENT_CONNECTED)
            {
                /* Either the server is confused, or the client is up to no
                   good. So ignore the request, and wait for the connections
                   to properly time out. */
                return;
            }

            /* As the connection was not properly closed, the account server
               has not yet updated its data, so ignore them. Instead, take the
               already present character, kill its current connection, and make
               it available for a new connection. */
            delete ch;
            GameState::remove(old_ch);
            kill(old_ch);
            ch = old_ch;
            break;
        }
    }

    // Mark the character as pending a connection.
    mTokenCollector.addPendingConnect(token, ch);
}

void
GameHandler::tokenMatched(GameClient* computer, Character* character)
{
    computer->character = character;
    computer->status = CLIENT_CONNECTED;

    character->setClient(computer);

    MessageOut result(GPMSG_CONNECT_RESPONSE);

    if (!GameState::insert(character))
    {
        result.writeByte(ERRMSG_SERVER_FULL);
        kill(character);
        delete character;
        computer->disconnect(result);
        return;
    }

    result.writeByte(ERRMSG_OK);
    computer->send(result);

    // Force sending the whole character to the client.
    Inventory(character).sendFull();
    for (int i = 0; i < NB_CHARACTER_ATTRIBUTES; ++i)
    {
        character->modifiedAttribute(i);
    }
}

void
GameHandler::deletePendingClient(GameClient* computer)
{
    // Something might have changed since it was inserted
    if (computer->status != CLIENT_QUEUED) return;

    MessageOut msg(GPMSG_CONNECT_RESPONSE);
    msg.writeByte(ERRMSG_TIME_OUT);

    // The computer will be deleted when the disconnect event is processed
    computer->disconnect(msg);
}

void
GameHandler::deletePendingConnect(Character* character)
{
    delete character;
}

GameClient *GameHandler::getClientByNameSlow(std::string const &name)
{
    for (NetComputers::const_iterator i = clients.begin(),
         i_end = clients.end(); i != i_end; ++i)
    {
        GameClient *c = static_cast< GameClient * >(*i);
        Character *ch = c->character;
        if (ch && ch->getName() == name)
        {
            return c;
        }
    }
    return NULL;
}

void GameHandler::sendError(NetComputer *computer, int id, std::string errorMsg)
{
    MessageOut msg(GPMSG_NPC_ERROR);
    msg.writeShort(id);
    msg.writeString(errorMsg, errorMsg.size());
    computer->send(msg);
}
