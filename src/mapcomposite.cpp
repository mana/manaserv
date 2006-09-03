/*
 *  The Mana World Server
 *  Copyright 2006 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id: $
 */

#include "mapcomposite.h"

#include <algorithm>
#include <cassert>

#include "defines.h"
#include "map.h"

static int const zoneDiam = 256;

void MapZone::insert(Object *obj)
{
    int type = obj->getType();
    switch (type)
    {
        case OBJECT_PLAYER:
        {
            if (nbPlayers != nbMovingObjects)
            {
                if (nbMovingObjects != objects.size())
                {
                    objects.push_back(objects[nbMovingObjects]);
                    objects[nbMovingObjects] = objects[nbPlayers];
                }
                else
                {
                    objects.push_back(objects[nbPlayers]);
                }
                objects[nbPlayers] = obj;
                ++nbPlayers;
                ++nbMovingObjects;
                break;
            }
            ++nbPlayers;
        } // no break!
        case OBJECT_MONSTER:
        case OBJECT_NPC:
        {
            if (nbMovingObjects != objects.size())
            {
                objects.push_back(objects[nbMovingObjects]);
                objects[nbMovingObjects] = obj;
                ++nbMovingObjects;
                break;
            }
            ++nbMovingObjects;
        } // no break!
        default:
        {
            objects.push_back(obj);
        }
    }
}

void MapZone::remove(Object *obj)
{
    std::vector< Object * >::iterator i_beg = objects.begin(), i, i_end;
    int type = obj->getType();
    switch (type)
    {
        case OBJECT_PLAYER:
        {
            i_end = objects.begin() + nbPlayers;
            i = std::find(i_beg, i_end, obj);
            assert(i != i_end);
            int pos = i - i_beg;
            if (pos != nbPlayers - 1)
            {
                objects[pos] = objects[nbPlayers - 1];
            }
            if (nbPlayers != nbMovingObjects)
            {
                objects[nbPlayers - 1] = objects[nbMovingObjects - 1];
            }
            --nbPlayers;
            if (nbMovingObjects != objects.size())
            {
                objects[nbMovingObjects - 1] = objects[objects.size() - 1];
            }
            --nbMovingObjects;
            objects.pop_back();
        } break;
        case OBJECT_MONSTER:
        case OBJECT_NPC:
        {
            i_end = objects.begin() + nbMovingObjects;
            i = std::find(objects.begin() + nbPlayers, i_end, obj);
            assert(i != i_end);
            int pos = i - i_beg;
            if (pos != nbMovingObjects - 1)
            {
                objects[pos] = objects[nbMovingObjects - 1];
            }
            if (nbMovingObjects != objects.size())
            {
                objects[nbMovingObjects - 1] = objects[objects.size() - 1];
            }
            --nbMovingObjects;
            objects.pop_back();
        } break;
        default:
        {
            i_end = objects.end();
            i = std::find(objects.begin() + nbMovingObjects, i_end, obj);
            assert(i != i_end);
            unsigned pos = i - i_beg;
            if (pos != objects.size() - 1)
            {
                objects[pos] = objects[objects.size() - 1];
            }
            objects.pop_back();
        }
    }
}

static void addZone(MapRegion &r, unsigned z)
{
    MapRegion::iterator i_end = r.end(),
                        i = std::lower_bound(r.begin(), i_end, z);
    /*
    if (i == i_end)
    {
        r.push_back(z);
    }
    else if (*i != z)
    {
        r.insert(i, z);
    }
    */
    if (i == i_end || *i != z)
    {
        r.insert(i, z);
    }
}

ZoneIterator::ZoneIterator(MapRegion const &r, MapComposite const *m)
  : region(r), pos(0), map(m)
{
    current = &map->zones[r.empty() ? 0 : r[0]];
}

void ZoneIterator::operator++()
{
    current = NULL;
    if (!region.empty())
    {
        if (++pos != region.size())
        {
            current = &map->zones[region[pos]];
        }
    }
    else
    {
        if (++pos != (unsigned)map->mapWidth * map->mapHeight)
        {
            current = &map->zones[pos];
        }
    }
}

