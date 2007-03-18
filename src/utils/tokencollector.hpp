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

#include "utils/logger.h"

#define NB_ACTIONS_BETWEEN_OUTDATED_CHECKS 100

/**
 * Because the timeStamp is not updated for every new list item and the
 * removeOutdated function is not called on regular intervals, a list item
 * might persist significantly longer then this.
 */
#define TIMEOUT 10 // in seconds


/**
 * A structure used for the list items,
 * prefixed with 'TC_' to avoid any conflicts.
 */
template <typename T>
struct TC_ListItem
{
    /** The magic_token aka passToken */
    std::string token;

    /** The actual data, which will be sent back when matched */
    T payload;

    /**
     * The amount of seconds since TokenCollector was running,
     * at the last outdated check.
     */
    time_t timeStamp;
};

// Prototype

/**
 * \brief A class for storing and matching magic_tokens
 *
 *  T is used for the class of the owner, U is the client payload,
 *  V is the connect payload
 *
 * The owning class must implement as it's member functions;
 * - destroyPendingClient(U clientPayload),
 * - destroyPendingConnect(V connectPayload) and
 * - tokenMatched(U clientPayload, V connectPayload),
 * in a way that TokenCollector can reach them
 * (public, or declare TokenCollector a friend class).
 */
template <class T, class U, class V>
class TokenCollector
{
        public:
        /**
         * Constructor.
         */
        TokenCollector(T * owner);

        /**
         * Destructor.
         */
        ~TokenCollector();

        /**
         * \brief Adds a pending client
         *
         * Searches the pending connects for a match.
         * Calls mOwner->tokenMatched when a match is found.
         * Inserts in mPendingClients if no match is found.
         */
        void addPendingClient(const std::string & token, U clientPayload);

        /**
         * \brief Removes a pending client from the list
         *
         * Searches the pending client for the payload.
         */
        void deletePendingClient(U clientPayload);

        /**
         * \brief Adds a pending connect
         *
         * Searches the pending clients for a match.
         * Calls mOwner->tokenMatched when a match is found.
         * Inserts in mPendingConnects if no match is found.
         */
        void addPendingConnect(const std::string & token, V connectPayload);

    private:

        /**
         * \brief A simple list of all pending clients
         *
         * On a well setup system, this list is mostly empty.
         * The servers send the magic_token to the other server, before they
         * send it to the client. In a well setup system, the route
         * server1 -> server2 is faster then server1 -> client -> server2.
         *
         * A std::list is used because the basic work cycle for one client
         * (search PendingClients, insert PendingConnects, search
         * PendingConnects, remove pendingConnect) will run in (allmost)
         * constant time, when the size of PendingClients is small.
         * This can not be said for a map or sorted vector.
         *
         * The list items are pointers to void, because a list can not be
         * instantiationised with an incomplete type.
         */
        std::list< void* > mPendingClients;

        /**
         * \brief A simple list of all pending clients.
         *
         * See mPendingClients for details.
         */
        std::list< void* > mPendingConnects;

        /**
         * \brief The number of actions since the last time that the lists
         *        where checked for outdated pendingClients or
         *        pendingConnects.
         */
        int mNumberOfActions;

        /**
         * \brief The number of seconds between the creation of TokenCollector
         *        and the last removeOutdated.
         */
        time_t mTimeNow;

        /**
         * \brief The time that TokenCollector was created, used for keeping
         *        the variable times low numbers.
         */
        time_t mTimeStart;

        /**
         * \brief Pointer to the owner of this TokenCollector object.
         */
        T * mOwner;

        /**
         * \brief Removes outdated entries.
         */
        void removeOutdated();
};

// Implementation

template <class T, class U, class V>
TokenCollector<T, U, V>::
TokenCollector(T * owner):
    mNumberOfActions(0), mTimeStart(time(NULL)), mTimeNow(0), mOwner(owner)
{
}

template <class T, class U, class V>
TokenCollector<T, U, V>::
~TokenCollector()
{
    if (mPendingClients.size())
        LOG_INFO("TokenCollector deleted with " <<
                 mPendingClients.size() <<
                 " clients still pending.");

    if (mPendingConnects.size())
        LOG_INFO("TokenCollector deleted with " <<
                 mPendingConnects.size() <<
                 " connects still pending.");
}

