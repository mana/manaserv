/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
 *  Copyright (C) 2010  The Mana Developers
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


#include <cassert>

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

#include "common/resourcemanager.h"
#include "game-server/accountconnection.h"
#include "game-server/buysell.h"
#include "game-server/character.h"
#include "game-server/collisiondetection.h"
#include "game-server/effect.h"
#include "game-server/gamehandler.h"
#include "game-server/inventory.h"
#include "game-server/item.h"
#include "game-server/itemmanager.h"
#include "game-server/mapcomposite.h"
#include "game-server/mapmanager.h"
#include "game-server/monster.h"
#include "game-server/monstermanager.h"
#include "game-server/npc.h"
#include "game-server/postman.h"
#include "game-server/quest.h"
#include "game-server/state.h"
#include "game-server/trigger.h"
#include "net/messageout.h"
#include "scripting/luautil.h"
#include "scripting/luascript.h"
#include "utils/logger.h"
#include "utils/speedconv.h"

#include <string.h>
#include <math.h>

/*
 * This file includes all script bindings available to LUA scripts.
 * When you add or change a script binding please document it on
 *
 * http://doc.manasource.org/scripting
 */

/**
 * mana.npc_message(NPC*, Character*, string): void
 * Callback for sending a NPC_MESSAGE.
 */
static int npc_message(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    size_t l;
    const char *m = luaL_checklstring(s, 3, &l);
    if (!p || !q)
    {
        raiseScriptError(s, "npc_message called with incorrect parameters.");
        return 0;
    }
    MessageOut msg(GPMSG_NPC_MESSAGE);
    msg.writeInt16(p->getPublicID());
    msg.writeString(std::string(m), l);
    gameHandler->sendTo(q, msg);
    return 0;
}

/**
 * mana.npc_choice(NPC*, Character*, string...): void
 * Callback for sending a NPC_CHOICE.
 */
static int npc_choice(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q)
    {
        raiseScriptError(s, "npc_choice called with incorrect parameters.");
        return 0;
    }
    MessageOut msg(GPMSG_NPC_CHOICE);
    msg.writeInt16(p->getPublicID());
    for (int i = 3, i_end = lua_gettop(s); i <= i_end; ++i)
    {
        if (lua_isstring(s, i))
        {
            msg.writeString(lua_tostring(s, i));
        }
        else if (lua_istable(s, i))
        {
            lua_pushnil(s);
            while (lua_next(s, i) != 0) {
                if (lua_isstring(s, -1))
                {
                    msg.writeString(lua_tostring(s, -1));
                }
                else
                {
                    raiseScriptError(s, "npc_choice called "
                                     "with incorrect parameters.");
                    return 0;
                }
                lua_pop(s, 1);
            }
        }
        else
        {
            raiseScriptError(s, "npc_choice called with incorrect parameters.");
            return 0;
        }
    }
    gameHandler->sendTo(q, msg);
    return 0;
}

/**
 * mana.npc_integer(NPC*, Character*, int min, int max, int defaut = min): void
 * Callback for sending a NPC_INTEGER.
 */
static int npc_ask_integer(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q)
    {
        raiseScriptError(s, "npc_ask_integer called "
                         "with incorrect parameters.");
        return 0;
    }
    MessageOut msg(GPMSG_NPC_NUMBER);
    msg.writeInt16(p->getPublicID());

    int min = lua_tointeger(s, 3);
    int max = lua_tointeger(s, 4);
    int default_num = min;
    if (lua_gettop(s) == 5)
        default_num = lua_tointeger(s, 5);

    msg.writeInt32(min);
    msg.writeInt32(max);
    msg.writeInt32(default_num);
    gameHandler->sendTo(q, msg);

    return 0;
}

/**
 * mana.npc_ask_string(NPC*, Character*): void
 * Callback for sending a NPC_STRING.
 */
static int npc_ask_string(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q)
    {
        raiseScriptError(s, "npc_ask_string called with incorrect parameters.");
        return 0;
    }
    MessageOut msg(GPMSG_NPC_STRING);
    msg.writeInt16(p->getPublicID());
    gameHandler->sendTo(q, msg);

    return 0;
}

/**
 * mana.npc_create(string name, int id, int x, int y): NPC*
 * Callback for creating a NPC on the current map with the current script.
 */
static int npc_create(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    const int id = luaL_checkint(s, 2);
    const int x = luaL_checkint(s, 3);
    const int y = luaL_checkint(s, 4);

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    NPC *q = new NPC(name, id, t);
    MapComposite *m = t->getMap();
    if (!m)
    {
        raiseScriptError(s, "npc_create called outside a map.");
        return 0;
    }
    q->setMap(m);
    q->setPosition(Point(x, y));
    GameState::enqueueInsert(q);
    lua_pushlightuserdata(s, q);
    return 1;
}

/**
 * mana.npc_end(NPC*, Character*): void
 * Callback for ending a NPC conversation with the given character.
 */
static int npc_end(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q)
    {
        raiseScriptError(s, "npc_end called with incorrect parameters.");
        return 0;
    }

    MessageOut msg(GPMSG_NPC_CLOSE);
    msg.writeInt16(p->getPublicID());
    gameHandler->sendTo(q, msg);
    return 0;
}

/**
 * mana.npc_post(NPC*, Character*): void
 * Callback for sending a NPC_POST.
 */
static int npc_post(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);

    if (!p || !q)
    {
        raiseScriptError(s, "npc_post called with incorrect parameters.");
        return 0;
    }

    MessageOut msg(GPMSG_NPC_POST);
    msg.writeInt16(p->getPublicID());
    gameHandler->sendTo(q, msg);

    return 0;
}

/**
 * mana.npc_enable(NPC*): void
 * Enable a NPC if it has previously disabled
 */
static int npc_enable(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    if (p)
    {
        p->enable(true);
        GameState::enqueueInsert(p);
    }

    return 0;
}

/**
 * mana.npc_disable(NPC*): void
 * Disable a NPC.
 */
static int npc_disable(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    if (p)
    {
        p->enable(false);
        GameState::remove(p);
    }

    return 0;
}

/**
 * mana.chr_warp(Character*, nil/int map, int x, int y): void
 * Callback for warping a player to another place.
 */
static int chr_warp(lua_State *s)
{
    int x = luaL_checkint(s, 3);
    int y = luaL_checkint(s, 4);

    Character *q = getCharacter(s, 1);
    bool b = lua_isnil(s, 2);
    if (!q || !(b || lua_isnumber(s, 2) || lua_isstring(s, 2)))
    {
        raiseScriptError(s, "chr_warp called with incorrect parameters.");
        return 0;
    }
    MapComposite *m;
    if (b)
    {
        lua_pushlightuserdata(s, (void *)&registryKey);
        lua_gettable(s, LUA_REGISTRYINDEX);
        Script *t = static_cast<Script *>(lua_touserdata(s, -1));
        m = t->getMap();
    }
    else if (lua_isnumber(s, 2))
    {
        m = MapManager::getMap(lua_tointeger(s, 2));
    }
    else
    {
        m = MapManager::getMap(lua_tostring(s, 2));
    }
    if (!m)
    {
        raiseScriptError(s, "chr_warp called with a non-existing map.");
        return 0;
    }

    Map *map = m->getMap();

    // If the wanted warp place is unwalkable
    if (!map->getWalk(x / map->getTileWidth(), y / map->getTileHeight()))
    {
        int c = 50;
        LOG_INFO("chr_warp called with a non-walkable place.");
        do
        {
            x = rand() % map->getWidth();
            y = rand() % map->getHeight();
        } while (!map->getWalk(x, y) && --c);
        x *= map->getTileWidth();
        y *= map->getTileHeight();
    }
    GameState::enqueueWarp(q, m, x, y);

    return 0;
}

