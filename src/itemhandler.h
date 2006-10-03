/*
 *  The Mana World
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
 *  $Id: $
 */

#ifndef _TMW_ITEMHANDLER_H
#define _TMW_ITEMHANDLER_H

#include <item.h>
#include <mapcomposite.h>

/**
 *  The Item Handler loads the item reference database
 *  and also manage everyone items interaction with other objects
 *  (including other players) and the world.
 */
class ItemHandler
{
    public:
        ItemHandler(std::string itemReferenceFile);

        /**
         * Drop items on the map.
         */
        bool
        drop(BeingPtr beingPtr, unsigned int itemId, unsigned short amount);

        /**
         * Pick an item on the ground
         */
        bool
        getItem(BeingPtr beingPtr, ItemPtr itemPtr);

    private:
        std::pair<unsigned int, ItemPtr> ItemReference;
};

#endif