PlayerIterator::PlayerIterator(ZoneIterator const &it)
  : iterator(it), pos(0)
{
    while (iterator && (*iterator)->nbPlayers == 0) ++iterator;
    if (iterator)
    {
        current = static_cast< Player * >((*iterator)->objects[pos]);
    }
}

void PlayerIterator::operator++()
{
    if (++pos == (*iterator)->nbPlayers)
    {
        do ++iterator; while (iterator && (*iterator)->nbPlayers == 0);
        pos = 0;
    }
    if (iterator)
    {
        current = static_cast< Player * >((*iterator)->objects[pos]);
    }
}

MovingObjectIterator::MovingObjectIterator(ZoneIterator const &it)
  : iterator(it), pos(0)
{
    while (iterator && (*iterator)->nbMovingObjects == 0) ++iterator;
    if (iterator)
    {
        current = static_cast< MovingObject * >((*iterator)->objects[pos]);
    }
}

void MovingObjectIterator::operator++()
{
    if (++pos == (*iterator)->nbMovingObjects)
    {
        do ++iterator; while (iterator && (*iterator)->nbMovingObjects == 0);
        pos = 0;
    }
    if (iterator)
    {
        current = static_cast< MovingObject * >((*iterator)->objects[pos]);
    }
}

ObjectIterator::ObjectIterator(ZoneIterator const &it)
  : iterator(it), pos(0)
{
    while (iterator && (*iterator)->objects.empty()) ++iterator;
    if (iterator)
    {
        current = (*iterator)->objects[pos];
    }
}

void ObjectIterator::operator++()
{
    if (++pos == (*iterator)->objects.size())
    {
        do ++iterator; while (iterator && (*iterator)->objects.empty());
        pos = 0;
    }
    if (iterator)
    {
        current = (*iterator)->objects[pos];
    }
}

ObjectBucket::ObjectBucket()
  : free(256), next_object(0)
{
    for (unsigned i = 0; i < 256 / sizeof(unsigned); ++i)
    {
        bitmap[i] = ~0;
    }
}

int ObjectBucket::allocate()
{
    if (!free)
    {
        return -1;
    }

    if (bitmap[next_object / sizeof(unsigned)] & (1 << (next_object % sizeof(unsigned))))
    {
        bitmap[next_object / sizeof(unsigned)] &= ~(1 << (next_object % sizeof(unsigned)));
        int i = next_object;
        next_object = (i + 1) & 255;
        --free;
        return i;
    }

    for (unsigned i = 0; i < 256 / sizeof(unsigned); ++i)
    {
        unsigned &current = bitmap[(i + next_object / sizeof(unsigned)) & 255];
        unsigned b = current;
        if (b)
        {
            int j = 0;
            while (!(b & 1))
            {
                b >>= 1;
                ++j;
            }
            current &= ~(1 << j);
            next_object = (j + 1) & 255;
            --free;
            return j;
        }
    }
    return -1;
}

void ObjectBucket::deallocate(int i)
{
    bitmap[i / sizeof(unsigned)] |= 1 << (i % sizeof(unsigned));
    ++free;
}

MapComposite::MapComposite(Map *m)
  : map(m), last_bucket(0)
{
    buckets[0] = new ObjectBucket;
    for (int i = 1; i < 256; ++i)
    {
        buckets[i] = NULL;
    }
    mapWidth = (map->getWidth() * 32 + zoneDiam - 1) / zoneDiam;
    mapHeight = (map->getHeight() * 32 + zoneDiam - 1) / zoneDiam;
    zones = new MapZone[mapWidth * mapHeight];
}

MapComposite::~MapComposite()
{
    for (int i = 0; i < 256; ++i)
    {
        delete buckets[i];
    }
    delete[] zones;
    delete map;
}

void MapComposite::allocate(MovingObject *obj)
{
    ObjectBucket *b = buckets[last_bucket];
    int i = b->allocate();
    if (i >= 0)
    {
        b->objects[i] = obj;
        int id = last_bucket * 256 + i;
        obj->setPublicID(id);
        return;
    }

    for (i = 0; i < 256; ++i)
    {
        b = buckets[i];
        if (!b)
        {
            b = new ObjectBucket;
            buckets[i] = b;
        }
        int j = b->allocate();
        if (j >= 0)
        {
            last_bucket = i;
            b->objects[j] = obj;
            int id = i * 256 + j;
            obj->setPublicID(id);
            return;
        }
    }
}

