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

/*
 * This file includes all script bindings available to LUA scripts.
 * When you add or change a script binding please document it on
 *
 * http://doc.manasource.org/scripting
 */

/**
 * Callback for sending a NPC_MESSAGE.
 * mana.npc_message(npc, character, string)
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
 * Callback for sending a NPC_CHOICE.
 * mana.npc_choice(npc, character, string...)
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
                    raiseScriptError(s, "npc_choice called with incorrect parameters.");
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
 * Callback for sending a NPC_INTEGER.
 * mana.npc_integer(npc, character, min, max, defaut)
 */
static int npc_ask_integer(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q)
    {
        raiseScriptError(s, "npc_integer called with incorrect parameters.");
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
 * Callback for sending a NPC_STRING.
 * mana.npc_ask_string(npc, character)
 */
static int npc_ask_string(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q)
    {
        raiseScriptError(s, "npc_string called with incorrect parameters.");
        return 0;
    }
    MessageOut msg(GPMSG_NPC_STRING);
    msg.writeInt16(p->getPublicID());
    gameHandler->sendTo(q, msg);

    return 0;
}

/**
 * Callback for creating a NPC on the current map with the current script.
 * mana.npc_create(string name, int id, int x, int y): npc
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
    bool b = GameState::insert(q);
    /* Do not try to deal with a failure there. There are some serious issues
       if an insertion failed on an almost empty map. */
    assert(b); (void)b;
    lua_pushlightuserdata(s, q);
    return 1;
}

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
 * Callback for sending a NPC_POST.
 * mana.npc_post(npc, character)
 */
static int npc_post(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);

    if (!p || !q)
    {
        raiseScriptError(s, "npc_Choice called with incorrect parameters.");
        return 0;
    }

    MessageOut msg(GPMSG_NPC_POST);
    msg.writeInt16(p->getPublicID());
    gameHandler->sendTo(q, msg);

    return 0;
}

/**
 * Enable a NPC if it has previously disabled
 * mana.npc_enable(npc)
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
 * Disable a NPC
 * mana.npc_disable(npc)
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
 * Callback for warping a player to another place.
 * mana.chr_warp(character, nil/int map, int x, int y)
 */
