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
#include <limits>

#include "utils/xml.h"

enum ScopeType
{
    BeingScope = 0,
    CharacterScope,
    MonsterScope,
    // Add new types here as needed
    MaxScope
};

/**
 * Stackable types.
 * @todo non-stackable malus layers
 */
enum StackableType
{
    Stackable,
    NonStackable,
    NonStackableBonus
};

/**
 * Attribute augmentation methods.
 */
enum ModifierEffectType
{
    Multiplicative,
    Additive
};

struct AttributeModifier
{
    AttributeModifier(StackableType s, ModifierEffectType effect) :
        stackableType(s),
        effectType(effect)
    {}

    StackableType stackableType;
    ModifierEffectType effectType;
};

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
        struct AttributeInfo {
            AttributeInfo():
                minimum(std::numeric_limits<double>::min()),
                maximum(std::numeric_limits<double>::max()),
                modifiable(false)
            {}

            /** The minimum and maximum permitted attribute values. */
            double minimum;
            double maximum;
            /** Tells whether the base attribute is modifiable by the player */
            bool modifiable;
            /** Effect modifier type: stackability and modification type. */
            std::vector<struct AttributeModifier> modifiers;
        };

        AttributeManager()
        {}

        /**
         * Loads attribute reference file.
         */
        void initialize();

        /**
         * Reloads attribute reference file.
         */
        void reload();

        const std::vector<AttributeModifier> *getAttributeInfo(int id) const;

        // being type id -> (*{ stackable type, effect type })[]
        typedef std::map<int, AttributeInfo*> AttributeScope;

        const AttributeScope &getAttributeScope(ScopeType) const;

        bool isAttributeDirectlyModifiable(int id) const;

        ModifierLocation getLocation(const std::string &tag) const;

        const std::string *getTag(const ModifierLocation &location) const;

        void readAttributeNode(xmlNodePtr attributeNode);

        void checkStatus();

    private:
        void readModifierNode(xmlNodePtr modifierNode, int attributeId);

        typedef std::map<int, AttributeInfo> AttributeMap;

        /** Maps tag names to specific modifiers. */
        typedef std::map<std::string, ModifierLocation> TagMap;

        AttributeScope mAttributeScopes[MaxScope];
        AttributeMap mAttributeMap;
        TagMap mTagMap;
};

extern AttributeManager *attributeManager;

#endif // ATTRIBUTEMANAGER_H
