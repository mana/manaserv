/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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

#include "game-server/monstermanager.hpp"

#include "common/resourcemanager.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/monster.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"

#include <map>

typedef std::map< int, MonsterClass * > MonsterClasses;
static MonsterClasses monsterClasses; /**< Monster reference */
static std::string monsterReferenceFile;

Element elementFromString (const std::string &name)
{
    static std::map<const std::string, Element> table;

    if (table.empty())
    {
        table["neutral"]    = ELEMENT_NEUTRAL;
        table["fire"]       = ELEMENT_FIRE;
        table["water"]      = ELEMENT_WATER;
        table["earth"]      = ELEMENT_EARTH;
        table["air"]        = ELEMENT_AIR;
        table["lightning"]  = ELEMENT_LIGHTNING;
        table["metal"]      = ELEMENT_METAL;
        table["wood"]       = ELEMENT_WOOD;
        table["ice"]        = ELEMENT_ICE;
    }

    std::map<const std::string, Element>::iterator val = table.find(name);

    return val == table.end() ? ELEMENT_ILLEGAL : (*val).second;
}

void MonsterManager::initialize(const std::string &file)
{
    monsterReferenceFile = file;
    reload();
}

void MonsterManager::reload()
{
    std::string absPathFile = ResourceManager::resolve(monsterReferenceFile);
    if (absPathFile.empty()) {
        LOG_ERROR("Monster Manager: Could not find " << monsterReferenceFile << "!");
        return;
    }

    XML::Document doc(absPathFile, false);
    xmlNodePtr rootNode = doc.rootNode();

    if (!rootNode || !xmlStrEqual(rootNode->name, BAD_CAST "monsters"))
    {
        LOG_ERROR("Monster Manager: Error while parsing monster database ("
                  << absPathFile << ")!");
        return;
    }

    LOG_INFO("Loading monster reference: " << absPathFile);
    int nbMonsters = 0;
    for_each_xml_child_node(node, rootNode)
    {
        if (!xmlStrEqual(node->name, BAD_CAST "monster"))
            continue;

        int id = XML::getProperty(node, "id", -1);
        std::string name = XML::getProperty(node, "name", "unnamed");

        if (id == -1)
        {
            LOG_WARN("Monster Manager: There is a monster ("
                     << name << ") without ID in "
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

        for_each_xml_child_node(subnode, node)
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
                float speed = (XML::getFloatProperty(subnode, "speed", -1.0f));
                monster->setMutation(XML::getProperty(subnode, "mutation", 0));

                //checking attributes for completeness and plausibility
                if (monster->getMutation() > 99)
                {
                    LOG_WARN(monsterReferenceFile
                    <<": Mutation of monster #"<<id
                    <<" more than 99% - ignored");
                    monster->setMutation(0);
                }

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
                if (speed == -1.0f)
                {
                    speed = 4.0f;
                    attributesComplete = false;
                }

                if (!attributesComplete) LOG_WARN(monsterReferenceFile
                    << ": Attributes incomplete for monster #" << id);

                //The speed is set in tiles per second in the monsters.xml
                monster->setSpeed(speed);

            }
            else if (xmlStrEqual(subnode->name, BAD_CAST "exp"))
            {
                xmlChar *exp = subnode->xmlChildrenNode->content;
                monster->setExp(atoi((const char*)exp));
                monster->setOptimalLevel(XML::getProperty(subnode, "level", 0));
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
                monster->setAttackDistance(XML::getProperty(subnode, "attack-distance", 0));
            }
            else if (xmlStrEqual(subnode->name, BAD_CAST "attack"))
            {
                MonsterAttack *att = new MonsterAttack;
                att->id = XML::getProperty(subnode, "id", 0);
                att->priority = XML::getProperty(subnode, "priority", 1);
                att->damageFactor = XML::getFloatProperty(subnode, "damage-factor", 1.0f);
                att->preDelay = XML::getProperty(subnode, "pre-delay", 1);
                att->aftDelay = XML::getProperty(subnode, "aft-delay", 0);
                att->range = XML::getProperty(subnode, "range", 0);
                att->scriptFunction = XML::getProperty(subnode, "script-function", "");
                std::string sElement = XML::getProperty(subnode, "element", "neutral");
                att->element = elementFromString(sElement);
                std::string sType = XML::getProperty(subnode, "type", "physical");
                if (sType == "physical") {att->type = DAMAGE_PHYSICAL; }
                else if (sType == "magical" || sType == "magic") {att->type = DAMAGE_MAGICAL; }
                else if (sType == "other") {att->type = DAMAGE_OTHER; }
                else { att->type = -1; }

                if (att->id == 0)
                {
                    LOG_WARN(monsterReferenceFile
                             << ": Attack without ID for monster #"
                             << id << " (" << name << ") - attack ignored");
                }
                else if (att->element == ELEMENT_ILLEGAL)
                {
                    LOG_WARN(monsterReferenceFile
                             << ": Attack with unknown element \""
                             << sElement << "\" for monster #" << id
                             << " (" << name << ") - attack ignored");
                }
                else if (att->type == -1)
                {
                    LOG_WARN(monsterReferenceFile
                             << ": Attack with unknown type \"" << sType << "\""
                             << " for monster #" << id << " (" << name << ")");
                }
                else
                {
                    monster->addAttack(att);
                }

            }
            else if (xmlStrEqual(subnode->name, BAD_CAST "script"))
            {
                xmlChar *filename = subnode->xmlChildrenNode->content;
                std::string val = (char *)filename;
                monster->setScript(val);
            }
        }

        monster->setDrops(drops);
        if (!attributesSet) LOG_WARN(monsterReferenceFile
                                    << ": No attributes defined for monster #"
                                    << id << " (" << name << ")");
        if (!behaviorSet) LOG_WARN(monsterReferenceFile
                            << ": No behavior defined for monster #"
                            << id << " (" << name << ")");
        if (monster->getExp() == -1)
        {
            LOG_WARN(monsterReferenceFile
                    << ": No experience defined for monster #"
                    << id << " (" << name << ")");
            monster->setExp(0);
        }
        ++nbMonsters;
    }

    LOG_INFO("Loaded " << nbMonsters << " monsters from "
             << monsterReferenceFile << '.');
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
    return i != monsterClasses.end() ? i->second : 0;
}