static int chr_warp(lua_State *s)
{
    int x = luaL_checkint(s, 3);
    int y = luaL_checkint(s, 4);

    Character *q = getCharacter(s, 1);
    bool b = lua_isnil(s, 2);
    if (!q || !(b || lua_isnumber(s, 2)))
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
    else
    {
        m = MapManager::getMap(lua_tointeger(s, 2));
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
 * mana.chr_inv_change(character, (int id, int nb)...): bool success
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
    Inventory inv(q, true);
    for (int i = 0; i < nb_items; ++i)
    {
        if (!lua_isnumber(s, i * 2 + 2) || !lua_isnumber(s, i * 2 + 3))
        {
            raiseScriptError(s, "chr_inv_change called with incorrect parameters.");
            return 0;
        }

        int id = lua_tointeger(s, i * 2 + 2);
        int nb = lua_tointeger(s, i * 2 + 3);
        if (id == 0)
        {
            LOG_WARN("mana.chr_inv_change() called with id 0! "
                     "Currency is now handled through attributes!");
        }
        else if (nb < 0)
        {
            nb = inv.remove(id, -nb);
            if (nb)
            {
                inv.cancel();
                lua_pushboolean(s, 0);
                return 1;
            }
        }
        else
        {
            ItemClass *ic = itemManager->getItem(id);
            if (!ic)
            {
                raiseScriptError(s, "chr_inv_change called with an unknown item.");
                continue;
            }
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
 * Callback for counting items in inventory.
 * mana.chr_inv_count(character, int id...): int count...
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
        id = luaL_checkint(s, i);
        if (id == 0)
        {
            LOG_WARN("mana.chr_inv_count() called with id 0! "
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
 * Callback for trading between a player and an NPC.
 * mana.npc_trade(npc, character, bool sell, table items)
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
            raiseWarning(s, "npc_trade[Buy] called with invalid or empty items table parameter.");
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
            raiseWarning(s, "npc_trade called with invalid or empty items table parameter.");
            t->cancel();
            lua_pushinteger(s, 2);
            return 1;
        }

        int v[3];
        for (int i = 0; i < 3; ++i)
        {
            lua_rawgeti(s, -1, i + 1);
            if (!lua_isnumber(s, -1))
            {
                raiseWarning(s, "npc_trade called with incorrect parameters in item table.");
                t->cancel();
                lua_pushinteger(s, 2);
                return 1;
            }
            v[i] = lua_tointeger(s, -1);
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
 * Applies a status effect with id to the being given for a amount of time
 * mana.being_apply_status(Being *being, int id, int time)
 */

static int being_apply_status(lua_State *s)
{
    const int id = luaL_checkint(s, 2);
    const int time = luaL_checkint(s, 3);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_apply_status called with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    being->applyStatusEffect(id, time);
    return 0;
}

/**
 * Removes the given status effect
 * mana.being_remove_status(Being *being, int id)
 */
static int being_remove_status(lua_State *s)
{
    const int id = luaL_checkint(s, 2);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_remove_status called with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    being->removeStatusEffect(id);
    return 0;
}

/**
 * Returns true if a being has the given status effect
 * mana.being_has_status(Being *being, int id)
 */
static int being_has_status(lua_State *s)
{
    const int id = luaL_checkint(s, 2);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_has_status called with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    lua_pushboolean(s, being->hasStatusEffect(id));
    return 1;
}

/**
 * Returns the time left on the given status effect
 * mana.being_get_status_time(Being *being, int id)
 */
static int being_get_status_time(lua_State *s)
{
    const int id = luaL_checkint(s, 2);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_time_status called with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    lua_pushinteger(s, being->getStatusEffectTime(id));
    return 1;
}

/**
 * Sets the time left on the given status effect
 * mana.being_set_status_time(Being *being, int id, int time)
 */
static int being_set_status_time(lua_State *s)
{
    const int id = luaL_checkint(s, 2);
    const int time = luaL_checkint(s, 3);

    if (!lua_isuserdata(s, 1))
    {
        raiseScriptError(s, "being_time_status called with incorrect parameters.");
        return 0;
    }
    Being *being = getBeing(s, 1);
    being->setStatusEffectTime(id, time);
    return 0;
}

/**
 * Returns the Thing type of the given Being
 * mana.being_type(Being *being)
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
 * Function for making a being walk to a position
 * being_walk(Being *being, int x, int y[, float speed])
 * The speed is in tile per second
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
 * Makes the being say something
 * mana.being_say(source, message)
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
 * Applies combat damage to a being
 * mana.being_damage(victim, value, delta, cth, type, element)
 */
static int being_damage(lua_State *s)
{
    Being *being = getBeing(s, 1);

    if (!being->canFight())
        return 0;

    Damage dmg((unsigned short)    lua_tointeger(s, 2), /* base */
               (unsigned short) lua_tointeger(s, 3),    /* delta */
               (unsigned short) lua_tointeger(s, 4),    /* cth */
               (unsigned char)  lua_tointeger(s, 6),    /* element */
               DAMAGE_PHYSICAL);                        /* type */
    being->damage(NULL, dmg);

    return 0;
}

/**
 * Restores hit points of a being
 * mana.being_heal(being[, value])
 * Without a value it heals fully.
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
 * Gets the base attribute of a being
 * mana.being_get_base_attribute(being, attribute)
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
 * Gets the modified attribute of a being
 * mana.being_get_modified_attribute(being, attribute)
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
 * Sets the base attribute of a being
 * mana.being_set_base_attribute(being, attribute, value)
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
 * Applies an attribute modifier to a being.
 * mana.being_apply_attribute_modifier(
 *     attribute, value, layer, [duration, [effect-id]])
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
 * Gets the being's name
 * mana.being_get_name(being)
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
 * Gets the being's current action
 * mana.being_get_action(being)
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
 * Sets the being's current action
 * mana.being_set_action(being, action)
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
 * Gets the being's current direction
 * mana.being_get_direction(being)
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
 * Sets the being's current direction
 * mana.being_set_direction(being, direction)
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
 * Function for getting the x-coordinate of the position of a being
 */
static int posX(lua_State *s)
{
    int x = getBeing(s, 1)->getPosition().x;
    lua_pushinteger(s, x);

    return 1;
}

/**
 * Function for getting the y-coordinate of the position of a being
 */
static int posY(lua_State *s)
{
    int y = getBeing(s, 1)->getPosition().y;
    lua_pushinteger(s, y);

    return 1;
}

/**
 * Callback for creating a monster on the current map.
 * mana.monster_create(int type, int x, int y)
 */
static int monster_create(lua_State *s)
{
    const int monsterId = luaL_checkint(s, 1);
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

    MonsterClass *spec = monsterManager->getMonster(monsterId);
    if (!spec)
    {
        raiseScriptError(s, "monster_create called with invalid monster ID: %d", monsterId);
        //LOG_WARN("LuaMonster_Create invalid monster ID: " << monsterId);
        return 0;
    }

    Monster *q = new Monster(spec);
    q->setMap(m);
    q->setPosition(Point(x, y));
    if (!GameState::insertSafe(q))
    {
        LOG_WARN("Monster_Create failed to insert monster");
        return 0;
    }

    lua_pushlightuserdata(s, q);
    return 1;
}

/**
 * mana.monster_load_script(mob, scriptfilename)
 * loads a LUA script given for mob
 */
static int monster_load_script(lua_State *s)
{
    Monster *m = static_cast< Monster* >(getBeing(s, 1));
    if (!m)
    {
         raiseScriptError(s, "monster_load_script called for a nonexistance monster.");
         return 0;
    }

    const char *scriptName = luaL_checkstring(s, 2);
    if (scriptName[0] == 0)
    {
        raiseScriptError(s, "monster_load_script called with empty script file name.");
        return 0;
    }

    m->loadScript(scriptName);
    return 0;
}


/**
 * Callback for getting a quest variable. Starts a recovery and returns
 * immediatly, if the variable is not known yet.
 * mana.chr_get_chest(character, string): nil or string
 */
static int chr_get_quest(lua_State *s)
{
    Character *q = getCharacter(s, 1);
    if (!q)
    {
        raiseScriptError(s, "chr_get_quest called for nonexistent character.");
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
 * gets the value of a persistent map variable.
 * mana.getvar_map(string): string
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
 * sets the value of a persistent map variable.
 * mana.setvar_map(string, string)
 */
 static int setvar_map(lua_State *s)
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
    std::string key = lua_tostring(s, 1);
    std::string value = lua_tostring(s, 2);
    script->getMap()->setVariable(key, value);

    return 0;
}

/**
 * gets the value of a persistent global variable.
 * mana.getvar_world(string): string
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
 * sets the value of a persistent global variable.
 * mana.setvar_world(string, string)
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
 * Callback for setting a quest variable.
 * mana.chr_set_chest(character, string, string)
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
 * Creates a trigger area. Whenever an actor enters this area, a Lua function
 * is called.
 * mana.trigger_create (x, y, width, height, function, id)
 */
static int trigger_create(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    const int width = luaL_checkint(s, 3);
    const int height = luaL_checkint(s, 4);
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
 * Creates a chat message in the users chatlog(s)
 * global message: mana.chatmessage (message)
 * private massage: mana.chatmessage (recipent, message)
 */
static int chatmessage(lua_State *s)
{
    if (lua_gettop(s) == 2 && lua_isuserdata(s, 1) && lua_isstring(s, 2) )
    {
        Being *being = getBeing(s, 1);
        std::string message = lua_tostring(s, 2);

        if (being && message != "")
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
        raiseScriptError(s, "being_say called with incorrect parameters.");
        return 0;
    }

    return 0;
}

/**
 * Gets a LUA table with the being IDs of all beings
 * inside of a circular area of the current map.
 * mana.get_beings_in_circle (x, y, radius)
 */
static int get_beings_in_circle(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    const int r = luaL_checkint(s, 3);

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
 * Gets the post for the character
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
 * Makes the server call the lua functions deathEvent
 * and removeEvent when the being dies or is removed
 * from the map.
 * mana.being_register (being)
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
 * Triggers a special effect from the clients effects.xml
 * mana.effect_create (id, x, y)
 * mana.effect_create (id, being)
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
 * Gets the exp total in a skill of a specific character
 * mana.chr_get_exp (being, skill)
 */
static int chr_get_exp(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "luaChr_GetExp called for nonexistent character.");
        return 0;
    }

    const int skill = luaL_checkint(s, 2);
    const int exp = c->getExperience(skill);

    lua_pushinteger(s, exp);
    return 1;
}


/**
 * Gives the character a certain amount of experience points
 * in a skill. Can also be used to reduce the exp amount when
 * desired.
 * mana.chr_give_exp (being, skill, amount)
 */
static int chr_give_exp(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "luaChr_GiveExp called for nonexistent character.");
        return 0;
    }

    const int skill = luaL_checkint(s, 2);
    const int exp = luaL_checkint(s, 3);
    const int optimalLevel = luaL_optint(s, 4, 0);

    c->receiveExperience(skill, exp, optimalLevel);

    return 0;
}

/**
 * Sets the given character's hair style to the given style id
 * mana.chr_set_hair_style (character, styleid)
 */
static int chr_set_hair_style(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_set_hair_style called for nonexistent character.");
        return 0;
    }

    const int style = luaL_checkint(s, 2);
    if (style < 0)
    {
        raiseScriptError(s, "chr_set_hair_style called for nonexistent style id %d.", style);
        return 0;
    }

    c->setHairStyle(style);
    c->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);

    return 0;
}

/**
 * Gets the hair style of the given character
 * mana.chr_get_hair_style (character)
 */
static int chr_get_hair_style(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_hair_style called for nonexistent character.");
        return 0;
    }

    lua_pushinteger(s, c->getHairStyle());
    return 1;
}

/**
 * Set the hair color of the given character to the given color id
 * mana.chr_set_hair_color (character, colorid)
 */
static int chr_set_hair_color(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_set_hair_color called for nonexistent character.");
        return 0;
    }

    const int color = luaL_checkint(s, 2);
    if (color < 0)
    {
        raiseScriptError(s, "chr_set_hair_color called for nonexistent style id %d.", color);
        return 0;
    }

    c->setHairColor(color);
    c->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);

    return 0;
}

