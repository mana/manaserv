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

#include "game-server/monstermanager.h"

#include "common/defines.h"

#include "game-server/attributemanager.h"
#include "game-server/itemmanager.h"
#include "game-server/monster.h"
#include "utils/logger.h"

#define MAX_MUTATION 99
#define DEFAULT_MONSTER_SIZE 16
#define DEFAULT_MONSTER_SPEED 4.0f

void MonsterManager::reload()
{
    deinitialize();
    initialize();
}

void MonsterManager::initialize()
{

}

void MonsterManager::deinitialize()
{
    for (MonsterClasses::iterator i = mMonsterClasses.begin(),
         i_end = mMonsterClasses.end(); i != i_end; ++i)
    {
        delete i->second;
    }
    mMonsterClasses.clear();
    mMonsterClassesByName.clear();
}

MonsterClass *MonsterManager::getMonsterByName(const std::string &name) const
{
    return mMonsterClassesByName.value(name);
}

MonsterClass *MonsterManager::getMonster(int id) const
{
    MonsterClasses::const_iterator i = mMonsterClasses.find(id);
    return i != mMonsterClasses.end() ? i->second : 0;
}

/**
 * Read a <monster> element from settings.
 * Used by SettingsManager.
 */
void MonsterManager::readMonsterNode(xmlNodePtr node, const std::string &filename)
{
    if (!xmlStrEqual(node->name, BAD_CAST "monster"))
        return;

    int id = XML::getProperty(node, "id", 0);
    std::string name = XML::getProperty(node, "name", std::string());

    if (id < 1)
    {
        LOG_WARN("Monster Manager: Ignoring monster ("
                 << name << ") without Id in "
                 << filename << "! It has been ignored.");
        return;
    }

    MonsterClasses::iterator i = mMonsterClasses.find(id);
    if (i != mMonsterClasses.end())
    {
        LOG_WARN("Monster Manager: Ignoring duplicate definition of "
                 "monster '" << id << "'!");
        return;
    }

    MonsterClass *monster = new MonsterClass(id);
    mMonsterClasses[id] = monster;

    if (!name.empty())
    {
        monster->setName(name);

        if (mMonsterClassesByName.contains(name))
            LOG_WARN("Monster Manager: Name not unique for monster " << id);
        else
            mMonsterClassesByName.insert(name, monster);
    }

    MonsterDrops drops;
    bool attributesSet = false;
    bool behaviorSet = false;

    for_each_xml_child_node(subnode, node)
    {
        if (xmlStrEqual(subnode->name, BAD_CAST "drop"))
        {
            MonsterDrop drop;
            std::string item = XML::getProperty(subnode, "item",
                                                std::string());
            ItemClass *itemClass;
            if (utils::isNumeric(item))
                itemClass = itemManager->getItem(utils::stringToInt(item));
            else
                itemClass = itemManager->getItemByName(item);

            if (!itemClass)
            {
                LOG_WARN("Monster Manager: Invalid item name \"" << item
                         << "\"");
                break;
            }

            drop.item = itemClass;
            drop.probability = XML::getFloatProperty(subnode, "percent",
                                                     0.0) * 100 + 0.5;

            if (drop.probability)
                drops.push_back(drop);
        }
        else if (xmlStrEqual(subnode->name, BAD_CAST "attributes"))
        {
            attributesSet = true;

            const int hp = XML::getProperty(subnode, "hp", -1);
            monster->setAttribute(ATTR_MAX_HP, hp);
            monster->setAttribute(ATTR_HP, hp);

            monster->setAttribute(ATTR_DODGE,
                XML::getProperty(subnode, "evade", -1));
            monster->setAttribute(ATTR_MAGIC_DODGE,
                XML::getProperty(subnode, "magic-evade", -1));
            monster->setAttribute(ATTR_ACCURACY,
                XML::getProperty(subnode, "hit", -1));
            monster->setAttribute(ATTR_DEFENSE,
                XML::getProperty(subnode, "physical-defence", -1));
            monster->setAttribute(ATTR_MAGIC_DEFENSE,
                XML::getProperty(subnode, "magical-defence", -1));
            monster->setSize(XML::getProperty(subnode, "size", -1));
            float speed = (XML::getFloatProperty(subnode, "speed", -1.0f));
            monster->setMutation(XML::getProperty(subnode, "mutation", 0));
            std::string genderString = XML::getProperty(subnode, "gender",
                                                        std::string());
            monster->setGender(getGender(genderString));

            // Checking attributes for completeness and plausibility
            if (monster->getMutation() > MAX_MUTATION)
            {
                LOG_WARN(filename
                << ": Mutation of monster Id:" << id << " more than "
                << MAX_MUTATION << "%. Defaulted to 0.");
                monster->setMutation(0);
            }

            bool attributesComplete = true;
            const AttributeManager::AttributeScope &mobAttr =
                        attributeManager->getAttributeScope(MonsterScope);

            for (AttributeManager::AttributeScope::const_iterator it =
                mobAttr.begin(), it_end = mobAttr.end(); it != it_end; ++it)
            {
                if (!monster->mAttributes.count(it->first))
                {
                    LOG_WARN(filename << ": No attribute "
                             << it->first << " for monster Id: "
                             << id << ". Defaulted to 0.");
                    attributesComplete = false;
                    monster->setAttribute(it->first, 0);
                }
            }

            if (monster->getSize() == -1)
            {
                LOG_WARN(filename
                         << ": No size set for monster Id:" << id << ". "
                         << "Defaulted to " << DEFAULT_MONSTER_SIZE
                         << " pixels.");
                monster->setSize(DEFAULT_MONSTER_SIZE);
                attributesComplete = false;
            }

            if (speed == -1.0f)
            {
                LOG_WARN(filename
                         << ": No speed set for monster Id:" << id << ". "
                         << "Defaulted to " << DEFAULT_MONSTER_SPEED
                         << " tiles/second.");
                speed = DEFAULT_MONSTER_SPEED;
                attributesComplete = false;
            }
            monster->setAttribute(ATTR_MOVE_SPEED_TPS, speed);

            if (!attributesComplete)
            {
                LOG_WARN(filename
                         << ": Attributes incomplete for monster Id:" << id
                         << ". Defaults values may have been applied!");
            }

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
            if (XML::getBoolProperty(subnode, "aggressive", false))
                monster->setAggressive(true);

            monster->setTrackRange(
                           XML::getProperty(subnode, "track-range", 1));
            monster->setStrollRange(
                           XML::getProperty(subnode, "stroll-range", 0));
            monster->setAttackDistance(
                           XML::getProperty(subnode, "attack-distance", 0));
        }
        else if (xmlStrEqual(subnode->name, BAD_CAST "vulnerability"))
        {
            Element element = elementFromString(
                    XML::getProperty(subnode, "element", std::string()));
            double factor =  XML::getFloatProperty(subnode, "factor", 1.0);
            monster->setVulnerability(element, factor);
        }
    }

    monster->setDrops(drops);
    if (!attributesSet)
    {
        LOG_WARN(filename
                 << ": No attributes defined for monster Id:" << id
                 << " (" << name << ")");
    }
    if (!behaviorSet)
    {
        LOG_WARN(filename
            << ": No behavior defined for monster Id:" << id
            << " (" << name << ")");
    }
    if (monster->getExp() == -1)
    {
        LOG_WARN(filename
                << ": No experience defined for monster Id:" << id
                << " (" << name << ")");
        monster->setExp(0);
    }
}

/**
 * Check the status of recently loaded configuration.
 */
void MonsterManager::checkStatus()
{
    LOG_INFO("Loaded " << mMonsterClasses.size() << " monsters");
}
