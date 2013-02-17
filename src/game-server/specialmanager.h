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


#ifndef SPECIALMANAGER_H
#define SPECIALMANAGER_H

#include "utils/string.h"
#include "utils/xml.h"

#include "scripting/script.h"



class SpecialManager
{
public:
    enum TargetMode
    {
        TARGET_BEING,
        TARGET_POINT
    };

    struct SpecialInfo
    {
        SpecialInfo() :
            id(0),
            rechargeable(false),
            defaultRechargeSpeed(0),
            neededMana(0),
            target(TARGET_BEING)
        {}

        unsigned id;
        std::string name;
        std::string setName;
        bool rechargeable;
        int defaultRechargeSpeed;
        unsigned neededMana;
        TargetMode target;
        Script::Ref rechargedCallback;
        Script::Ref useCallback;
    };

    SpecialManager()
    { }

    ~SpecialManager()
    { clear(); }

    /**
     * Loads special reference file.
     */
    void initialize();

    /**
     * Reloads special reference file.
     */
    void reload();

    /**
     * Gets the specials Id from a set and a special string.
     */
    unsigned getId(const std::string &set, const std::string &name) const;

    /**
     * Gets the specials Id from a string formatted in this way:
     * "setname_specialname"
     */
    unsigned getId(const std::string &specialName) const;

    const std::string getSpecialName(int id) const;
    const std::string getSetName(int id) const;

    SpecialInfo *getSpecialInfo(int id);


    void readSpecialSetNode(xmlNodePtr node, const std::string &filename);

    void checkStatus();

private:
    /**
     * Clears up the special maps.
     */
    void clear();

    void readSpecialNode(xmlNodePtr specialNode,
                         const std::string &setName);

    typedef std::map<unsigned, SpecialInfo*> SpecialsInfo;
    SpecialsInfo mSpecialsInfo;
    typedef utils::NameMap<SpecialInfo*> NamedSpecialsInfo;
    NamedSpecialsInfo mNamedSpecialsInfo;

};

extern SpecialManager *specialManager;

#endif // SPECIALMANAGER_H
