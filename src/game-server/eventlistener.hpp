/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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

#ifndef GAMESERVER_EVENTLISTENER_HPP
#define GAMESERVER_EVENTLISTENER_HPP

class Thing;
class Being;
class Character;

struct EventDispatch;

/**
 * Pointer to a dispatch table.
 */
struct EventListener
{
    const EventDispatch *dispatch;
    EventListener(const EventDispatch *d): dispatch(d) {}
};

/**
 * Dispatch table for event notification.
 */
struct EventDispatch
{
    /**
     * Called just after something is inserted in a map.
     */
    void (*inserted)(const EventListener *, Thing *);

    /**
     * Called just before something is removed from a map.
     */
    void (*removed)(const EventListener *, Thing *);

    /**
     * Called just after a being has died.
     */
    void (*died)(const EventListener *, Being *);

    /**
     * Called just before a character is deleted.
     */
    void (*disconnected)(const EventListener *, Character *);

    /**
     * Initializes dispatch methods as missing.
     */
    EventDispatch():
        inserted(0), removed(0), died(0), disconnected(0)
    {}
};

/**
 * Helper for using member functions as dispatch methods. The 3-level structure
 * is due to default template parameter not being allowed on functions yet.
 * Conceptually, this helper takes two parameters: the name of the member
 * variable pointing to the dispatch table and the name of the member function
 * to call on dispatch. With these two parameters, it creates a dispatch
 * method. When called, this free function forwards the call to the member
 * function.
 * Pseudo-syntax for getting a dispatch method:
 * <code>&amp;EventListenerFactory&lt; _, DispatchPointerName &gt;::create&lt; _, MemberFunctionName &gt;::function</code>
 * See the start of the spawnarea.cpp file for a complete example.
 */
template< class T, EventListener T::*D >
struct EventListenerFactory
{
    template< class U, void (T::*F)(U *), class V = U >
    struct create
    {
        static void function(const EventListener *d, V *u)
        {
            /* Get the address of the T object by substracting the offset of D
               from the pointer d. */
            T *t = (T *)((char *)d -
                   ((char *)&(((T *)42)->*D) - (char *)&(*(T *)42)));
            // Then call the method F of this T object.
            (t->*F)(u);
        }
    };
};

#endif
