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

#include "game-server/itemmanager.h"

#include "common/defines.h"
#include "common/resourcemanager.h"
#include "game-server/attributemanager.h"
#include "game-server/item.h"
#include "game-server/skillmanager.h"
#include "scripting/script.h"
#include "utils/logger.h"

#include <map>
#include <set>
#include <sstream>

void ItemManager::initialize()
{
    reload();
}

void ItemManager::reload()
{
    mVisibleEquipSlotCount = 0;
    readEquipSlotsFile();
    readItemsFile();
}

void ItemManager::deinitialize()
{
    for (ItemClasses::iterator i = mItemClasses.begin(),
         i_end = mItemClasses.end(); i != i_end; ++i)
    {
        delete i->second;
    }

    for (std::map< unsigned int, EquipSlotInfo* >::iterator it =
        mEquipSlotsInfo.begin(), it_end = mEquipSlotsInfo.end(); it != it_end;
        ++it)
    {
        delete it->second;
    }

    mItemClasses.clear();
    mItemClassesByName.clear();
}

ItemClass *ItemManager::getItem(int itemId) const
{
    ItemClasses::const_iterator i = mItemClasses.find(itemId);
    return i != mItemClasses.end() ? i->second : 0;
}

ItemClass *ItemManager::getItemByName(const std::string &name) const
{
    return mItemClassesByName.find(name);
}

unsigned int ItemManager::getDatabaseVersion() const
{
    return mItemDatabaseVersion;
}

unsigned int ItemManager::getEquipSlotIdFromName(const std::string &name) const
{
    EquipSlotInfo *slotInfo = mNamedEquipSlotsInfo.find(name);
    return slotInfo ? slotInfo->slotId : 0;
}

unsigned int ItemManager::getEquipSlotCapacity(unsigned int id) const
{
    EquipSlotsInfo::const_iterator i = mEquipSlotsInfo.find(id);
    return i != mEquipSlotsInfo.end() ? i->second->slotCapacity : 0;
}

bool ItemManager::isEquipSlotVisible(unsigned int id) const
{
    EquipSlotsInfo::const_iterator i = mEquipSlotsInfo.find(id);
    return i != mEquipSlotsInfo.end() ? i->second->visibleSlot : false;
}

void ItemManager::readEquipSlotsFile()
{
    XML::Document doc(mEquipSlotsFile);
    xmlNodePtr rootNode = doc.rootNode();

    if (!rootNode || !xmlStrEqual(rootNode->name, BAD_CAST "equip-slots"))
    {
        LOG_ERROR("Item Manager: Error while parsing equip slots database ("
                  << mEquipSlotsFile << ")!");
        return;
    }

    LOG_INFO("Loading equip slots: " << mEquipSlotsFile);

    unsigned totalCapacity = 0;
    unsigned slotCount = 0;
    mVisibleEquipSlotCount = 0;

    for_each_xml_child_node(node, rootNode)
    {
        if (xmlStrEqual(node->name, BAD_CAST "slot"))
        {
            const int slotId = XML::getProperty(node, "id", 0);
            const std::string name = XML::getProperty(node, "name",
                                                      std::string());
            const int capacity = XML::getProperty(node, "capacity", 0);

            if (slotId <= 0 || name.empty() || capacity <= 0)
            {
                LOG_WARN("Item Manager: equip slot " << slotId
                    << ": (" << name << ") has no name or zero count. "
                    "The slot has been ignored.");
                continue;
            }

            if (slotId > 255)
            {
                LOG_WARN("Item Manager: equip slot " << slotId
                    << ": (" << name << ") is superior to 255 "
                    "and has been ignored.");
                continue;
            }

            bool visible = XML::getBoolProperty(node, "visible", false);
            if (visible)
                ++mVisibleEquipSlotCount;

            EquipSlotsInfo::iterator i = mEquipSlotsInfo.find(slotId);

            if (i != mEquipSlotsInfo.end())
            {
                LOG_WARN("Item Manager: Ignoring duplicate definition "
                         "of equip slot '" << slotId << "'!");
                continue;
            }

            LOG_DEBUG("Adding equip slot, id: " << slotId << ", name: " << name
                << ", capacity: " << capacity << ", visible? " << visible);
            EquipSlotInfo *equipSlotInfo =
                new EquipSlotInfo(slotId, name, capacity, visible);
            mEquipSlotsInfo.insert(
                std::make_pair<unsigned int, EquipSlotInfo*>(slotId, equipSlotInfo));
            mNamedEquipSlotsInfo.insert(name, equipSlotInfo);

            totalCapacity += capacity;
            ++slotCount;
        }
    }

    LOG_INFO("Loaded '" << slotCount << "' slot types with '"
             << totalCapacity << "' slots.");
}

