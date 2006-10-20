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

#ifndef _TMW_ITEMMANAGER_H
#define _TMW_ITEMMANAGER_H

#include "item.h"

#include <map>

/**
 * The Item Manager loads the item reference database
 * and also offers an API to items information, and more.
 * For item objects, see the WorldItem class.
 */
class ItemManager
{
    public:
        /**
         * Constructor (loads item reference file)
         */
        ItemManager(const std::string &itemReferenceFile);

        /**
         * Destructor
         */
        ~ItemManager();

        /**
         * Gives an Item having the demanded information.
         */
        ItemPtr getItem(const unsigned int itemId)
        { return mItemReference[itemId]; };

        bool use(BeingPtr beingPtr, const unsigned int itemId)
        { return mItemReference[itemId].get()->use(beingPtr); };

        /**
         * Return item Type
         */
        unsigned short getItemType(const unsigned int itemId)
        { return mItemReference[itemId].get()->getItemType(); };

        /**
         * Return Weight of item
         */
        unsigned int getWeight(const unsigned int itemId)
        { return mItemReference[itemId].get()->getWeight(); };

        /**
         * Return gold value of item
         */
        unsigned int getGoldValue(const unsigned int itemId)
        { return mItemReference[itemId].get()->getGoldValue(); };

        /**
         * Return max item per slot
         */
        unsigned short getMaxPerSlot(const unsigned int itemId)
        { return mItemReference[itemId].get()->getMaxPerSlot(); };

        /**
         * Return item's modifiers
         */
        Modifiers
        getItemModifiers(const unsigned int itemId)
        { return mItemReference[itemId].get()->getItemModifiers(); };

    private:
        std::map<unsigned int, ItemPtr> mItemReference; /**< Item reference */
};

extern ItemManager *itemManager;

#endif
