/* This file is for testing purpose only. It hardcodes some events related
   to the game. It should be removed once all the related managers have been
   implemented. There are no headers for this file on purpose. */

#include <cassert>

#include "controller.h"
#include "game-server/itemmanager.hpp"
#include "game-server/state.hpp"
#include "game-server/trigger.hpp"

static Rectangle rectA = { 56 * 32, 12 * 32, 5 * 32, 32 };
static WarpAction warpA(3, 44 * 32 + 16, 80 * 32 + 16);
static Rectangle rectB = { 42 * 32, 88 * 32, 5 * 32, 32 };
static WarpAction warpB(1, 58 * 32 + 16, 17 * 32 + 16);

void testingMap(int id)
{
    switch (id)
    {
        case 1:
        {
            gameState->insert(new TriggerArea(1, rectA, &warpA));
            for (int i = 0; i < 10; i++)
            {
                Being *being = new Controlled(OBJECT_MONSTER);
                being->setSpeed(150);
                being->setMapId(1);
                Point pos = { 720, 900 };
                being->setPosition(pos);
                gameState->insert(being);
            }
            ItemClass *ic = itemManager->getItem(508);
            assert(ic);
            Item *i = new Item(ic);
            i->setMapId(1);
            Point pos = { 58 * 32 + 16, 20 * 32 + 16 };
            i->setPosition(pos);
            gameState->insert(i);
        } break;

        case 3:
        {
            gameState->insert(new TriggerArea(3, rectB, &warpB));
        } break;
    }
}
