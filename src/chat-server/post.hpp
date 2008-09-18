/*
 *  The Mana World Server
 *  Copyright 2008 The Mana World Development Team
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
 *  $Id$
 */

#ifndef _TMWSERV_POST_H_
#define _TMWSERV_POST_H_

#include <list>
#include <map>
#include <vector>

#include "../common/inventorydata.hpp"

class Item;
class Character;

class Letter
{
public:
    /**
     * Constructor
     * @param type Type of Letter
     */
    Letter(int type, Character *sender, Character *receiver);

    /**
     * Set the expiry
     */
    void setExpiry(unsigned long expiry);

    /**
     * Get the expiry
     */
    unsigned long getExpiry() const;

    /**
     * Add an attachment
     * @param aitem The attachment to add to the letter
     * @return Returns true if the letter doesnt have too many attachments
     */
    bool addAttachment(InventoryItem item);

    /**
     * Get the character receiving the letter
     * @return Returns the Character who will receive the letter
     */
    Character* getReceiver();

private:
    unsigned int mType;
    unsigned long mExpiry;
    std::vector<InventoryItem> mAttachments;
    Character *mSender;
    Character *mReceiver;
};

class Post
{
public:
    /**
     * Add letter to post
     * @param letter Letter to add
     * @return Returns true if post isnt full
     */
     bool addLetter(Letter *letter);

private:
    std::vector<Letter*> mLetters;
};

class PostManager
{
public:
    /**
     * Add letter to post box
     * @param letter Letter to add
     */
    void addLetter(Letter *letter);

    /**
     * Get post for character
     * @param player Character that is getting post
     * @return Returns the post for that character
     */
    Post* getPost(Character *player);

private:
    std::map<Character*, Post*> mPostBox;
};

#endif
