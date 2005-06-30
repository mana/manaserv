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


#ifndef _TMWSERV_COUNTED_PTR_H_
#define _TMWSERV_COUNTED_PTR_H_


namespace tmwserv
{
namespace utils
{


/**
 * Class for counted reference semantics. It deletes the object to which it
 * refers when the last CountedPtr that refers to it is destroyed.
 *
 * @copyright Nicolai M. Josuttis
 *            (The C++ standard library: a tutorial and reference).
 *
 * Notes from kindjal:
 *     - should we use the Boost's shared_ptr instead?
 *     - reference-counted smart pointers are very useful if we want automatic
 *       memory management with STL containers (e.g. vector of pointers).
 */
template <typename T>
class CountedPtr
{
    public:
        /**
         * Constructor.
         * Initialize pointer with existing pointer.
         *
         * It is required that the pointer p is a return value of new.
         */
        explicit
        CountedPtr(T* p = 0)
                : ptr(p),
                  count(new long(1))
        {
            // NOOP
        }


        /**
         * Destructor.
         * Delete value if this was the last owner.
         */
        ~CountedPtr(void)
            throw()
        {
            dispose();
        }


        /**
         * Copy pointer (one more owner).
         */
        CountedPtr(const CountedPtr<T>& p)
            throw()
                : ptr(p.ptr),
                  count(p.count)
        {
            ++*count;
        }


        /**
         * Assignment (unshare old and share new value).
         */
        CountedPtr<T>&
        operator=(const CountedPtr<T>& p)
            throw()
        {
            if (this != &p) {
                dispose();
                ptr = p.ptr;
                count = p.count;
                ++*count;
            }

            return *this;
        }


        /**
         * Access the value to which the pointer refers.
         */
        T&
        operator*(void) const
            throw()
        {
            return *ptr;
        }


        /**
         * Access the pointer.
         */
        T*
        operator->(void) const
            throw()
        {
            return ptr;
        }


        /**
         * Get the pointer (mimic std::auto_ptr::get()).
         */
        T*
        get(void) const
            throw()
        {
            return ptr;
        }


    private:
        /**
         * Manage the counter and free memory.
         */
        void
        dispose(void)
        {
            if (--*count == 0) {
                delete count;
                delete ptr;
            }
        }


    private:
        T* ptr;      /**< pointer to the value */
        long* count; /**< shared number of owners */
};


} // namespace utils
} // namespace tmwserv


#endif // _TMWSERV_COUNTED_PTR_H_