void ItemManager::readItemsFile()
{
    XML::Document doc2(mItemsFile);
    xmlNodePtr rootNode = doc2.rootNode();

    if (!rootNode || !xmlStrEqual(rootNode->name, BAD_CAST "items"))
    {
        LOG_ERROR("Item Manager: Error while parsing item database ("
                  << mItemsFile << ")!");
        return;
    }

    LOG_INFO("Loading item reference: " << mItemsFile);

    for_each_xml_child_node(node, rootNode)
    {
        if (xmlStrEqual(node->name, BAD_CAST "item"))
        {
            readItemNode(node);
        }
    }

    LOG_INFO("Loaded " << mItemClasses.size() << " items from "
             << mItemsFile << ".");
}

void ItemManager::readItemNode(xmlNodePtr itemNode)
{
    const int id = XML::getProperty(itemNode, "id", 0);
    if (id < 1)
    {
        LOG_WARN("Item Manager: Item ID: " << id << " is invalid in "
                 << mItemsFile << ", and will be ignored.");
        return;
    }

    // Type is mostly unused, but still serves for hairsheets and race sheets
    const std::string type = XML::getProperty(itemNode, "type", std::string());
    if (type == "hairsprite" || type == "racesprite")
        return;

    ItemClasses::iterator i = mItemClasses.find(id);

    if (i != mItemClasses.end())
    {
        LOG_WARN("Item Manager: Ignoring duplicate definition of item '" << id
                 << "'!");
        return;
    }

    unsigned int maxPerSlot = XML::getProperty(itemNode, "max-per-slot", 0);
    if (!maxPerSlot)
    {
        LOG_WARN("Item Manager: Missing max-per-slot property for "
                 "item " << id << " in " << mItemsFile << '.');
        maxPerSlot = 1;
    }

    ItemClass *item = new ItemClass(id, maxPerSlot);
    mItemClasses.insert(std::make_pair(id, item));

    const std::string name = XML::getProperty(itemNode, "name", std::string());
    if (!name.empty())
    {
        item->setName(name);

        if (mItemClassesByName.contains(name))
            LOG_WARN("Item Manager: Name not unique for item " << id);
        else
            mItemClassesByName.insert(name, item);
    }

    int value = XML::getProperty(itemNode, "value", 0);
    // Should have multiple value definitions for multiple currencies?
    item->mCost = value;

    for_each_xml_child_node(subNode, itemNode)
    {
        if (xmlStrEqual(subNode->name, BAD_CAST "equip"))
        {
            readEquipNode(subNode, item);
        }
        else if (xmlStrEqual(subNode->name, BAD_CAST "effect"))
        {
            readEffectNode(subNode, item);
        }
        // More properties go here
    }
}

void ItemManager::readEquipNode(xmlNodePtr equipNode, ItemClass *item)
{
    for_each_xml_child_node(subNode, equipNode)
    {
        if (xmlStrEqual(subNode->name, BAD_CAST "slot"))
        {
            if (item->mEquipReq.equipSlotId)
            {
                LOG_WARN("Item Manager: duplicate equip slot definitions!"
                         " Only the first will apply.");
                break;
            }

            std::string slot = XML::getProperty(subNode, "type", std::string());
            if (slot.empty())
            {
                LOG_WARN("Item Manager: empty equip slot definition!");
                continue;
            }
            if (utils::isNumeric(slot))
            {
                // When the slot id is given
                item->mEquipReq.equipSlotId = utils::stringToInt(slot);
            }
            else
            {
                // When its name is given
                item->mEquipReq.equipSlotId = getEquipSlotIdFromName(slot);
            }
            item->mEquipReq.capacityRequired =
                XML::getProperty(subNode, "required", 1);
        }
    }

    if (!item->mEquipReq.equipSlotId)
    {
        LOG_WARN("Item Manager: empty equip requirement "
                 "definition for item " << item->getDatabaseID() << "!"
                 " The item will be unequippable.");
        return;
    }
}

