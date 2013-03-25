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

#include "game-server/gamehandler.h"

#include "common/configuration.h"
#include "common/transaction.h"
#include "game-server/accountconnection.h"
#include "game-server/buysell.h"
#include "game-server/commandhandler.h"
#include "game-server/emotemanager.h"
#include "game-server/inventory.h"
#include "game-server/item.h"
#include "game-server/itemmanager.h"
#include "game-server/map.h"
#include "game-server/mapcomposite.h"
#include "game-server/npc.h"
#include "game-server/postman.h"
#include "game-server/state.h"
#include "game-server/trade.h"
#include "net/messagein.h"
#include "net/messageout.h"
#include "net/netcomputer.h"
#include "utils/logger.h"
#include "utils/tokendispenser.h"

const unsigned TILES_TO_BE_NEAR = 7;

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
    // See map.h for tiles constants
    const int pixelDist = DEFAULT_TILE_LENGTH * TILES_TO_BE_NEAR;
    for (ActorIterator i(map->getAroundPointIterator(ppos, pixelDist)); i; ++i)
    {
        Actor *a = *i;
        if (a->getPublicID() != id)
            continue;
        return ppos.inRangeOf(a->getPosition(), pixelDist) ? a : 0;
    }
    return 0;
}

static Being *findBeingNear(Actor *p, int id)
{
    MapComposite *map = p->getMap();
    const Point &ppos = p->getPosition();
    // See map.h for tiles constants
    const int pixelDist = DEFAULT_TILE_LENGTH * TILES_TO_BE_NEAR;
    for (BeingIterator i(map->getAroundPointIterator(ppos, pixelDist)); i; ++i)
    {
        Being *b = *i;
        if (b->getPublicID() != id)
            continue;
        return ppos.inRangeOf(b->getPosition(), pixelDist) ? b : 0;
    }
    return 0;
}