/**
 * mana.chr_inv_change(Character*, (int id || string name,
 *                     int nb)...): bool success
 * Callback for inserting/removing items in inventory.
 * The function can be called several times in a row, but it is better to
 * perform all the changes at once, so as to reduce bandwidth. Removals
 * (negative amount) should be passed first, then insertions (positive amount).
 * If a removal fails, all the previous operations are canceled (except for
 * items dropped on the floor, hence why removals should be passed first), and
 * the function returns false. Otherwise the function will return true.
 * Note that previously when the item identifier was zero, money was modified;
 * however currency is now handled through attributes. This breaks backwards
 * compatibility with old scripts, and so logs a warning.
 * Note: If an insertion fails, extra items are dropped on the floor.
 */
static int chr_inv_change(lua_State *s)
{
    Character *q = getCharacter(s, 1);
    if (!q)
    {
        raiseScriptError(s, "chr_inv_change called with incorrect parameters.");
        return 0;
    }
    int nb_items = (lua_gettop(s) - 1) / 2;
    Inventory inv(q);
    for (int i = 0; i < nb_items; ++i)
    {
        if (!(lua_isnumber(s, i * 2 + 2) || lua_isstring(s, i * 2 + 2)) ||
            !lua_isnumber(s, i * 2 + 3))
        {
            raiseScriptError(s, "chr_inv_change called with "
                             "incorrect parameters.");
            return 0;
        }

        int nb = lua_tointeger(s, i * 2 + 3);
        ItemClass *ic;
        int id;
        if (lua_isnumber(s, i * 2 + 2))
        {
            int id = lua_tointeger(s, i * 2 + 2);
            if (id == 0)
            {
                LOG_WARN("chr_inv_change called with id 0! "
                         "Currency is now handled through attributes!");
                continue;
            }
            ic = itemManager->getItem(id);
        }
        else
        {
            ic = itemManager->getItemByName(lua_tostring(s, i * 2 + 2));
        }

        if (!ic)
        {
            raiseScriptError(s, "chr_inv_change called with an unknown item.");
            continue;
        }
        id = ic->getDatabaseID();
        if (nb < 0)
        {
            // Removing too much item is a success as for the scripter's
            // point of view. We log it anyway.
            nb = inv.remove(id, -nb);
            if (nb)
            {
                LOG_WARN("mana.chr_inv_change() removed more items than owned: "
                     << "character: " << q->getName() << " item id: " << id);
            }
        }
        else
        {
            nb = inv.insert(id, nb);
            if (nb)
            {
                Item *item = new Item(ic, nb);
                item->setMap(q->getMap());
                item->setPosition(q->getPosition());
                GameState::enqueueInsert(item);
            }
        }
    }
    lua_pushboolean(s, 1);
    return 1;
}

/**
 * mana.chr_inv_count(Character*, int item_id...): int count...
 * Callback for counting items in inventory.
 */
static int chr_inv_count(lua_State *s)
{
    Character *q = getCharacter(s, 1);
    if (!q)
    {
        raiseScriptError(s, "chr_inv_count called with incorrect parameters.");
        return 0;
    }
    int nb_items = lua_gettop(s) - 1;
    lua_checkstack(s, nb_items);
    Inventory inv(q);

    int id, nb = 0;
    for (int i = 2; i <= nb_items + 1; ++i)
    {
        ItemClass *it;
        if (lua_isnumber(s, i))
            it = itemManager->getItem(lua_tointeger(s, i));
        else
            it = itemManager->getItemByName(lua_tostring(s, i));

        if (!it)
        {
            raiseScriptError(s, "chr_inv_count called with invalid "
                             "item id or name.");
            return 0;
        }
        id = it->getDatabaseID();
        if (id == 0)
        {
            LOG_WARN("chr_inv_count called with id 0! "
                     "Currency is now handled through attributes!");
        }
        else
        {
            nb = inv.count(id);
            lua_pushinteger(s, nb);
        }
    }
    return nb_items;
}

/**
 * mana.chr_get_level(Character*): int level
 * Tells the character current level.
 */
static int chr_get_level(lua_State *s)
{
    Character *ch = getCharacter(s, 1);
    if (!ch)
    {
        raiseScriptError(s, "chr_get_level "
                         "called for nonexistent character.");
    }

    lua_pushinteger(s, ch->getLevel());
    return 1;
}

/**
 * mana.npc_trade(NPC*, Character*, bool sell, table items): int
 * Callback for trading between a player and an NPC.
 * Let the player buy or sell only a subset of predeterminded items.
 * @param table items: a subset of buyable/sellable items.
 * When selling, if the 4 parameter is omitted or invalid,
 * everything in the player inventory can be sold.
 * @return 0 if something to buy/sell, 1 if no items, 2 in case of errors.
 */
static int npc_trade(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q || !lua_isboolean(s, 3))
    {
        raiseWarning(s, "npc_trade called with incorrect parameters.");
        lua_pushinteger(s, 2); // return value
        return 1; // Returns 1 parameter
    }

    bool sellMode = lua_toboolean(s, 3);
    BuySell *t = new BuySell(q, sellMode);
    if (!lua_istable(s, 4))
    {
        if (sellMode)
        {
            // Can sell everything
            if (!t->registerPlayerItems())
            {
                // No items to sell in player inventory
                t->cancel();
                lua_pushinteger(s, 1);
                return 1;
            }

            if (t->start(p))
            {
              lua_pushinteger(s, 0);
              return 1;
            }
            else
            {
              lua_pushinteger(s, 1);
              return 1;
            }
        }
        else
        {
            raiseWarning(s, "npc_trade[Buy] called with invalid "
                         "or empty items table parameter.");
            t->cancel();
            lua_pushinteger(s, 2);
            return 1;
        }
    }

    int nbItems = 0;

    lua_pushnil(s);
    while (lua_next(s, 4))
    {
        if (!lua_istable(s, -1))
        {
            raiseWarning(s, "npc_trade called with invalid "
                         "or empty items table parameter.");
            t->cancel();
            lua_pushinteger(s, 2);
            return 1;
        }

        int v[3];
        for (int i = 0; i < 3; ++i)
        {
            lua_rawgeti(s, -1, i + 1);
            if (i == 0) // item id or name
            {
                ItemClass *it;
                if (lua_isnumber(s, -1))
                    it = itemManager->getItem(lua_tointeger(s, -1));
                else
                    it = itemManager->getItemByName(lua_tostring(s, -1));

                if (!it)
                {
                    raiseWarning(s, "npc_trade called with incorrect "
                                 "item id or name.");
                    t->cancel();
                    lua_pushinteger(s, 2);
                    return 1;
                }
                v[0] = it->getDatabaseID();
            }
            else if (!lua_isnumber(s, -1))
            {
                raiseWarning(s, "npc_trade called with incorrect parameters "
                             "in item table.");
                t->cancel();
                lua_pushinteger(s, 2);
                return 1;
            }
            else
            {
                v[i] = lua_tointeger(s, -1);
            }
            lua_pop(s, 1);
        }
        if (t->registerItem(v[0], v[1], v[2]))
            nbItems++;
        lua_pop(s, 1);
    }

    if (nbItems == 0)
    {
        t->cancel();
        lua_pushinteger(s, 1);
        return 1;
    }
    if (t->start(p))
    {
      lua_pushinteger(s, 0);
      return 1;
    }
    else
    {
      lua_pushinteger(s, 1);
      return 1;
    }
}

