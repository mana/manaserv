/*
 *  The Mana World
 *  Copyright 2007 The Mana World Development Team
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

#include <map>

#include "defines.h"
#include "resourcemanager.h"
#include "game-server/itemmanager.hpp"
#include "game-server/monster.hpp"
#include "game-server/monstermanager.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"

typedef std::map< int, MonsterClass * > MonsterClasses;
static MonsterClasses monsterClasses; /**< Monster reference */

void MonsterManager::initialize(std::string const &file)
{
    ResourceManager *resman = ResourceManager::getInstance();
    int size;
    char *data = (char *)resman->loadFile(file, size);

    if (!data) {
        LOG_ERROR("Monster Manager: Could not find " << file << "!");
        free(data);
        return;
    }

    xmlDocPtr doc = xmlParseMemory(data, size);
    free(data);

    if (!doc)
    {
        LOG_ERROR("Monster Manager: Error while parsing item database ("
                  << file << ")!");
        return;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "monsters"))
    {
        LOG_ERROR("Monster Manager: " << file
                  << " is not a valid database file!");
        xmlFreeDoc(doc);
        return;
    }

    LOG_INFO("Loading monster reference...");
    int nbMonsters = 0;
    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (!xmlStrEqual(node->name, BAD_CAST "monster"))
        {
            continue;
        }

        int id = XML::getProperty(node, "id", 0);

        if (id == 0)
        {
            LOG_WARN("Monster Manager: There is a monster without ID in "
                     << file << "! It has been ignored.");
            continue;
        }

        MonsterDrops drops;

        for (xmlNodePtr subnode = node->xmlChildrenNode; subnode != NULL;
             subnode = subnode->next)
        {
            if (xmlStrEqual(subnode->name, BAD_CAST "drop"))
            {
                MonsterDrop drop;
                drop.item = ItemManager::getItem(XML::getProperty(subnode, "item", 0));
                drop.probability = XML::getProperty(subnode, "percent", 0) * 100;
                if (drop.item && drop.probability)
                {
                    drops.push_back(drop);
                }
            }
        }

        MonsterClass *monster = new MonsterClass(id);
        monster->setDrops(drops);
        monsterClasses[id] = monster;
        ++nbMonsters;
    }

    LOG_INFO("Loaded " << nbMonsters << " monsters from " << file << '.');

    xmlFreeDoc(doc);
}

void MonsterManager::deinitialize()
{
    for (MonsterClasses::iterator i = monsterClasses.begin(),
         i_end = monsterClasses.end(); i != i_end; ++i)
    {
        delete i->second;
    }
    monsterClasses.clear();
}

MonsterClass *MonsterManager::getMonster(int id)
{
    MonsterClasses::const_iterator i = monsterClasses.find(id);
    return i != monsterClasses.end() ? i->second : NULL;
}
