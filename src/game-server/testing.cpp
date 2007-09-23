/* This file is for testing purpose only. It hardcodes some events related
   to the game. It should be removed once all the related managers have been
   implemented. There are no headers for this file on purpose. */

#include <cassert>

#include "game-server/gamehandler.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/state.hpp"
#include "scripting/script.hpp"

static void dropItem(MapComposite *map, int x, int y, int type)
{
    ItemClass *ic = ItemManager::getItem(type);
    assert(ic);
    Item *i = new Item(ic, 1);
    i->setMap(map);
    Point pos(x, y);
    i->setPosition(pos);
    GameState::insertSafe(i);
}

void testingMap(MapComposite *map)
{
    switch (map->getID())
    {
        case 1:
        {
            // Drop some items.
            dropItem(map, 58 * 32 + 16, 20 * 32 + 16, 508);
            dropItem(map, 58 * 32 + 16, 21 * 32 + 16, 524);
        } break;
    }
}