static Character *findCharacterNear(Actor *p, int id)
{
    MapComposite *map = p->getMap();
    const Point &ppos = p->getPosition();
    // See map.h for tiles constants
    const int pixelDist = DEFAULT_TILE_LENGTH * TILES_TO_BE_NEAR;
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

void GameHandler::processMessage(NetComputer *computer, MessageIn &message)
{
    GameClient &client = *static_cast<GameClient *>(computer);

    if (client.status == CLIENT_LOGIN)
    {
        if (message.getId() != PGMSG_CONNECT)
            return;

        std::string magic_token = message.readString(MAGIC_TOKEN_LENGTH);
        client.status = CLIENT_QUEUED; // Before the addPendingClient
        mTokenCollector.addPendingClient(magic_token, &client);
        return;
    }
    else if (client.status != CLIENT_CONNECTED)
    {
        return;
    }

    switch (message.getId())
    {
        case PGMSG_SAY:
            handleSay(client, message);
            break;

        case PGMSG_NPC_TALK:
        case PGMSG_NPC_TALK_NEXT:
        case PGMSG_NPC_SELECT:
        case PGMSG_NPC_NUMBER:
        case PGMSG_NPC_STRING:
            handleNpc(client, message);
            break;

        case PGMSG_PICKUP:
            handlePickup(client, message);
            break;

        case PGMSG_USE_ITEM:
            handleUseItem(client, message);
            break;

        case PGMSG_DROP:
            handleDrop(client, message);
            break;

        case PGMSG_WALK:
            handleWalk(client, message);
            break;

        case PGMSG_EQUIP:
            handleEquip(client, message);
            break;

        case PGMSG_UNEQUIP:
            handleUnequip(client, message);
            break;

        case PGMSG_MOVE_ITEM:
            handleMoveItem(client, message);
            break;

        case PGMSG_ATTACK:
            handleAttack(client, message);
            break;

        case PGMSG_USE_SPECIAL_ON_BEING:
            handleUseSpecialOnBeing(client, message);
            break;

        case PGMSG_USE_SPECIAL_ON_POINT:
            handleUseSpecialOnPoint(client, message);
            break;

        case PGMSG_ACTION_CHANGE:
            handleActionChange(client, message);
            break;

        case PGMSG_DIRECTION_CHANGE:
            handleDirectionChange(client, message);
            break;

        case PGMSG_DISCONNECT:
            handleDisconnect(client, message);
            break;

        case PGMSG_TRADE_REQUEST:
            handleTradeRequest(client, message);
            break;

        case PGMSG_TRADE_CANCEL:
        case PGMSG_TRADE_AGREED:
        case PGMSG_TRADE_CONFIRM:
        case PGMSG_TRADE_ADD_ITEM:
        case PGMSG_TRADE_SET_MONEY:
            handleTrade(client, message);
            break;

        case PGMSG_NPC_BUYSELL:
            handleNpcBuySell(client, message);
            break;

        case PGMSG_RAISE_ATTRIBUTE:
            handleRaiseAttribute(client, message);
            break;

        case PGMSG_LOWER_ATTRIBUTE:
            handleLowerAttribute(client, message);
            break;

        case PGMSG_RESPAWN:
            // plausibility check is done by character class
            client.character->respawn();
            break;

        case PGMSG_NPC_POST_SEND:
            handleNpcPostSend(client, message);
            break;

        case PGMSG_PARTY_INVITE:
            handlePartyInvite(client, message);
            break;

        case PGMSG_BEING_EMOTE:
            handleTriggerEmoticon(client, message);
            break;

        default:
            LOG_WARN("Invalid message type");
            client.send(MessageOut(XXMSG_INVALID));
            break;
    }
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
    // Trigger login script bind
    character->triggerLoginCallback();

    result.writeInt8(ERRMSG_OK);
    computer->send(result);

    // Force sending the whole character to the client.
    Inventory(character).sendFull();
    character->modifiedAllAttribute();
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

Character *GameHandler::getCharacterByNameSlow(const std::string &name) const
{
    for (NetComputers::const_iterator i = clients.begin(),
         i_end = clients.end(); i != i_end; ++i)
    {
        GameClient *c = static_cast< GameClient * >(*i);
        Character *ch = c->character;
        if (ch && ch->getName() == name &&
                c->status == CLIENT_CONNECTED)
        {
            return ch;
        }
    }
    return 0;
}

void GameHandler::handleSay(GameClient &client, MessageIn &message)
{
    const std::string say = message.readString();
    if (say.empty())
        return;

    if (say[0] == '@')
    {
        CommandHandler::handleCommand(client.character, say);
        return;
    }
    if (!client.character->isMuted())
    {
        GameState::sayAround(client.character, say);
    }
    else
    {
        GameState::sayTo(client.character, NULL,
                         "You are not allowed to talk right now.");
    }
}

void GameHandler::handleNpc(GameClient &client, MessageIn &message)
{
    int id = message.readInt16();
    Actor *actor = findActorNear(client.character, id);
    if (!actor || actor->getType() != OBJECT_NPC)
    {
        sendNpcError(client, id, "Not close enough to NPC\n");
        return;
    }

    Being *npc = static_cast<Being*>(actor);

    switch (message.getId())
    {
        case PGMSG_NPC_SELECT:
            Npc::integerReceived(client.character, message.readInt8());
            break;
        case PGMSG_NPC_NUMBER:
            Npc::integerReceived(client.character, message.readInt32());
            break;
        case PGMSG_NPC_STRING:
            Npc::stringReceived(client.character, message.readString());
            break;
        case PGMSG_NPC_TALK:
            Npc::start(npc, client.character);
            break;
        case PGMSG_NPC_TALK_NEXT:
        default:
            Npc::resume(client.character);
            break;
    }
}

void GameHandler::handlePickup(GameClient &client, MessageIn &message)
{
    const int x = message.readInt16();
    const int y = message.readInt16();
    const Point ppos = client.character->getPosition();

    // TODO: use a less arbitrary value.
    if (std::abs(x - ppos.x) + std::abs(y - ppos.y) < 48)
    {
        MapComposite *map = client.character->getMap();
        Point ipos(x, y);
        for (FixedActorIterator i(map->getAroundPointIterator(ipos, 0)); i; ++i)
        {
            Actor *o = *i;
            Point opos = o->getPosition();

            if (o->getType() == OBJECT_ITEM && opos.x == x && opos.y == y)
            {
                ItemComponent *item = o->getComponent<ItemComponent>();
                ItemClass *ic = item->getItemClass();
                int amount = item->getAmount();

                if (!Inventory(client.character).insert(ic->getDatabaseID(),
                                                        amount))
                {
                    GameState::remove(o);

                    // We only do this when items are to be kept in memory
                    // between two server restart.
                    if (!Configuration::getValue("game_floorItemDecayTime", 0))
                    {
                        // Remove the floor item from map
                        accountHandler->removeFloorItems(map->getID(),
                                                         ic->getDatabaseID(),
                                                         amount, x, y);
                    }

                    // log transaction
                    std::stringstream str;
                    str << "User picked up item " << ic->getDatabaseID()
                        << " at " << opos.x << "x" << opos.y;
                    accountHandler->sendTransaction(
                                              client.character->getDatabaseID(),
                                              TRANS_ITEM_PICKUP, str.str()
                                                   );
                }
                break;
            }
        }
    }
}

void GameHandler::handleUseItem(GameClient &client, MessageIn &message)
{
    if (client.character->getAction() == DEAD)
        return;

    const int slot = message.readInt16();

    Inventory inv(client.character);

    if (ItemClass *ic = itemManager->getItem(inv.getItem(slot)))
    {
        if (ic->hasTrigger(ITT_ACTIVATE))
        {
            std::stringstream str;
            str << "User activated item " << ic->getDatabaseID()
                << " from slot " << slot;
            accountHandler->sendTransaction(client.character->getDatabaseID(),
                                            TRANS_ITEM_USED, str.str());
            if (ic->useTrigger(client.character, ITT_ACTIVATE))
                inv.removeFromSlot(slot, 1);
        }
    }
}

void GameHandler::handleDrop(GameClient &client, MessageIn &message)
{
    const int slot = message.readInt16();
    const int amount = message.readInt16();
    Inventory inv(client.character);

    if (ItemClass *ic = itemManager->getItem(inv.getItem(slot)))
    {
        int nb = inv.removeFromSlot(slot, amount);
        MapComposite *map = client.character->getMap();
        Point pos = client.character->getPosition();

        Entity *item = Item::create(map, pos, ic, amount - nb);

        if (!GameState::insertOrDelete(item))
        {
            // The map is full. Put back into inventory.
            inv.insert(ic->getDatabaseID(), amount - nb);
            return;
        }

        // We store the item in database only when the floor items are meant
        // to be persistent between two server restarts.
        if (!Configuration::getValue("game_floorItemDecayTime", 0))
        {
            // Create the floor item on map
            accountHandler->createFloorItems(client.character->getMap()->getID(),
                                             ic->getDatabaseID(),
                                             amount, pos.x, pos.y);
        }

        // log transaction
        std::stringstream str;
        str << "User dropped item " << ic->getDatabaseID()
            << " at " << pos.x << "x" << pos.y;
        accountHandler->sendTransaction(client.character->getDatabaseID(),
                                        TRANS_ITEM_DROP, str.str());
    }
}

void GameHandler::handleWalk(GameClient &client, MessageIn &message)
{
    const int x = message.readInt16();
    const int y = message.readInt16();

    Point dst(x, y);
    client.character->setDestination(dst);
}

void GameHandler::handleEquip(GameClient &client, MessageIn &message)
{
    const int slot = message.readInt16();
    if (!Inventory(client.character).equip(slot))
    {
        MessageOut msg(GPMSG_SAY);
        msg.writeInt16(0); // From the server
        msg.writeString("Unable to equip.");
        client.send(msg);
    }
}

void GameHandler::handleUnequip(GameClient &client, MessageIn &message)
{
    const int itemInstance = message.readInt16();
    if (!Inventory(client.character).unequip(itemInstance))
    {
        MessageOut msg(GPMSG_SAY);
        msg.writeInt16(0); // From the server
        msg.writeString("Unable to unequip.");
        client.send(msg);
    }
}

void GameHandler::handleMoveItem(GameClient &client, MessageIn &message)
{
    const int slot1 = message.readInt16();
    const int slot2 = message.readInt16();
    const int amount = message.readInt16();

    Inventory(client.character).move(slot1, slot2, amount);
    // log transaction
    std::stringstream str;
    str << "User moved item "
        << " from slot " << slot1 << " to slot " << slot2;
    accountHandler->sendTransaction(client.character->getDatabaseID(),
                                    TRANS_ITEM_MOVE, str.str());
}

void GameHandler::handleAttack(GameClient &client, MessageIn &message)
{
    int id = message.readInt16();
    LOG_DEBUG("Character " << client.character->getPublicID()
              << " attacked being " << id);

    Being *being = findBeingNear(client.character, id);
    if (being && being->getType() != OBJECT_NPC)
    {
        client.character->setTarget(being);
        client.character->setAction(ATTACK);
    }
}

void GameHandler::handleUseSpecialOnBeing(GameClient &client, MessageIn &message)
{
    if (client.character->getAction() == DEAD)
        return;

    const int specialID = message.readInt8();
    const int targetID = message.readInt16(); // 0 when no target is selected
    Being *being = 0;
    if (targetID != 0)
        being = findBeingNear(client.character, targetID);
    LOG_DEBUG("Character " << client.character->getPublicID()
              << " tries to use his special attack " << specialID);
    client.character->useSpecialOnBeing(specialID, being);
}

void GameHandler::handleUseSpecialOnPoint(GameClient &client, MessageIn &message)
{
    if (client.character->getAction() == DEAD)
        return;

    const int specialID = message.readInt8();
    const int x = message.readInt16();
    const int y = message.readInt16();

    LOG_DEBUG("Character " << client.character->getPublicID()
              << " tries to use his special attack " << specialID);
    client.character->useSpecialOnPoint(specialID, x, y);
}

void GameHandler::handleActionChange(GameClient &client, MessageIn &message)
{
    const BeingAction action = (BeingAction) message.readInt8();
    const BeingAction current = (BeingAction) client.character->getAction();
    bool logActionChange = true;

    switch (action)
    {
        case STAND:
            if (current == SIT)
            {
                client.character->setAction(STAND);
                logActionChange = false;
            }
            break;
        case SIT:
            if (current == STAND)
            {
                client.character->setAction(SIT);
                logActionChange = false;
            }
            break;
        default:
            break;
    }

    // Log the action change only when this is relevant.
    if (logActionChange)
    {
        // log transaction
        std::stringstream str;
        str << "User changed action from " << current << " to " << action;
        accountHandler->sendTransaction(client.character->getDatabaseID(),
                                        TRANS_ACTION_CHANGE, str.str());
    }

}

void GameHandler::handleDirectionChange(GameClient &client, MessageIn &message)
{
    const BeingDirection direction = (BeingDirection) message.readInt8();
    client.character->setDirection(direction);
}

void GameHandler::handleDisconnect(GameClient &client, MessageIn &message)
{
    const bool reconnectAccount = (bool) message.readInt8();

    MessageOut result(GPMSG_DISCONNECT_RESPONSE);
    result.writeInt8(ERRMSG_OK); // It is, when control reaches here

    if (reconnectAccount)
    {
        std::string magic_token(utils::getMagicToken());
        result.writeString(magic_token, MAGIC_TOKEN_LENGTH);
        // No accountserver data, the client should remember that
        accountHandler->playerReconnectAccount(
                    client.character->getDatabaseID(),
                    magic_token);
    }
    accountHandler->sendCharacterData(client.character);

    // Done with the character, also handle possible respawn case
    client.character->disconnected();
    delete client.character;
    client.character = 0;
    client.status = CLIENT_LOGIN;

    client.send(result);
}

void GameHandler::handleTradeRequest(GameClient &client, MessageIn &message)
{
    const int id = message.readInt16();

    if (Trade *t = client.character->getTrading())
        if (t->request(client.character, id))
            return;

    Character *q = findCharacterNear(client.character, id);
    if (!q || q->isBusy())
    {
        client.send(MessageOut(GPMSG_TRADE_CANCEL));
        return;
    }

    new Trade(client.character, q);

    // log transaction
    std::string str;
    str = "User requested trade with " + q->getName();
    accountHandler->sendTransaction(client.character->getDatabaseID(),
                                    TRANS_TRADE_REQUEST, str);
}

void GameHandler::handleTrade(GameClient &client, MessageIn &message)
{
    std::stringstream str;
    Trade *t = client.character->getTrading();
    if (!t)
        return;

    switch (message.getId())
    {
        case PGMSG_TRADE_CANCEL:
            t->cancel();
            break;
        case PGMSG_TRADE_CONFIRM:
            t->confirm(client.character);
            break;
        case PGMSG_TRADE_AGREED:
            t->agree(client.character);
            // log transaction
            accountHandler->sendTransaction(client.character->getDatabaseID(),
                                            TRANS_TRADE_END,
                                            "User finished trading");
            break;
        case PGMSG_TRADE_SET_MONEY:
        {
            int money = message.readInt32();
            t->setMoney(client.character, money);
            // log transaction
            str << "User added " << money << " money to trade.";
            accountHandler->sendTransaction(client.character->getDatabaseID(),
                                            TRANS_TRADE_MONEY, str.str());
        } break;
        case PGMSG_TRADE_ADD_ITEM:
        {
            int slot = message.readInt8();
            t->addItem(client.character, slot, message.readInt8());
            // log transaction
            str << "User add item from slot " << slot;
            accountHandler->sendTransaction(client.character->getDatabaseID(),
                                            TRANS_TRADE_ITEM, str.str());
        } break;
    }
}

void GameHandler::handleNpcBuySell(GameClient &client, MessageIn &message)
{
    BuySell *t = client.character->getBuySell();
    if (!t)
        return;
    const int id = message.readInt16();
    const int amount = message.readInt16();
    t->perform(id, amount);
}

void GameHandler::handleRaiseAttribute(GameClient &client, MessageIn &message)
{
    const int attribute = message.readInt16();
    AttribmodResponseCode retCode;
    retCode = client.character->useCharacterPoint(attribute);

    MessageOut result(GPMSG_RAISE_ATTRIBUTE_RESPONSE);
    result.writeInt8(retCode);
    result.writeInt16(attribute);
    client.send(result);

    if (retCode == ATTRIBMOD_OK)
    {
        accountHandler->updateCharacterPoints(
            client.character->getDatabaseID(),
            client.character->getCharacterPoints(),
            client.character->getCorrectionPoints());

        // log transaction
        std::stringstream str;
        str << "User increased attribute " << attribute;
        accountHandler->sendTransaction(client.character->getDatabaseID(),
                                        TRANS_ATTR_INCREASE, str.str());
    }
}

void GameHandler::handleLowerAttribute(GameClient &client, MessageIn &message)
{
    const int attribute = message.readInt32();
    AttribmodResponseCode retCode;
    retCode = client.character->useCorrectionPoint(attribute);

    MessageOut result(GPMSG_LOWER_ATTRIBUTE_RESPONSE);
    result.writeInt8(retCode);
    result.writeInt16(attribute);
    client.send(result);

    if (retCode == ATTRIBMOD_OK)
    {
        accountHandler->updateCharacterPoints(
            client.character->getDatabaseID(),
            client.character->getCharacterPoints(),
            client.character->getCorrectionPoints());

        // log transaction
        std::stringstream str;
        str << "User decreased attribute " << attribute;
        accountHandler->sendTransaction(client.character->getDatabaseID(),
                                        TRANS_ATTR_DECREASE, str.str());
    }
}

void GameHandler::handleNpcPostSend(GameClient &client, MessageIn &message)
{
    // add the character so that the post man knows them
    postMan->addCharacter(client.character);
    accountHandler->sendPost(client.character, message);
}

void GameHandler::handlePartyInvite(GameClient &client, MessageIn &message)
{
    MapComposite *map = client.character->getMap();
    const int visualRange = Configuration::getValue("game_visualRange", 448);
    std::string invitee = message.readString();

    if (invitee == client.character->getName())
        return;

    for (CharacterIterator it(map->getWholeMapIterator()); it; ++it)
    {
        if ((*it)->getName() == invitee)
        {
            // calculate if the invitee is within the visual range
            const int xInviter = client.character->getPosition().x;
            const int yInviter = client.character->getPosition().y;
            const int xInvitee = (*it)->getPosition().x;
            const int yInvitee = (*it)->getPosition().y;
            const int dx = std::abs(xInviter - xInvitee);
            const int dy = std::abs(yInviter - yInvitee);
            if (visualRange > std::max(dx, dy))
            {
                MessageOut out(GCMSG_PARTY_INVITE);
                out.writeString(client.character->getName());
                out.writeString(invitee);
                accountHandler->send(out);
                return;
            }
            break;
        }
    }

    // Invitee was not found or is too far away
    MessageOut out(GPMSG_PARTY_INVITE_ERROR);
    out.writeString(invitee);
    client.send(out);
}

void GameHandler::handleTriggerEmoticon(GameClient &client, MessageIn &message)
{
    const int id = message.readInt16();
    if (emoteManager->isIdAvailable(id))
        client.character->triggerEmote(id);
}

void GameHandler::sendNpcError(GameClient &client, int id,
                               const std::string &errorMsg)
{
    MessageOut msg(GPMSG_NPC_ERROR);
    msg.writeInt16(id);
    msg.writeString(errorMsg, errorMsg.size());
    client.send(msg);
}