/**
 * mana.being_apply_status(Being*, int id, int time): void
 * Applies a status effect with id to the being given for a amount of time.
 */
static int being_apply_status(lua_State *s)
{
    const int id = luaL_checkint(s, 2);
    const int time = luaL_checkint(s, 3);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_apply_status called "
                         "with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    being->applyStatusEffect(id, time);
    return 0;
}

/**
 * mana.being_remove_status(Being*, int id): void
 * Removes the given status effect.
 */
static int being_remove_status(lua_State *s)
{
    const int id = luaL_checkint(s, 2);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_remove_status called "
                         "with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    being->removeStatusEffect(id);
    return 0;
}

/**
 * mana.being_has_status(Being*, int id): bool
 * Returns whether a being has the given status effect.
 */
static int being_has_status(lua_State *s)
{
    const int id = luaL_checkint(s, 2);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_has_status called "
                         "with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    lua_pushboolean(s, being->hasStatusEffect(id));
    return 1;
}

/**
 * mana.being_get_status_time(Being*, int id): int
 * Returns the time left on the given status effect.
 */
static int being_get_status_time(lua_State *s)
{
    const int id = luaL_checkint(s, 2);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_get_status_time called "
                         "with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    lua_pushinteger(s, being->getStatusEffectTime(id));
    return 1;
}

/**
 * mana.being_set_status_time(Being*, int id, int time): void
 * Sets the time left on the given status effect.
 */
static int being_set_status_time(lua_State *s)
{
    const int id = luaL_checkint(s, 2);
    const int time = luaL_checkint(s, 3);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_set_status_time called "
                         "with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    being->setStatusEffectTime(id, time);
    return 0;
}

/**
 * mana.being_type(Being*): ThingType
 * Returns the Thing type of the given being.
 */
static int being_type(lua_State *s)
{
    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_type called with incorrect parameters.");
        return 0;
    }

    Being *being = getBeing(s, 1);
    if (!being)
        return 0;
    lua_pushinteger(s, being->getType());
    return 1;
}


/**
 * being_walk(Being *, int x, int y[, float speed]): void
 * Function for making a being walk to a position.
 * The speed is in tile per second.
 */
static int being_walk(lua_State *s)
{
    const int x = luaL_checkint(s, 2);
    const int y = luaL_checkint(s, 3);

    Being *being = getBeing(s, 1);
    being->setDestination(Point(x, y));

    if (lua_isnumber(s, 4))
    {
        being->setAttribute(ATTR_MOVE_SPEED_TPS, lua_tonumber(s, 4));
        being->setAttribute(ATTR_MOVE_SPEED_RAW, utils::tpsToRawSpeed(
                being->getModifiedAttribute(ATTR_MOVE_SPEED_TPS)));
    }

    return 0;
}

/**
 * mana.being_say(Being* source, string message): void
 * Makes the being say something.
 */
static int being_say(lua_State *s)
{
    const char *message = luaL_checkstring(s, 2);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_say called with incorrect parameters.");
        return 0;
    }

    Being *being = getBeing(s, 1);

    if (being && message[0] != 0)
    {
        GameState::sayAround(being, message);
    }
    else
    {
        raiseScriptError(s, "being_say called with incorrect parameters.");
        return 0;
    }

    return 0;
}


/**
 * mana.being_damage(Being* victim, int value, int delta, int cth, int type,
 *                   int element): void
 * Applies combat damage to a being.
 */
static int being_damage(lua_State *s)
{
    Being *being = getBeing(s, 1);

    if (!being->canFight())
        return 0;

    Damage dmg((unsigned short) lua_tointeger(s, 2), /* base */
               (unsigned short) lua_tointeger(s, 3), /* delta */
               (unsigned short) lua_tointeger(s, 4), /* cth */
               (unsigned char)  lua_tointeger(s, 6), /* element */
               DAMAGE_PHYSICAL);                     /* type */
    being->damage(NULL, dmg);

    return 0;
}

/**
 * mana.being_heal(Being* [, int value]): void
 * Restores hit points of a being.
 * Without a value the being is fully healed.
 */
static int being_heal(lua_State *s)
{
    Being *being = getBeing(s, 1);
    if (!being)
    {
        raiseScriptError(s,
            "being_heal called for nonexistent being.");
        return 0;
    }

    if (lua_gettop(s) == 1) // when there is only one argument
    {
        being->heal();
    }
    else if (lua_isnumber(s, 2))
    {
        being->heal(lua_tointeger(s, 2));
    }
    else
    {
        raiseScriptError(s,
            "being_heal called with illegal healing value.");
    }

    return 0;
}

/**
 * mana.being_get_base_attribute(Being*, int attribute): int
 * Gets the base attribute of a being.
 */
static int being_get_base_attribute(lua_State *s)
{
    Being *being = getBeing(s, 1);

    if (being)
    {
        lua_Integer attr = lua_tointeger(s, 2);
        if (attr)
        {
            lua_pushinteger(s, being->getAttribute(attr));
            return 1;
        }
        raiseScriptError(s,
            "being_get_base_attribute called with incorrect parameters.");
    }

    return 0;
}

/**
 * mana.being_get_modified_attribute(Being*, int attribute): int
 * Gets the modified attribute of a being.
 */
static int being_get_modified_attribute(lua_State *s)
{
    Being *being = getBeing(s, 1);

    if (being)
    {
        lua_Integer attr = lua_tointeger(s, 2);
        if (attr)
        {
            lua_pushinteger(s, being->getModifiedAttribute(attr));
            return 1;
        }
        raiseScriptError(s,
            "being_get_modified_attribute called with incorrect parameters.");
    }

    return 0;
}

/**
 * mana.being_set_base_attribute(Being*, int attribute, double value): void
 * Sets the base attribute of a being.
 */
static int being_set_base_attribute(lua_State *s)
{
    Being *being = getBeing(s, 1);
    if (!being)
        return 0;

    lua_Integer attr = lua_tointeger(s, 2);
    lua_Number value = lua_tonumber(s, 3);

    being->setAttribute(attr, value);

    return 0;
}

/**
 * mana.being_apply_attribute_modifier(Being*, int attribute, double value,
 *     int layer, int [duration, int [effect-id]]): void
 * Applies an attribute modifier to a being.
 */
static int being_apply_attribute_modifier(lua_State *s)
{
    Being *being = getBeing(s, 1);
    if (!being)
        return 0;

    lua_Integer attr  = lua_tointeger(s,2);
    lua_Number value  = lua_tonumber(s, 3);
    lua_Integer layer = lua_tonumber(s, 4);
    lua_Integer duration = 0;
    lua_Integer effectId = 0;
    if (lua_isnumber(s, 5))
    {
        duration = lua_tointeger(s, 5);
        if (lua_isnumber(s, 6))
            effectId = lua_tointeger(s, 6);
    }

    being->applyModifier(attr, value, layer, duration, effectId);

    return 0;
}

/**
 * mana.being_remove_attribute_modifier(Being*, int attribute, double value,
 *     int layer, int [effect-id]]): void
 * Removes an attribute modifier to a being.
 */
