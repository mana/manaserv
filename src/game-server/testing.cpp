/* This file is for testing purpose only. It hardcodes some events related
   to the game. It should be removed once all the related managers have been
   implemented. There are no headers for this file on purpose. */

#include <cassert>

#include "defines.h"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/spawnarea.hpp"
#include "game-server/state.hpp"
#include "game-server/trigger.hpp"

static void dropItem(MapComposite *map, int x, int y, int type)
{
    ItemClass *ic = ItemManager::getItem(type);
    assert(ic);
    Item *i = new Item(ic, 1);
    i->setMap(map);
    Point pos(x, y);
    i->setPosition(pos);
    GameState::insert(i);
}

void testingMap(MapComposite *map)
{
    static Rectangle rectA = { 56 * 32, 12 * 32, 5 * 32, 32 };
    static WarpAction warpA(MapManager::getMap(3), 44 * 32 + 16, 80 * 32 + 16);
    static Rectangle rectB = { 42 * 32, 88 * 32, 5 * 32, 32 };
    static WarpAction warpB(MapManager::getMap(1), 58 * 32 + 16, 17 * 32 + 16);

    switch (map->getID())
    {
        case 1:
        {
            // Create maggot spawn area
            Rectangle maggotSpawnRect = { 720, 900, 320, 320 };
            GameState::insert(new SpawnArea(map, maggotSpawnRect));

            // Portal to map 3
            GameState::insert(new TriggerArea(map, rectA, &warpA));

            // Drop some items
            dropItem(map, 58 * 32 + 16, 20 * 32 + 16, 508);
            dropItem(map, 58 * 32 + 16, 21 * 32 + 16, 524);
        } break;

        case 3:
        {
            // Portal to map 1
            GameState::insert(new TriggerArea(map, rectB, &warpB));
        } break;
    }
}
