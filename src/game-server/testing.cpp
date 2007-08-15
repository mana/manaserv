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
    GameState::insert(i);
}

static char const *npc1 =
   "npc2_times = 1\n"
   "function npc_handler(npc, ch)\n"
   "  do_message(npc, ch, \"You know what?\")\n"
   "  do_message(npc, ch, string.format(\"I have already asked this question %d times today.\", npc2_times))\n"
   "  npc2_times = npc2_times + 1\n"
   "end\n";

static char const *npc2 =
  "function npc_handler(npc, ch)\n"
  "  do_message(npc, ch, \"Don't you think the guy behind me is my evil twin?\")\n"
  "end\n";

void testingMap(MapComposite *map)
{
    switch (map->getID())
    {
        case 1:
        {
            // Drop some items.
            dropItem(map, 58 * 32 + 16, 20 * 32 + 16, 508);
            dropItem(map, 58 * 32 + 16, 21 * 32 + 16, 524);

            // Create a Lua context.
            if (Script *s = Script::create("lua"))
            {
                // Load a script.
                s->loadFile("test.lua");

                // Create two NPCs.
                s->loadNPC(107, 53 * 32 + 16, 21 * 32 + 16, npc1);
                s->loadNPC(107, 53 * 32 + 16, 23 * 32 + 16, npc2);

                // Associate the script context to the map.
                map->setScript(s);
                s->setMap(map);
                s->prepare("initialize");
                s->execute();
            }
        } break;
    }
}
