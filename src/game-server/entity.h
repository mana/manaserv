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

        EntityType getType() const;

        template <class T> void addComponent(T *component);
        template <class T> T *getComponent() const;

        Component *getComponent(ComponentType type) const;

        bool isVisible() const;
        bool canMove() const;
        bool canFight() const;

        virtual void update();

        MapComposite *getMap() const;
        void setMap(MapComposite *map);

        sigc::signal<void, Entity *> signal_inserted;
        sigc::signal<void, Entity *> signal_removed;
        sigc::signal<void, Entity *> signal_map_changed;

    private:
        MapComposite *mMap;     /**< Map the entity is on */
        EntityType mType;       /**< Type of this entity. */

        Component *mComponents[ComponentTypeCount];
};

/**
 * Gets type of this entity.
 *
 * @return the type of this entity.
 */
inline EntityType Entity::getType() const
{
    return mType;
}

/**
 * Adds a component. Only one component of a given type can be added.
 * Entity takes ownership of \a component.
 */
template <class T>
inline void Entity::addComponent(T *component)
{
    mComponents[T::type] = component;
}

/**
 * Returns the component of the given type, or 0 when no such component
 * was set.
 */
inline Component *Entity::getComponent(ComponentType type) const
{
    assert(mComponents[type]);
    return mComponents[type];
}

/**
 * Get a component by its class. Avoids the need for doing a static-
 * cast in the calling code.
 */
template <class T>
inline T *Entity::getComponent() const
{
    return static_cast<T*>(getComponent(T::type));
}

/**
 * Returns whether this entity is visible on the map or not. (Actor)
 */
inline bool Entity::isVisible() const
{
    return mType != OBJECT_OTHER;
}

/**
 * Returns whether this entity can move on the map or not. (Actor)
 */
inline bool Entity::canMove() const
{
    return mType == OBJECT_CHARACTER || mType == OBJECT_MONSTER ||
           mType == OBJECT_NPC;
}

/**
 * Returns whether this entity can fight or not. (Being)
 */
inline bool Entity::canFight() const
{
    return mType == OBJECT_CHARACTER || mType == OBJECT_MONSTER;
}

/**
 * Gets the map this entity is located on.
 */
inline MapComposite *Entity::getMap() const
{
    return mMap;
}

/**
 * Sets the map this entity is located on.
 */
inline void Entity::setMap(MapComposite *map)
{
    mMap = map;
    signal_map_changed.emit(this);
}

#endif // ENTITY_H
