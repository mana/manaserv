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

#ifndef ATTRIBUTEMANAGER_HPP
#define ATTRIBUTEMANAGER_HPP

#include <map>
#include <vector>
#include <string>

typedef struct AttributeInfoType AttributeInfoType_t;

enum SCOPE_TYPES {
    ATTR_BEING = 0,
    ATTR_CHAR,
    ATTR_MOB,
    // Add new types here as needed
    ATTR_MAX
};

typedef std::map< int, std::vector<struct AttributeInfoType> * > AttributeScopes;

class AttributeManager
{
    public:
        AttributeManager(const std::string &file) : mAttributeReferenceFile(file) {}
        /**
         * Loads attribute reference file.
         */
        void initialize();

        /**
         * Reloads attribute reference file.
         */
        void reload();
        const std::vector<struct AttributeInfoType> *getAttributeInfo(unsigned int) const;

        const AttributeScopes &getAttributeInfoForType(SCOPE_TYPES) const;

        bool isAttributeDirectlyModifiable(unsigned int) const;

        std::pair<unsigned int,unsigned int> getInfoFromTag(const std::string &) const;

        const std::string *getTagFromInfo(unsigned int, unsigned int) const;
    private:
        // attribute id -> { modifiable, { stackable type, effect type }[] }
        typedef std::map< int, std::pair< bool, std::vector<struct AttributeInfoType> > > AttributeMap;
        // tag name -> { attribute id, layer }
        typedef std::map< std::string, std::pair<unsigned int, unsigned int> > TagMap;

        // being type id -> (*{ stackable type, effect type })[]
        AttributeScopes mAttributeScopes[ATTR_MAX];
        AttributeMap mAttributeMap;
        TagMap mTagMap;

        const std::string mAttributeReferenceFile;
};

extern AttributeManager *attributeManager;

#endif // ATTRIBUTEMANAGER_HPP
