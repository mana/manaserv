/*
 *  The Mana World
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

#ifndef _TMW_POSTMAN_H_
#define _TMW_POSTMAN_H_

#include <map>

class Character;

struct PostCallback
{
    void (*handler)(Character *, std::string const &sender,
                    std::string const &letter, void *data);
    void *data;
};

class PostMan
{
public:
    Character* getCharacter(int id)
    {
        std::map<int, Character*>::iterator itr = mCharacters.find(id);
        if (itr != mCharacters.end())
        {
            return itr->second;
        }

        return NULL;
    }

    void addCharacter(Character *player)
    {
        std::map<int, Character*>::iterator itr = mCharacters.find(player->getDatabaseID());
        if (itr == mCharacters.end())
        {
            mCharacters.insert(std::pair<int, Character*>(player->getDatabaseID(), player));
        }
    }

    void getPost(Character *player, PostCallback &f)
    {
        mCallbacks.insert(std::pair<Character*, PostCallback>(player, f));
        accountHandler->getPost(player);
    }

    void gotPost(Character *player, std::string sender, std::string letter)
    {
        std::map<Character*, PostCallback>::iterator itr = mCallbacks.find(player);
        if (itr != mCallbacks.end())
        {
            itr->second.handler(player, sender, letter, itr->second.data);
        }
    }

private:
    std::map<int, Character*> mCharacters;
    std::map<Character*, PostCallback> mCallbacks;
};

extern PostMan *postMan;

#endif
