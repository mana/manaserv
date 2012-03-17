/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *  Copyright (C) 2012  The Mana Developers
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

#ifndef ENTITY_H
#define ENTITY_H

#include "common/manaserv_protocol.h"

#include "game-server/component.h"

#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include <cassert>

using namespace ManaServ;

class MapComposite;

/**
 * Base class for in-game objects.
 *
 * Knows its type, the map it resides on and is host to a number of optional
 * components.
 */
class Entity : public sigc::trackable
{
    public:
        Entity(EntityType type, MapComposite *map = 0);

        virtual ~Entity();

        /**
         * Gets type of this entity.
         *
         * @return the type of this entity.
         */
        EntityType getType() const
        { return mType; }

        /**
         * Adds a component. Only one component of a given type can be added.
         * Entity takes ownership of \a component.
         */
        template <class T>
        void addComponent(T *component)
        {
            assert(!mComponents[T::type]);
            mComponents[T::type] = component;
        }

        /**
         * Returns the component of the given type, or 0 when no such component
         * was set.
         */
        Component *getComponent(ComponentType type) const
        { return mComponents[type]; }

        /**
         * Get a component by its class. Avoids the need for doing a static-
         * cast in the calling code.
         */
        template <class T>
        T *getComponent() const
        { return static_cast<T*>(getComponent(T::type)); }

        /**
         * Returns whether this entity is visible on the map or not. (Actor)
         */
        bool isVisible() const
        { return mType != OBJECT_OTHER; }

        /**
         * Returns whether this entity can move on the map or not. (Actor)
         */
        bool canMove() const
        { return mType == OBJECT_CHARACTER || mType == OBJECT_MONSTER ||
                 mType == OBJECT_NPC; }

        /**
         * Returns whether this entity can fight or not. (Being)
         */
        bool canFight() const
        { return mType == OBJECT_CHARACTER || mType == OBJECT_MONSTER; }

        /**
         * Updates the internal status. By default, calls update on all its
         * components.
         */
        virtual void update();

        /**
         * Gets the map this entity is located on.
         */
        MapComposite *getMap() const
        { return mMap; }

        /**
         * Sets the map this entity is located on.
         */
        virtual void setMap(MapComposite *map)
        { mMap = map; }

        sigc::signal<void, Entity *> signal_inserted;
        sigc::signal<void, Entity *> signal_removed;

    private:
        MapComposite *mMap;     /**< Map the entity is on */
        EntityType mType;       /**< Type of this entity. */

        Component *mComponents[ComponentTypeCount];
};

#endif // ENTITY_H