static int being_remove_attribute_modifier(lua_State *s)
{
    Being *being = getBeing(s, 1);
    if (!being)
        return 0;

    lua_Integer attr  = lua_tointeger(s,2);
    lua_Number value  = lua_tonumber(s, 3);
    lua_Integer layer = lua_tonumber(s, 4);
    lua_Integer effectId = 0;
    if (lua_isnumber(s, 5))
        effectId = lua_tointeger(s, 5);

    being->removeModifier(attr, value, layer, effectId);

    return 0;
}

/**
 * mana.being_get_name(Being*): string
 * Gets the being's name.
 */
static int being_get_name(lua_State *s)
{
    Being *being = getBeing(s, 1);

    if (being)
    {
        lua_pushstring(s, being->getName().c_str());
    }

    return 1;
}

/**
 * mana.being_get_action(Being*): BeingAction
 * Gets the being's current action.
 */
static int being_get_action(lua_State *s)
{
    Being *being = getBeing(s, 1);

    if (being)
    {
        lua_pushinteger(s, being->getAction());
    }

    return 1;
}

/**
 * mana.being_set_action(Being*, BeingAction): void
 * Sets the being's current action.
 */
static int being_set_action(lua_State *s)
{
    Being *being = getBeing(s, 1);

    int act = lua_tointeger(s, 2);

    if (being)
    {
        being->setAction((BeingAction) act);
    }

    return 0;
}

/**
 * mana.being_get_direction(Being*): BeingDirection
 * Gets the being's current direction.
 */
static int being_get_direction(lua_State *s)
{
    Being *being = getBeing(s, 1);

    if (being)
    {
        lua_pushinteger(s, being->getDirection());
    }

    return 1;
}

/**
 * mana.being_set_direction(Being*, BeingDirection): void
 * Sets the being's current direction.
 */
static int being_set_direction(lua_State *s)
{
    Being *being = getBeing(s, 1);

    BeingDirection dir = (BeingDirection) lua_tointeger(s, 2);

    if (being)
    {
        being->setDirection(dir);
    }

    return 0;
}

/**
 * mana.posX(Being*): int xcoord
 * Function for getting the x-coordinate of the position of a being.
 */
static int posX(lua_State *s)
{
    int x = getBeing(s, 1)->getPosition().x;
    lua_pushinteger(s, x);

    return 1;
}

/**
 * mana.posY(Being*): int ycoord
 * Function for getting the y-coordinate of the position of a being.
 */
static int posY(lua_State *s)
{
    int y = getBeing(s, 1)->getPosition().y;
    lua_pushinteger(s, y);

    return 1;
}

/**
 * mana.monster_create(int id || string name, int x, int y): Monster*
 * Callback for creating a monster on the current map.
 */
static int monster_create(lua_State *s)
{
    const int x = luaL_checkint(s, 2);
    const int y = luaL_checkint(s, 3);

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    MapComposite *m = t->getMap();
    if (!m)
    {
        raiseScriptError(s, "monster_create called outside a map.");
        return 0;
    }

    MonsterClass *spec;
    if (lua_isnumber(s, 1))
    {
        int monsterId = luaL_checkint(s, 1);
        spec = monsterManager->getMonster(monsterId);
        if (!spec)
        {
            raiseScriptError(s, "monster_create called with invalid "
                             "monster ID: %d", monsterId);
            //LOG_WARN("LuaMonster_Create invalid monster ID: " << monsterId);
            return 0;
        }
    }
    else
    {
        std::string monsterName = lua_tostring(s, 1);
        spec = monsterManager->getMonsterByName(monsterName);
        if (!spec)
        {
            raiseScriptError(s, "monster_create called with "
                             "invalid monster name: %s", monsterName.c_str());
            //LOG_WARN("LuaMonster_Create invalid monster name: "
            // << monsterName);
            return 0;
         }
    }

    Monster *q = new Monster(spec);
    q->setMap(m);
    q->setPosition(Point(x, y));
    GameState::enqueueInsert(q);

    lua_pushlightuserdata(s, q);
    return 1;
}

/**
 * mana.monster_get_name(int monster_id): string monster_name
 * Returns the name of the monster with the given id.
 */
static int monster_get_name(lua_State *s)
{
    const int id = luaL_checkint(s, 1);
    MonsterClass *spec = monsterManager->getMonster(id);
    if (!spec)
    {
        raiseScriptError(s, "monster_get_name "
                         "called with unknown monster id.");
        return 0;
    }
    lua_pushstring(s, spec->getName().c_str());
    return 1;
}

/**
 * mana.monster_change_anger(Monster*, Being*, int anger)
 * Makes a monster angry at a being
 */
static int monster_change_anger(lua_State *s)
{
    const int anger = luaL_checkint(s, 3);
    Monster *m = getMonster(s, 1);
    if (!m)
    {
        raiseScriptError(s, "monster_change_anger called "
                         "for a nonexisting monster");
        return 0;
    }
    Being *being = getBeing(s, 2);
    if (!being)
    {
        raiseScriptError(s, "monster_change_anger called "
                         "for a nonexisting being");
    }
    m->changeAnger(being, anger);
    return 0;
}

/**
 * mana.monster_remove(Monster*): bool success
 * Remove a monster object without kill event.
 * return whether the monster was enqueued for removal.
 */
static int monster_remove(lua_State *s)
{
    bool monsterRemoved = false;
    Monster *m = getMonster(s, 1);
    if (m)
    {
        GameState::remove(m);
        monsterRemoved = true;
    }
    lua_pushboolean(s, monsterRemoved);
    return 1;
}

/**
 * mana.monster_load_script(Monster*, string script_filename): void
 * loads a LUA script for the given monster.
 */
static int monster_load_script(lua_State *s)
{
    Monster *m = getMonster(s, 1);
    if (!m)
    {
         raiseScriptError(s, "monster_load_script called "
                          "for a nonexistent monster.");
         return 0;
    }

    const char *scriptName = luaL_checkstring(s, 2);
    if (scriptName[0] == 0)
    {
        raiseScriptError(s, "monster_load_script called "
                         "with empty script file name.");
        return 0;
    }

    m->loadScript(scriptName);
    return 0;
}

/**
 * mana.chr_get_chest(Character*, string): nil or string
 * Callback for getting a quest variable. Starts a recovery and returns
 * immediatly, if the variable is not known yet.
 */
static int chr_get_quest(lua_State *s)
{
    Character *q = getCharacter(s, 1);
    if (!q)
    {
        raiseScriptError(s, "chr_get_quest "
                         "called for nonexistent character.");
    }

    const char *m = luaL_checkstring(s, 2);
    if (m[0] == 0)
    {
        raiseScriptError(s, "chr_get_quest called with empty string.");
        return 0;
    }
    std::string value, name = m;
    bool res = getQuestVar(q, name, value);
    if (res)
    {
        lua_pushstring(s, value.c_str());
        return 1;
    }
    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    QuestCallback f = { &LuaScript::getQuestCallback, t };
    recoverQuestVar(q, name, f);
    return 0;
}

/**
 * mana.getvar_map(string): string
 * Gets the value of a persistent map variable.
 */
static int getvar_map(lua_State *s)
{
    const char *m = luaL_checkstring(s, 1);
    if (m[0] == 0)
    {
        raiseScriptError(s, "getvar_map called for unnamed variable.");
        return 0;
    }

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *script = static_cast<Script *>(lua_touserdata(s, -1));
    std::string value = script->getMap()->getVariable(m);

    lua_pushstring(s, value.c_str());
    return 1;
}

