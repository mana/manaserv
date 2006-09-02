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

MapComposite::MapComposite()
  : map(NULL), last_bucket(0)
{
    buckets[0] = new ObjectBucket;
    for (int i = 1; i < 256; ++i)
    {
        buckets[i] = NULL;
    }
}

MapComposite::~MapComposite()
{
    for (int i = 0; i < 256; ++i)
    {
        delete buckets[i];
    }
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
