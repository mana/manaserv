/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
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

#ifndef SINGLETON_H
#define SINGLETON_H

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
        static T &instance()
        {
            static T theInstance;
            return theInstance;
        }

    protected:
        /**
         * Default constructor.
         */
        Singleton()
            throw()
        {}

        /**
         * Destructor.
         */
        virtual ~Singleton()
            throw()
        {}

    private:
        /**
         * Copy constructor.
         */
        Singleton(const Singleton &);

        /**
         * Assignment operator.
         */
        Singleton &operator=(const Singleton &);
};

} // namespace utils

#endif // SINGLETON_H