void ItemManager::readEffectNode(xmlNodePtr effectNode, ItemClass *item)
{
    std::pair<ItemTriggerType, ItemTriggerType> triggerTypes;
    {
        const std::string triggerName = XML::getProperty(
                    effectNode, "trigger", std::string());
        const std::string dispellTrigger = XML::getProperty(
                    effectNode, "dispell", std::string());
        // label -> { trigger (apply), trigger (cancel (default)) }
        // The latter can be overridden.
        static std::map<const std::string,
                        std::pair<ItemTriggerType, ItemTriggerType> >
                        triggerTable;
        if (triggerTable.empty())
        {
            /*
             * The following is a table of all triggers for item
             *     effects.
             * The first element defines the trigger used for this
             *     trigger, and the second defines the default
             *     trigger to use for dispelling.
             */
            triggerTable["in-inventory"].first         = ITT_IN_INVY;
            triggerTable["in-inventory"].second        = ITT_LEAVE_INVY;
            triggerTable["activation"].first        = ITT_ACTIVATE;
            triggerTable["activation"].second       = ITT_NULL;
            triggerTable["equip"].first             = ITT_EQUIP;
            triggerTable["equip"].second            = ITT_UNEQUIP;
            triggerTable["leave-inventory"].first   = ITT_LEAVE_INVY;
            triggerTable["leave-inventory"].second  = ITT_NULL;
            triggerTable["unequip"].first           = ITT_UNEQUIP;
            triggerTable["unequip"].second          = ITT_NULL;
            triggerTable["equip-change"].first      = ITT_EQUIPCHG;
            triggerTable["equip-change"].second     = ITT_NULL;
            triggerTable["null"].first              = ITT_NULL;
            triggerTable["null"].second             = ITT_NULL;
        }
        std::map<const std::string, std::pair<ItemTriggerType,
                                    ItemTriggerType> >::iterator
                 it = triggerTable.find(triggerName);

        if (it == triggerTable.end()) {
            LOG_WARN("Item Manager: Unable to find effect trigger type \""
                     << triggerName << "\", skipping!");
            return;
        }
        triggerTypes = it->second;
        if (!dispellTrigger.empty())
        {
            if ((it = triggerTable.find(dispellTrigger)) == triggerTable.end())
                LOG_WARN("Item Manager: Unable to find dispell effect "
                         "trigger type \"" << dispellTrigger << "\"!");
            else
                triggerTypes.second = it->second.first;
        }
    }

    for_each_xml_child_node(subNode, effectNode)
    {
        if (xmlStrEqual(subNode->name, BAD_CAST "modifier"))
        {
            std::string tag = XML::getProperty(subNode, "attribute", std::string());
            if (tag.empty())
            {
                LOG_WARN("Item Manager: Warning, modifier found "
                         "but no attribute specified!");
                continue;
            }
            unsigned int duration = XML::getProperty(subNode,
                                                     "duration",
                                                     0);
            ModifierLocation location = attributeManager->getLocation(tag);
            double value = XML::getFloatProperty(subNode, "value", 0.0);
            item->addEffect(new ItemEffectAttrMod(location.attributeId,
                                                  location.layer,
                                                  value,
                                                  item->getDatabaseID(),
                                                  duration),
                            triggerTypes.first, triggerTypes.second);
        }
        else if (xmlStrEqual(subNode->name, BAD_CAST "autoattack"))
        {
            // TODO - URGENT
        }
        // Having a dispell for the next three is nonsensical.
        else if (xmlStrEqual(subNode->name, BAD_CAST "cooldown"))
        {
            LOG_WARN("Item Manager: Cooldown property not implemented yet!");
            // TODO: Also needs unique items before this action will work
        }
        else if (xmlStrEqual(subNode->name, BAD_CAST "g-cooldown"))
        {
            LOG_WARN("Item Manager: G-Cooldown property not implemented yet!");
            // TODO
        }
        else if (xmlStrEqual(subNode->name, BAD_CAST "consumes"))
        {
            item->addEffect(new ItemEffectConsumes(), triggerTypes.first);
        }
        else if (xmlStrEqual(subNode->name, BAD_CAST "script"))
        {
            std::string activateFunctionName = XML::getProperty(subNode,
                                                                "function",
                                                                std::string());
            if (activateFunctionName.empty())
            {
                LOG_WARN("Item Manager: Empty function definition "
                         "for script effect, skipping!");
                continue;
            }

            std::string src = XML::getProperty(subNode, "src", std::string());
            if (src.empty())
            {
                LOG_WARN("Item Manager: Empty src definition for script effect,"
                         " skipping!");
                continue;
            }
            std::stringstream filename;
            filename << "scripts/items/" << src;
            if (!ResourceManager::exists(filename.str()))
            {
                LOG_WARN("Could not find script file \"" << filename.str()
                         << "\" for item #" << item->mDatabaseID);
                continue;
            }

            LOG_INFO("Loading item script: " << filename.str());
            Script *script = Script::create("lua");
            if (!script->loadFile(filename.str()))
            {
                // Delete the script as it's invalid.
                delete script;

                LOG_WARN("Could not load script file \"" << filename.str()
                          << "\" for item #" << item->mDatabaseID);
                continue;
            }

            for_each_xml_child_node(scriptSubNode, subNode)
            {
                // TODO: Load variables from variable subnodes
            }
            std::string dispellFunctionName = XML::getProperty(subNode,
                                                             "dispell-function",
                                                             std::string());

            item->addEffect(new ItemEffectScript(item->mDatabaseID, script,
                                                 activateFunctionName,
                                                 dispellFunctionName),
                                                 triggerTypes.first,
                                                 triggerTypes.second);
        }
    }
}
