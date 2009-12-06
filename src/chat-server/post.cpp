/*
 *  The Mana Server
 *  Copyright (C) 2008  The Mana World Development Team
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

std::string Letter::getContents() const
{
    return mContents;
}

bool Letter::addAttachment(InventoryItem item)
{
    unsigned int max = Configuration::getValue("mail_maxAttachments", 3);
    if (mAttachments.size() > max)
    {
        return false;
    }

    mAttachments.push_back(item);

    return true;
}

Character *Letter::getReceiver() const
{
    return mReceiver;
}

Character *Letter::getSender() const
{
    return mSender;
}

std::vector<InventoryItem> Letter::getAttachments() const
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
    unsigned int max = Configuration::getValue("mail_maxLetters", 10);
    if (mLetters.size() > max)
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

Post *PostManager::getPost(Character *player) const
{
    std::map<Character*, Post*>::const_iterator itr = mPostBox.find(player);
    return (itr == mPostBox.end()) ? NULL : itr->second;
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