/**
 * Get the hair color of the given character
 * mana.chr_get_hair_color (character)
 */
static int chr_get_hair_color(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_hair_color called for nonexistent character.");
        return 0;
    }

    lua_pushinteger(s, c->getHairColor());
    return 1;
}

/**
 * Get the number of monsters the player killed of a type
 * mana.chr_get_kill_count (character, monsterType)
 */
static int chr_get_kill_count(lua_State *s)
{
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_get_kill_count called for nonexistent character.");
        return 0;
    }

    const int id = luaL_checkint(s, 2);

    lua_pushinteger(s, c->getKillCount(id));
    return 1;
}

/**
 * Get the gender of the character
 * mana.chr_get_gender (character)
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
 * Enables a special for a character
 * mana.chr_give_special (character, special)
 */
static int chr_give_special(lua_State *s)
{
    // cost_type is ignored until we have more than one cost type
    Character *c = getCharacter(s, 1);
    if (!c)
    {
        raiseScriptError(s, "chr_give_special called for nonexistent character.");
        return 0;
    }
    const int special = luaL_checkint(s, 2);

    c->giveSpecial(special);
    return 0;
}

/**
 * Checks if a character has a special and returns true or false
 * mana.chr_has_special (character, special)
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
 * Removes a special from a character
 * mana.chr_take_special (character, special)
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
 * Returns the rights level of a character.
 * mana.chr_get_rights (being)
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
 * Returns the exp total necessary to reach a specific skill level.
 * mana.exp_for_level (level)
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
 * Returns the ID of the current map
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
 * Creates an item stack on the floor
 * mana.drop_item(x, y, id[, number])
 */
