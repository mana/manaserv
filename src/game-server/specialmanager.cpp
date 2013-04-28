/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *  Copyright (C) 2010-2012  The Mana Developers
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

#include "specialmanager.h"

#include "utils/xml.h"
#include "utils/logger.h"

static SpecialManager::TargetMode getTargetByString(const std::string &str)
{
    std::string strLower = utils::toLower(str);
    if (strLower == "being")
        return SpecialManager::TARGET_BEING;
    else if (strLower == "point")
        return SpecialManager::TARGET_POINT;

    LOG_WARN("Unknown targetmode " << str << " assuming being.");
    return SpecialManager::TARGET_BEING;
}


/**
 * Read a <special> element from settings.
 * Used by SettingsManager.
 */
void SpecialManager::readSpecialSetNode(xmlNodePtr node, const std::string &filename)
{
    std::string setName = XML::getProperty(node, "name", std::string());
    if (setName.empty())
    {
        LOG_WARN("The " << filename << " file contains unamed <set> tags and will be ignored.");
        return;
    }

    setName = utils::toLower(setName);

    for_each_xml_child_node(specialNode, node)
    {
        if (xmlStrEqual(specialNode->name, BAD_CAST "special")) {
            readSpecialNode(specialNode, setName);
        }
    }

}

/**
 * Check the status of recently loaded configuration.
 */
void SpecialManager::checkStatus()
{
    LOG_INFO("Loaded " << mSpecialsInfo.size() << " specials");
}

void SpecialManager::readSpecialNode(xmlNodePtr specialNode,
                                     const std::string &setName)
{
    std::string name = utils::toLower(
                XML::getProperty(specialNode, "name", std::string()));
    int id = XML::getProperty(specialNode, "id", 0);

    if (id <= 0 || name.empty())
    {
        LOG_WARN("Invalid special (empty name or id <= 0) in set: " << setName);
        return;
    }

    SpecialsInfo::iterator it = mSpecialsInfo.find(id);
    if (it != mSpecialsInfo.end())
    {
        LOG_WARN("SpecialManager: The same id: " << id
                 << " is given for special names: " << it->first
                 << " and " << name);
        LOG_WARN("The special reference: " << id
                 << ": '" << name << "' will be ignored.");
        return;
    }

    bool rechargeable = XML::getBoolProperty(specialNode, "rechargeable", true);
    int neededMana = XML::getProperty(specialNode, "needed", 0);
    int defaultRechargeSpeed = XML::getProperty(specialNode,
                                                "rechargespeed", 0);

    if (rechargeable && neededMana <= 0)
    {
        LOG_WARN("Invalid special '" << name
                 << "' (rechargable but no needed attribute) in set: "
                 << setName);
        return;
    }


    SpecialInfo *newInfo = new SpecialManager::SpecialInfo;
    newInfo->setName = setName;
    newInfo->name = name;
    newInfo->id = id;
    newInfo->rechargeable = rechargeable;
    newInfo->neededMana = neededMana;
    newInfo->defaultRechargeSpeed = defaultRechargeSpeed;

    newInfo->target = getTargetByString(XML::getProperty(specialNode, "target",
                                                         std::string()));

    mSpecialsInfo[newInfo->id] = newInfo;

    std::string keyName = setName + "_" + newInfo->name;
    mNamedSpecialsInfo[keyName] = newInfo;
}

void SpecialManager::initialize()
{
    clear();
}

void SpecialManager::reload()
{
    clear();
}

void SpecialManager::clear()
{
    for (SpecialsInfo::iterator it = mSpecialsInfo.begin(),
         it_end = mSpecialsInfo.end(); it != it_end; ++it)
    {
        delete it->second;
    }
    mSpecialsInfo.clear();
    mNamedSpecialsInfo.clear();
}

unsigned SpecialManager::getId(const std::string &set,
                               const std::string &name) const
{
    std::string key = utils::toLower(set) + "_" + utils::toLower(name);
    return getId(key);
}

unsigned SpecialManager::getId(const std::string &specialName) const
{
    if (mNamedSpecialsInfo.contains(specialName))
        return mNamedSpecialsInfo.value(specialName)->id;
    else
        return 0;
}

const std::string SpecialManager::getSpecialName(int id) const
{
    SpecialsInfo::const_iterator it = mSpecialsInfo.find(id);
    return it != mSpecialsInfo.end() ? it->second->name : "";
}

const std::string SpecialManager::getSetName(int id) const
{
    SpecialsInfo::const_iterator it = mSpecialsInfo.find(id);
    return it != mSpecialsInfo.end() ? it->second->setName : "";
}

SpecialManager::SpecialInfo *SpecialManager::getSpecialInfo(int id)
{
    SpecialsInfo::const_iterator it = mSpecialsInfo.find(id);
    return it != mSpecialsInfo.end() ? it->second : 0;
}
