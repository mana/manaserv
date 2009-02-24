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
 */

#include "post.hpp"

#include "../account-server/character.hpp"
#include "../common/configuration.hpp"

Letter::Letter(unsigned int type, Character *sender, Character *receiver)
 : mId(0), mType(type), mSender(sender), mReceiver(receiver)
{

}

Letter::~Letter()
{
    if (mSender)
        delete mSender;

    if (mReceiver)
        delete mReceiver;

}

void Letter::setExpiry(unsigned long expiry)
{
    mExpiry = expiry;
}

unsigned long Letter::getExpiry() const
{
    return mExpiry;
}

void Letter::addText(const std::string &text)
{
    mContents = text;
}

std::string Letter::getContents()
{
    return mContents;
}

bool Letter::addAttachment(InventoryItem item)
{
    if (mAttachments.size() > Configuration::getValue("mail_maxAttachments", 3))
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

Character* Letter::getSender()
{
    return mSender;
}

std::vector<InventoryItem> Letter::getAttachments()
{
    return mAttachments;
}

Post::~Post()
{
    std::vector<Letter*>::iterator itr_end = mLetters.end();
    for (std::vector<Letter*>::iterator itr = mLetters.begin();
         itr != itr_end;
         ++itr)
    {
        delete (*itr);
    }

    mLetters.clear();
}

bool Post::addLetter(Letter *letter)
{
    if (mLetters.size() > Configuration::getValue("mail_maxLetters", 10))
    {
        return false;
    }

    mLetters.push_back(letter);

    return true;
}

Letter* Post::getLetter(int letter) const
{
    if (letter < 0 || (size_t) letter > mLetters.size())
    {
        return NULL;
    }
    return mLetters[letter];
}

unsigned int Post::getNumberOfLetters() const
{
    return mLetters.size();
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

void PostManager::clearPost(Character *player)
{
    std::map<Character*, Post*>::iterator itr =
        mPostBox.find(player);
    if (itr != mPostBox.end())
    {
        delete itr->second;
        mPostBox.erase(itr);
    }
}