void MapComposite::deallocate(MovingObject *obj)
{
    int id = obj->getPublicID();
    if (id == 65535)
    {
        return;
    }

    obj->setPublicID(65535);
    buckets[id / 256]->deallocate(id % 256);
}

void MapComposite::fillRegion(MapRegion &r, Point const &p) const
{
    int radius = AROUND_AREA,
        ax = p.x > radius ? (p.x - radius) / zoneDiam : 0,
        ay = p.y > radius ? (p.y - radius) / zoneDiam : 0,
        bx = std::max((p.x + radius) / zoneDiam, mapWidth - 1),
        by = std::max((p.y + radius) / zoneDiam, mapHeight - 1);
    for (int y = ay; y <= by; ++y)
    {
        for (int x = ax; x <= bx; ++x)
        {
            addZone(r, x + y * mapWidth);
        }
    }
}

ZoneIterator MapComposite::getAroundObjectIterator(Object *obj) const
{
    MapRegion r;
    fillRegion(r, obj->getPosition());
    return ZoneIterator(r, this);
}

ZoneIterator MapComposite::getAroundPlayerIterator(Player *obj) const
{
    MapRegion r1;
    fillRegion(r1, obj->getOldPosition());
    MapRegion r2 = r1;
    for (MapRegion::iterator i = r1.begin(), i_end = r1.end(); i != i_end; ++i)
    {
        /* Fills region with destinations taken around the old position.
           This is necessary to detect two moving objects changing zones at the
           same time and at the border, and going in opposite directions (or
           more simply to detect teleportations, if any). */
        MapRegion &r4 = zones[*i].destinations;
        if (!r4.empty())
        {
            MapRegion r3;
            std::set_union(r2.begin(), r2.end(), r4.begin(), r4.end(),
                           std::back_insert_iterator< MapRegion >(r3));
            r2.swap(r3);
        }
    }
    fillRegion(r2, obj->getPosition());
    return ZoneIterator(r2, this);
}

void MapComposite::insert(ObjectPtr obj)
{
    Point const &pos = obj->getPosition();
    zones[(pos.x / zoneDiam) + (pos.y / zoneDiam) * mapWidth].insert(obj.get());

    int type = obj->getType();
    if (type == OBJECT_MONSTER || type == OBJECT_PLAYER || type == OBJECT_NPC)
    {
        allocate(static_cast< MovingObject * >(obj.get()));
    }

    objects.push_back(obj);
}

void MapComposite::remove(ObjectPtr obj)
{
    Point const &pos = obj->getPosition();
    zones[(pos.x / zoneDiam) + (pos.y / zoneDiam) * mapWidth].remove(obj.get());

    int type = obj->getType();
    if (type == OBJECT_MONSTER || type == OBJECT_PLAYER || type == OBJECT_NPC)
    {
        deallocate(static_cast< MovingObject * >(obj.get()));
    }

    for (Objects::iterator o = objects.begin(),
         o_end = objects.end(); o != o_end; ++o)
    {
        if (o->get() == obj.get())
        {
            *o = *(o_end - 1);
            objects.pop_back();
            return;
        }
    }
    assert(false);
}

void MapComposite::update()
{
    for (int i = 0; i < mapHeight * mapWidth; ++i)
    {
        zones[i].destinations.clear();
    }

    // Cannot use a WholeMap iterator as objects will change zones under its feet.
    for (Objects::iterator o = objects.begin(),
         o_end = objects.end(); o != o_end; ++o)
    {
        int type = (*o)->getType();
        if (type != OBJECT_PLAYER && type != OBJECT_MONSTER && type != OBJECT_NPC)
        {
            continue;
        }
        MovingObject *obj = static_cast< MovingObject * >(o->get());

        Point const &pos1 = obj->getOldPosition(),
                    &pos2 = obj->getPosition();
        int src = (pos1.x / zoneDiam) + (pos1.y / zoneDiam) * mapWidth,
            dst = (pos2.x / zoneDiam) + (pos2.y / zoneDiam) * mapWidth;
        if (src != dst)
        {
            addZone(zones[src].destinations, dst);
            zones[src].remove(obj);
            zones[dst].insert(obj);
        }
    }
}