/**
 * mana.setvar_map(string, string): void
 * Sets the value of a persistent map variable.
 */
static int setvar_map(lua_State *s)
{
    const char *m = luaL_checkstring(s, 1);
    if (m[0] == 0)
    {
        raiseScriptError(s, "setvar_map called for unnamed variable.");
        return 0;
    }

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *script = static_cast<Script *>(lua_touserdata(s, -1));
    std::string key = lua_tostring(s, 1);
    std::string value = lua_tostring(s, 2);
    script->getMap()->setVariable(key, value);

    return 0;
}

/**
 * mana.getvar_world(string): string
 * Gets the value of a persistent global variable.
 */
static int getvar_world(lua_State *s)
{
    const char *m = luaL_checkstring(s, 1);
    if (m[0] == 0)
    {
        raiseScriptError(s, "getvar_world called for unnamed variable.");
        return 0;
    }

    std::string value = GameState::getVariable(m);

    lua_pushstring(s, value.c_str());
    return 1;
}

/**
 * mana.setvar_world(string, string): void
 * Sets the value of a persistent global variable.
 */
static int setvar_world(lua_State *s)
{
    const char *m = luaL_checkstring(s, 1);
    if (m[0] == 0)
    {
        raiseScriptError(s, "setvar_world called with unnamed variable.");
        return 0;
    }

    std::string key = lua_tostring(s, 1);
    std::string value = lua_tostring(s, 2);
    GameState::setVariable(key, value);

    return 0;
}

/**
 * mana.chr_set_chest(Character*, string, string): void
 * Callback for setting a quest variable.
 */
static int chr_set_quest(lua_State *s)
{
    Character *q = getCharacter(s, 1);
    const char *m = luaL_checkstring(s, 2);
    const char *n = luaL_checkstring(s, 3);
    if (m[0] == 0 || strlen(m) == 0)
    {
        raiseScriptError(s, "chr_set_quest called with incorrect parameters.");
        return 0;
    }
    if (!q)
    {
        raiseScriptError(s, "chr_set_quest called for nonexistent character.");
        return 0;
    }
    setQuestVar(q, m, n);
    return 0;
}

/**
 * mana.trigger_create(int x, int y, int width, int height,
 *                     string function, int id)
 * Creates a trigger area. Whenever an actor enters this area, a Lua function
 * is called.
 */
static int trigger_create(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    const int width = luaL_checkint(s, 3);
    const int height = luaL_checkint(s, 4);
    //TODO: Turn the function string to a lua function pointer
    const char *function = luaL_checkstring(s, 5);
    const int id = luaL_checkint(s, 6);

    if (!lua_isboolean(s, 7))
    {
        raiseScriptError(s, "trigger_create called with incorrect parameters.");
        return 0;
    }

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *script = static_cast<Script *>(lua_touserdata(s, -1));
    bool once = lua_toboolean(s, 7);

    LOG_INFO("Created script trigger at " << x << ":" << y
             << " (" << width << "x" << height << ") function: " << function
             << " (" << id << ")");

    MapComposite *m = script->getMap();

    if (!m)
    {
        raiseScriptError(s, "trigger_create called for nonexistent a map.");
        return 0;
    }

    ScriptAction *action = new ScriptAction(script, function, id);
    Rectangle r = { x, y, width, height };
    TriggerArea *area = new TriggerArea(m, r, action, once);

    bool ret = GameState::insert(area);
    lua_pushboolean(s, ret);
    return 1;
}

/**
 * private message: mana.chat_message(Being* recipent, string message): void
 * @todo global message: mana.chat_message(string message): void
 * Creates a chat message in the users chatlog(s).
 */
static int chat_message(lua_State *s)
{
    if (lua_gettop(s) == 2 && lua_isuserdata(s, 1) && lua_isstring(s, 2) )
    {
        Being *being = getBeing(s, 1);
        const std::string message = lua_tostring(s, 2);

        if (being && !message.empty())
        {
            GameState::sayTo(being, NULL, message);
        }
    }
    else if (lua_gettop(s) == 1 && lua_isstring(s, 1))
    {
        // TODO: make chatserver send a global message
    }
    else
    {
        raiseScriptError(s, "chat_message called with incorrect parameters.");
        return 0;
    }

    return 0;
}

/**
 * mana.get_beings_in_circle(int x, int y, int radius): table of Being*
 * mana.get_beings_in_circle(handle centerBeing, int radius): table of Being*
 * Gets a LUA table with the Being* pointers of all beings
 * inside of a circular area of the current map.
 */
static int get_beings_in_circle(lua_State *s)
{
    int x, y, r;
    if (lua_islightuserdata(s, 1))
    {
        Being *b = getBeing(s, 1);
        const Point &pos = b->getPosition();
        x = pos.x;
        y = pos.y;
        r = luaL_checkint(s, 2);
    }
    else
    {
        x = luaL_checkint(s, 1);
        y = luaL_checkint(s, 2);
        r = luaL_checkint(s, 3);
    }

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    MapComposite *m = t->getMap();

    //create a lua table with the beings in the given area.
    lua_newtable(s);
    int tableStackPosition = lua_gettop(s);
    int tableIndex = 1;
    for (BeingIterator i(m->getAroundPointIterator(Point(x, y), r)); i; ++i)
    {
        char t = (*i)->getType();
        if (t == OBJECT_NPC || t == OBJECT_CHARACTER || t == OBJECT_MONSTER)
        {
            Being *b = static_cast<Being *> (*i);
            if (Collision::circleWithCircle(b->getPosition(), b->getSize(),
                                            Point(x, y), r))
            {
                lua_pushinteger(s, tableIndex);
                lua_pushlightuserdata (s, b);
                lua_settable (s, tableStackPosition);
                tableIndex++;
            }
        }
    }

    return 1;
}

/**
 * mana.get_beings_in_rectangle(int x, int y, int width,
 *                              int height): table of Being*
 * Gets a LUA table with the Being* pointers of all beings
 * inside of a rectangular area of the current map.
 */
static int get_beings_in_rectangle(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    const int w = luaL_checkint(s, 3);
    const int h = luaL_checkint(s, 4);

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    MapComposite *m = t->getMap();

    //create a lua table with the beings in the given area.
    lua_newtable(s);
    int tableStackPosition = lua_gettop(s);
    int tableIndex = 1;
    Rectangle rect = {x, y ,w, h};
    for (BeingIterator i(
         m->getInsideRectangleIterator(rect)); i; ++i)
    {
        char t = (*i)->getType();
        if (t == OBJECT_NPC || t == OBJECT_CHARACTER || t == OBJECT_MONSTER)
        {
            Being *b = static_cast<Being *> (*i);
            lua_pushinteger(s, tableIndex);
            lua_pushlightuserdata (s, b);
            lua_settable (s, tableStackPosition);
            tableIndex++;
        }
    }
     return 1;
 }

/**
 * mana.chr_get_post(Character*): void
 * Gets the post for the character.
 */
static int chr_get_post(lua_State *s)
{
    if (lua_isuserdata(s, 1))
    {
        Character *c = getCharacter(s, 1);

        if (c)
        {
            lua_pushlightuserdata(s, (void *)&registryKey);
            lua_gettable(s, LUA_REGISTRYINDEX);
            Script *t = static_cast<Script *>(lua_touserdata(s, -1));
            PostCallback f = { &LuaScript::getPostCallback, t };
            postMan->getPost(c, f);
        }
    }

    return 0;
}

