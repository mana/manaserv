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


#ifndef SKILLMANAGER_H
#define SKILLMANAGER_H

#include "utils/string.h"
#include "utils/xml.h"

class SkillManager
{
  public:
    SkillManager():
        mDefaultSkillId(0)
    {}

    ~SkillManager()
    { clear(); }

    /**
     * Loads skill reference file.
     */
    void initialize();

    /**
     * Reloads skill reference file.
     */
    void reload();

    /**
     * Gets the skill Id from a set and a skill string.
     */
    unsigned getId(const std::string &set, const std::string &name) const;

    /**
     * Gets the skill Id from a string formatted in this way:
     * "setname_skillname"
     */
    unsigned getId(const std::string &skillName) const;

    const std::string getSkillName(unsigned id) const;
    const std::string getSetName(unsigned id) const;

    bool exists(unsigned id) const;

    unsigned getDefaultSkillId() const
    { return mDefaultSkillId; }

    /**
     * Read a <skill-set> element from settings.
     * Used by SettingsManager.
     */
    void readSkillSetNode(xmlNodePtr node, const std::string& filename);

    /**
     * Check the status of recently loaded configuration.
     */
    void checkStatus();

  private:
    struct SkillInfo {
        SkillInfo():
            id(0)
        {}

        unsigned id;
        std::string setName;
        std::string skillName;
    };

    /*
     * Clears up the skill maps.
     */
    void clear();

    void printDebugSkillTable();

    void readSkillNode(xmlNodePtr skillNode, const std::string& setName);

    // The skill map
    typedef std::map<unsigned, SkillInfo*> SkillsInfo;
    SkillsInfo mSkillsInfo;
    // A map used to get skills per name.
    utils::NameMap<SkillInfo*> mNamedSkillsInfo;

    // The default skill id
    unsigned mDefaultSkillId;
};

extern SkillManager *skillManager;

#endif // SKILLMANAGER_H
