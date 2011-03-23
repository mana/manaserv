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

#include "game-server/attributemanager.h"

#include "common/defines.h"
#include "utils/string.h"
#include "utils/logger.h"

void AttributeManager::initialize()
{
    reload();
}

void AttributeManager::reload()
{
    mTagMap.clear();
    mAttributeMap.clear();
    for (unsigned int i = 0; i < MaxScope; ++i)
        mAttributeScopes[i].clear();

    readAttributesFile();

    LOG_DEBUG("attribute map:");
    LOG_DEBUG("Stackable is " << Stackable << ", NonStackable is " << NonStackable
              << ", NonStackableBonus is " << NonStackableBonus << ".");
    LOG_DEBUG("Additive is " << Additive << ", Multiplicative is " << Multiplicative << ".");
    const std::string *tag;
    unsigned int count = 0;
    for (AttributeMap::const_iterator i = mAttributeMap.begin();
         i != mAttributeMap.end(); ++i)
    {
        unsigned int lCount = 0;
        LOG_DEBUG("  "<<i->first<<" : ");
        for (std::vector<struct AttributeInfoType>::const_iterator j = i->second.second.begin();
             j != i->second.second.end();
             ++j)
        {
            tag = getTag(ModifierLocation(i->first, lCount));
            std::string end = tag ? "tag of '" + (*tag) + "'." : "no tag.";
            LOG_DEBUG("    stackableType: " << j->stackableType
                      << ", effectType: " << j->effectType << ", and " << end);
            ++lCount;
            ++count;
        }
    }
    LOG_INFO("Loaded '" << mAttributeMap.size() << "' attributes with '"
             << count << "' modifier layers.");

    for (TagMap::const_iterator i = mTagMap.begin(), i_end = mTagMap.end();
         i != i_end; ++i)
    {
        LOG_DEBUG("Tag '" << i->first << "': '" << i->second.attributeId
                  << "', '" << i->second.layer << "'.");
    }

    LOG_INFO("Loaded '" << mTagMap.size() << "' modifier tags.");
}

const std::vector<struct AttributeInfoType> *AttributeManager::getAttributeInfo(int id) const
{
    AttributeMap::const_iterator ret = mAttributeMap.find(id);
    if (ret == mAttributeMap.end())
        return 0;
    return &ret->second.second;
}

const AttributeScope &AttributeManager::getAttributeScope(ScopeType type) const
{
    return mAttributeScopes[type];
}

bool AttributeManager::isAttributeDirectlyModifiable(int id) const
{
    AttributeMap::const_iterator ret = mAttributeMap.find(id);
    if (ret == mAttributeMap.end())
        return false;
    return ret->second.first;
}

ModifierLocation AttributeManager::getLocation(const std::string &tag) const
{
    return mTagMap.at(tag);
}

const std::string *AttributeManager::getTag(const ModifierLocation &location) const
{
    for (TagMap::const_iterator it = mTagMap.begin(),
         it_end = mTagMap.end(); it != it_end; ++it)
        if (it->second == location)
            return &it->first;
    return 0;
}

void AttributeManager::readAttributesFile()
{
    XML::Document doc(mAttributeReferenceFile);
    xmlNodePtr node = doc.rootNode();

    if (!node || !xmlStrEqual(node->name, BAD_CAST "attributes"))
    {
        LOG_FATAL("Attribute Manager: " << mAttributeReferenceFile
                  << " is not a valid database file!");
        exit(EXIT_XML_BAD_PARAMETER);
    }

    LOG_INFO("Loading attribute reference...");

    for_each_xml_child_node(childNode, node)
    {
        if (xmlStrEqual(childNode->name, BAD_CAST "attribute"))
            readAttributeNode(childNode);
    }
}

void AttributeManager::readAttributeNode(xmlNodePtr attributeNode)
{
    int id = XML::getProperty(attributeNode, "id", 0);

    if (id <= 0)
    {
        LOG_WARN("Attribute manager: attribute '" << id
                 << "' has an invalid id and will be ignored.");
        return;
    }

    mAttributeMap[id] =
            AttributeInfoMap(false, std::vector<struct AttributeInfoType>());

    for_each_xml_child_node(subNode, attributeNode)
    {
        if (xmlStrEqual(subNode->name, BAD_CAST "modifier"))
        {
            readModifierNode(subNode, id);
        }
    }

    const std::string scope = utils::toUpper(
                XML::getProperty(attributeNode, "scope", std::string()));

    if (scope.empty())
    {
        // Give a warning unless scope has been explicitly set to "NONE"
        LOG_WARN("Attribute manager: attribute '" << id
                 << "' has no default scope.");
    }
    else if (scope == "CHARACTER")
    {
        mAttributeScopes[CharacterScope][id] = &mAttributeMap.at(id).second;
        LOG_DEBUG("Attribute manager: attribute '" << id
                  << "' added to default character scope.");
    }
    else if (scope == "MONSTER")
    {
        mAttributeScopes[MonsterScope][id] = &mAttributeMap.at(id).second;
        LOG_DEBUG("Attribute manager: attribute '" << id
                  << "' added to default monster scope.");
    }
    else if (scope == "BEING")
    {
        mAttributeScopes[BeingScope][id] = &mAttributeMap.at(id).second;
        LOG_DEBUG("Attribute manager: attribute '" << id
                  << "' added to default being scope.");
    }
    else if (scope == "NONE")
    {
        LOG_DEBUG("Attribute manager: attribute '" << id
                  << "' set to have no default scope.");
    }
}

void AttributeManager::readModifierNode(xmlNodePtr modifierNode,
                                        int attributeId)
{
    const std::string stackableTypeString = utils::toUpper(
                XML::getProperty(modifierNode, "stacktype", std::string()));
    const std::string effectTypeString = utils::toUpper(
                XML::getProperty(modifierNode, "modtype", std::string()));
    const std::string tag = XML::getProperty(modifierNode, "tag",
                                             std::string());

    if (stackableTypeString.empty())
    {
        LOG_WARN("Attribute manager: attribute '" << attributeId <<
                 "' has undefined stackable type, skipping modifier!");
        return;
    }

    if (effectTypeString.empty())
    {
        LOG_WARN("Attribute manager: attribute '" << attributeId
                 << "' has undefined modification type, skipping modifier!");
        return;
    }

    StackableType stackableType;
    ModifierEffectType effectType;

    if (stackableTypeString == "STACKABLE")
        stackableType = Stackable;
    else if (stackableTypeString == "NON STACKABLE")
        stackableType = NonStackable;
    else if (stackableTypeString == "NON STACKABLE BONUS")
        stackableType = NonStackableBonus;
    else
    {
        LOG_WARN("Attribute manager: attribute '"
                 << attributeId << "' has unknown stackable type '"
                 << stackableTypeString << "', skipping modifier!");
        return;
    }

    if (effectTypeString == "ADDITIVE")
        effectType = Additive;
    else if (effectTypeString == "MULTIPLICATIVE")
        effectType = Multiplicative;
    else
    {
        LOG_WARN("Attribute manager: attribute '" << attributeId
                 << "' has unknown modification type '"
                 << effectTypeString << "', skipping modifier!");
        return;
    }

    mAttributeMap[attributeId].second.push_back(
                AttributeInfoType(stackableType, effectType));

    if (!tag.empty())
    {
        const int layer = mAttributeMap[attributeId].second.size() - 1;
        mTagMap.insert(std::make_pair(tag, ModifierLocation(attributeId,
                                                            layer)));
    }
}