/**
 * mana.being_register(Being*): void
 * Makes the server call the lua functions deathEvent
 * and removeEvent when the being dies or is removed
 * from the map.
 */
static int being_register(lua_State *s)
{
    if (!lua_islightuserdata(s, 1) || lua_gettop(s) != 1)
    {
        raiseScriptError(s, "being_register called with incorrect parameters.");
        return 0;
    }

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    Being *being = getBeing(s, 1);
    if (!being)
    {
        raiseScriptError(s, "being_register called for nonexistent being.");
        return 0;
    }

    being->addListener(t->getScriptListener());
    return 0;
}

/**
 * mana.effect_create (int id, int x, int y): void
 * mana.effect_create (int id, Being*): void
 * Triggers a special effect from the clients effects.xml.
 */
static int effect_create(lua_State *s)
{
    const int id = luaL_checkint(s, 1);

    if (((!lua_isnumber(s, 2) || !lua_isnumber(s, 3))
         && (!lua_isuserdata(s, 2))))
    {
        raiseScriptError(s, "effect_create called with incorrect parameters.");
        return 0;
    }
    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));

    MapComposite *m = t->getMap();

    if (lua_isuserdata(s, 2))
    {
        // being mode
        Being *b = getBeing(s, 2);
        if (!b)
        {
            raiseScriptError(s, "effect_create called on non-existent being");
            return 0;
        }
        Effects::show(id, m, b);
    }
    else
    {
        // positional mode
        int x = lua_tointeger(s, 2);
        int y = lua_tointeger(s, 3);
        Effects::show(id, m, Point(x, y));
    }

    return 0;
}

/**
 * mana.chr_shake_screen(Character*, int x, int y[, float strength,
 *                       int radius]): void
 * Shake the screen for a given character.
 */
static int chr_shake_screen(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_shake_screen called "
                         "for nonexistent character.");
        return 0;
    }

    const int x = luaL_checkint(s, 2);
    const int y = luaL_checkint(s, 3);

    MessageOut msg(GPMSG_SHAKE);
    msg.writeInt16(x);
    msg.writeInt16(y);

    if (lua_isnumber(s, 4))
        msg.writeInt16((int) (lua_tonumber(s, 4) * 10000));
    if (lua_isnumber(s, 5))
        msg.writeInt16(lua_tointeger(s, 5));

    c->getClient()->send(msg);

    return 0;
}


/**
 * mana.chr_get_exp(Character*, int skill): int
 * Gets the exp total in a skill of a specific character
 */
static int chr_get_exp(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_exp called for nonexistent character.");
        return 0;
    }

    const int skill = luaL_checkint(s, 2);
    const int exp = c->getExperience(skill);

    lua_pushinteger(s, exp);
    return 1;
}

/**
 * mana.chr_give_exp(Character*, int skill,
 *                   int amount[, int optimal_level]): void
 * Gives the character a certain amount of experience points
 * in a skill. Can also be used to reduce the exp amount when
 * desired.
 */
static int chr_give_exp(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_give_exp called for nonexistent character.");
        return 0;
    }

    const int skill = luaL_checkint(s, 2);
    const int exp = luaL_checkint(s, 3);
    const int optimalLevel = luaL_optint(s, 4, 0);

    c->receiveExperience(skill, exp, optimalLevel);

    return 0;
}

/**
 * mana.chr_set_hair_style(Character*, int style_id): void
 * Sets the given character's hair style to the given style id.
 */
static int chr_set_hair_style(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_set_hair_style called "
                         "for nonexistent character.");
        return 0;
    }

    const int style = luaL_checkint(s, 2);
    if (style < 0)
    {
        raiseScriptError(s, "chr_set_hair_style called "
                         "for nonexistent style id %d.", style);
        return 0;
    }

    c->setHairStyle(style);
    c->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);

    return 0;
}

/**
 * mana.chr_get_hair_style(Character*): int hair_style
 * Gets the hair style of the given character.
 */
static int chr_get_hair_style(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_hair_style called "
                         "for nonexistent character.");
        return 0;
    }

    lua_pushinteger(s, c->getHairStyle());
    return 1;
}

/**
 * mana.chr_set_hair_color(Character*, int color_id): void
 * Set the hair color of the given character to the given color id.
 */
static int chr_set_hair_color(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_set_hair_color called "
                         "for nonexistent character.");
        return 0;
    }

    const int color = luaL_checkint(s, 2);
    if (color < 0)
    {
        raiseScriptError(s, "chr_set_hair_color called "
                         "for nonexistent style id %d.", color);
        return 0;
    }

    c->setHairColor(color);
    c->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);

    return 0;
}

/**
 * mana.chr_get_hair_color(Character*): int hair_color
 * Get the hair color of the given character.
 */
static int chr_get_hair_color(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_hair_color called "
                         "for nonexistent character.");
        return 0;
    }

    lua_pushinteger(s, c->getHairColor());
    return 1;
}

/**
 * mana.chr_get_kill_count(Character*, int monster_type): int
 * Get the number of monsters the player killed of a type.
 */
static int chr_get_kill_count(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_kill_count called "
                         "for nonexistent character.");
        return 0;
    }

    const int id = luaL_checkint(s, 2);

    lua_pushinteger(s, c->getKillCount(id));
    return 1;
}

/**
 * mana.chr_get_gender(Character*): int
 * Get the gender of the character.
 */
static int chr_get_gender(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_gender called for nonexistent character.");
        return 0;
    }

    lua_pushinteger(s, c->getGender());
    return 1;
}

/**
 * mana.chr_set_gender(Character*, int gender): void
 * Set the gender of the character.
 */
static int chr_set_gender(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_set_gender called for nonexistent character.");
        return 0;
    }

    const int gender = luaL_checkinteger(s, 2);
    c->setGender(gender);

    return 0;
}

/**
 * mana.chr_give_special(Character*, int special): void
 * Enables a special for a character.
 */
static int chr_give_special(lua_State *s)
{
    // cost_type is ignored until we have more than one cost type
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_give_special called "
                         "for nonexistent character.");
        return 0;
    }
    const int special = luaL_checkint(s, 2);

    c->giveSpecial(special);
    return 0;
}

/**
 * mana.chr_has_special(Character*, int special): bool
 * Checks whether a character has a given special.
 */
static int chr_has_special(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_has_special called for nonexistent character.");
        return 0;
    }
    const int special = luaL_checkint(s, 2);

    lua_pushboolean(s, c->hasSpecial(special));
    return 1;
}

/**
 * mana.chr_take_special(Character*, int special): bool success
 * Removes a special from a character.
 */
static int chr_take_special(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_take_special called for nonexistent character.");
        return 0;
    }
    const int special = luaL_checkint(s, 2);

    lua_pushboolean(s, c->hasSpecial(special));
    c->takeSpecial(special);
    return 1;
}

/**
 * mana.chr_get_rights(Character*): int
 * Returns the rights level of a character.
 */
static int chr_get_rights(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_rights called for nonexistent character.");
        return 0;
    }
    lua_pushinteger(s, c->getAccountLevel());
    return 1;
}

/**
 * mana.exp_for_level(int level): int
 * Returns the exp total necessary to reach a specific skill level.
 */
static int exp_for_level(lua_State *s)
{
    const int level = luaL_checkint(s, 1);
    lua_pushinteger(s, Character::expForLevel(level));
    return 1;
}

