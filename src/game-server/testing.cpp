/* This file is for testing purpose only. It hardcodes some events related
   to the game. It should be removed once all the related managers have been
   implemented. There are no headers for this file on purpose. */

#include <cassert>

#include "controller.h"

#include "defines.h"
#include "game-server/itemmanager.hpp"
#include "game-server/state.hpp"
#include "game-server/trigger.hpp"

static Rectangle rectA = { 56 * 32, 12 * 32, 5 * 32, 32 };
static WarpAction warpA(3, 44 * 32 + 16, 80 * 32 + 16);
static Rectangle rectB = { 42 * 32, 88 * 32, 5 * 32, 32 };
static WarpAction warpB(1, 58 * 32 + 16, 17 * 32 + 16);

static void dropItem(int map, int x, int y, int type)
{
    ItemClass *ic = itemManager->getItem(type);
    assert(ic);
    Item *i = new Item(ic, 1);
    i->setMapId(map);
    Point pos(x, y);
    i->setPosition(pos);
    gameState->insert(i);
}

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
                being->setSize(8);

                // some bogus stats for testing
                being->setCompoundAttribute(ATT_HP_MAXIMUM, 42);
                being->setCompoundAttribute(ATT_PHYSICAL_ATTACK_MINIMUM, 1);
                being->setCompoundAttribute(ATT_PHYSICAL_ATTACK_FLUCTUATION, 0);
                being->setCompoundAttribute(ATT_PHYSICAL_DEFENCE, 5);

                being->setHitpoints(42);

                being->setMapId(1);
                Point pos(720, 900);
                being->setPosition(pos);
                gameState->insert(being);
            }
            dropItem(1, 58 * 32 + 16, 20 * 32 + 16, 508);
            dropItem(1, 58 * 32 + 16, 21 * 32 + 16, 524);
        } break;

        case 3:
        {
            gameState->insert(new TriggerArea(3, rectB, &warpB));
        } break;
    }
}
