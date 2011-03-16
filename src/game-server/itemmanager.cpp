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

#include "defines.h"
#include "common/resourcemanager.h"
#include "game-server/attributemanager.h"
#include "game-server/item.h"
#include "game-server/skillmanager.h"
#include "scripting/script.h"
#include "utils/logger.h"
#include "utils/string.h"
#include "utils/xml.h"

#include <map>
#include <set>
#include <sstream>

void ItemManager::initialize()
{
    mVisibleEquipSlotCount = 0;
    reload();
}

void ItemManager::reload()
{
    std::string absPathFile;
    xmlNodePtr rootNode;

    // ####################################################################
    // ### Load the equip slots that a character has available to them. ###
    // ####################################################################

    absPathFile = ResourceManager::resolve(mEquipCharSlotReferenceFile);
    if (absPathFile.empty()) {
        LOG_ERROR("Item Manager: Could not find " << mEquipCharSlotReferenceFile << "!");
        return;
    }

    XML::Document doc(absPathFile, int());
    rootNode = doc.rootNode();

    if (!rootNode || !xmlStrEqual(rootNode->name, BAD_CAST "equip-slots"))
    {
        LOG_ERROR("Item Manager: Error while parsing equip slots database ("
                  << absPathFile << ")!");
        return;
    }

    LOG_INFO("Loading equip slots: " << absPathFile);

    {
        unsigned int totalCount = 0, slotCount = 0, visibleSlotCount = 0;
        for_each_xml_child_node(node, rootNode)
        {
            if (xmlStrEqual(node->name, BAD_CAST "slot"))
            {
                const std::string name = XML::getProperty(node, "name",
                                                          std::string());
                const int count = XML::getProperty(node, "count", 0);
                if (name.empty() || !count || count < 0)
                    LOG_WARN("Item Manager: equip slot has no name or zero count");
                else
                {
                    bool visible = XML::getProperty(node, "visible", "false") != "false";
                    if (visible)
                    {
                        visibleEquipSlots.push_back(equipSlots.size());
                        if (++visibleSlotCount > 7)
                            LOG_WARN("Item Manager: More than 7 visible equip slot!"
                                     "This will not work with current netcode!");
                    }
                    equipSlots.push_back(std::pair<std::string, unsigned int>
                                         (name, count));
                    totalCount += count;
                    ++slotCount;
                }
            }
        }
        LOG_INFO("Loaded '" << slotCount << "' slot types with '"
                 << totalCount << "' slots.");
    }

    // ####################################
    // ### Load the main item database. ###
    // ####################################

    absPathFile = ResourceManager::resolve(mItemReferenceFile);
    if (absPathFile.empty()) {
        LOG_ERROR("Item Manager: Could not find " << mItemReferenceFile << "!");
        return;
    }

    XML::Document doc2(absPathFile, int());
    rootNode = doc2.rootNode();

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
        if (!xmlStrEqual(node->name, BAD_CAST "item"))
            continue;

        int id = XML::getProperty(node, "id", 0);
        if (id < 1)
        {
            LOG_WARN("Item Manager: Item ID: " << id << " is invalid in "
                     << mItemReferenceFile << ", and will be ignored.");
            continue;
        }


        // Type is mostly unused, but still serves for
        // hairsheets and race sheets.
        std::string sItemType = XML::getProperty(node, "type", std::string());
        if (sItemType == "hairsprite" || sItemType == "racesprite")
            continue;

        ItemClass *item;
        ItemClasses::iterator i = itemClasses.find(id);

        unsigned int maxPerSlot = XML::getProperty(node, "max-per-slot", 0);
        if (!maxPerSlot)
        {
            LOG_WARN("Item Manager: Missing max-per-slot property for "
                     "item " << id << " in " << mItemReferenceFile << '.');
            maxPerSlot = 1;
        }

        if (i == itemClasses.end())
        {
            item = new ItemClass(id, maxPerSlot);
            itemClasses[id] = item;
        }
        else
        {
            LOG_WARN("Multiple defintions of item '" << id << "'!");
            item = i->second;
        }

        std::string name = XML::getProperty(node, "name", "unnamed");
        item->setName(name);

        int value = XML::getProperty(node, "value", 0);
        // Should have multiple value definitions for multiple currencies?
        item->mCost = value;

        for_each_xml_child_node(subnode, node)
        {
            if (xmlStrEqual(subnode->name, BAD_CAST "equip"))
            {
                ItemEquipInfo req;
                for_each_xml_child_node(equipnode, subnode)
                    if (xmlStrEqual(equipnode->name, BAD_CAST "slot"))
                    {
                        std::string slot = XML::getProperty(equipnode, "type",
                                                            std::string());
                        if (slot.empty())
                        {
                            LOG_WARN("Item Manager: empty equip slot definition!");
                            continue;
                        }
                        req.push_back(std::make_pair(getEquipIdFromName(slot),
                                       XML::getProperty(equipnode, "required",
                                                        1)));
                    }
                if (req.empty())
                {
                    LOG_WARN("Item Manager: empty equip requirement "
                             "definition for item " << id << "!");
                    continue;
                }
                item->mEquip.push_back(req);
            }
            else if (xmlStrEqual(subnode->name, BAD_CAST "effect"))
            {
                std::pair< ItemTriggerType, ItemTriggerType> triggerTypes;
                {
                    const std::string triggerName = XML::getProperty(
                                subnode, "trigger", std::string());
                    const std::string dispellTrigger = XML::getProperty(
                                subnode, "dispell", std::string());
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
                        continue;
                    }
                    triggerTypes = it->second;
                    if (!dispellTrigger.empty())
                    {
                        if ((it = triggerTable.find(dispellTrigger))
                             == triggerTable.end())
                            LOG_WARN("Item Manager: Unable to find dispell effect "
                                     "trigger type \"" << dispellTrigger << "\"!");
                        else
                            triggerTypes.second = it->second.first;
                    }
                }
                for_each_xml_child_node(effectnode, subnode)
                {
                    if (xmlStrEqual(effectnode->name, BAD_CAST "modifier"))
                    {
                        std::string tag = XML::getProperty(effectnode, "attribute", std::string());
                        if (tag.empty())
                        {
                            LOG_WARN("Item Manager: Warning, modifier found "
                                     "but no attribute specified!");
                            continue;
                        }
                        unsigned int duration = XML::getProperty(effectnode,
                                                                 "duration",
                                                                 0);
                        std::pair<unsigned int, unsigned int> info = attributeManager->getInfoFromTag(tag);
                        double value = XML::getFloatProperty(effectnode, "value", 0.0);
                        item->addEffect(new ItemEffectAttrMod(info.first,
                                                              info.second,
                                                              value, id,
                                                              duration),
                                        triggerTypes.first, triggerTypes.second);
                    }
                    else if (xmlStrEqual(effectnode->name, BAD_CAST "autoattack"))
                    {
                        // TODO - URGENT
                    }
                    // Having a dispell for the next three is nonsensical.
                    else if (xmlStrEqual(effectnode->name, BAD_CAST "cooldown"))
                    {
                        LOG_WARN("Item Manager: Cooldown property not implemented yet!");
                        // TODO: Also needs unique items before this action will work
                    }
                    else if (xmlStrEqual(effectnode->name, BAD_CAST "g-cooldown"))
                    {
                        LOG_WARN("Item Manager: G-Cooldown property not implemented yet!");
                        // TODO
                    }
                    else if (xmlStrEqual(effectnode->name, BAD_CAST "consumes"))
                        item->addEffect(new ItemEffectConsumes(), triggerTypes.first);
                    else if (xmlStrEqual(effectnode->name, BAD_CAST "script"))
                    {
                        std::string src = XML::getProperty(effectnode, "src", std::string());
                        if (src.empty())
                        {
                            LOG_WARN("Item Manager: Empty src definition for script effect, skipping!");
                            continue;
                        }
                        std::string func = XML::getProperty(effectnode, "function", std::string());
                        if (func.empty())
                        {
                            LOG_WARN ("Item Manager: Empty func definition for script effect, skipping!");
                            continue;
                        }
                        for_each_xml_child_node(scriptnode, effectnode)
                        {
                            // TODO: Load variables from variable subnodes
                        }
                        std::string dfunc = XML::getProperty(effectnode, "dispell-function", std::string());
                        // STUB
                        item->addEffect(new ItemEffectScript(), triggerTypes.first, triggerTypes.second);
                    }
                }
            }
            // More properties go here
        }
        ++nbItems;
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

