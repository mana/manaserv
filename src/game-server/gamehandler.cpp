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

#include <cassert>
#include <map>

#include "game-server/gamehandler.hpp"

#include "common/transaction.hpp"
#include "game-server/accountconnection.hpp"
#include "game-server/buysell.hpp"
#include "game-server/commandhandler.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/npc.hpp"
#include "game-server/postman.hpp"
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
    assert(client);
    client->character = NULL;
    client->status = CLIENT_LOGIN;
    ch->setClient(0);
}

void GameHandler::prepareServerChange(Character *ch)
{
    GameClient *client = ch->getClient();
    assert(client);
    client->status = CLIENT_CHANGE_SERVER;
}

void GameHandler::completeServerChange(int id, const std::string &token,
                                       const std::string &address, int port)
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
            msg.writeInt16(port);
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

static Actor *findActorNear(Actor *p, int id)
{
    MapComposite *map = p->getMap();
    const Point &ppos = p->getPosition();
    // See map.hpp for tiles constants
    const int pixelDist = DEFAULT_TILE_WIDTH * TILES_TO_BE_NEAR;
    for (ActorIterator i(map->getAroundPointIterator(ppos, pixelDist)); i; ++i)
    {
        Actor *a = *i;
        if (a->getPublicID() != id)
            continue;
        return ppos.inRangeOf(a->getPosition(), pixelDist) ? a : 0;
    }
    return 0;
}

static Character *findCharacterNear(Actor *p, int id)
{
    MapComposite *map = p->getMap();
    const Point &ppos = p->getPosition();
    // See map.hpp for tiles constants
    const int pixelDist = DEFAULT_TILE_WIDTH * TILES_TO_BE_NEAR;
    for (CharacterIterator i(map->getAroundPointIterator(ppos,
                                                         pixelDist)); i; ++i)
    {
        Character *c = *i;
        if (c->getPublicID() != id)
            continue;
        return ppos.inRangeOf(c->getPosition(), pixelDist) ? c : 0;
    }
    return 0;
}

