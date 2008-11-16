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
 */

#ifndef _TMWSERV_FUNCTORS_H_
#define _TMWSERV_FUNCTORS_H_


#include <functional>
#include <sstream>
#include <string>


namespace
{


/**
 * Functor used for the search of an object by name in a list.
 *
 * Note:
 *     - this functor assumes that the object defines as public the following
 *       method: std::string getName(void).
 *     - this functor assumes that the list is a list of pointers.
 */
template <typename T>
struct obj_name_is
    : public std::binary_function<T, std::string, bool>
{
    bool
    operator()(const T& obj,
               const std::string& name) const
    {
        return (obj->getName() == name);
    }
};


/**
 * Functor to convert a string into another type using
 * std::istringstream.operator>>().
 */
template <typename T>
struct string_to: public std::unary_function<std::string, T>
{
    T
    operator()(const std::string& s) const
    {
        std::istringstream is(s);
        T value;
        is >> value;

        return value;
    }
};


} // anonymous namespace


#endif // _TMWSERV_FUNCTORS_H_
