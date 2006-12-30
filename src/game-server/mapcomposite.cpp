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
 *  $Id$
 */

#include <algorithm>
#include <cassert>

#include "map.h"
#include "game-server/mapcomposite.hpp"

/* TODO: Implement overlapping map zones instead of strict partitioning.
   Purpose: to decrease the number of zone changes, as overlapping allows for
   hysteresis effect and prevents an object from changing zone each server
   tick. It requires storing the zone in the object since it will not be
   uniquely defined any longer. */

/* Pixel-based width and height of the squares used in partitioning the map.
   Squares should be big enough so that a moving object cannot cross several
   ones in one world tick.
   TODO: Tune for decreasing server load. The higher the value, the closer we
   regress to quadratic behavior; the lower the value, the more we waste time
   in dealing with zone changes. */
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
            i = i_beg;
            i_end = objects.begin() + nbPlayers;
        } break;
        case OBJECT_MONSTER:
        case OBJECT_NPC:
        {
            i = objects.begin() + nbPlayers;
            i_end = objects.begin() + nbMovingObjects;
        } break;
        default:
        {
            i = objects.begin() + nbMovingObjects;
            i_end = objects.end();
        }
    }
    i = std::find(i, i_end, obj);
    assert(i != i_end);
    unsigned pos = i - i_beg;
    if (pos < nbPlayers)
    {
        objects[pos] = objects[nbPlayers - 1];
        pos = nbPlayers - 1;
        --nbPlayers;
    }
    if (pos < nbMovingObjects)
    {
        objects[pos] = objects[nbMovingObjects - 1];
        pos = nbMovingObjects - 1;
        --nbMovingObjects;
    }
    objects[pos] = objects[objects.size() - 1];
    objects.pop_back();
}

static void addZone(MapRegion &r, unsigned z)
{
    MapRegion::iterator i_end = r.end(),
                        i = std::lower_bound(r.begin(), i_end, z);
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
    for (unsigned i = 0; i < 256 / int_bitsize; ++i)
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

    if (bitmap[next_object / int_bitsize] & (1 << (next_object % int_bitsize)))
    {
        bitmap[next_object / int_bitsize] &= ~(1 << (next_object % int_bitsize));
        int i = next_object;
        next_object = (i + 1) & 255;
        --free;
        return i;
    }

    for (unsigned i = 0; i < 256 / int_bitsize; ++i)
    {
        int k = (i + next_object / int_bitsize) & 255;
        if (unsigned b = bitmap[k])
        {
            int j = 0;
            while (!(b & 1))
            {
                b >>= 1;
                ++j;
            }
            bitmap[k] &= ~(1 << j);
            j += k * int_bitsize;
            next_object = (j + 1) & 255;
            --free;
            return j;
        }
    }
    return -1;
}

void ObjectBucket::deallocate(int i)
{
    assert(!(bitmap[i / int_bitsize] & (1 << (i % int_bitsize))));
    bitmap[i / int_bitsize] |= 1 << (i % int_bitsize);
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

bool MapComposite::allocate(MovingObject *obj)
{
    // First, try allocating from the last used bucket.
    ObjectBucket *b = buckets[last_bucket];
    int i = b->allocate();
    if (i >= 0)
    {
        b->objects[i] = obj;
        obj->setPublicID(last_bucket * 256 + i);
        return true;
    }

    /* If the last used bucket is already full, scan all the buckets for an
       empty place. If none is available, create a new bucket. */
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
            obj->setPublicID(last_bucket * 256 + j);
            return true;
        }
    }

    // All the IDs are currently used, fail.
    return false;
}

void MapComposite::deallocate(MovingObject *obj)
{
    unsigned short id = obj->getPublicID();
    buckets[id / 256]->deallocate(id % 256);
}

void MapComposite::fillRegion(MapRegion &r, Point const &p, int radius) const
{
    int ax = p.x > radius ? (p.x - radius) / zoneDiam : 0,
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

ZoneIterator MapComposite::getAroundObjectIterator(Object *obj, int radius) const
{
    MapRegion r;
    fillRegion(r, obj->getPosition(), radius);
    return ZoneIterator(r, this);
}

ZoneIterator MapComposite::getAroundPlayerIterator(Player *obj, int radius) const
{
    MapRegion r1;
    fillRegion(r1, obj->getOldPosition(), radius);
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
            r3.reserve(r2.size() + r4.size());
            std::set_union(r2.begin(), r2.end(), r4.begin(), r4.end(),
                           std::back_insert_iterator< MapRegion >(r3));
            r2.swap(r3);
        }
    }
    fillRegion(r2, obj->getPosition(), radius);
    return ZoneIterator(r2, this);
}

bool MapComposite::insert(Object *obj)
{
    Point const &pos = obj->getPosition();
    zones[(pos.x / zoneDiam) + (pos.y / zoneDiam) * mapWidth].insert(obj);

    int type = obj->getType();
    if (type == OBJECT_MONSTER || type == OBJECT_PLAYER || type == OBJECT_NPC)
    {
        if (!allocate(static_cast< MovingObject * >(obj)))
        {
            return false;
        }
    }

    objects.push_back(obj);
    return true;
}

void MapComposite::remove(Object *obj)
{
    Point const &pos = obj->getPosition();
    zones[(pos.x / zoneDiam) + (pos.y / zoneDiam) * mapWidth].remove(obj);

    int type = obj->getType();
    if (type == OBJECT_MONSTER || type == OBJECT_PLAYER || type == OBJECT_NPC)
    {
        deallocate(static_cast< MovingObject * >(obj));
    }

    for (Objects::iterator o = objects.begin(),
         o_end = objects.end(); o != o_end; ++o)
    {
        if (*o == obj)
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
        MovingObject *obj = static_cast< MovingObject * >(*o);

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
