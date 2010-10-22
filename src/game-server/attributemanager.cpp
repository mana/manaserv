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

#include "game-server/attributemanager.hpp"

#include "common/resourcemanager.hpp"
#include "utils/string.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"
#include "defines.h"

void AttributeManager::initialize()
{
    reload();
}

void AttributeManager::reload()
{
    mTagMap.clear();
    mAttributeMap.clear();
    for (unsigned int i = 0; i < ATTR_MAX; ++i)
        mAttributeScopes[i].clear();

    std::string absPathFile = ResourceManager::resolve(mAttributeReferenceFile);
    if (absPathFile.empty())
    {
        LOG_FATAL("Attribute Manager: Could not find "
                  << mAttributeReferenceFile << "!");
        exit(EXIT_XML_NOT_FOUND);
    }

    XML::Document doc(absPathFile, int());
    xmlNodePtr node = doc.rootNode();
    if (!node || !xmlStrEqual(node->name, BAD_CAST "attributes"))
    {
        LOG_FATAL("Attribute Manager: " << mAttributeReferenceFile
                  << " is not a valid database file!");
        exit(EXIT_XML_BAD_PARAMETER);
    }

    LOG_INFO("Loading attribute reference...");

    for_each_xml_child_node(attributenode, node)
    {
        if (xmlStrEqual(attributenode->name, BAD_CAST "attribute"))
        {
            int id = XML::getProperty(attributenode, "id", 0);

            if (id <= 0)
            {
                LOG_WARN("Attribute manager: attribute '" << id
                         << "' is invalid and will be ignored.");
                continue;
            }

            mAttributeMap[id] = AttributeInfoMap(false,
                                       std::vector<struct AttributeInfoType>());

            unsigned int layerCount = 0;
            for_each_xml_child_node(subnode, attributenode)
            {
                if (xmlStrEqual(subnode->name, BAD_CAST "modifier"))
                {
                    std::string sType = utils::toUpper(
                                    XML::getProperty(subnode, "stacktype", ""));
                    std::string eType = utils::toUpper(
                                    XML::getProperty(subnode, "modtype", ""));
                    std::string tag = utils::toUpper(
                                    XML::getProperty(subnode, "tag", ""));
                    AT_TY pSType;
                    AME_TY pEType;
                    if (!sType.empty())
                    {
                        if (!eType.empty())
                        {
                            bool fail = false;
                            if (sType == "STACKABLE")
                                pSType = TY_ST;
                            else if (sType == "NON STACKABLE")
                                pSType = TY_NST;
                            else if (sType == "NON STACKABLE BONUS")
                                pSType = TY_NSTB;
                            else
                            {
                                LOG_WARN("Attribute manager: attribute '"
                                         << id << "' has unknown stack type '"
                                         << sType << "', skipping modifier!");
                                fail = true;
                            }

                            if (!fail)
                            {
                                if (eType == "ADDITIVE")
                                    pEType = AME_ADD;
                                else if (eType == "MULTIPLICATIVE")
                                    pEType = AME_MULT;
                                else
                                {
                                    LOG_WARN(
                                          "Attribute manager: attribute '" << id
                                          << "' has unknown modification type '"
                                          << sType << "', skipping modifier!");
                                    fail = true;
                                }
                                if (!fail)
                                {
                                    mAttributeMap[id].second.push_back(
                                          AttributeInfoType(pSType, pEType));
                                    std::string tag = XML::getProperty(
                                                            subnode, "tag", "");

                                    if (!tag.empty())
                                        mTagMap.insert(
                                        std::make_pair(tag,
                                            std::make_pair(id, layerCount)));
                                    ++layerCount;
                                }
                            }
                        }
                        else
                            LOG_WARN("Attribute manager: attribute '" << id
                                     << "' has undefined modification type, "
                                     << "skipping modifier!");
                    }
                    else
                    {
                        LOG_WARN("Attribute manager: attribute '" << id <<
                              "' has undefined stack type, skipping modifier!");
                    }
                }
            }
            std::string scope = utils::toUpper(
                    XML::getProperty(attributenode, "scope", std::string()));
            if (scope.empty())
            {
                // Give a warning unless scope has been explicitly set to "NONE"
                LOG_WARN("Attribute manager: attribute '" << id
                         << "' has no default scope.");
            }
            else if (scope == "CHARACTER")
            {
                mAttributeScopes[ATTR_CHAR][id] = &mAttributeMap.at(id).second;
                LOG_DEBUG("Attribute manager: attribute '" << id
                          << "' added to default character scope.");
            }
            else if (scope == "MONSTER")
            {
                mAttributeScopes[ATTR_MOB][id] = &mAttributeMap.at(id).second;
                LOG_DEBUG("Attribute manager: attribute '" << id
                          << "' added to default monster scope.");
            }
            else if (scope == "BEING")
            {
                mAttributeScopes[ATTR_BEING][id] = &mAttributeMap.at(id).second;
                LOG_DEBUG("Attribute manager: attribute '" << id
                          << "' added to default being scope.");
            }
            else if (scope == "NONE")
            {
                LOG_DEBUG("Attribute manager: attribute '" << id
                          << "' set to have no default scope.");
            }
        } // End 'attribute'
    }

    LOG_DEBUG("attribute map:");
    LOG_DEBUG("TY_ST is " << TY_ST << ", TY_ NST is " << TY_NST
              << ", TY_NSTB is " << TY_NSTB << ".");
    LOG_DEBUG("AME_ADD is " << AME_ADD << ", AME_MULT is " << AME_MULT << ".");
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
            tag = getTagFromInfo(i->first, lCount);
            std::string end = tag ? "tag of '" + (*tag) + "'." : "no tag.";
            LOG_DEBUG("    sType: " << j->sType << ", eType: " << j->eType << ", "
                      "and " << end);
            ++lCount;
            ++count;
        }
    }
    LOG_INFO("Loaded '" << mAttributeMap.size() << "' attributes with '"
             << count << "' modifier layers.");

    for(TagMap::const_iterator i = mTagMap.begin(), i_end = mTagMap.end();
        i != i_end; ++i)
    {
        LOG_DEBUG("Tag '" << i->first << "': '" << i->second.first << "', '"
                  << i->second.second << "'.");
    }

    LOG_INFO("Loaded '" << mTagMap.size() << "' modifier tags.");
}

const std::vector<struct AttributeInfoType> *AttributeManager::getAttributeInfo(unsigned int id) const
{
    AttributeMap::const_iterator ret = mAttributeMap.find(id);
    if (ret == mAttributeMap.end()) return 0;
    return &ret->second.second;
}

const AttributeScopes &AttributeManager::getAttributeInfoForType(SCOPE_TYPES type) const
{
    return mAttributeScopes[type];
}

bool AttributeManager::isAttributeDirectlyModifiable(unsigned int id) const
{
    AttributeMap::const_iterator ret = mAttributeMap.find(id);
    if (ret == mAttributeMap.end()) return false;
    return ret->second.first;
}

// { attribute id, layer }
std::pair<unsigned int,unsigned int> AttributeManager::getInfoFromTag(const std::string &tag) const
{
    return mTagMap.at(tag);
}

const std::string *AttributeManager::getTagFromInfo(unsigned int attribute, unsigned int layer) const
{
    for (TagMap::const_iterator it = mTagMap.begin(),
         it_end = mTagMap.end(); it != it_end; ++it)
        if (it->second.first == attribute && it->second.second == layer)
            return &it->first;
    return 0;
}
