/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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

#include "game-server/itemmanager.hpp"

#include "defines.h"
#include "common/resourcemanager.hpp"
#include "game-server/item.hpp"
#include "game-server/skillmanager.hpp"
#include "scripting/script.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"

#include <map>
#include <set>
#include <sstream>

typedef std::map< int, ItemClass * > ItemClasses;
static ItemClasses itemClasses; /**< Item reference */
static std::string itemReferenceFile;
static unsigned int itemDatabaseVersion = 0; /**< Version of the loaded items database file.*/

void ItemManager::initialize(const std::string &file)
{
    itemReferenceFile = file;
    reload();
}

void ItemManager::reload()
{
    std::string absPathFile = ResourceManager::resolve(itemReferenceFile);
    if (absPathFile.empty()) {
        LOG_ERROR("Item Manager: Could not find " << itemReferenceFile << "!");
        return;
    }

    XML::Document doc(absPathFile, false);
    xmlNodePtr rootNode = doc.rootNode();

    if (!rootNode || !xmlStrEqual(rootNode->name, BAD_CAST "items"))
    {
        LOG_ERROR("Item Manager: Error while parsing item database ("
                  << absPathFile << ")!");
        return;
    }

    LOG_INFO("Loading item reference: " << absPathFile);
    unsigned nbItems = 0;
    for_each_xml_child_node(node, rootNode)
    {
        // Try to load the version of the item database. The version is defined
        // as subversion tag embedded as XML attribute. So every modification
        // to the items.xml file will increase the revision automatically.
        if (xmlStrEqual(node->name, BAD_CAST "version"))
        {
            std::string revision = XML::getProperty(node, "revision", std::string());
            itemDatabaseVersion = atoi(revision.c_str());

            LOG_INFO("Loading item database version " << itemDatabaseVersion);
            continue;
        }

        if (!xmlStrEqual(node->name, BAD_CAST "item"))
        {
            continue;
        }

        int id = XML::getProperty(node, "id", 0);
        if (id == 0)
        {
            LOG_WARN("Item Manager: An (ignored) item has no ID in "
                     << itemReferenceFile << "!");
            continue;
        }

        std::string sItemType = XML::getProperty(node, "type", "");
        ItemType itemType = itemTypeFromString(sItemType);

        if (itemType == ITEM_UNKNOWN)
        {
            LOG_WARN(itemReferenceFile << ": Unknown item type \"" << sItemType
                     << "\" for item #" << id <<
                     " - treating it as \"generic\".");
            itemType = ITEM_UNUSABLE;
        }

        if (itemType == ITEM_HAIRSPRITE || itemType == ITEM_RACESPRITE)
        {
            continue;
        }

        ItemClass *item;
        ItemClasses::iterator i = itemClasses.find(id);
        if (i == itemClasses.end())
        {
            item = new ItemClass(id, itemType);
            itemClasses[id] = item;
        }
        else
        {
            item = i->second;
        }

        int weight = XML::getProperty(node, "weight", 0);
        int value = XML::getProperty(node, "value", 0);
        int maxPerSlot = XML::getProperty(node, "max-per-slot", 0);
        int sprite = XML::getProperty(node, "sprite_id", 0);
        std::string scriptFile = XML::getProperty(node, "script", "");
        unsigned attackRange = XML::getProperty(node, "attack-range", 0);

        ItemModifiers modifiers;
        if (itemType == ITEM_EQUIPMENT_ONE_HAND_WEAPON ||
            itemType == ITEM_EQUIPMENT_TWO_HANDS_WEAPON)
        {
            int weaponType = 0;
            std::string strWeaponType = XML::getProperty(node, "weapon-type", "");
            if (strWeaponType == "")
            {
                LOG_WARN(itemReferenceFile << ": Empty weapon type \""
                         << "\" for item #" << id <<
                         " - treating it as generic item.");
            }
            else
                weaponType = SkillManager::getIdFromString(strWeaponType);

            modifiers.setValue(MOD_WEAPON_TYPE, weaponType);
            modifiers.setValue(MOD_WEAPON_RANGE, XML::getProperty(node, "range",   0));
            modifiers.setValue(MOD_ELEMENT_TYPE, XML::getProperty(node, "element", 0));
        }
        modifiers.setValue(MOD_LIFETIME, XML::getProperty(node, "lifetime", 0) * 10);
        //TODO: add child nodes for these modifiers (additive and factor)
        modifiers.setAttributeValue(BASE_ATTR_PHY_ATK_MIN,   XML::getProperty(node, "attack-min",   0));
        modifiers.setAttributeValue(BASE_ATTR_PHY_ATK_DELTA, XML::getProperty(node, "attack-delta", 0));
        modifiers.setAttributeValue(BASE_ATTR_HP,            XML::getProperty(node, "hp",           0));
        modifiers.setAttributeValue(BASE_ATTR_PHY_RES,       XML::getProperty(node, "defense",      0));
        modifiers.setAttributeValue(CHAR_ATTR_STRENGTH,      XML::getProperty(node, "strength",     0));
        modifiers.setAttributeValue(CHAR_ATTR_AGILITY,       XML::getProperty(node, "agility",      0));
        modifiers.setAttributeValue(CHAR_ATTR_DEXTERITY,     XML::getProperty(node, "dexterity",    0));
        modifiers.setAttributeValue(CHAR_ATTR_VITALITY,      XML::getProperty(node, "vitality",     0));
        modifiers.setAttributeValue(CHAR_ATTR_INTELLIGENCE,  XML::getProperty(node, "intelligence", 0));
        modifiers.setAttributeValue(CHAR_ATTR_WILLPOWER,     XML::getProperty(node, "willpower",    0));

        if (maxPerSlot == 0)
        {
            //LOG_WARN("Item Manager: Missing max-per-slot property for "
            //         "item " << id << " in " << itemReferenceFile << '.');
            maxPerSlot = 1;
        }

        if (itemType > ITEM_USABLE && itemType < ITEM_EQUIPMENT_AMMO &&
            maxPerSlot != 1)
        {
            LOG_WARN("Item Manager: Setting max-per-slot property to 1 for "
                     "equipment " << id << " in " << itemReferenceFile << '.');
            maxPerSlot = 1;
        }

        if (weight == 0)
        {
            LOG_WARN("Item Manager: Missing weight for item "
                     << id << " in " << itemReferenceFile << '.');
            weight = 1;
        }

        // TODO: Clean this up some
        if (scriptFile != "")
        {
            std::stringstream filename;
            filename << "scripts/items/" << scriptFile;
            if (ResourceManager::exists(filename.str()))       // file exists!
            {
                LOG_INFO("Loading item script: " << filename.str());
                Script *s = Script::create("lua");
                s->loadFile(filename.str());
                item->setScript(s);
            } else {
                LOG_WARN("Could not find script file \"" << filename.str() << "\" for item #"<<id);
            }
        }

        item->setWeight(weight);
        item->setCost(value);
        item->setMaxPerSlot(maxPerSlot);
        item->setModifiers(modifiers);
        item->setSpriteID(sprite ? sprite : id);
        ++nbItems;
        item->setAttackRange(attackRange);

        LOG_DEBUG("Item: ID: " << id << ", itemType: " << itemType
                  << ", weight: " << weight << ", value: " << value <<
                  ", script: " << scriptFile << ", maxPerSlot: " << maxPerSlot << ".");
    }

    LOG_INFO("Loaded " << nbItems << " items from "
             << absPathFile << ".");
}

void ItemManager::deinitialize()
{
    for (ItemClasses::iterator i = itemClasses.begin(), i_end = itemClasses.end(); i != i_end; ++i)
    {
        delete i->second;
    }
    itemClasses.clear();
}

ItemClass *ItemManager::getItem(int itemId)
{
    ItemClasses::const_iterator i = itemClasses.find(itemId);
    return i != itemClasses.end() ? i->second : NULL;
}

unsigned ItemManager::getDatabaseVersion()
{
    return itemDatabaseVersion;
}
