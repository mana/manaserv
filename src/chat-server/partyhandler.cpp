/*
 *  The Mana Server
 *  Copyright (C) 2008-2010  The Mana World Development Team
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

#include "chathandler.h"
#include "chatclient.h"
#include "party.h"

#include "account-server/storage.h"
#include "account-server/serverhandler.h"

#include "net/messagein.h"
#include "net/messageout.h"

#include "common/manaserv_protocol.h"

#include <algorithm>

using namespace ManaServ;

void updateInfo(ChatClient *client, int partyId)
{
    Character *character = storage->getCharacter(client->characterName);
    GameServerHandler::sendPartyChange(character, partyId);
}

bool ChatHandler::handlePartyJoin(const std::string &invited, const std::string &inviter)
{
    // Get inviting client
    ChatClient *c1 = getClient(inviter);
    if (c1)
    {
        // if party doesnt exist, create it
        if (!c1->party)
        {
            c1->party = new Party();
            // add inviter to the party
            c1->party->addUser(inviter);
            MessageOut out(CPMSG_PARTY_NEW_MEMBER);
            out.writeInt32(c1->characterId);
            out.writeString(inviter);
            c1->send(out);
            // tell game server to update info
            updateInfo(c1, c1->party->getId());
        }

        // Get invited client
        ChatClient *c2 = getClient(invited);
        if (c2)
        {
            // add invited to the party
            c1->party->addUser(invited);
            c2->party = c1->party;
            // was successful so return success to inviter
            MessageOut out(CPMSG_PARTY_INVITE_RESPONSE);
            out.writeString(invited);
            out.writeInt8(ERRMSG_OK);
            c1->send(out);

            // tell everyone a player joined
            informPartyMemberJoined(*c2);

            // tell game server to update info
            updateInfo(c2, c2->party->getId());
            return true;
        }
    }

    // there was an error, return false
    return false;

}

void ChatHandler::handlePartyInvite(MessageIn &msg)
{
    std::string inviterName = msg.readString();
    std::string inviteeName = msg.readString();
    ChatClient *inviter = getClient(inviterName);

    if (!inviter)
        return;

    ChatClient *invitee = getClient(inviteeName);

    if (!invitee)
    {
        // TODO: Send error message
        return;
    }

    ++invitee->numInvites;
    // TODO: Check number of invites
    // and do something if too many in a short time

    // store the invite
    PartyInvite invite;
    invite.mInvited = inviteeName;
    invite.mInviter = inviterName;
    if (inviter->party)
        invite.mPartyId = inviter->party->getId();
    else
        invite.mPartyId = 0;

    mPartyInvitedUsers.push_back(invite);

    MessageOut out(CPMSG_PARTY_INVITED);
    out.writeString(inviterName);
    invitee->send(out);
}

void ChatHandler::handlePartyAcceptInvite(ChatClient &client, MessageIn &msg)
{
    MessageOut out(CPMSG_PARTY_ACCEPT_INVITE_RESPONSE);

    std::string inviter = msg.readString();

    // Check that the player was invited
    std::vector<PartyInvite>::iterator itr, itr_end;
    itr = mPartyInvitedUsers.begin();
    itr_end = mPartyInvitedUsers.end();

    bool found = false;

    while (itr != itr_end)
    {
        if ((*itr).mInvited == client.characterName &&
            (*itr).mInviter == inviter)
        {
            // make them join the party
            if (handlePartyJoin(client.characterName, inviter))
            {
                out.writeInt8(ERRMSG_OK);
                Party::PartyUsers users = client.party->getUsers();
                const unsigned usersSize = users.size();
                for (unsigned i = 0; i < usersSize; i++)
                    out.writeString(users[i]);

                mPartyInvitedUsers.erase(itr);
                found = true;
                break;
            }
        }

        ++itr;
    }

    if (!found)
    {
        out.writeInt8(ERRMSG_FAILURE);
    }

    client.send(out);
}

void ChatHandler::handlePartyQuit(ChatClient &client)
{
    removeUserFromParty(client);
    MessageOut out(CPMSG_PARTY_QUIT_RESPONSE);
    out.writeInt8(ERRMSG_OK);
    client.send(out);

    // tell game server to update info
    updateInfo(&client, 0);
}

void ChatHandler::handlePartyRejectInvite(ChatClient &client, MessageIn &msg)
{
    MessageOut out(CPMSG_PARTY_REJECTED);

    std::string inviter = msg.readString();


    std::vector<PartyInvite>::iterator itr, itr_end;

    itr = mPartyInvitedUsers.begin();
    itr_end = mPartyInvitedUsers.end();
    bool found = false;

    while (itr != itr_end)
    {
        // Check that the player was invited
        if ((*itr).mInvited == client.characterName &&
            (*itr).mInviter == inviter)
        {
            // remove them from invited users list
            mPartyInvitedUsers.erase(itr);
            found = true;
            break;
        }

        ++itr;
    }

    if (!found)
    {
        out.writeInt8(ERRMSG_FAILURE);
    }

    // send rejection to inviter
    ChatClient *inviterClient = getClient(inviter);

    inviterClient->send(out);
}

void ChatHandler::removeUserFromParty(ChatClient &client)
{
    if (client.party)
    {
        client.party->removeUser(client.characterName);
        informPartyMemberQuit(client);

        // if theres less than 1 member left, remove the party
        if (client.party->userCount() < 1)
        {
            delete client.party;
            client.party = 0;
        }
    }
}

void ChatHandler::informPartyMemberQuit(ChatClient &client)
{
    std::map<std::string, ChatClient*>::iterator itr;
    std::map<std::string, ChatClient*>::const_iterator itr_end = mPlayerMap.end();

    for (itr = mPlayerMap.begin(); itr != itr_end; ++itr)
    {
        if (itr->second->party == client.party)
        {
            MessageOut out(CPMSG_PARTY_MEMBER_LEFT);
            out.writeInt32(client.characterId);
            itr->second->send(out);
        }
    }
}

void ChatHandler::informPartyMemberJoined(ChatClient &client)
{
    std::map<std::string, ChatClient*>::iterator itr;
    std::map<std::string, ChatClient*>::const_iterator itr_end = mPlayerMap.end();

    for (itr = mPlayerMap.begin(); itr != itr_end; ++itr)
    {
        if (itr->second->party == client.party)
        {
            MessageOut out(CPMSG_PARTY_NEW_MEMBER);
            out.writeInt32(client.characterId);
            out.writeString(client.characterName);
            itr->second->send(out);
        }
    }
}
