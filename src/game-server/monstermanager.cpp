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

#include "game-server/monstermanager.hpp"

#include "defines.h"
#include "game-server/itemmanager.hpp"
#include "game-server/monster.hpp"
#include "game-server/resourcemanager.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"

typedef std::map< int, MonsterClass * > MonsterClasses;
static MonsterClasses monsterClasses; /**< Monster reference */
static std::string monsterReferenceFile;

void MonsterManager::initialize(std::string const &file)
{
    monsterReferenceFile = file;
    reload();
}

void MonsterManager::reload()
{
    int size;
    char *data = ResourceManager::loadFile(monsterReferenceFile, size);

    if (!data) {
        LOG_ERROR("Monster Manager: Could not find "
                  << monsterReferenceFile << "!");
        free(data);
        return;
    }

    xmlDocPtr doc = xmlParseMemory(data, size);
    free(data);

    if (!doc)
    {
        LOG_ERROR("Monster Manager: Error while parsing monster database ("
                  << monsterReferenceFile << ")!");
        return;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "monsters"))
    {
        LOG_ERROR("Monster Manager: " << monsterReferenceFile
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
        std::string name = XML::getProperty(node, "name", "unnamed");

        if (id == 0)
        {
            LOG_WARN("Monster Manager: There is a monster ("<<name<<") without ID in "
                     << monsterReferenceFile << "! It has been ignored.");
            continue;
        }

        MonsterClass *monster;
        MonsterClasses::iterator i = monsterClasses.find(id);
        if (i == monsterClasses.end())
        {
            monster = new MonsterClass(id);
            monsterClasses[id] = monster;
        }
        else
        {
            monster = i->second;
        }

        MonsterDrops drops;
        bool attributesSet = false;
        bool behaviorSet = false;

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
            else if (xmlStrEqual(subnode->name, BAD_CAST "attributes"))
            {
                attributesSet = true;
                monster->setAttribute(BASE_ATTR_HP,
                    XML::getProperty(subnode, "hp", -1));
                monster->setAttribute(BASE_ATTR_PHY_ATK_MIN,
                    XML::getProperty(subnode, "attack-min", -1));
                monster->setAttribute(BASE_ATTR_PHY_ATK_DELTA,
                    XML::getProperty(subnode, "attack-delta", -1));
                monster->setAttribute(BASE_ATTR_MAG_ATK,
                    XML::getProperty(subnode, "attack-magic", -1));
                monster->setAttribute(BASE_ATTR_EVADE,
                    XML::getProperty(subnode, "evade", -1));
                monster->setAttribute(BASE_ATTR_HIT,
                    XML::getProperty(subnode, "hit", -1));
                monster->setAttribute(BASE_ATTR_PHY_RES,
                    XML::getProperty(subnode, "physical-defence", -1));
                monster->setAttribute(BASE_ATTR_MAG_RES,
                    XML::getProperty(subnode, "magical-defence", -1));
                monster->setSize(XML::getProperty(subnode, "size", 0));
                int speed = (XML::getProperty(subnode, "speed", 0));

                //check for completeness
                bool attributesComplete = true;
                for (int i = BASE_ATTR_BEGIN; i < BASE_ATTR_END; i++)
                {
                    if (monster->getAttribute(i) == -1)
                    {
                        attributesComplete = false;
                        monster->setAttribute(i, 0);
                    }
                }
                if (monster->getSize() == 0)
                {
                    monster->setSize(16);
                    attributesComplete = false;
                }
                if (speed == 0)
                {
                    speed = 1;
                    attributesComplete = false;
                }

                if (!attributesComplete) LOG_WARN(monsterReferenceFile
                    <<": Attributes incomplete for monster #"<<id);

                //for usability reasons we set the speed in the monsters.xml as tiles per second
                //instead of miliseconds per tile.
                monster->setSpeed(1000/speed);

            }
            else if (xmlStrEqual(subnode->name, BAD_CAST "exp"))
            {
                xmlChar *exp = subnode->xmlChildrenNode->content;
                monster->setExp(atoi((const char*)exp));
            }
            else if (xmlStrEqual(subnode->name, BAD_CAST "behavior"))
            {
                behaviorSet = true;
                if (XML::getProperty(subnode, "aggressive", "") == "true")
                {
                    monster->setAggressive(true);
                }
                monster->setTrackRange(XML::getProperty(subnode, "track-range", 1));
                monster->setStrollRange(XML::getProperty(subnode, "stroll-range", 0) * 32);
            }
        }

        monster->setDrops(drops);
        if (!attributesSet) LOG_WARN(monsterReferenceFile
                                    <<": No attributes defined for monster #"
                                    <<id<<" ("<<name<<")");
        if (!behaviorSet) LOG_WARN(monsterReferenceFile
                            <<": No behavior defined for monster #"
                            <<id<<" ("<<name<<")");
        if (monster->getExp() == -1)
        {
            LOG_WARN(monsterReferenceFile
                    <<": No experience defined for monster #"
                    <<id<<" ("<<name<<")");
            monster->setExp(0);
        }
        ++nbMonsters;
    }

    LOG_INFO("Loaded " << nbMonsters << " monsters from "
             << monsterReferenceFile << '.');

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
