/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
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
 *  $Id$
 */


#ifndef _TMWSERV_SINGLETON_H_
#define _TMWSERV_SINGLETON_H_

namespace utils
{


/**
 * An abstract Meyer's singleton class.
 */
template <typename T>
class Singleton
{
    public:
        /**
         * Create an instance of Singleton.
         *
         * @return the unique instance of Singleton.
         */
        static T&
        instance(void)
        {
            static T theInstance;

            return theInstance;
        }


    protected:
        /**
         * Default constructor.
         */
        Singleton(void)
            throw()
        {
            // NOOP
        }


        /**
         * Destructor.
         */
        virtual
        ~Singleton(void)
            throw()
        {
            // NOOP
        }


    private:
        /**
         * Copy constructor.
         */
        Singleton(const Singleton& rhs);


        /**
         * Assignment operator.
         */
        Singleton&
        operator=(const Singleton& rhs);
};


} // namespace utils

#endif // _TMWSERV_SINGLETON_H_