void GameHandler::processMessage(NetComputer *comp, MessageIn &message)
{
    GameClient &computer = *static_cast< GameClient * >(comp);
    MessageOut result;

    if (computer.status == CLIENT_LOGIN)
    {
        if (message.getId() != PGMSG_CONNECT)
            return;

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

            if (say[0] == '@')
            {
                CommandHandler::handleCommand(computer.character, say);
                break;
            }
            if (!computer.character->isMuted())
            {
                GameState::sayAround(computer.character, say);
                std::string msg = computer.character->getName() + " said " + say;
                accountHandler->sendTransaction(computer.character->getDatabaseID(), TRANS_MSG_PUBLIC, msg);
            }else {
                GameState::sayTo(computer.character, NULL, "You are not allowed to talk right now.");
            }
        } break;

        case PGMSG_NPC_TALK:
        case PGMSG_NPC_TALK_NEXT:
        case PGMSG_NPC_SELECT:
        case PGMSG_NPC_NUMBER:
        case PGMSG_NPC_STRING:
        {
            int id = message.readInt16();
            Actor *o = findActorNear(computer.character, id);
            if (!o || o->getType() != OBJECT_NPC)
            {
                sendError(comp, id, "Not close enough to NPC\n");
                break;
            }

            NPC *q = static_cast< NPC * >(o);
            if (message.getId() == PGMSG_NPC_SELECT)
            {
                q->select(computer.character, message.readInt8());
            }
            else if (message.getId() == PGMSG_NPC_NUMBER)
            {
                q->integerReceived(computer.character, message.readInt32());
            }
            else if (message.getId() == PGMSG_NPC_STRING)
            {
                q->stringReceived(computer.character, message.readString());
            }
            else
            {
                q->prompt(computer.character, message.getId() == PGMSG_NPC_TALK);
            }
        } break;

        case PGMSG_PICKUP:
        {
            int x = message.readInt16();
            int y = message.readInt16();
            Point ppos = computer.character->getPosition();

            // TODO: use a less arbitrary value.
            if (std::abs(x - ppos.x) + std::abs(y - ppos.y) < 48)
            {
                MapComposite *map = computer.character->getMap();
                Point ipos(x, y);
                for (FixedActorIterator i(map->getAroundPointIterator(ipos, 0)); i; ++i)
                {
                    Actor *o = *i;
                    Point opos = o->getPosition();
                    if (o->getType() == OBJECT_ITEM && opos.x == x && opos.y == y)
                    {
                        Item *item = static_cast< Item * >(o);
                        ItemClass *ic = item->getItemClass();
                        Inventory(computer.character)
                            .insert(ic->getDatabaseID(), item->getAmount());
                        GameState::remove(item);
                        // log transaction
                        std::stringstream str;
                        str << "User picked up item " << ic->getDatabaseID()
                            << " at " << opos.x << "x" << opos.y;
                        accountHandler->sendTransaction(computer.character->getDatabaseID(),
                            TRANS_ITEM_PICKUP, str.str());
                        break;
                    }
                }
            }
        } break;

        case PGMSG_USE_ITEM:
        {
            int slot = message.readInt8();
            Inventory inv(computer.character);
            if (ItemClass *ic = itemManager->getItem(inv.getItem(slot)))
            {
                if (ic->hasTrigger(ITT_ACTIVATE))
                {
                    std::stringstream str;
                    str << "User activated item " << ic->getDatabaseID()
                        << " from slot " << slot;
                    accountHandler->sendTransaction(computer.character->getDatabaseID(),
                                                    TRANS_ITEM_USED, str.str());
                    if (ic->useTrigger(computer.character, ITT_ACTIVATE))
                        inv.removeFromSlot(slot, 1);
                }
            }
        } break;

        case PGMSG_DROP:
        {
            int slot = message.readInt8();
            int amount = message.readInt8();
            Inventory inv(computer.character);
            if (ItemClass *ic = itemManager->getItem(inv.getItem(slot)))
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
                    break;
                }
                // log transaction
                Point pt = computer.character->getPosition();
                std::stringstream str;
                str << "User dropped item " << ic->getDatabaseID()
                    << " at " << pt.x << "x" << pt.y;
                accountHandler->sendTransaction(computer.character->getDatabaseID(),
                    TRANS_ITEM_DROP, str.str());
            }
        } break;

        case PGMSG_WALK:
        {
            handleWalk(&computer, message);
        } break;

        case PGMSG_EQUIP:
        {
            int slot = message.readInt8();
            Inventory(computer.character).equip(slot);
        } break;

        case PGMSG_UNEQUIP:
        {
            int slot = message.readInt8();
            if (slot >= 0 && slot < INVENTORY_SLOTS)
                Inventory(computer.character).unequip(slot);
        } break;

        case PGMSG_MOVE_ITEM:
        {
            int slot1 = message.readInt8();
            int slot2 = message.readInt8();
            int amount = message.readInt8();
            Inventory(computer.character).move(slot1, slot2, amount);
            // log transaction
            std::stringstream str;
            str << "User moved item "
                << " from slot " << slot1 << " to slot " << slot2;
            accountHandler->sendTransaction(computer.character->getDatabaseID(),
                TRANS_ITEM_MOVE, str.str());
        } break;

        case PGMSG_ATTACK:
        {
            int id = message.readInt16();
            LOG_DEBUG("Character " << computer.character->getPublicID()
                      << " attacked being " << id);

            Actor *o = findActorNear(computer.character, id);
            if (o && o->getType() != OBJECT_NPC)
            {
                Being *being = static_cast<Being*>(o);
                computer.character->setTarget(being);
                computer.character->setAction(Being::ATTACK);
            }
        } break;

        case PGMSG_USE_SPECIAL:
        {
            int specialID = message.readInt8();
            LOG_DEBUG("Character " << computer.character->getPublicID()
                      << " tries to use his special attack "<<specialID);
            computer.character->useSpecial(specialID);
        }

        case PGMSG_ACTION_CHANGE:
        {
            Being::Action action = (Being::Action)message.readInt8();
            Being::Action current = (Being::Action)computer.character->getAction();
            bool logActionChange = true;

            switch (action)
            {
                case Being::STAND:
                {
                    if (current == Being::SIT)
                    {
                        computer.character->setAction(Being::STAND);
                        logActionChange = false;
                    }
                } break;
                case Being::SIT:
                {
                    if (current == Being::STAND)
                    {
                        computer.character->setAction(Being::SIT);
                        logActionChange = false;
                    }
                } break;
                default:
                    break;
            }

            // Log the action change only when this is relevant.
            if (logActionChange)
            {
                // log transaction
                std::stringstream str;
                str << "User changed action from " << current
                    << " to " << action;
                accountHandler->sendTransaction(
                    computer.character->getDatabaseID(),
                    TRANS_ACTION_CHANGE, str.str());
            }

        } break;

        case PGMSG_DIRECTION_CHANGE:
        {
            computer.character->setDirection(message.readInt8());
        } break;

        case PGMSG_DISCONNECT:
        {
            bool reconnectAccount = (bool) message.readInt8();

            result.writeInt16(GPMSG_DISCONNECT_RESPONSE);
            result.writeInt8(ERRMSG_OK); // It is, when control reaches here

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
            int id = message.readInt16();

            if (Trade *t = computer.character->getTrading())
            {
                if (t->request(computer.character, id)) break;
            }

            Character *q = findCharacterNear(computer.character, id);
            if (!q || q->isBusy())
            {
                result.writeInt16(GPMSG_TRADE_CANCEL);
                break;
            }

            new Trade(computer.character, q);

            // log transaction
            std::string str;
            str = "User requested trade with " + q->getName();
            accountHandler->sendTransaction(computer.character->getDatabaseID(),
                TRANS_TRADE_REQUEST, str);
        } break;

        case PGMSG_TRADE_CANCEL:
        case PGMSG_TRADE_AGREED:
        case PGMSG_TRADE_CONFIRM:
        case PGMSG_TRADE_ADD_ITEM:
        case PGMSG_TRADE_SET_MONEY:
        {
            std::stringstream str;
            Trade *t = computer.character->getTrading();
            if (!t) break;

            switch (message.getId())
            {
                case PGMSG_TRADE_CANCEL:
                    t->cancel();
                    break;
                case PGMSG_TRADE_CONFIRM:
                    t->confirm(computer.character);
                    break;
                case PGMSG_TRADE_AGREED:
                    t->agree(computer.character);
                    // log transaction
                    accountHandler->sendTransaction(computer.character->getDatabaseID(),
                        TRANS_TRADE_END, "User finished trading");
                    break;
                case PGMSG_TRADE_SET_MONEY:
                {
                    int money = message.readInt32();
                    t->setMoney(computer.character, money);
                    // log transaction
                    str << "User added " << money << " money to trade.";
                    accountHandler->sendTransaction(computer.character->getDatabaseID(),
                        TRANS_TRADE_MONEY, str.str());
                } break;
                case PGMSG_TRADE_ADD_ITEM:
                {
                    int slot = message.readInt8();
                    t->addItem(computer.character, slot, message.readInt8());
                    // log transaction
                    str << "User add item from slot " << slot;
                    accountHandler->sendTransaction(computer.character->getDatabaseID(),
                        TRANS_TRADE_ITEM, str.str());
                } break;
            }
        } break;

        case PGMSG_NPC_BUYSELL:
        {
            BuySell *t = computer.character->getBuySell();
            if (!t) break;
            int id = message.readInt16();
            int amount = message.readInt16();
            t->perform(id, amount);
        } break;

        case PGMSG_RAISE_ATTRIBUTE:
        {
            int attribute = message.readInt32();
            AttribmodResponseCode retCode;
            retCode = computer.character->useCharacterPoint(attribute);
            result.writeInt16(GPMSG_RAISE_ATTRIBUTE_RESPONSE);
            result.writeInt8(retCode);
            result.writeInt32(attribute);

            if (retCode == ATTRIBMOD_OK )
            {
                accountHandler->updateCharacterPoints(
                    computer.character->getDatabaseID(),
                    computer.character->getCharacterPoints(),
                    computer.character->getCorrectionPoints());

                // log transaction
                std::stringstream str;
                str << "User increased attribute " << attribute;
                accountHandler->sendTransaction(computer.character->getDatabaseID(),
                    TRANS_ATTR_INCREASE, str.str());
            }
        } break;

        case PGMSG_LOWER_ATTRIBUTE:
        {
            int attribute = message.readInt32();
            AttribmodResponseCode retCode;
            retCode = computer.character->useCorrectionPoint(attribute);
            result.writeInt16(GPMSG_LOWER_ATTRIBUTE_RESPONSE);
            result.writeInt8(retCode);
            result.writeInt32(attribute);

            if (retCode == ATTRIBMOD_OK )
            {
                accountHandler->updateCharacterPoints(
                    computer.character->getDatabaseID(),
                    computer.character->getCharacterPoints(),
                    computer.character->getCorrectionPoints());

                // log transaction
                std::stringstream str;
                str << "User decreased attribute " << attribute;
                accountHandler->sendTransaction(computer.character->getDatabaseID(),
                    TRANS_ATTR_DECREASE, str.str());
            }
        } break;

        case PGMSG_RESPAWN:
        {
            computer.character->respawn(); // plausibility check is done by character class
        } break;

        case PGMSG_NPC_POST_SEND:
        {
            handleSendPost(&computer, message);
        } break;

        default:
            LOG_WARN("Invalid message type");
            result.writeInt16(XXMSG_INVALID);
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