/**
 * Returns four useless tables for testing the STL container push wrappers.
 * This function can be removed when there are more useful functions which use
 * them.
 */
static int test_tableget(lua_State *s)
{

    std::list<float> list;
    std::vector<std::string> svector;
    std::vector<int> ivector;
    std::map<std::string, std::string> map;
    std::set<int> set;

    LOG_INFO("Pushing Float List");
    list.push_back(12.636);
    list.push_back(0.0000000045656);
    list.push_back(185645445634566.346);
    list.push_back(7835458.11);
    pushSTLContainer<float>(s, list);

    LOG_INFO("Pushing String Vector");
    svector.push_back("All");
    svector.push_back("your");
    svector.push_back("base");
    svector.push_back("are");
    svector.push_back("belong");
    svector.push_back("to");
    svector.push_back("us!");
    pushSTLContainer<std::string>(s, svector);

    LOG_INFO("Pushing Integer Vector");
    ivector.resize(10);
    for (int i = 1; i < 10; i++)
    {
        ivector[i-1] = i * i;
    }
    pushSTLContainer<int>(s, ivector);

    LOG_INFO("Pushing String/String Map");
    map["Apple"] = "red";
    map["Banana"] = "yellow";
    map["Lime"] = "green";
    map["Plum"] = "blue";
    pushSTLContainer<std::string, std::string>(s, map);

    LOG_INFO("Pushing Integer Set");
    set.insert(12);
    set.insert(8);
    set.insert(14);
    set.insert(10);
    pushSTLContainer<int>(s, set);


    return 5;
}

/**
 * mana.get_map_id(): int
 * Returns the id of the current map.
 */
static int get_map_id(lua_State *s)
{
    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    int id = t->getMap()->getID();
    lua_pushinteger(s, id);
    return 1;
}

/**
 * mana.get_map_property(string property): string
 * Returns the value of a map property.
 */
static int get_map_property(lua_State *s)
{
    const char *property = luaL_checkstring(s, 1);
    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    MapComposite *m = t->getMap();
    if (!m)
    {
        raiseScriptError(s, "get_map_property called outside a map.");
        return 0;
    }
    Map *map = m->getMap();
    std::string value = map->getProperty(property);
    const char *v = &value[0];

    lua_pushstring(s, v);
    return 1;
}

/**
 * mana.is_walkable(int x, int y): bool
 * Returns whether the pixel on the map is walkable.
 */
static int is_walkable(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    MapComposite *m = t->getMap();
    if (!m)
    {
        raiseScriptError(s, "is_walkable called outside a map.");
        return 0;
    }
    Map *map = m->getMap();

    // If the wanted warp place is unwalkable
    if (map->getWalk(x / map->getTileWidth(), y / map->getTileHeight()))
        lua_pushboolean(s, 1);
    else
        lua_pushboolean(s, 0);

    return 1;
}

/**
 * mana.drop_item(int x, int y, int id || string name[, int number]): void
 * Creates an item stack on the floor.
 */
static int item_drop(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    const int number = luaL_optint(s, 4, 1);

    ItemClass *ic;
    if (lua_isnumber(s, 3))
        ic = itemManager->getItem(lua_tointeger(s, 3));
    else
        ic = itemManager->getItemByName(lua_tostring(s, 3));

    if (!ic)
    {
        raiseScriptError(s, "item_drop called with unknown item id or name.");
        return 0;
    }
    Item *i = new Item(ic, number);

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    MapComposite* map = t->getMap();

    i->setMap(map);
    Point pos(x, y);
    i->setPosition(pos);
    GameState::insertOrDelete(i);

    return 0;
}

/**
 * mana.item_get_name(int item_id): string item_name
 * Returns the name of the item with the given id.
 */
static int item_get_name(lua_State *s)
{
    const int id = luaL_checkint(s, 1);
    ItemClass *it = itemManager->getItem(id);
    if (!it)
    {
        raiseScriptError(s, "item_get_name called with unknown item id.");
        return 0;
    }
    lua_pushstring(s, it->getName().c_str());
    return 1;
}

/**
 * mana.log(int log_level, string log_message): void
 * Logs the given message to the log.
 */
static int log(lua_State *s)
{
    const int loglevel = luaL_checkint(s, 1);
    const std::string message = lua_tostring(s, 2);

    if (loglevel >= utils::Logger::Fatal && loglevel <= utils::Logger::Debug)
         utils::Logger::output(message, (utils::Logger::Level) loglevel);
    else
        raiseScriptError(s, "log called with unknown loglevel");

    return 0;
}

/**
 * mana.get_distance(Being*, Being*): int
 * mana.get_distance(int x1, int y1, int x2, int y2): int
 * Gets the distance between two beings or two points.
 */
static int get_distance(lua_State *s)
{
    int x1, y1, x2, y2;
    if (lua_gettop(s) == 2)
    {
        Being *being1 = getBeing(s, 1);
        Being *being2 = getBeing(s, 2);
        x1 = being1->getPosition().x;
        y1 = being1->getPosition().y;
        x2 = being2->getPosition().x;
        y2 = being2->getPosition().y;
    }
    else
    {
        x1 = luaL_checkint(s, 1);
        y1 = luaL_checkint(s, 2);
        x2 = luaL_checkint(s, 3);
        y2 = luaL_checkint(s, 4);
    }
    const int dx = x1 - x2;
    const int dy = y1 - y2;
    const float dist = sqrt((dx * dx) + (dy * dy));
    lua_pushinteger(s, dist);

    return 1;
}

/**
 * mana.map_get_objects(): table of all objects
 * mana.map_get_objects(string type): table of all objects of type
 * Gets the objects of a map.
 */
static int map_get_objects(lua_State *s)
{
    const bool filtered = (lua_gettop(s) == 1);
    std::string filter;
    if (filtered)
        filter = luaL_checkstring(s, 1);

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    const std::vector<MapObject*> &objects = t->getMap()->getMap()->getObjects();

    if (!filtered)
        pushSTLContainer<MapObject*>(s, objects);
    else
    {
        std::vector<MapObject*> filteredObjects;
        for (std::vector<MapObject*>::const_iterator it = objects.begin();
             it != objects.end(); ++it)
        {
            if (utils::compareStrI((*it)->getType(), filter) == 0)
            {
                filteredObjects.push_back(*it);
            }
        }
        pushSTLContainer<MapObject*>(s, filteredObjects);
    }
    return 1;
}

/**
 * mana.map_object_get_property(handle object, string key)
 * Returns the value of the object property 'key'.
 */
static int map_object_get_property(lua_State *s)
{
    std::string key = luaL_checkstring(s, 2);
    if (!lua_islightuserdata(s, 1))
    {
        raiseScriptError(s, "map_object_get_property called with invalid"
                            "object handle");
        return 0;
    }
    MapObject *obj = static_cast<MapObject *>(lua_touserdata(s, 1));
    if (obj)
    {
        std::string property = obj->getProperty(key);
        if (!property.empty())
        {
            lua_pushstring(s, property.c_str());
            return 1;
        }
        else
        {
            // scripts can check for nil
            return 0;
        }
    }
    else
    {
        raiseScriptError(s, "map_object_get_property called with invalid"
                            "object handle");
        return 0;
    }
}

/**
 * mana.map_object_get_bounds(object)
 * Returns 4 int: x/y/width/height of object.
 */