ItemClass *ItemManager::getItem(int itemId) const
{
    ItemClasses::const_iterator i = itemClasses.find(itemId);
    return i != itemClasses.end() ? i->second : NULL;
}

ItemClass *ItemManager::getItemByName(std::string name) const
{
    name = utils::toLower(name);
    for (ItemClasses::const_iterator i = itemClasses.begin(),
         i_end = itemClasses.end(); i != i_end; ++i)
    {
        if(utils::toLower(i->second->getName()) == name)
        {
            return i->second;
        }
    }
    return 0;
}

unsigned int ItemManager::getDatabaseVersion() const
{
    return mItemDatabaseVersion;
}

const std::string &ItemManager::getEquipNameFromId(unsigned int id) const
{
    return equipSlots.at(id).first;
}

unsigned int ItemManager::getEquipIdFromName(const std::string &name) const
{
    for (unsigned int i = 0; i < equipSlots.size(); ++i)
        if (name == equipSlots.at(i).first)
            return i;
    LOG_WARN("Item Manager: attempt to find equip id from name \"" <<
             name << "\" not found, defaulting to 0!");
    return 0;
}

unsigned int ItemManager::getMaxSlotsFromId(unsigned int id) const
{
    return equipSlots.at(id).second;
}

unsigned int ItemManager::getVisibleSlotCount() const
{
    if (!mVisibleEquipSlotCount)
        for (VisibleEquipSlots::const_iterator it = visibleEquipSlots.begin(),
                                               it_end = visibleEquipSlots.end();
             it != it_end;
             ++it)
            mVisibleEquipSlotCount += equipSlots.at(*it).second;
    return mVisibleEquipSlotCount;
}

bool ItemManager::isEquipSlotVisible(unsigned int id) const
{
    for (VisibleEquipSlots::const_iterator it = visibleEquipSlots.begin(),
                                           it_end = visibleEquipSlots.end();
         it != it_end;
         ++it)
        if (*it == id)
            return true;
    return false;
}