void GameHandler::addPendingCharacter(const std::string &token, Character *ch)
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

void GameHandler::tokenMatched(GameClient *computer, Character *character)
{
    computer->character = character;
    computer->status = CLIENT_CONNECTED;

    character->setClient(computer);

    MessageOut result(GPMSG_CONNECT_RESPONSE);

    if (!GameState::insert(character))
    {
        result.writeInt8(ERRMSG_SERVER_FULL);
        kill(character);
        delete character;
        computer->disconnect(result);
        return;
    }

    result.writeInt8(ERRMSG_OK);
    computer->send(result);

    // Force sending the whole character to the client.
    Inventory(character).sendFull();
    character->modifiedAllAttribute();
    std::map<int, int>::const_iterator skill_it;
    for (skill_it = character->getSkillBegin(); skill_it != character->getSkillEnd(); skill_it++)
    {
        character->updateDerivedAttributes(skill_it->first);
    }
}

void GameHandler::deletePendingClient(GameClient *computer)
{
    // Something might have changed since it was inserted
    if (computer->status != CLIENT_QUEUED)
        return;

    MessageOut msg(GPMSG_CONNECT_RESPONSE);
    msg.writeInt8(ERRMSG_TIME_OUT);

    // The computer will be deleted when the disconnect event is processed
    computer->disconnect(msg);
}

void GameHandler::deletePendingConnect(Character *character)
{
    delete character;
}

GameClient *GameHandler::getClientByNameSlow(const std::string &name) const
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
    return 0;
}

void GameHandler::sendError(NetComputer *computer, int id, std::string errorMsg)
{
    MessageOut msg(GPMSG_NPC_ERROR);
    msg.writeInt16(id);
    msg.writeString(errorMsg, errorMsg.size());
    computer->send(msg);
}

void GameHandler::handleWalk(GameClient *client, MessageIn &message)
{
    int x = message.readInt16();
    int y = message.readInt16();

    Point dst(x, y);
    client->character->setDestination(dst);

}

void GameHandler::handleSendPost(GameClient *client, MessageIn &message)
{
    // add the character so that the post man knows them
    postMan->addCharacter(client->character);
    accountHandler->sendPost(client->character, message);
}
