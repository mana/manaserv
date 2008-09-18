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

#include "post.hpp"

#include "../defines.h"

Letter::Letter(int type, Character *sender, Character *receiver)
 : mType(type), mSender(sender), mReceiver(receiver)
{

}

void Letter::setExpiry(unsigned long expiry)
{
    mExpiry = expiry;
}

unsigned long Letter::getExpiry() const
{
    return mExpiry;
}

bool Letter::addAttachment(InventoryItem item)
{
    if (mAttachments.size() > MAX_ATTACHMENTS)
    {
        return false;
    }

    mAttachments.push_back(item);

    return true;
}

Character* Letter::getReceiver()
{
    return mReceiver;
}

bool Post::addLetter(Letter *letter)
{
    if (mLetters.size() > MAX_LETTERS)
    {
        return false;
    }

    mLetters.push_back(letter);

    return true;
}

void PostManager::addLetter(Letter *letter)
{
    std::map<Character*, Post*>::iterator itr =
        mPostBox.find(letter->getReceiver());
    if (itr != mPostBox.end())
    {
        itr->second->addLetter(letter);
    }
    else
    {
        Post *post = new Post();
        post->addLetter(letter);
        mPostBox.insert(
            std::pair<Character*, Post*>(letter->getReceiver(), post)
            );
    }
}

Post* PostManager::getPost(Character *player)
{
    std::map<Character*, Post*>::iterator itr =
        mPostBox.find(player);
    if (itr == mPostBox.end())
    {
        return NULL;
    }

    return itr->second;
}