static int map_object_get_bounds(lua_State *s)
{
    if (!lua_islightuserdata(s, 1))
    {
        raiseScriptError(s, "map_object_get_bounds called with invalid"
                            "object handle");
        return 0;
    }
    MapObject *obj = static_cast<MapObject *>(lua_touserdata(s, 1));
    const Rectangle &bounds = obj->getBounds();
    lua_pushinteger(s, bounds.x);
    lua_pushinteger(s, bounds.y);
    lua_pushinteger(s, bounds.w);
    lua_pushinteger(s, bounds.h);
    return 4;
}

/**
 * mana.map_object_get_name(object)
 * Returns the name of the object.
 */
static int map_object_get_name(lua_State *s)
{
    if (!lua_islightuserdata(s, 1))
    {
        raiseScriptError(s, "map_object_get_name called with invalid"
                            "object handle");
        return 0;
    }
    MapObject *obj = static_cast<MapObject *>(lua_touserdata(s, 1));
    lua_pushstring(s, obj->getName().c_str());
    return 1;
}

/**
 * mana.map_object_get_type(object)
 * Returns the type of the object.
 */
static int map_object_get_type(lua_State *s)
{
    if (!lua_islightuserdata(s, 1))
    {
        raiseScriptError(s, "map_object_get_type called with invalid"
                            "object handle");
        return 0;
    }
    MapObject *obj = static_cast<MapObject *>(lua_touserdata(s, 1));
    lua_pushstring(s, obj->getType().c_str());
    return 1;
}

static int require_loader(lua_State *s)
{
    // Add .lua extension (maybe only do this when it doesn't have it already)
    std::string filename = luaL_checkstring(s, 1);
    filename.append(".lua");

    const std::string path = ResourceManager::resolve(filename);
    if (!path.empty())
        luaL_loadfile(s, path.c_str());
    else
        lua_pushstring(s, "File not found");

    return 1;
}


LuaScript::LuaScript():
    nbArgs(-1)
{
    mState = luaL_newstate();
    luaL_openlibs(mState);

    // Register package loader that goes through the resource manager
    // table.insert(package.loaders, 2, require_loader)
    lua_getglobal(mState, "package");
    lua_getfield(mState, -1, "loaders");
    lua_pushcfunction(mState, require_loader);
    lua_rawseti(mState, -2, 2);
    lua_pop(mState, 2);

    // Put the callback functions in the scripting environment.
    static luaL_Reg const callbacks[] = {
        { "npc_create",                      &npc_create                      },
        { "npc_message",                     &npc_message                     },
        { "npc_choice",                      &npc_choice                      },
        { "npc_trade",                       &npc_trade                       },
        { "npc_post",                        &npc_post                        },
        { "npc_enable",                      &npc_enable                      },
        { "npc_disable",                     &npc_disable                     },
        { "chr_warp",                        &chr_warp                        },
        { "chr_inv_change",                  &chr_inv_change                  },
        { "chr_inv_count",                   &chr_inv_count                   },
        { "chr_get_level",                   &chr_get_level                   },
        { "chr_get_quest",                   &chr_get_quest                   },
        { "chr_set_quest",                   &chr_set_quest                   },
        { "getvar_map",                      &getvar_map                      },
        { "setvar_map",                      &setvar_map                      },
        { "getvar_world",                    &getvar_world                    },
        { "setvar_world",                    &setvar_world                    },
        { "chr_get_post",                    &chr_get_post                    },
        { "chr_get_exp",                     &chr_get_exp                     },
        { "chr_give_exp",                    &chr_give_exp                    },
        { "chr_get_rights",                  &chr_get_rights                  },
        { "chr_set_hair_style",              &chr_set_hair_style              },
        { "chr_get_hair_style",              &chr_get_hair_style              },
        { "chr_set_hair_color",              &chr_set_hair_color              },
        { "chr_get_hair_color",              &chr_get_hair_color              },
        { "chr_get_kill_count",              &chr_get_kill_count              },
        { "chr_get_gender",                  &chr_get_gender                  },
        { "chr_set_gender",                  &chr_set_gender                  },
        { "chr_give_special",                &chr_give_special                },
        { "chr_has_special",                 &chr_has_special                 },
        { "chr_take_special",                &chr_take_special                },
        { "exp_for_level",                   &exp_for_level                   },
        { "monster_create",                  &monster_create                  },
        { "monster_get_name",                &monster_get_name                },
        { "monster_change_anger",            &monster_change_anger            },
        { "monster_remove",                  &monster_remove                  },
        { "monster_load_script",             &monster_load_script             },
        { "being_apply_status",              &being_apply_status              },
        { "being_remove_status",             &being_remove_status             },
        { "being_has_status",                &being_has_status                },
        { "being_set_status_time",           &being_set_status_time           },
        { "being_get_status_time",           &being_get_status_time           },
        { "being_type",                      &being_type                      },
        { "being_walk",                      &being_walk                      },
        { "being_say",                       &being_say                       },
        { "being_damage",                    &being_damage                    },
        { "being_heal",                      &being_heal                      },
        { "being_get_name",                  &being_get_name                  },
        { "being_get_action",                &being_get_action                },
        { "being_set_action",                &being_set_action                },
        { "being_get_direction",             &being_get_direction             },
        { "being_set_direction",             &being_set_direction             },
        { "being_apply_attribute_modifier",  &being_apply_attribute_modifier  },
        { "being_remove_attribute_modifier", &being_remove_attribute_modifier },
        { "being_set_base_attribute",        &being_set_base_attribute        },
        { "being_get_modified_attribute",    &being_get_modified_attribute    },
        { "being_get_base_attribute",        &being_get_base_attribute        },
        { "posX",                            &posX                            },
        { "posY",                            &posY                            },
        { "trigger_create",                  &trigger_create                  },
        { "chat_message",                    &chat_message                    },
        { "get_beings_in_circle",            &get_beings_in_circle            },
        { "get_beings_in_rectangle",         &get_beings_in_rectangle         },
        { "being_register",                  &being_register                  },
        { "effect_create",                   &effect_create                   },
        { "chr_shake_screen",                &chr_shake_screen                },
        { "test_tableget",                   &test_tableget                   },
        { "get_map_id",                      &get_map_id                      },
        { "get_map_property",                &get_map_property                },
        { "is_walkable",                     &is_walkable                     },
        { "item_drop",                       &item_drop                       },
        { "item_get_name",                   &item_get_name                   },
        { "npc_ask_integer",                 &npc_ask_integer                 },
        { "npc_end",                         &npc_end                         },
        { "npc_ask_string",                  &npc_ask_string                  },
        { "log",                             &log                             },
        { "get_distance",                    &get_distance                    },
        { "map_get_objects",                 &map_get_objects                 },
        { "map_object_get_property",         &map_object_get_property         },
        { "map_object_get_bounds",           &map_object_get_bounds           },
        { "map_object_get_name",             &map_object_get_name             },
        { "map_object_get_type",             &map_object_get_type             },
        { NULL, NULL }
    };
    luaL_register(mState, "mana", callbacks);
    lua_pop(mState, 1);                     // pop the 'mana' table

    // Make script object available to callback functions.
    lua_pushlightuserdata(mState, (void *)&registryKey);
    lua_pushlightuserdata(mState, this);
    lua_settable(mState, LUA_REGISTRYINDEX);

    // Push the error handler to first index of the stack
    lua_getglobal(mState, "debug");
    lua_getfield(mState, -1, "traceback");
    lua_remove(mState, 1);                  // remove the 'debug' table

    loadFile("scripts/lua/libmana.lua");
}
