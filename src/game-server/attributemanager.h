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

#ifndef ATTRIBUTEMANAGER_H
#define ATTRIBUTEMANAGER_H

#include <map>
#include <vector>
#include <string>

#include "utils/xml.h"

enum ScopeType
{
    BeingScope = 0,
    CharacterScope,
    MonsterScope,
    // Add new types here as needed
    MaxScope
};

typedef std::map<int, std::vector<struct AttributeInfoType> *> AttributeScope;

/**
 * Identifies a modifier by the attribute id that it applies to and its layer
 * index in the stack of modifiers for that attribute.
 */
struct ModifierLocation
{
    int attributeId;
    int layer;

    ModifierLocation(int attributeId, int layer)
        : attributeId(attributeId)
        , layer(layer)
    {}

    bool operator==(const ModifierLocation &other) const
    { return attributeId == other.attributeId && layer == other.layer; }
};

class AttributeManager
{
    public:
        AttributeManager(const std::string &file) :
            mAttributeReferenceFile(file)
        {}

        /**
         * Loads attribute reference file.
         */
        void initialize();

        /**
         * Reloads attribute reference file.
         */
        void reload();

        const std::vector<struct AttributeInfoType> *getAttributeInfo(int id) const;

        const AttributeScope &getAttributeScope(ScopeType) const;

        bool isAttributeDirectlyModifiable(int id) const;

        ModifierLocation getLocation(const std::string &tag) const;

        const std::string *getTag(const ModifierLocation &location) const;

    private:
        void readAttributesFile();
        void readAttributeNode(xmlNodePtr attributeNode);
        void readModifierNode(xmlNodePtr modifierNode, int attributeId);

        // modifiable, { stackable type, effect type }[]
        typedef std::pair<bool,
                       std::vector<struct AttributeInfoType> > AttributeInfoMap;

        // Attribute id -> { modifiable, { stackable type, effect type }[] }
        typedef std::map<int, AttributeInfoMap> AttributeMap;

        /** Maps tag names to specific modifiers. */
        typedef std::map<std::string, ModifierLocation> TagMap;

        // being type id -> (*{ stackable type, effect type })[]
        AttributeScope mAttributeScopes[MaxScope];
        AttributeMap mAttributeMap;
        TagMap mTagMap;

        const std::string mAttributeReferenceFile;
};

extern AttributeManager *attributeManager;

#endif // ATTRIBUTEMANAGER_H
