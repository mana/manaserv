/* This file is for testing purpose only. It hardcodes some events related
   to the game. It should be removed once all the related managers have been
   implemented. There are no headers for this file on purpose. */

#include <cassert>

#include "defines.h"
#include "game-server/gamehandler.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/npc.hpp"
#include "game-server/state.hpp"
#include "net/messageout.hpp"
#include "scripting/script.hpp"

// For testing purpose only, the NPC class is not meant to be inherited!!
struct DummyNPC: NPC
{
    DummyNPC(): NPC(110, Script::create("lua", "test.lua"))
    {}
};

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
    switch (map->getID())
    {
        case 1:
        {
            // Drop some items
            dropItem(map, 58 * 32 + 16, 20 * 32 + 16, 508);
            dropItem(map, 58 * 32 + 16, 21 * 32 + 16, 524);

            // Add an NPC
            NPC *q = new DummyNPC;
            q->setMap(map);
            q->setPosition(Point(50 * 32 + 16, 19 * 32 + 16));
            GameState::insert(q);
        } break;
    }
}
