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

#include "gamehandler.h"

#include <cassert>
#include <iostream>
#include <map>

#include "messagein.h"
#include "messageout.h"
#include "netcomputer.h"
#include "packet.h"
#include "state.h"
#include "utils/logger.h"

using tmwserv::BeingPtr;

class GameClient: public NetComputer
{
    public:
        /**
         * Constructor.
         */
        GameClient(GameHandler *, ENetPeer *);

        /**
         * Destructor.
         */
        ~GameClient();

        /**
         * Set the selected character associated with connection.
         */
        void setCharacter(BeingPtr ch);

        /**
         * Deselect the character associated with connection.
         */
        void unsetCharacter();

        /**
         * Get character associated with the connection.
         */
        BeingPtr getCharacter() { return mCharacterPtr; }

    private:
        /** Character associated with the conneciton. */
        BeingPtr mCharacterPtr;
};

GameClient::GameClient(GameHandler *handler, ENetPeer *peer):
    NetComputer(handler, peer),
    mCharacterPtr(NULL)
{
}

GameClient::~GameClient()
{
    unsetCharacter();
}


void GameClient::setCharacter(tmwserv::BeingPtr ch)
{
    assert(mCharacterPtr.get() == NULL);
    tmwserv::State &state = tmwserv::State::instance();
    mCharacterPtr = ch;
    state.addBeing(mCharacterPtr, mCharacterPtr->getMapId());
}

void GameClient::unsetCharacter()
{
    if (mCharacterPtr.get() == NULL) return;
    // remove being from world
    tmwserv::State &state = tmwserv::State::instance();
    state.removeBeing(mCharacterPtr);
    mCharacterPtr = tmwserv::BeingPtr(NULL);
}

struct GamePendingLogin
{
    tmwserv::BeingPtr character;
    int timeout;
};

typedef std::map< std::string, GamePendingLogin > GamePendingLogins;
static GamePendingLogins pendingLogins;

typedef std::map< std::string, GameClient * > GamePendingClients;
static GamePendingClients pendingClients;

void registerGameClient(std::string const &token, tmwserv::BeingPtr ch)
{
    GamePendingClients::iterator i = pendingClients.find(token);
    if (i != pendingClients.end())
    {
        GameClient *computer = i->second;
        computer->setCharacter(ch);
        pendingClients.erase(i);
        MessageOut result;
        result.writeShort(GPMSG_CONNECT_RESPONSE);
        result.writeByte(ERRMSG_OK);
        computer->send(result.getPacket());
    }
    else
    {
        GamePendingLogin p;
        p.character = ch;
        p.timeout = 300; // world ticks
        pendingLogins.insert(std::make_pair(token, p));
    }
}

void GameHandler::removeOutdatedPending()
{
    GamePendingLogins::iterator i = pendingLogins.begin(), next;
    while (i != pendingLogins.end())
    {
        next = i; ++next;
        if (--i->second.timeout <= 0) pendingLogins.erase(i);
        i = next;
    }
}

NetComputer *GameHandler::computerConnected(ENetPeer *peer)
{
    return new GameClient(this, peer);
}

void GameHandler::computerDisconnected(NetComputer *computer)
{
    for (GamePendingClients::iterator i = pendingClients.begin(), i_end = pendingClients.end();
         i != i_end; ++i)
    {
        if (i->second == computer)
        {
            pendingClients.erase(i);
            break;
        }
    }
    delete computer;
}

void GameHandler::process()
{
    ConnectionHandler::process();
    removeOutdatedPending();
}

void GameHandler::processMessage(NetComputer *comp, MessageIn &message)
{
    GameClient &computer = *static_cast< GameClient * >(comp);
    MessageOut result;

    if (computer.getCharacter().get() == NULL) {
        if (message.getId() != PGMSG_CONNECT) return;
        std::string magic_token = message.readString(32);
        GamePendingLogins::iterator i = pendingLogins.find(magic_token);
        if (i == pendingLogins.end())
        {
            for (GamePendingClients::iterator i = pendingClients.begin(), i_end = pendingClients.end();
                 i != i_end; ++i) {
                if (i->second == &computer) return;
            }
            pendingClients.insert(std::make_pair(magic_token, &computer));
            return;
        }
        computer.setCharacter(i->second.character);
        pendingLogins.erase(i);
        result.writeShort(GPMSG_CONNECT_RESPONSE);
        result.writeByte(ERRMSG_OK);
        computer.send(result.getPacket());
        return;
    }

    switch (message.getId())
    {
        case PGMSG_SAY:
            {
                std::string say = message.readString();
                sayAround(computer, say);
            } break;

        case PGMSG_PICKUP:
            {
                // add item to inventory (this is too simplistic atm)
                unsigned int itemId = message.readLong();

                // remove the item from world map

                // send feedback
                computer.getCharacter()->addInventory(itemId);
                result.writeShort(GPMSG_PICKUP_RESPONSE);
                result.writeByte(ERRMSG_OK);
            } break;

        case PGMSG_USE_ITEM:
            {
                unsigned int itemId = message.readLong();

                result.writeShort(GPMSG_USE_RESPONSE);

                if (computer.getCharacter()->hasItem(itemId)) {
                    // use item
                    // this should execute a script which will do the appropriate action
                    // (the script will determine if the item is 1 use only)
                    result.writeByte(ERRMSG_OK);                    
                } else {
                    result.writeByte(ERRMSG_FAILURE);
                }
            } break;

        case PGMSG_WALK:
            {
                long x = message.readLong();
                long y = message.readLong();

                // simplistic "teleport" walk
                computer.getCharacter()->setX(x);
                computer.getCharacter()->setY(y);

                // no response should be required
            } break;

        case PGMSG_EQUIP:
            {
                int itemId = message.readLong();
                char slot = message.readByte();

                result.writeShort(GPMSG_EQUIP_RESPONSE);
                result.writeByte(computer.getCharacter()->equip(itemId, slot) ?
                                 ERRMSG_OK : ERRMSG_FAILURE);
            } break;

        default:
            LOG_WARN("Invalid message type", 0);
            result.writeShort(XXMSG_INVALID);
            break;
    }

    if (result.getPacket()->length > 0)
        computer.send(result.getPacket());
}

void GameHandler::sayAround(GameClient &computer, std::string const &text)
{
    BeingPtr beingPtr = computer.getCharacter();
    MessageOut msg;
    msg.writeShort(GPMSG_SAY);
    msg.writeString(beingPtr->getName());
    msg.writeString(text);
    unsigned speakerMapId = beingPtr->getMapId();
    std::pair<unsigned, unsigned> speakerXY = beingPtr->getXY();
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i)
    {
        // See if the other being is near enough, then send the message
        tmwserv::Being const *listener = static_cast< GameClient * >(*i)->getCharacter().get();
        if (!listener || listener->getMapId() != speakerMapId) continue;
        std::pair<unsigned, unsigned> listenerXY = listener->getXY();
        if (abs(listenerXY.first  - speakerXY.first ) > (int)AROUND_AREA_IN_TILES) continue;
        if (abs(listenerXY.second - speakerXY.second) > (int)AROUND_AREA_IN_TILES) continue;
        (*i)->send(msg.getPacket());
    }
}

void GameHandler::sendTo(BeingPtr beingPtr, MessageOut &msg)
{
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i) {
        if (static_cast< GameClient * >(*i)->getCharacter().get() == beingPtr.get()) {
            (*i)->send(msg.getPacket());
            break;
        }
    }
}
