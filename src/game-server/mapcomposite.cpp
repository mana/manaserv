/*
 *  The Mana Server
 *  Copyright (C) 2006-2010  The Mana World Development Team
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

#include <algorithm>
#include <cassert>

#include "point.h"
#include "common/configuration.h"
#include "game-server/map.h"
#include "game-server/mapcomposite.h"
#include "game-server/character.h"
#include "scripting/script.h"
#include "utils/logger.h"

/* TODO: Implement overlapping map zones instead of strict partitioning.
   Purpose: to decrease the number of zone changes, as overlapping allows for
   hysteresis effect and prevents an actor from changing zone each server
   tick. It requires storing the zone in the actor since it will not be
   uniquely defined any longer. */

/* Pixel-based width and height of the squares used in partitioning the map.
   Squares should be big enough so that an actor cannot cross several ones
   in one world tick.
   TODO: Tune for decreasing server load. The higher the value, the closer we
   regress to quadratic behavior; the lower the value, the more we waste time
   in dealing with zone changes. */
static int const zoneDiam = 256;

void MapZone::insert(Actor *obj)
{
    int type = obj->getType();
    switch (type)
    {
        case OBJECT_CHARACTER:
        {
            if (nbCharacters != nbMovingObjects)
            {
                if (nbMovingObjects != objects.size())
                {
                    objects.push_back(objects[nbMovingObjects]);
                    objects[nbMovingObjects] = objects[nbCharacters];
                }
                else
                {
                    objects.push_back(objects[nbCharacters]);
                }
                objects[nbCharacters] = obj;
                ++nbCharacters;
                ++nbMovingObjects;
                break;
            }
            ++nbCharacters;
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

void MapZone::remove(Actor *obj)
{
    std::vector< Actor * >::iterator i_beg = objects.begin(), i, i_end;
    int type = obj->getType();
    switch (type)
    {
        case OBJECT_CHARACTER:
        {
            i = i_beg;
            i_end = objects.begin() + nbCharacters;
        } break;
        case OBJECT_MONSTER:
        case OBJECT_NPC:
        {
            i = objects.begin() + nbCharacters;
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
    if (pos < nbCharacters)
    {
        objects[pos] = objects[nbCharacters - 1];
        pos = nbCharacters - 1;
        --nbCharacters;
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

ZoneIterator::ZoneIterator(const MapRegion &r, const MapContent *m)
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

CharacterIterator::CharacterIterator(const ZoneIterator &it)
  : iterator(it), pos(0)
{
    while (iterator && (*iterator)->nbCharacters == 0) ++iterator;
    if (iterator)
    {
        current = static_cast< Character * >((*iterator)->objects[pos]);
    }
}

void CharacterIterator::operator++()
{
    if (++pos == (*iterator)->nbCharacters)
    {
        do ++iterator; while (iterator && (*iterator)->nbCharacters == 0);
        pos = 0;
    }
    if (iterator)
    {
        current = static_cast< Character * >((*iterator)->objects[pos]);
    }
}

BeingIterator::BeingIterator(const ZoneIterator &it)
  : iterator(it), pos(0)
{
    while (iterator && (*iterator)->nbMovingObjects == 0) ++iterator;
    if (iterator)
    {
        current = static_cast< Being * >((*iterator)->objects[pos]);
    }
}

void BeingIterator::operator++()
{
    if (++pos == (*iterator)->nbMovingObjects)
    {
        do ++iterator; while (iterator && (*iterator)->nbMovingObjects == 0);
        pos = 0;
    }
    if (iterator)
    {
        current = static_cast< Being * >((*iterator)->objects[pos]);
    }
}

FixedActorIterator::FixedActorIterator(const ZoneIterator &it)
  : iterator(it), pos(0)
{
    while (iterator && (*iterator)->nbMovingObjects == (*iterator)->objects.size()) ++iterator;
    if (iterator)
    {
        pos = (*iterator)->nbMovingObjects;
        current = (*iterator)->objects[pos];
    }
}

void FixedActorIterator::operator++()
{
    if (++pos == (*iterator)->objects.size())
    {
        do ++iterator; while (iterator && (*iterator)->nbMovingObjects == (*iterator)->objects.size());
        if (iterator)
        {
            pos = (*iterator)->nbMovingObjects;
        }
    }
    if (iterator)
    {
        current = (*iterator)->objects[pos];
    }
}

ActorIterator::ActorIterator(const ZoneIterator &it)
  : iterator(it), pos(0)
{
    while (iterator && (*iterator)->objects.empty()) ++iterator;
    if (iterator)
    {
        current = (*iterator)->objects[pos];
    }
}

void ActorIterator::operator++()
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
        // An occupied ID is represented by zero in the bitmap.
        bitmap[i] = ~0u;
    }
}

int ObjectBucket::allocate()
{
    // Any free ID in the bucket?
    if (!free)
    {
        LOG_INFO("No free id in bucket");
        return -1;
    }

    int freeBucket = -1;
    // See if the the next_object bucket is free
    if (bitmap[next_object] != 0)
    {
        freeBucket = next_object;
    }
    else
    {
        /* next_object was not free. Check the whole bucket until one ID is found,
           starting from the IDs around next_object. */
        for (unsigned i = 0; i < 256 / int_bitsize; ++i)
        {
            // Check to see if this subbucket is free
            if (bitmap[i] != 0)
            {
                freeBucket = i;
                break;
            }
        }
    }

    assert(freeBucket >= 0);

    // One of them is free. Find it by looking bit-by-bit.
    int b = bitmap[freeBucket];
    int j = 0;
    while (!(b & 1))
    {
        b >>= 1;
        ++j;
    }
    // Flip that bit to on, and return the value
    bitmap[freeBucket] &= ~(1 << j);
    j += freeBucket * int_bitsize;
    next_object = freeBucket;
    --free;
    return j;
}

void ObjectBucket::deallocate(int i)
{
    assert(!(bitmap[i / int_bitsize] & (1 << (i % int_bitsize))));
    bitmap[i / int_bitsize] |= 1 << (i % int_bitsize);
    ++free;
}

MapContent::MapContent(Map *map)
  : last_bucket(0), zones(NULL)
{
    buckets[0] = new ObjectBucket;
    buckets[0]->allocate(); // Skip ID 0
    for (int i = 1; i < 256; ++i)
    {
        buckets[i] = NULL;
    }
    mapWidth = (map->getWidth() * map->getTileWidth() + zoneDiam - 1)
               / zoneDiam;
    mapHeight = (map->getHeight() * map->getTileHeight() + zoneDiam - 1)
                / zoneDiam;
    zones = new MapZone[mapWidth * mapHeight];
}

MapContent::~MapContent()
{
    for (int i = 0; i < 256; ++i)
    {
        delete buckets[i];
    }
    delete[] zones;
}

bool MapContent::allocate(Actor *obj)
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
            /* Buckets are created in order. If there is nothing at position i,
               there will not be anything in the next positions. So create a
               new bucket. */
            b = new ObjectBucket;
            buckets[i] = b;
            LOG_DEBUG("New bucket created");
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
    LOG_ERROR("unable to allocate id");
    return false;
}

void MapContent::deallocate(Actor *obj)
{
    unsigned short id = obj->getPublicID();
    buckets[id / 256]->deallocate(id % 256);
}

void MapContent::fillRegion(MapRegion &r, const Point &p, int radius) const
{
    int ax = p.x > radius ? (p.x - radius) / zoneDiam : 0,
        ay = p.y > radius ? (p.y - radius) / zoneDiam : 0,
        bx = std::min((p.x + radius) / zoneDiam, mapWidth - 1),
        by = std::min((p.y + radius) / zoneDiam, mapHeight - 1);
    for (int y = ay; y <= by; ++y)
    {
        for (int x = ax; x <= bx; ++x)
        {
            addZone(r, x + y * mapWidth);
        }
    }
}

void MapContent::fillRegion(MapRegion &r, const Rectangle &p) const
{
    int ax = p.x / zoneDiam,
        ay = p.y / zoneDiam,
        bx = std::min((p.x + p.w) / zoneDiam, mapWidth - 1),
        by = std::min((p.y + p.h) / zoneDiam, mapHeight - 1);
    for (int y = ay; y <= by; ++y)
    {
        for (int x = ax; x <= bx; ++x)
        {
            addZone(r, x + y * mapWidth);
        }
    }
}

MapZone& MapContent::getZone(const Point &pos) const
{
    return zones[(pos.x / zoneDiam) + (pos.y / zoneDiam) * mapWidth];
}

MapComposite::MapComposite(int id, const std::string &name):
    mMap(NULL),
    mContent(NULL),
    mScript(NULL),
    mName(name),
    mID(id)
{
}

MapComposite::~MapComposite()
{
    delete mMap;
    delete mContent;
    delete mScript;
}

ZoneIterator MapComposite::getAroundPointIterator(const Point &p, int radius) const
{
    MapRegion r;
    mContent->fillRegion(r, p, radius);
    return ZoneIterator(r, mContent);
}

ZoneIterator MapComposite::getAroundActorIterator(Actor *obj, int radius) const
{
    MapRegion r;
    mContent->fillRegion(r, obj->getPosition(), radius);
    return ZoneIterator(r, mContent);
}

ZoneIterator MapComposite::getInsideRectangleIterator(const Rectangle &p) const
{
    MapRegion r;
    mContent->fillRegion(r, p);
    return ZoneIterator(r, mContent);
}

ZoneIterator MapComposite::getAroundBeingIterator(Being *obj, int radius) const
{
    MapRegion r1;
    mContent->fillRegion(r1, obj->getOldPosition(), radius);
    MapRegion r2 = r1;
    for (MapRegion::iterator i = r1.begin(), i_end = r1.end(); i != i_end; ++i)
    {
        /* Fills region with destinations taken around the old position.
           This is necessary to detect two moving objects changing zones at the
           same time and at the border, and going in opposite directions (or
           more simply to detect teleportations, if any). */
        MapRegion &r4 = mContent->zones[*i].destinations;
        if (!r4.empty())
        {
            MapRegion r3;
            r3.reserve(r2.size() + r4.size());
            std::set_union(r2.begin(), r2.end(), r4.begin(), r4.end(),
                           std::back_insert_iterator< MapRegion >(r3));
            r2.swap(r3);
        }
    }
    mContent->fillRegion(r2, obj->getPosition(), radius);
    return ZoneIterator(r2, mContent);
}

bool MapComposite::insert(Thing *ptr)
{
    if (ptr->isVisible())
    {
        if (ptr->canMove() && !mContent->allocate(static_cast< Being * >(ptr)))
        {
            return false;
        }

        Actor *obj = static_cast< Actor * >(ptr);
        mContent->getZone(obj->getPosition()).insert(obj);
    }

    ptr->setMap(this);
    mContent->things.push_back(ptr);
    return true;
}

void MapComposite::remove(Thing *ptr)
{
    for (std::vector<Thing*>::iterator i = mContent->things.begin(),
         i_end = mContent->things.end(); i != i_end; ++i)
    {
        if ((*i)->canFight())
        {
            Being *being = static_cast<Being*>(*i);
            if (being->getTarget() == ptr)
            {
                being->setTarget(NULL);
            }
        }
        if (*i == ptr)
        {
            i = mContent->things.erase(i);
        }
    }

    if (ptr->isVisible())
    {
        Actor *obj = static_cast< Actor * >(ptr);
        mContent->getZone(obj->getPosition()).remove(obj);

        if (ptr->canMove())
        {
            mContent->deallocate(static_cast< Being * >(ptr));
        }
    }
}

void MapComposite::setMap(Map *m)
{
    assert(!mMap && m);
    mMap = m;
    mContent = new MapContent(m);

    std::string sPvP = m->getProperty("pvp");
    if (sPvP == "")
        sPvP = Configuration::getValue("game_defaultPvp", "");

    if (sPvP == "free")
        mPvPRules = PVP_FREE;
    else
        mPvPRules = PVP_NONE;
}

void MapComposite::update()
{
    for (int i = 0; i < mContent->mapHeight * mContent->mapWidth; ++i)
    {
        mContent->zones[i].destinations.clear();
    }

    // Cannot use a WholeMap iterator as objects will change zones under its feet.
    for (std::vector< Thing * >::iterator i = mContent->things.begin(),
         i_end = mContent->things.end(); i != i_end; ++i)
    {
        if (!(*i)->canMove())
            continue;

        Being *obj = static_cast< Being * >(*i);

        const Point &pos1 = obj->getOldPosition(),
                    &pos2 = obj->getPosition();

        MapZone &src = mContent->getZone(pos1),
                &dst = mContent->getZone(pos2);
        if (&src != &dst)
        {
            addZone(src.destinations, &dst - mContent->zones);
            src.remove(obj);
            dst.insert(obj);
        }
    }
}

const std::vector< Thing * > &MapComposite::getEverything() const
{
    return mContent->things;
}