template <class T, class U, class V>
void TokenCollector<T, U, V>::
addPendingClient(const std::string & token, U clientPayload)
{
    // Find could also be used for a list, but because new items are
    // inserted at the top, we want to start looking there.
    for (std::list< void* >::iterator it = mPendingConnects.begin(),
                          it_end = mPendingConnects.end(); it != it_end; it++)
    {
        // Because the pointer to the listItem was stored as a pointer to
        // void, we have to cast it back to a pointer to a listItem.
        // Examples: it = void**; *it = void*;
        // ((TC_ListItem< V >*)*it) = TC_ListItem< V >*;
        // ---------------------------------------------
        if (((TC_ListItem< V >*)*it)->token == token) // Found a match
        {
            mOwner->tokenMatched(clientPayload,
                                    ((TC_ListItem< V >*)*it)->payload);
            delete ((TC_ListItem< V >*)*it);
            mPendingConnects.erase(it);
            return; // Done
        }
    }

    TC_ListItem< U >* listItem = new TC_ListItem< U >;
    listItem->token = token;
    listItem->payload = clientPayload;
    listItem->timeStamp = mTimeNow;

    mPendingClients.push_front((void*)listItem);

    if (!(++mNumberOfActions < NB_ACTIONS_BETWEEN_OUTDATED_CHECKS))
        removeOutdated();
}

template <class T, class U, class V>
void TokenCollector<T, U, V>::
deletePendingClient(U clientPayload)
{
    // Find could also be used for a list, but because new items are
    // inserted at the top, we want to start looking there.
    for (std::list< void* >::iterator it = mPendingClients.begin(),
                          it_end = mPendingClients.end(); it != it_end; it++)
    {
        // Because the pointer to the listItem was stored as a pointer to
        // void, we have to cast it back to a pointer to a listItem.
        // Examples: it = void**; *it = void*;
        // ((TC_ListItem< U >*)*it) = TC_ListItem< U >*;
        // ---------------------------------------------
        if (((TC_ListItem< U >*)*it)->payload == clientPayload) // Found a match
        {
            delete ((TC_ListItem< U >*)*it);
            mPendingConnects.erase(it);
            return; // Done
        }
    }
}

template <class T, class U, class V>
void TokenCollector<T, U, V>::
addPendingConnect(const std::string & token, V connectPayload)
{
    // Find could also be used for a list, but because new items are
    // inserted at the top, we want to start looking there.
    for (std::list< void* >::iterator it = mPendingClients.begin(),
                          it_end = mPendingClients.end(); it != it_end; it++)
    {
        // Because the pointer to the listItem was stored as a pointer to
        // void, we have to cast it back to a pointer to a listItem.
        // Examples: it = void**; *it = void*;
        // ((TC_ListItem< U >*)*it) = TC_ListItem< U >*;
        // ---------------------------------------------
        if (((TC_ListItem< U >*)*it)->token == token) // Found a match
        {
            mOwner->tokenMatched(((TC_ListItem< U >*)*it)->payload,
                                                              connectPayload);
            delete ((TC_ListItem< U >*)*it);
            mPendingClients.erase(it);
            return; // Done
        }
    }

    TC_ListItem< V >* listItem = new TC_ListItem< V >;
    listItem->token = token;
    listItem->payload = connectPayload;
    listItem->timeStamp = mTimeNow;

    mPendingConnects.push_front((void*)listItem);

    if (!(++mNumberOfActions < NB_ACTIONS_BETWEEN_OUTDATED_CHECKS))
        removeOutdated();
}

template <class T, class U, class V>
void TokenCollector<T, U, V>::
removeOutdated()
{
    time_t eraseTime = (mTimeNow > (time_t)TIMEOUT) ?
                                    (mTimeNow - (time_t)TIMEOUT) : (time_t) 0;

    // See addPendingClient for a comment about the casting.
    while ((mPendingClients.size()) &&
       (((TC_ListItem< U >*)(mPendingClients.back()))->timeStamp < eraseTime))
    {
        mOwner->deletePendingClient(
                      ((TC_ListItem< U >*)(mPendingClients.back()))->payload);
        delete ((TC_ListItem< U >*)(mPendingClients.back()));
        mPendingClients.pop_back();
    }

    while ((mPendingConnects.size()) &&
       (((TC_ListItem< V >*)(mPendingConnects.back()))->timeStamp < eraseTime))
    {
        mOwner->deletePendingConnect(
                     ((TC_ListItem< V >*)(mPendingConnects.back()))->payload);
        delete ((TC_ListItem< U >*)(mPendingConnects.back()));
        mPendingConnects.pop_back();
    }

    /**
     * Change the timeStap after the check, else everything that was just
     * inserted might be thrown away.
     */
    mTimeNow = time(NULL) - mTimeStart;

    mNumberOfActions = 0; // Reset the counter.
}

#endif // _TMW_TOKENCOLLECTOR_HPP
