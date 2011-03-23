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

const std::string &ItemManager::getEquipNameFromId(unsigned int id) const
{
    return mEquipSlots.at(id).first;
}

unsigned int ItemManager::getEquipIdFromName(const std::string &name) const
{
    for (unsigned int i = 0; i < mEquipSlots.size(); ++i)
        if (name == mEquipSlots.at(i).first)
            return i;
    LOG_WARN("Item Manager: attempt to find equip id from name \"" <<
             name << "\" not found, defaulting to 0!");
    return 0;
}

unsigned int ItemManager::getMaxSlotsFromId(unsigned int id) const
{
    return mEquipSlots.at(id).second;
}

unsigned int ItemManager::getVisibleSlotCount() const
{
    if (!mVisibleEquipSlotCount)
    {
        for (VisibleEquipSlots::const_iterator it = mVisibleEquipSlots.begin(),
                                               it_end = mVisibleEquipSlots.end();
             it != it_end;
             ++it)
        {
            mVisibleEquipSlotCount += mEquipSlots.at(*it).second;
        }
    }
    return mVisibleEquipSlotCount;
}

bool ItemManager::isEquipSlotVisible(unsigned int id) const
{
    for (VisibleEquipSlots::const_iterator it = mVisibleEquipSlots.begin(),
                                           it_end = mVisibleEquipSlots.end();
         it != it_end;
         ++it)
    {
        if (*it == id)
            return true;
    }
    return false;
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

    unsigned totalCount = 0;
    unsigned slotCount = 0;
    unsigned visibleSlotCount = 0;

    for_each_xml_child_node(node, rootNode)
    {
        if (xmlStrEqual(node->name, BAD_CAST "slot"))
        {
            const std::string name = XML::getProperty(node, "name",
                                                      std::string());
            const int count = XML::getProperty(node, "count", 0);

            if (name.empty() || count <= 0)
            {
                LOG_WARN("Item Manager: equip slot has no name or zero count");
            }
            else
            {
                bool visible = XML::getProperty(node, "visible", "false") != "false";
                if (visible)
                {
                    mVisibleEquipSlots.push_back(mEquipSlots.size());
                    if (++visibleSlotCount > 7)
                        LOG_WARN("Item Manager: More than 7 visible equip slot!"
                                 "This will not work with current netcode!");
                }
                mEquipSlots.push_back(std::pair<std::string, unsigned int>
                                     (name, count));
                totalCount += count;
                ++slotCount;
            }
        }
    }

    LOG_INFO("Loaded '" << slotCount << "' slot types with '"
             << totalCount << "' slots.");
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
    ItemEquipInfo req;
    for_each_xml_child_node(subNode, equipNode)
    {
        if (xmlStrEqual(subNode->name, BAD_CAST "slot"))
        {
            std::string slot = XML::getProperty(subNode, "type", std::string());
            if (slot.empty())
            {
                LOG_WARN("Item Manager: empty equip slot definition!");
                continue;
            }
            req.push_back(std::make_pair(getEquipIdFromName(slot),
                           XML::getProperty(subNode, "required", 1)));
        }
    }
    if (req.empty())
    {
        LOG_WARN("Item Manager: empty equip requirement "
                 "definition for item " << item->getDatabaseID() << "!");
        return;
    }
    item->mEquip.push_back(req);
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
            triggerTable["existence"].first         = ITT_IN_INVY;
            triggerTable["existence"].second        = ITT_LEAVE_INVY;
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
            std::string src = XML::getProperty(subNode, "src", std::string());
            if (src.empty())
            {
                LOG_WARN("Item Manager: Empty src definition for script effect, skipping!");
                continue;
            }
            std::string func = XML::getProperty(subNode, "function", std::string());
            if (func.empty())
            {
                LOG_WARN("Item Manager: Empty func definition for script effect, skipping!");
                continue;
            }
            for_each_xml_child_node(scriptSubNode, subNode)
            {
                // TODO: Load variables from variable subnodes
            }
            std::string dfunc = XML::getProperty(subNode, "dispell-function", std::string());
            // STUB
            item->addEffect(new ItemEffectScript(), triggerTypes.first, triggerTypes.second);
        }
    }
}