static int item_drop(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    const int type = luaL_checkint(s, 3);
    const int number = luaL_optint(s, 4, 1);

    ItemClass *ic = itemManager->getItem(type);
    if (!ic)
    {
        raiseScriptError(s, "item_drop called with unknown item ID");
    }
    Item *i = new Item(ic, number);

    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    MapComposite* map = t->getMap();

    i->setMap(map);
    Point pos(x, y);
    i->setPosition(pos);
    GameState::insertSafe(i);

    return 0;
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
        { "chr_give_special",                &chr_give_special                },
        { "chr_has_special",                 &chr_has_special                 },
        { "chr_take_special",                &chr_take_special                },
        { "exp_for_level",                   &exp_for_level                   },
        { "monster_create",                  &monster_create                  },
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
        { "chatmessage",                     &chatmessage                     },
        { "get_beings_in_circle",            &get_beings_in_circle            },
        { "being_register",                  &being_register                  },
        { "effect_create",                   &effect_create                   },
        { "test_tableget",                   &test_tableget                   },
        { "get_map_id",                      &get_map_id                      },
        { "item_drop",                       &item_drop                       },
        { "npc_ask_integer",                 &npc_ask_integer                 },
        { "npc_end",                         &npc_end                         },
        { "npc_ask_string",                  &npc_ask_string                  },
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
