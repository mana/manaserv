/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * $Id$
 */

#ifndef _TMW_TOKENCOLLECTOR_HPP
#define _TMW_TOKENCOLLECTOR_HPP

#include <string>
#include <list>
#include <time.h>

/**
 * Base class containing the generic implementation of TokenCollector.
 */
class TokenCollectorBase
{
        struct Item
        {
            std::string token; /**< Cookie used by the client. */
            intptr_t data;     /**< User data. */
            time_t timeStamp;  /**< Creation time. */
        };

        /**
         * List containing client already connected. Newer clients are at the
         * back of the list.
         */
        std::list<Item> mPendingClients;

        /**
         * List containing server data waiting for clients. Newer data are at
         * the back of the list.
         */
        std::list<Item> mPendingConnects;

        /**
         * Time at which the TokenCollector performed its last check.
         */
        time_t mLastCheck;

    protected:

        virtual void removedClient(intptr_t) = 0;
        virtual void removedConnect(intptr_t) = 0;
        virtual void foundMatch(intptr_t client, intptr_t connect) = 0;
        TokenCollectorBase();
        virtual ~TokenCollectorBase();
        void insertClient(std::string const &, intptr_t);
        void removeClient(intptr_t);
        void insertConnect(std::string const &, intptr_t);
        void removeOutdated(time_t);
};

/**
 * Compile-time check to ensure that Client and ServerData are simple enough
 * for TokenCollector.
 */
template< class T > struct _TC_CheckData;
template<> struct _TC_CheckData< int > {};
template< class T > struct _TC_CheckData< T * > {};

/**
 * A class for storing and matching tokens.
 *
 * The Handler class must provide three member functions:
 *  - deletePendingClient(Client),
 *  - deletePendingConnect(ServerData), and
 *  - tokenMatched(Client, ServerData).
 *
 * The delete members will be called whenever the collector considers that a
 * token has become obsolete and it is about to remove it.
 */
template< class Handler, class Client, class ServerData >
class TokenCollector: private TokenCollectorBase
{

    public:

        TokenCollector(Handler *h): mHandler(h)
        {
            _TC_CheckData<Client> ClientMustBeSimple;
            (void)&ClientMustBeSimple;
            _TC_CheckData<ServerData> ServerDataMustBeSimple;
            (void)&ServerDataMustBeSimple;
        }

        /**
         * Checks if the server expected this client token. If so, calls
         * Handler::tokenMatched. Otherwise marks the client as pending.
         */
        void addPendingClient(std::string const &token, Client data)
        { insertClient(token, (intptr_t)data); }

        /**
         * Checks if a client already registered this token. If so, calls
         * Handler::tokenMatched. Otherwise marks the data as pending.
         */
        void addPendingConnect(std::string const &token, ServerData data)
        { insertConnect(token, (intptr_t)data); }

        /**
         * Removes a pending client.
         * @note Does not call destroyPendingClient.
         */
        void deletePendingClient(Client data)
        { removeClient((intptr_t)data); }

    private:

        void removedClient(intptr_t data)
        { mHandler->deletePendingClient((Client)data); }

        void removedConnect(intptr_t data)
        { mHandler->deletePendingConnect((ServerData)data); }

        void foundMatch(intptr_t client, intptr_t data)
        { mHandler->tokenMatched((Client)client, (ServerData)data); }

        Handler *mHandler;
};

#endif // _TMW_TOKENCOLLECTOR_HPP
