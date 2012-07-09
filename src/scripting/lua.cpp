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

#include "common/defines.h"
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
#include "game-server/map.h"
#include "game-server/mapcomposite.h"
#include "game-server/mapmanager.h"
#include "game-server/monster.h"
#include "game-server/monstermanager.h"
#include "game-server/npc.h"
#include "game-server/postman.h"
#include "game-server/quest.h"
#include "game-server/state.h"
#include "game-server/statuseffect.h"
#include "game-server/statusmanager.h"
#include "game-server/trigger.h"
#include "net/messageout.h"
#include "scripting/luautil.h"
#include "scripting/luascript.h"
#include "scripting/scriptmanager.h"
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
 * on_character_death( function(Character*) ): void
 * Sets a listener function to the character death event.
 */
static int on_character_death(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    Character::setDeathCallback(getScript(s));
    return 0;
}

/**
 * on_character_death_accept( function(Character*) ): void
 * Sets a listener function that is called when the player clicks on the OK
 * button after the death message appeared. It should be used to implement the
 * respawn mechanic.
 */
static int on_character_death_accept(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    Character::setDeathAcceptedCallback(getScript(s));
    return 0;
}

static int on_character_login(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    Character::setLoginCallback(getScript(s));
    return 0;
}

static int on_being_death(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    LuaScript::setDeathNotificationCallback(getScript(s));
    return 0;
}

static int on_being_remove(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    LuaScript::setRemoveNotificationCallback(getScript(s));
    return 0;
}

static int on_update(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    Script::setUpdateCallback(getScript(s));
    return 0;
}

static int on_create_npc_delayed(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    Script::setCreateNpcDelayedCallback(getScript(s));
    return 0;
}

static int on_map_initialize(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    MapComposite::setInitializeCallback(getScript(s));
    return 0;
}

static int on_craft(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    ScriptManager::setCraftCallback(getScript(s));
    return 0;
}

static int on_mapvar_changed(lua_State *s)
{
    const char *key = luaL_checkstring(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    luaL_argcheck(s, key[0] != 0, 2, "empty variable name");
    MapComposite *m = checkCurrentMap(s);
    m->setMapVariableCallback(key, getScript(s));
    return 0;
}

static int on_worldvar_changed(lua_State *s)
{
    const char *key = luaL_checkstring(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    luaL_argcheck(s, key[0] != 0, 2, "empty variable name");
    MapComposite *m = checkCurrentMap(s);
    m->setWorldVariableCallback(key, getScript(s));
    return 0;
}

static int on_mapupdate(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    MapComposite::setUpdateCallback(getScript(s));
    return 0;
}

static int get_item_class(lua_State *s)
{
    LuaItemClass::push(s, checkItemClass(s, 1));
    return 1;
}

static int get_monster_class(lua_State *s)
{
    LuaMonsterClass::push(s, checkMonsterClass(s, 1));
    return 1;
}

static int get_status_effect(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    LuaStatusEffect::push(s, StatusManager::getStatusByName(name));
    return 1;
}

/**
 * npc_message(NPC*, Character*, string): void
 * Callback for sending a NPC_MESSAGE.
 */
static int npc_message(lua_State *s)
{
    NPC *p = checkNPC(s, 1);
    Character *q = checkCharacter(s, 2);
    const char *m = luaL_checkstring(s, 3);

    Script::Thread *thread = checkCurrentThread(s);

    MessageOut msg(GPMSG_NPC_MESSAGE);
    msg.writeInt16(p->getPublicID());
    msg.writeString(m);
    gameHandler->sendTo(q, msg);

    thread->mState = Script::ThreadPaused;
    return lua_yield(s, 0);
}

/**
 * npc_choice(NPC*, Character*, string...): void
 * Callback for sending a NPC_CHOICE.
 */
static int npc_choice(lua_State *s)
{
    NPC *p = checkNPC(s, 1);
    Character *q = checkCharacter(s, 2);

    Script::Thread *thread = checkCurrentThread(s);

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
            while (lua_next(s, i) != 0)
            {
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

    thread->mState = Script::ThreadExpectingNumber;
    return lua_yield(s, 0);
}

/**
 * npc_integer(NPC*, Character*, int min, int max, int default = min): void
 * Callback for sending a NPC_INTEGER.
 */
static int npc_ask_integer(lua_State *s)
{
    NPC *p = checkNPC(s, 1);
    Character *q = checkCharacter(s, 2);
    int min = luaL_checkint(s, 3);
    int max = luaL_checkint(s, 4);
    int defaultValue = luaL_optint(s, 5, min);

    Script::Thread *thread = checkCurrentThread(s);

    MessageOut msg(GPMSG_NPC_NUMBER);
    msg.writeInt16(p->getPublicID());
    msg.writeInt32(min);
    msg.writeInt32(max);
    msg.writeInt32(defaultValue);
    gameHandler->sendTo(q, msg);

    thread->mState = Script::ThreadExpectingNumber;
    return lua_yield(s, 0);
}

/**
 * npc_ask_string(NPC*, Character*): void
 * Callback for sending a NPC_STRING.
 */
static int npc_ask_string(lua_State *s)
{
    NPC *p = checkNPC(s, 1);
    Character *q = checkCharacter(s, 2);

    Script::Thread *thread = checkCurrentThread(s);

    MessageOut msg(GPMSG_NPC_STRING);
    msg.writeInt16(p->getPublicID());
    gameHandler->sendTo(q, msg);

    thread->mState = Script::ThreadExpectingString;
    return lua_yield(s, 0);
}

/**
 * npc_create(string name, int id, int gender, int x, int y,
 *            function talk, function update): NPC*
 *
 * Callback for creating a NPC on the current map.
 */
static int npc_create(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    const int id = luaL_checkint(s, 2);
    const int gender = luaL_checkint(s, 3);
    const int x = luaL_checkint(s, 4);
    const int y = luaL_checkint(s, 5);

    if (!lua_isnoneornil(s, 6))
        luaL_checktype(s, 6, LUA_TFUNCTION);
    if (!lua_isnoneornil(s, 7))
        luaL_checktype(s, 7, LUA_TFUNCTION);

    MapComposite *m = checkCurrentMap(s);

    NPC *q = new NPC(name, id);
    q->setGender(getGender(gender));
    q->setMap(m);
    q->setPosition(Point(x, y));

    if (lua_isfunction(s, 6))
    {
        lua_pushvalue(s, 6);
        q->setTalkCallback(luaL_ref(s, LUA_REGISTRYINDEX));
    }

    if (lua_isfunction(s, 7))
    {
        lua_pushvalue(s, 7);
        q->setUpdateCallback(luaL_ref(s, LUA_REGISTRYINDEX));
    }

    GameState::enqueueInsert(q);
    lua_pushlightuserdata(s, q);
    return 1;
}

/**
 * npc_post(NPC*, Character*): void
 * Callback for sending a NPC_POST.
 */
static int npc_post(lua_State *s)
{
    NPC *p = checkNPC(s, 1);
    Character *q = checkCharacter(s, 2);

    MessageOut msg(GPMSG_NPC_POST);
    msg.writeInt16(p->getPublicID());
    gameHandler->sendTo(q, msg);

    return 0;
}

/**
 * npc_enable(NPC*): void
 * Enable a NPC if it has previously disabled
 */
static int npc_enable(lua_State *s)
{
    NPC *p = checkNPC(s, 1);
    p->setEnabled(true);
    GameState::enqueueInsert(p);
    return 0;
}

/**
 * npc_disable(NPC*): void
 * Disable a NPC.
 */
static int npc_disable(lua_State *s)
{
    NPC *p = checkNPC(s, 1);
    p->setEnabled(false);
    GameState::remove(p);
    return 0;
}

/**
 * chr_warp(Character*, nil/int map, int x, int y): void
 * Callback for warping a player to another place.
 */
static int chr_warp(lua_State *s)
{
    Character *q = checkCharacter(s, 1);
    int x = luaL_checkint(s, 3);
    int y = luaL_checkint(s, 4);

    bool b = lua_isnil(s, 2);
    if (!(b || lua_isnumber(s, 2) || lua_isstring(s, 2)))
    {
        raiseScriptError(s, "chr_warp called with incorrect parameters.");
        return 0;
    }
    MapComposite *m;
    if (b)
    {
        m = checkCurrentMap(s);
    }
    else if (lua_isnumber(s, 2))
    {
        m = MapManager::getMap(lua_tointeger(s, 2));
        luaL_argcheck(s, m, 2, "invalid map id");
    }
    else
    {
        m = MapManager::getMap(lua_tostring(s, 2));
        luaL_argcheck(s, m, 2, "invalid map name");
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
 * Callback for gathering inventory information.
 * chr_get_inventory(character): table[]{slot, item id, name, amount}
 * Returns in the inventory slots order, the slot id, the item ids,
 * name and amount. Only slots not empty are returned.
 * @Example
 * To get a piece of information, you can do something like this:
 * -- This will print the 2nd non-empty slot id.
 * local my_table = chr_get_inventory(character)
 * print(my_table[2].slot)
 */
static int chr_get_inventory(lua_State *s)
{
    Character *q = checkCharacter(s, 1);

    // Create a lua table with the inventory ids.
    const InventoryData invData = q->getPossessions().getInventory();

    lua_newtable(s);
    int firstTableStackPosition = lua_gettop(s);
    int tableIndex = 1;

    std::string itemName = "";

    for (InventoryData::const_iterator it = invData.begin(),
        it_end = invData.end(); it != it_end; ++it)
    {
        if (!it->second.itemId || !it->second.amount)
            continue;

        // Create the sub-table (value of the main one)
        lua_createtable(s, 0, 4);
        int subTableStackPosition = lua_gettop(s);
        // Stores the item info in it.
        lua_pushstring(s, "slot");
        lua_pushinteger(s, it->first); // The slot id
        lua_settable(s, subTableStackPosition);

        lua_pushstring(s, "id");
        lua_pushinteger(s, it->second.itemId);
        lua_settable(s, subTableStackPosition);

        lua_pushstring(s, "name");
        itemName = itemManager->getItem(it->second.itemId)->getName();
        lua_pushstring(s, itemName.c_str());
        lua_settable(s, subTableStackPosition);

        lua_pushstring(s, "amount");
        lua_pushinteger(s, it->second.amount);
        lua_settable(s, subTableStackPosition);

        // Add the sub-table as value of the main one.
        lua_rawseti(s, firstTableStackPosition, tableIndex);
        ++tableIndex;
    }

    return 1;
}

/**
 * Callback for gathering equipment information.
 * chr_get_inventory(character): table[](slot, item id, name)
 * Returns in the inventory slots order, the slot id, the item ids, and name.
 * Only slots not empty are returned.
 * @Example
 * To get a piece of information, you can do something like this:
 * -- This will print the 2nd non-empty slot id.
 * local my_table = chr_get_equipment(character)
 * print(my_table[2].slot)
 */
static int chr_get_equipment(lua_State *s)
{
    Character *q = checkCharacter(s, 1);

    // Create a lua table with the inventory ids.
    const EquipData equipData = q->getPossessions().getEquipment();

    lua_newtable(s);
    int firstTableStackPosition = lua_gettop(s);
    int tableIndex = 1;

    std::string itemName;
    std::set<unsigned> itemInstances;

    for (EquipData::const_iterator it = equipData.begin(),
        it_end = equipData.end(); it != it_end; ++it)
    {
        if (!it->second.itemId || !it->second.itemInstance)
            continue;

        // Only count multi-slot items once.
        if (!itemInstances.insert(it->second.itemInstance).second)
            continue;

        // Create the sub-table (value of the main one)
        lua_createtable(s, 0, 3);
        int subTableStackPosition = lua_gettop(s);
        // Stores the item info in it.
        lua_pushstring(s, "slot");
        lua_pushinteger(s, it->first); // The slot id
        lua_settable(s, subTableStackPosition);

        lua_pushstring(s, "id");
        lua_pushinteger(s, it->second.itemId);
        lua_settable(s, subTableStackPosition);

        lua_pushstring(s, "name");
        itemName = itemManager->getItem(it->second.itemId)->getName();
        lua_pushstring(s, itemName.c_str());
        lua_settable(s, subTableStackPosition);

        // Add the sub-table as value of the main one.
        lua_rawseti(s, firstTableStackPosition, tableIndex);
        ++tableIndex;
    }

    return 1;
}

/**
 * chr_inv_change(Character*, (int id || string name,
 *                int nb)...): bool success
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
    Character *q = checkCharacter(s, 1);
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
        ItemClass *ic = checkItemClass(s, i * 2 + 2);
        int id = ic->getDatabaseID();
        if (nb < 0)
        {
            // Removing too much item is a success as for the scripter's
            // point of view. We log it anyway.
            nb = inv.remove(id, -nb);
            if (nb)
            {
                LOG_WARN("chr_inv_change() removed more items than owned: "
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
 * chr_inv_count(Character*, bool in inventory, bool in equipment,
 *               int item_id...): int count...
 * Callback for counting items in inventory.
 * The function can search in inventory and/or in the equipment.
 */
static int chr_inv_count(lua_State *s)
{
    Character *q = checkCharacter(s, 1);
    if (!lua_isboolean(s, 2) || !lua_isboolean(s, 3))
    {
        raiseScriptError(s, "chr_inv_count called with incorrect parameters.");
        return 0;
    }

    bool inInventory = lua_toboolean(s, 2);
    bool inEquipment = lua_toboolean(s, 3);

    int nb_items = lua_gettop(s) - 3;
    Inventory inv(q);

    int nb = 0;
    for (int i = 4; i < nb_items + 4; ++i)
    {
        ItemClass *it = checkItemClass(s, i);
        nb = inv.count(it->getDatabaseID(), inInventory, inEquipment);
        lua_pushinteger(s, nb);
    }
    return nb_items;
}

/**
 * chr_equip_slot(Character*, int inventorySlot): bool success
 * Makes the character equip the item in the given inventory slot.
 */
static int chr_equip_slot(lua_State *s)
{
    Character *ch = checkCharacter(s, 1);
    int inventorySlot = luaL_checkint(s, 2);

    Inventory inv(ch);
    lua_pushboolean(s, inv.equip(inventorySlot));
    return 1;
}

/**
 * chr_equip_item(Character*, int itemId || string itemName): bool success
 * Makes the character equip the item id when it's existing
 * in the player's inventory.
 */
static int chr_equip_item(lua_State *s)
{
    Character *ch = checkCharacter(s, 1);
    ItemClass *it = checkItemClass(s, 2);

    Inventory inv(ch);

    int inventorySlot = inv.getFirstSlot(it->getDatabaseID());
    bool success = false;

    if (inventorySlot > -1)
        success = inv.equip(inventorySlot);

    lua_pushboolean(s, success);
    return 1;
}

/**
 * chr_unequip_slot(Character*, int inventorySlot): bool success
 * Makes the character unequip the item in the given equipment slot.
 */
static int chr_unequip_slot(lua_State *s)
{
    Character *ch = checkCharacter(s, 1);
    int equipmentSlot = luaL_checkint(s, 2);

    Inventory inv(ch);

    lua_pushboolean(s, inv.unequip(inv.getSlotItemInstance(equipmentSlot)));
    return 1;
}

/**
 * chr_unequip_item(Character*, int itemId || string itemName): bool success
 * Makes the character unequip the item(s) corresponding to the id
 * when it's existing in the player's equipment.
 * Returns true when every item were unequipped from equipment.
 */
static int chr_unequip_item(lua_State *s)
{
    Character *ch = checkCharacter(s, 1);
    ItemClass *it = checkItemClass(s, 2);

    Inventory inv(ch);
    lua_pushboolean(s, inv.unequipItem(it->getDatabaseID()));
    return 1;
}

/**
 * chr_get_level(Character*): int level
 * Tells the character current level.
 */
static int chr_get_level(lua_State *s)
{
    Character *ch = checkCharacter(s, 1);
    lua_pushinteger(s, ch->getLevel());
    return 1;
}

/**
 * chr_kick(Character*)
 * Kicks the character.
 */
static int chr_kick(lua_State *s)
{
    Character *ch = checkCharacter(s, 1);
    MessageOut kickmsg(GPMSG_CONNECT_RESPONSE);
    kickmsg.writeInt8(ERRMSG_ADMINISTRATIVE_LOGOFF);
    ch->getClient()->disconnect(kickmsg);
    return 0;
}

/**
 * npc_trade(NPC*, Character*, bool sell, table items): int
 * Callback for trading between a player and an NPC.
 * Let the player buy or sell only a subset of predeterminded items.
 * @param table items: a subset of buyable/sellable items.
 * When selling, if the 4 parameter is omitted or invalid,
 * everything in the player inventory can be sold.
 * @return 0 if something to buy/sell, 1 if no items, 2 in case of errors.
 */
static int npc_trade(lua_State *s)
{
    NPC *p = checkNPC(s, 1);
    Character *q = checkCharacter(s, 2);
    if (!lua_isboolean(s, 3))
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
                ItemClass *it = getItemClass(s, -1);

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
 * being_apply_status(Being*, int id, int time): void
 * Applies a status effect with id to the being given for a amount of time.
 */
static int being_apply_status(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);
    const int time = luaL_checkint(s, 3);

    being->applyStatusEffect(id, time);
    return 0;
}

/**
 * being_remove_status(Being*, int id): void
 * Removes the given status effect.
 */
static int being_remove_status(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);

    being->removeStatusEffect(id);
    return 0;
}

/**
 * being_has_status(Being*, int id): bool
 * Returns whether a being has the given status effect.
 */
static int being_has_status(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);

    lua_pushboolean(s, being->hasStatusEffect(id));
    return 1;
}

/**
 * being_get_status_time(Being*, int id): int
 * Returns the time left on the given status effect.
 */
static int being_get_status_time(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);

    lua_pushinteger(s, being->getStatusEffectTime(id));
    return 1;
}

/**
 * being_set_status_time(Being*, int id, int time): void
 * Sets the time left on the given status effect.
 */
static int being_set_status_time(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);
    const int time = luaL_checkint(s, 3);

    being->setStatusEffectTime(id, time);
    return 0;
}

/**
 * being_type(Being*): EntityType
 * Returns the entity type of the given being.
 */
static int being_type(lua_State *s)
{
    Being *being = checkBeing(s, 1);
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
    Being *being = checkBeing(s, 1);
    const int x = luaL_checkint(s, 2);
    const int y = luaL_checkint(s, 3);

    being->setDestination(Point(x, y));

    if (lua_gettop(s) >= 4)
    {
        being->setAttribute(ATTR_MOVE_SPEED_TPS, luaL_checknumber(s, 4));
        being->setAttribute(ATTR_MOVE_SPEED_RAW, utils::tpsToRawSpeed(
                being->getModifiedAttribute(ATTR_MOVE_SPEED_TPS)));
    }

    return 0;
}

/**
 * being_say(Being* source, string message): void
 * Makes the being say something.
 */
static int being_say(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    const char *message = luaL_checkstring(s, 2);
    GameState::sayAround(being, message);
    return 0;
}


/**
 * being_damage(Being* victim, int value, int delta, int cth, int type,
 *              int element [, Being* source [, int skill]]): void
 * Applies combat damage to a being.
 */
static int being_damage(lua_State *s)
{
    Being *being = checkBeing(s, 1);

    if (!being->canFight())
    {
        raiseScriptError(s, "being_damage called with "
                            "victim that cannot fight");
        return 0;
    }

    Damage dmg;
    dmg.base = luaL_checkint(s, 2);
    dmg.delta = luaL_checkint(s, 3);
    dmg.cth = luaL_checkint(s, 4);
    dmg.type = (DamageType)luaL_checkint(s, 5);
    dmg.element = (Element)luaL_checkint(s, 6);
    Being *source = 0;
    if (lua_gettop(s) >= 7)
    {
        source = checkBeing(s, 7);

        if (!source->canFight())
        {
            raiseScriptError(s, "being_damage called with "
                                "source that cannot fight");
            return 0;
        }
    }
    if (lua_gettop(s) >= 8)
    {
        dmg.skill = checkSkill(s, 8);
    }
    being->damage(source, dmg);

    return 0;
}

/**
 * being_heal(Being* [, int value]): void
 * Restores hit points of a being.
 * Without a value the being is fully healed.
 */
static int being_heal(lua_State *s)
{
    Being *being = checkBeing(s, 1);

    if (lua_gettop(s) == 1) // when there is only one argument
    {
        being->heal();
    }
    else
    {
        being->heal(luaL_checkint(s, 2));
    }

    return 0;
}

/**
 * being_get_base_attribute(Being*, int attribute): int
 * Gets the base attribute of a being.
 */
static int being_get_base_attribute(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    int attr = luaL_checkint(s, 2);
    luaL_argcheck(s, attr > 0, 2, "invalid attribute id");

    lua_pushinteger(s, being->getAttribute(attr));
    return 1;
}

/**
 * being_get_modified_attribute(Being*, int attribute): int
 * Gets the modified attribute of a being.
 */
static int being_get_modified_attribute(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    int attr = luaL_checkint(s, 2);
    luaL_argcheck(s, attr > 0, 2, "invalid attribute id");

    lua_pushinteger(s, being->getModifiedAttribute(attr));
    return 1;
}

/**
 * being_set_base_attribute(Being*, int attribute, double value): void
 * Sets the base attribute of a being.
 */
static int being_set_base_attribute(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    int attr = luaL_checkint(s, 2);
    double value = luaL_checknumber(s, 3);

    being->setAttribute(attr, value);
    return 0;
}

/**
 * being_apply_attribute_modifier(Being*, int attribute, double value,
 *     int layer, int [duration, int [effect-id]]): void
 * Applies an attribute modifier to a being.
 */
static int being_apply_attribute_modifier(lua_State *s)
{
    Being *being    = checkBeing(s, 1);
    int attr        = luaL_checkint(s,2);
    double value    = luaL_checknumber(s, 3);
    int layer       = luaL_checkint(s, 4);
    int duration    = luaL_optint(s, 5, 0);
    int effectId    = luaL_optint(s, 6, 0);

    being->applyModifier(attr, value, layer, duration, effectId);
    return 0;
}

/**
 * being_remove_attribute_modifier(Being*, int attribute, double value,
 *     int layer, int [effect-id]]): void
 * Removes an attribute modifier to a being.
 */
static int being_remove_attribute_modifier(lua_State *s)
{
    Being *being    = checkBeing(s, 1);
    int attr        = luaL_checkint(s, 2);
    double value    = luaL_checknumber(s, 3);
    int layer       = luaL_checkint(s, 4);
    int effectId    = luaL_optint(s, 5, 0);

    being->removeModifier(attr, value, layer, effectId);
    return 0;
}

/**
 * being_get_name(Being*): string
 * Gets the being's name.
 */
static int being_get_name(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    lua_pushstring(s, being->getName().c_str());
    return 1;
}

/**
 * being_get_action(Being*): BeingAction
 * Gets the being's current action.
 */
static int being_get_action(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    lua_pushinteger(s, being->getAction());
    return 1;
}

/**
 * being_set_action(Being*, BeingAction): void
 * Sets the being's current action.
 */
static int being_set_action(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    int act = luaL_checkint(s, 2);
    being->setAction((BeingAction) act);
    return 0;
}

/**
 * being_get_direction(Being*): BeingDirection
 * Gets the being's current direction.
 */
static int being_get_direction(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    lua_pushinteger(s, being->getDirection());
    return 1;
}

/**
 * being_set_direction(Being*, BeingDirection): void
 * Sets the being's current direction.
 */
static int being_set_direction(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    BeingDirection dir = (BeingDirection) luaL_checkint(s, 2);
    being->setDirection(dir);
    return 0;
}

/**
 * being_set_walkmask(Being*, string mask)
 * Sets the walkmask of a being
 */
static int being_set_walkmask(lua_State *s)
{
   Being *being = checkBeing(s, 1);
   const char *stringMask = luaL_checkstring(s, 2);
   unsigned char mask = 0x00;
   if (strchr(stringMask, 'w'))
       mask |= Map::BLOCKMASK_WALL;
   else if (strchr(stringMask, 'c'))
       mask |= Map::BLOCKMASK_CHARACTER;
   else if (strchr(stringMask, 'm'))
       mask |= Map::BLOCKMASK_MONSTER;
   being->setWalkMask(mask);
   return 0;
}

/**
 * being_get_walkmask(Being*): string
 * Returns the walkmask of a being.
 */
static int being_get_walkmask(lua_State *s)
{
   Being *being = checkBeing(s, 1);
   const unsigned char mask = being->getWalkMask();
   luaL_Buffer buffer;
   luaL_buffinit(s, &buffer);
   if (mask & Map::BLOCKMASK_WALL)
       luaL_addstring(&buffer, "w");
   if (mask & Map::BLOCKMASK_CHARACTER)
       luaL_addstring(&buffer, "c");
   if (mask & Map::BLOCKMASK_MONSTER)
       luaL_addstring(&buffer, "m");
   luaL_pushresult(&buffer);
   return 1;
}

/**
 * being_get_mapid(Being*): int
 * Returns the id of the map where the being is located or nil if there is none.
 */
static int being_get_mapid(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    if (MapComposite *map = being->getMap())
        lua_pushinteger(s, map->getID());
    else
        lua_pushnil(s);

    return 1;
}

/**
 * posX(Being*): int xcoord
 * Function for getting the x-coordinate of the position of a being.
 */
static int posX(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    lua_pushinteger(s, being->getPosition().x);
    return 1;
}

/**
 * posY(Being*): int ycoord
 * Function for getting the y-coordinate of the position of a being.
 */
static int posY(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    lua_pushinteger(s, being->getPosition().y);
    return 1;
}

static int monster_class_on_update(lua_State *s)
{
    MonsterClass *monsterClass = LuaMonsterClass::check(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    monsterClass->setUpdateCallback(getScript(s));
    return 0;
}

static int monster_class_on_damage(lua_State *s)
{
    MonsterClass *monsterClass = LuaMonsterClass::check(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    monsterClass->setDamageCallback(getScript(s));
    return 0;
}

static int monster_class_attacks(lua_State *s)
{
    MonsterClass *monsterClass = LuaMonsterClass::check(s, 1);
    pushSTLContainer(s, monsterClass->getAttackInfos());
    return 1;
}

/**
 * monster_create(int id || string name, int x, int y): Monster*
 * Callback for creating a monster on the current map.
 */
static int monster_create(lua_State *s)
{
    MonsterClass *monsterClass = checkMonsterClass(s, 1);
    const int x = luaL_checkint(s, 2);
    const int y = luaL_checkint(s, 3);
    MapComposite *m = checkCurrentMap(s);

    Monster *q = new Monster(monsterClass);
    q->setMap(m);
    q->setPosition(Point(x, y));
    GameState::enqueueInsert(q);

    lua_pushlightuserdata(s, q);
    return 1;
}

/**
 * monster_get_name(int monster_id): string monster_name
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
 * monster_get_id(handle monster): int monsterid
 * Returns the id of the monster handle
 */
static int monster_get_id(lua_State *s)
{
    Monster *monster = checkMonster(s, 1);
    lua_pushinteger(s, monster->getSpecy()->getId());
    return 1;
}

/**
 * monster_change_anger(Monster*, Being*, int anger)
 * Makes a monster angry at a being
 */
static int monster_change_anger(lua_State *s)
{
    Monster *monster = checkMonster(s, 1);
    Being *being = checkBeing(s, 2);
    const int anger = luaL_checkint(s, 3);
    monster->changeAnger(being, anger);
    return 0;
}

/**
 * monster_remove(Monster*): bool success
 * Remove a monster object without kill event.
 * return whether the monster was enqueued for removal.
 */
static int monster_remove(lua_State *s)
{
    bool monsterRemoved = false;
    if (Monster *m = getMonster(s, 1))
    {
        GameState::remove(m);
        monsterRemoved = true;
    }
    lua_pushboolean(s, monsterRemoved);
    return 1;
}

/**
 * chr_get_quest(Character*, string): nil or string
 * Callback for getting a quest variable. Starts a recovery and returns
 * immediatly, if the variable is not known yet.
 */
static int chr_get_quest(lua_State *s)
{
    Character *q = checkCharacter(s, 1);
    const char *name = luaL_checkstring(s, 2);
    luaL_argcheck(s, name[0] != 0, 2, "empty variable name");

    Script::Thread *thread = checkCurrentThread(s);

    std::string value;
    bool res = getQuestVar(q, name, value);
    if (res)
    {
        lua_pushstring(s, value.c_str());
        return 1;
    }
    QuestCallback f = { &LuaScript::getQuestCallback, getScript(s) };
    recoverQuestVar(q, name, f);

    thread->mState = Script::ThreadExpectingString;
    return lua_yield(s, 0);
}

/**
 * getvar_map(string): string
 * Gets the value of a persistent map variable.
 */
static int getvar_map(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    luaL_argcheck(s, name[0] != 0, 1, "empty variable name");

    MapComposite *map = checkCurrentMap(s);
    std::string value = map->getVariable(name);

    lua_pushstring(s, value.c_str());
    return 1;
}

/**
 * setvar_map(string, string): void
 * Sets the value of a persistent map variable.
 */
static int setvar_map(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    const char *value = luaL_checkstring(s, 2);
    luaL_argcheck(s, name[0] != 0, 1, "empty variable name");

    MapComposite *map = checkCurrentMap(s);
    map->setVariable(name, value);

    return 0;
}

/**
 * getvar_world(string): string
 * Gets the value of a persistent global variable.
 */
static int getvar_world(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    luaL_argcheck(s, name[0] != 0, 1, "empty variable name");

    std::string value = GameState::getVariable(name);
    lua_pushstring(s, value.c_str());
    return 1;
}

/**
 * setvar_world(string, string): void
 * Sets the value of a persistent global variable.
 */
static int setvar_world(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    const char *value = luaL_checkstring(s, 2);
    luaL_argcheck(s, name[0] != 0, 1, "empty variable name");

    GameState::setVariable(name, value);
    return 0;
}

/**
 * chr_set_quest(Character*, string, string): void
 * Callback for setting a quest variable.
 */
static int chr_set_quest(lua_State *s)
{
    Character *q = checkCharacter(s, 1);
    const char *name = luaL_checkstring(s, 2);
    const char *value = luaL_checkstring(s, 3);
    luaL_argcheck(s, name[0] != 0, 2, "empty variable name");

    setQuestVar(q, name, value);
    return 0;
}

/**
 * trigger_create(int x, int y, int width, int height, function, int id,
 *                boolean once)
 * Creates a trigger area. Whenever an actor enters this area, the given Lua
 * function is called.
 */
static int trigger_create(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    const int width = luaL_checkint(s, 3);
    const int height = luaL_checkint(s, 4);
    luaL_checktype(s, 5, LUA_TFUNCTION);
    const int id = luaL_checkint(s, 6);

    if (!lua_isboolean(s, 7))
    {
        raiseScriptError(s, "trigger_create called with incorrect parameters.");
        return 0;
    }

    Script *script = getScript(s);
    MapComposite *m = checkCurrentMap(s, script);

    const bool once = lua_toboolean(s, 7);

    Script::Ref function;
    lua_pushvalue(s, 5);
    script->assignCallback(function);
    lua_pop(s, 1);

    ScriptAction *action = new ScriptAction(script, function, id);
    Rectangle r = { x, y, width, height };
    TriggerArea *area = new TriggerArea(m, r, action, once);

    LOG_INFO("Created script trigger at " << x << "," << y
             << " (" << width << "x" << height << ") id: " << id);

    bool ret = GameState::insert(area);
    lua_pushboolean(s, ret);
    return 1;
}

/**
 * private message: chat_message(Being* recipent, string message): void
 * Creates a chat message in the users chatlog(s).
 */
static int chat_message(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    const char *message = luaL_checkstring(s, 2);

    GameState::sayTo(being, NULL, message);
    return 0;
}

/**
 * get_beings_in_circle(int x, int y, int radius): table of Being*
 * get_beings_in_circle(handle centerBeing, int radius): table of Being*
 * Gets a LUA table with the Being* pointers of all beings
 * inside of a circular area of the current map.
 */
static int get_beings_in_circle(lua_State *s)
{
    int x, y, r;
    if (lua_islightuserdata(s, 1))
    {
        Being *b = checkBeing(s, 1);
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

    MapComposite *m = checkCurrentMap(s);

    //create a lua table with the beings in the given area.
    lua_newtable(s);
    int tableStackPosition = lua_gettop(s);
    int tableIndex = 1;
    for (BeingIterator i(m->getAroundPointIterator(Point(x, y), r)); i; ++i)
    {
        Being *b = *i;
        char t = b->getType();
        if (t == OBJECT_NPC || t == OBJECT_CHARACTER || t == OBJECT_MONSTER)
        {
            if (Collision::circleWithCircle(b->getPosition(), b->getSize(),
                                            Point(x, y), r))
            {
                lua_pushlightuserdata(s, b);
                lua_rawseti(s, tableStackPosition, tableIndex);
                tableIndex++;
            }
        }
    }

    return 1;
}

/**
 * get_beings_in_rectangle(int x, int y, int width, int height): table of Being*
 * Gets a LUA table with the Being* pointers of all beings
 * inside of a rectangular area of the current map.
 */
static int get_beings_in_rectangle(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    const int w = luaL_checkint(s, 3);
    const int h = luaL_checkint(s, 4);

    MapComposite *m = checkCurrentMap(s);

    //create a lua table with the beings in the given area.
    lua_newtable(s);
    int tableStackPosition = lua_gettop(s);
    int tableIndex = 1;
    Rectangle rect = {x, y ,w, h};
    for (BeingIterator i(m->getInsideRectangleIterator(rect)); i; ++i)
    {
        Being *b = *i;
        char t = b->getType();
        if (t == OBJECT_NPC || t == OBJECT_CHARACTER || t == OBJECT_MONSTER)
        {
            lua_pushlightuserdata(s, b);
            lua_rawseti(s, tableStackPosition, tableIndex);
            tableIndex++;
        }
    }
     return 1;
 }

/**
 * get_character_by_name(string name): Character*
 * Returns the character handle or NULL if there is none
 */
static int get_character_by_name(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);

    Character *ch = gameHandler->getCharacterByNameSlow(name);
    if (!ch)
        lua_pushnil(s);
    else
        lua_pushlightuserdata(s, ch);

    return 1;
}

/**
 * chr_get_post(Character*): void
 * Gets the post for the character.
 */
static int chr_get_post(lua_State *s)
{
    Character *c = checkCharacter(s, 1);

    Script *script = getScript(s);
    Script::Thread *thread = checkCurrentThread(s, script);

    PostCallback f = { &LuaScript::getPostCallback, script };
    postMan->getPost(c, f);

    thread->mState = Script::ThreadExpectingTwoStrings;
    return lua_yield(s, 0);
}

/**
 * being_register(Being*): void
 * Makes the server call the on_being_death and on_being_remove callbacks
 * when the being dies or is removed from the map.
 */
static int being_register(lua_State *s)
{
    Being *being = checkBeing(s, 1);
    being->addListener(getScript(s)->getScriptListener());
    return 0;
}

/**
 * effect_create (int id, int x, int y): void
 * effect_create (int id, Being*): void
 * Triggers a special effect from the clients effects.xml.
 */
static int effect_create(lua_State *s)
{
    const int id = luaL_checkint(s, 1);

    if (lua_isuserdata(s, 2))
    {
        // being mode
        Being *b = checkBeing(s, 2);
        Effects::show(id, b->getMap(), b);
    }
    else
    {
        // positional mode
        int x = luaL_checkint(s, 2);
        int y = luaL_checkint(s, 3);
        MapComposite *m = checkCurrentMap(s);
        Effects::show(id, m, Point(x, y));
    }

    return 0;
}

/**
 * chr_shake_screen(Character*, int x, int y[, float strength, int radius]): void
 * Shake the screen for a given character.
 */
static int chr_shake_screen(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
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
 * chr_get_exp(Character*, int skill): int
 * chr_get_exp(Character*, string skillname): int
 * Gets the exp total in a skill of a specific character.
 * If called with skillname this name has to be in this format:
 * "<setname>_<skillname>"
 */
static int chr_get_exp(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    int skill = checkSkill(s, 2);
    const int exp = c->getExperience(skill);

    lua_pushinteger(s, exp);
    return 1;
}

/**
 * chr_give_exp(Character*, int skill, int amount[, int optimal_level]): void
 * chr_give_exp(Character*, string skillname, int amount
 *              [, int optimal_level]): void
 * Gives the character a certain amount of experience points
 * in a skill. Can also be used to reduce the exp amount when
 * desired. If called with skillname this name has to be in this format:
 * "<setname>_<skillname>"
 */
static int chr_give_exp(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    int skill = checkSkill(s, 2);
    const int exp = luaL_checkint(s, 3);
    const int optimalLevel = luaL_optint(s, 4, 0);

    c->receiveExperience(skill, exp, optimalLevel);
    return 0;
}

/**
 * chr_set_hair_style(Character*, int style_id): void
 * Sets the given character's hair style to the given style id.
 */
static int chr_set_hair_style(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    const int style = luaL_checkint(s, 2);
    luaL_argcheck(s, style >= 0, 2, "invalid style id");

    c->setHairStyle(style);
    c->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);
    return 0;
}

/**
 * chr_get_hair_style(Character*): int hair_style
 * Gets the hair style of the given character.
 */
static int chr_get_hair_style(lua_State *s)
{
    Character *c = checkCharacter(s, 1);

    lua_pushinteger(s, c->getHairStyle());
    return 1;
}

/**
 * chr_set_hair_color(Character*, int color_id): void
 * Set the hair color of the given character to the given color id.
 */
static int chr_set_hair_color(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    const int color = luaL_checkint(s, 2);
    luaL_argcheck(s, color >= 0, 2, "invalid color id");

    c->setHairColor(color);
    c->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);

    return 0;
}

/**
 * chr_get_hair_color(Character*): int hair_color
 * Get the hair color of the given character.
 */
static int chr_get_hair_color(lua_State *s)
{
    Character *c = checkCharacter(s, 1);

    lua_pushinteger(s, c->getHairColor());
    return 1;
}

/**
 * chr_get_kill_count(Character*, int monster_type): int
 * Get the number of monsters the player killed of a type.
 */
static int chr_get_kill_count(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    MonsterClass *monster = checkMonsterClass(s, 2);

    lua_pushinteger(s, c->getKillCount(monster->getId()));
    return 1;
}

/**
 * being_get_gender(Being*): int
 * Get the gender of the being.
 */
static int being_get_gender(lua_State *s)
{
    Being *b = checkBeing(s, 1);
    lua_pushinteger(s, b->getGender());
    return 1;
}

/**
 * being_set_gender(Being*, int gender): void
 * Set the gender of the being.
 */
static int being_set_gender(lua_State *s)
{
    Being *b = checkBeing(s, 1);
    const int gender = luaL_checkinteger(s, 2);
    b->setGender(getGender(gender));
    return 0;
}

/**
 * chr_give_special(Character*, int special): void
 * Enables a special for a character.
 */
static int chr_give_special(lua_State *s)
{
    // cost_type is ignored until we have more than one cost type
    Character *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);
    const int currentMana = luaL_optint(s, 3, 0);

    c->giveSpecial(special, currentMana);
    return 0;
}

/**
 * chr_has_special(Character*, int special): bool
 * Checks whether a character has a given special.
 */
static int chr_has_special(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    const int special = luaL_checkint(s, 2);

    lua_pushboolean(s, c->hasSpecial(special));
    return 1;
}

/**
 * chr_take_special(Character*, int special): bool success
 * Removes a special from a character.
 */
static int chr_take_special(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    const int special = luaL_checkint(s, 2);

    lua_pushboolean(s, c->hasSpecial(special));
    c->takeSpecial(special);
    return 1;
}

/**
 * chr_set_special_recharge_speed(Character*, int special, int speed)
 * Sets recharge speed for a special.
 */
static int chr_set_special_recharge_speed(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);
    const int speed = luaL_checkint(s, 3);

    if (c->setSpecialRechargeSpeed(special, speed))
        raiseScriptError(s, "chr_set_special_mana called with special "
                         "that is not owned by character.");
    return 0;
}

/**
 * chr_get_special_recharge_speed(Character*, int special)
 * Gets recharge speed of a special.
 */
static int chr_get_special_recharge_speed(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);

    SpecialMap::iterator it = c->findSpecial(special);

    luaL_argcheck(s, it != c->getSpecialEnd(), 2,
                  "character does not have special");

    lua_pushinteger(s, it->second.rechargeSpeed);
    return 1;
}

/**
 * chr_set_special_mana(Character*, int special, int mana)
 * Sets the current charge of the special.
 */
static int chr_set_special_mana(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);
    const int mana = luaL_checkint(s, 3);
    if (!c->setSpecialMana(special, mana))
        raiseScriptError(s, "chr_set_special_mana called with special "
                         "that is not owned by character.");
    return 0;
}

/**
 * chr_get_special_mana(Character*, int special): int
 * Gets the current charge of a special.
 */
static int chr_get_special_mana(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);
    SpecialMap::iterator it = c->findSpecial(special);
    luaL_argcheck(s, it != c->getSpecialEnd(), 2,
                  "character does not have special");
    lua_pushinteger(s, it->second.currentMana);
    return 1;
}

/**
 * chr_get_rights(Character*): int
 * Returns the rights level of a character.
 */
static int chr_get_rights(lua_State *s)
{
    Character *c = checkCharacter(s, 1);
    lua_pushinteger(s, c->getAccountLevel());
    return 1;
}

/**
 * exp_for_level(int level): int
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
 * get_map_id(): int
 * Returns the id of the current map.
 */
static int get_map_id(lua_State *s)
{
    Script *script = getScript(s);

    if (MapComposite *mapComposite = script->getMap())
        lua_pushinteger(s, mapComposite->getID());
    else
        lua_pushnil(s);

    return 1;
}

/**
 * get_map_property(string property): string
 * Returns the value of a map property.
 */
static int get_map_property(lua_State *s)
{
    const char *property = luaL_checkstring(s, 1);
    Map *map = checkCurrentMap(s)->getMap();

    std::string value = map->getProperty(property);
    lua_pushstring(s, value.c_str());
    return 1;
}

/**
 * is_walkable(int x, int y): bool
 * Returns whether the pixel on the map is walkable.
 */
static int is_walkable(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    Map *map = checkCurrentMap(s)->getMap();

    // If the wanted warp place is unwalkable
    if (map->getWalk(x / map->getTileWidth(), y / map->getTileHeight()))
        lua_pushboolean(s, 1);
    else
        lua_pushboolean(s, 0);

    return 1;
}

static int map_get_pvp(lua_State *s)
{
    MapComposite *m = checkCurrentMap(s);
    lua_pushinteger(s, m->getPvP());
    return 1;
}

static int item_class_on(lua_State *s)
{
    ItemClass *itemClass = LuaItemClass::check(s, 1);
    const char *event = luaL_checkstring(s, 2);
    luaL_checktype(s, 3, LUA_TFUNCTION);
    itemClass->setEventCallback(event, getScript(s));
    return 0;
}

static int item_class_attacks(lua_State *s)
{
    ItemClass *itemClass = LuaItemClass::check(s, 1);
    std::vector<AttackInfo *> attacks = itemClass->getAttackInfos();
    pushSTLContainer<AttackInfo *>(s, attacks);
    return 1;
}

/**
 * drop_item(int x, int y, int id || string name[, int number]): bool
 * Creates an item stack on the floor.
 * @Returns whether the insertion was successful.
 */
static int item_drop(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    ItemClass *ic = checkItemClass(s, 3);
    const int number = luaL_optint(s, 4, 1);
    MapComposite *map = checkCurrentMap(s);

    Item *i = new Item(ic, number);

    i->setMap(map);
    Point pos(x, y);
    i->setPosition(pos);
    lua_pushboolean(s, GameState::insertOrDelete(i));
    return 1;
}

/**
 * item_get_name(int item_id): string item_name
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
 * log(int log_level, string log_message): void
 * Logs the given message to the log.
 */
static int log(lua_State *s)
{
    using utils::Logger;

    const int loglevel = luaL_checkint(s, 1);
    luaL_argcheck(s,
                  loglevel >= Logger::Fatal && loglevel <= Logger::Debug,
                  1,
                  "invalid log level");

    const std::string message = luaL_checkstring(s, 2);

    Logger::output(message, (Logger::Level) loglevel);
    return 0;
}

/**
 * get_distance(Being*, Being*): int
 * get_distance(int x1, int y1, int x2, int y2): int
 * Gets the distance between two beings or two points.
 */
static int get_distance(lua_State *s)
{
    int x1, y1, x2, y2;
    if (lua_gettop(s) == 2)
    {
        Being *being1 = checkBeing(s, 1);
        Being *being2 = checkBeing(s, 2);

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
 * map_get_objects(): table of all objects
 * map_get_objects(string type): table of all objects of type
 * Gets the objects of a map.
 */
static int map_get_objects(lua_State *s)
{
    const bool filtered = (lua_gettop(s) == 1);
    const char *filter;
    if (filtered)
    {
        filter = luaL_checkstring(s, 1);
    }

    MapComposite *m = checkCurrentMap(s);
    const std::vector<MapObject*> &objects = m->getMap()->getObjects();

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
 * map_object_get_property(handle object, string key)
 * Returns the value of the object property 'key'.
 */
static int map_object_get_property(lua_State *s)
{
    const char *key = luaL_checkstring(s, 2);
    MapObject *obj = LuaMapObject::check(s, 1);

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

/**
 * map_object_get_bounds(object)
 * Returns 4 int: x/y/width/height of object.
 */
static int map_object_get_bounds(lua_State *s)
{
    MapObject *obj = LuaMapObject::check(s, 1);
    const Rectangle &bounds = obj->getBounds();
    lua_pushinteger(s, bounds.x);
    lua_pushinteger(s, bounds.y);
    lua_pushinteger(s, bounds.w);
    lua_pushinteger(s, bounds.h);
    return 4;
}

/**
 * map_object_get_name(object)
 * Returns the name of the object.
 */
static int map_object_get_name(lua_State *s)
{
    MapObject *obj = LuaMapObject::check(s, 1);
    lua_pushstring(s, obj->getName().c_str());
    return 1;
}

/**
 * map_object_get_type(object)
 * Returns the type of the object.
 */
static int map_object_get_type(lua_State *s)
{
    MapObject *obj = LuaMapObject::check(s, 1);
    lua_pushstring(s, obj->getType().c_str());
    return 1;
}

static int status_effect_on_tick(lua_State *s)
{
    StatusEffect *statusEffect = LuaStatusEffect::check(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    statusEffect->setTickCallback(getScript(s));
    return 0;
}

/**
 * announce(text [, sender])
 * Does a global announce
 */
static int announce(lua_State *s)
{
    const char *message = luaL_checkstring(s, 1);
    const char *sender = luaL_optstring(s, 2, "Server");

    MessageOut msg(GAMSG_ANNOUNCE);
    msg.writeString(message);
    msg.writeInt16(0); // Announce from server so id = 0
    msg.writeString(sender);
    accountHandler->send(msg);
    return 0;
}

static int get_special_info(lua_State *s)
{
    const int special = checkSpecial(s, 1);
    SpecialManager::SpecialInfo *info = specialManager->getSpecialInfo(special);
    luaL_argcheck(s, info, 1, "invalid special");
    LuaSpecialInfo::push(s, info);
    return 1;
}

static int specialinfo_get_name(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    lua_pushstring(s, info->name.c_str());
    return 1;
}

static int specialinfo_get_needed_mana(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    lua_pushinteger(s, info->neededMana);
    return 1;
}

static int specialinfo_is_rechargeable(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    lua_pushboolean(s, info->rechargeable);
    return 1;
}

static int specialinfo_get_category(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    lua_pushstring(s, info->setName.c_str());
    return 1;
}

static int specialinfo_on_recharged(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    Script *script = getScript(s);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    script->assignCallback(info->rechargedCallback);
    return 0;
}

static int specialinfo_on_use(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    Script *script = getScript(s);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    script->assignCallback(info->useCallback);
    return 0;
}

static int attack_get_priority(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    lua_pushinteger(s, attack->getPriority());
    return 1;
}

static int attack_get_cooldowntime(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    lua_pushinteger(s, attack->getCooldownTime());
    return 1;
}

static int attack_get_warmuptime(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    lua_pushinteger(s, attack->getWarmupTime());
    return 1;
}

static int attack_get_reusetime(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    lua_pushinteger(s, attack->getReuseTime());
    return 1;
}

static int attack_get_damage(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    LuaDamage::push(s, &attack->getDamage());
    return 1;
}

static int attack_on_attack(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    attack->setCallback(getScript(s));
    return 0;
}

static int damage_get_id(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->id);
    return 1;
}


static int damage_get_skill(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->skill);
    return 1;
}

static int damage_get_base(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->base);
    return 1;
}

static int damage_get_delta(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->delta);
    return 1;
}

static int damage_get_cth(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->cth);
    return 1;
}

static int damage_get_element(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->element);
    return 1;
}

static int damage_get_type(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->type);
    return 1;
}

static int damage_is_truestrike(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushboolean(s, damage->trueStrike);
    return 1;
}

static int damage_get_range(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->range);
    return 1;
}

static int require_loader(lua_State *s)
{
    // Add .lua extension (maybe only do this when it doesn't have it already)
    const char *file = luaL_checkstring(s, 1);
    std::string filename = file;
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
    mRootState = luaL_newstate();
    mCurrentState = mRootState;
    luaL_openlibs(mRootState);

    // Register package loader that goes through the resource manager
    // table.insert(package.loaders, 2, require_loader)
    lua_getglobal(mRootState, "package");
    lua_getfield(mRootState, -1, "loaders");
    lua_pushcfunction(mRootState, require_loader);
    lua_rawseti(mRootState, -2, 2);
    lua_pop(mRootState, 2);

    // Put the callback functions in the scripting environment.
    static luaL_Reg const callbacks[] = {
        { "on_character_death",              &on_character_death              },
        { "on_character_death_accept",       &on_character_death_accept       },
        { "on_character_login",              &on_character_login              },
        { "on_being_death",                  &on_being_death                  },
        { "on_being_remove",                 &on_being_remove                 },
        { "on_update",                       &on_update                       },
        { "on_create_npc_delayed",           &on_create_npc_delayed           },
        { "on_map_initialize",               &on_map_initialize               },
        { "on_craft",                        &on_craft                        },
        { "on_mapvar_changed",               &on_mapvar_changed               },
        { "on_worldvar_changed",             &on_worldvar_changed             },
        { "on_mapupdate",                    &on_mapupdate                    },
        { "get_item_class",                  &get_item_class                  },
        { "get_monster_class",               &get_monster_class               },
        { "get_status_effect",               &get_status_effect               },
        { "npc_create",                      &npc_create                      },
        { "npc_message",                     &npc_message                     },
        { "npc_choice",                      &npc_choice                      },
        { "npc_trade",                       &npc_trade                       },
        { "npc_post",                        &npc_post                        },
        { "npc_enable",                      &npc_enable                      },
        { "npc_disable",                     &npc_disable                     },
        { "chr_warp",                        &chr_warp                        },
        { "chr_get_inventory",               &chr_get_inventory               },
        { "chr_inv_change",                  &chr_inv_change                  },
        { "chr_inv_count",                   &chr_inv_count                   },
        { "chr_get_equipment",               &chr_get_equipment               },
        { "chr_equip_slot",                  &chr_equip_slot                  },
        { "chr_equip_item",                  &chr_equip_item                  },
        { "chr_unequip_slot",                &chr_unequip_slot                },
        { "chr_unequip_item",                &chr_unequip_item                },
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
        { "chr_give_special",                &chr_give_special                },
        { "chr_has_special",                 &chr_has_special                 },
        { "chr_take_special",                &chr_take_special                },
        { "chr_set_special_recharge_speed",  &chr_set_special_recharge_speed  },
        { "chr_get_special_recharge_speed",  &chr_get_special_recharge_speed  },
        { "chr_set_special_mana",            &chr_set_special_mana            },
        { "chr_get_special_mana",            &chr_get_special_mana            },
        { "chr_kick",                        &chr_kick                        },
        { "exp_for_level",                   &exp_for_level                   },
        { "monster_create",                  &monster_create                  },
        { "monster_get_name",                &monster_get_name                },
        { "monster_get_id",                  &monster_get_id                  },
        { "monster_change_anger",            &monster_change_anger            },
        { "monster_remove",                  &monster_remove                  },
        { "being_apply_status",              &being_apply_status              },
        { "being_remove_status",             &being_remove_status             },
        { "being_has_status",                &being_has_status                },
        { "being_set_status_time",           &being_set_status_time           },
        { "being_get_status_time",           &being_get_status_time           },
        { "being_get_gender",                &being_get_gender                },
        { "being_set_gender",                &being_set_gender                },
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
        { "being_set_walkmask",              &being_set_walkmask              },
        { "being_get_walkmask",              &being_get_walkmask              },
        { "being_get_mapid",                 &being_get_mapid                 },
        { "posX",                            &posX                            },
        { "posY",                            &posY                            },
        { "trigger_create",                  &trigger_create                  },
        { "chat_message",                    &chat_message                    },
        { "get_beings_in_circle",            &get_beings_in_circle            },
        { "get_beings_in_rectangle",         &get_beings_in_rectangle         },
        { "get_character_by_name",           &get_character_by_name           },
        { "being_register",                  &being_register                  },
        { "effect_create",                   &effect_create                   },
        { "chr_shake_screen",                &chr_shake_screen                },
        { "test_tableget",                   &test_tableget                   },
        { "get_map_id",                      &get_map_id                      },
        { "get_map_property",                &get_map_property                },
        { "is_walkable",                     &is_walkable                     },
        { "map_get_pvp",                     &map_get_pvp                     },
        { "item_drop",                       &item_drop                       },
        { "item_get_name",                   &item_get_name                   },
        { "npc_ask_integer",                 &npc_ask_integer                 },
        { "npc_ask_string",                  &npc_ask_string                  },
        { "log",                             &log                             },
        { "get_distance",                    &get_distance                    },
        { "map_get_objects",                 &map_get_objects                 },
        { "announce",                        &announce                        },
        { "get_special_info",                &get_special_info                },
        { NULL, NULL }
    };
    lua_pushvalue(mRootState, LUA_GLOBALSINDEX);
    luaL_register(mRootState, NULL, callbacks);
    lua_pop(mRootState, 1);                     // pop the globals table

    static luaL_Reg const members_AttackInfo[] = {
        { "priority",                        &attack_get_priority             },
        { "cooldowntime",                    &attack_get_cooldowntime         },
        { "warmuptime",                      &attack_get_warmuptime           },
        { "reusetime",                       &attack_get_reusetime            },
        { "damage",                          &attack_get_damage               },
        { "on_attack",                       &attack_on_attack                },
        { NULL, NULL }
    };

    static luaL_Reg const members_Damage[] = {
        { "id",                              &damage_get_id                   },
        { "skill",                           &damage_get_skill                },
        { "base",                            &damage_get_base                 },
        { "delta",                           &damage_get_delta                },
        { "cth",                             &damage_get_cth                  },
        { "element",                         &damage_get_element              },
        { "type",                            &damage_get_type                 },
        { "is_truestrike",                   &damage_is_truestrike            },
        { "range",                           &damage_get_range                },
        { NULL, NULL }
    };

    static luaL_Reg const members_ItemClass[] = {
        { "on",                              &item_class_on                   },
        { "attacks",                         &item_class_attacks              },
        { NULL, NULL }
    };

    static luaL_Reg const members_MapObject[] = {
        { "property",                        &map_object_get_property         },
        { "bounds",                          &map_object_get_bounds           },
        { "name",                            &map_object_get_name             },
        { "type",                            &map_object_get_type             },
        { NULL, NULL }
    };

    static luaL_Reg const members_MonsterClass[] = {
        { "on_update",                       &monster_class_on_update         },
        { "on_damage",                       &monster_class_on_damage         },
        { "attacks",                         &monster_class_attacks           },
        { NULL, NULL }
    };

    static luaL_Reg const members_StatusEffect[] = {
        { "on_tick",                         &status_effect_on_tick           },
        { NULL, NULL }
    };

    static luaL_Reg const members_SpecialInfo[] = {
        { "name",                            &specialinfo_get_name            },
        { "needed_mana",                     &specialinfo_get_needed_mana     },
        { "rechargeable",                    &specialinfo_is_rechargeable     },
        { "on_use",                          &specialinfo_on_use              },
        { "on_recharged",                    &specialinfo_on_recharged        },
        { "category",                        &specialinfo_get_category        },
        { NULL, NULL}
    };

    LuaAttackInfo::registerType(mRootState, "Attack", members_AttackInfo);
    LuaDamage::registerType(mRootState, "Damage", members_Damage);
    LuaItemClass::registerType(mRootState, "ItemClass", members_ItemClass);
    LuaMapObject::registerType(mRootState, "MapObject", members_MapObject);
    LuaMonsterClass::registerType(mRootState, "MonsterClass", members_MonsterClass);
    LuaStatusEffect::registerType(mRootState, "StatusEffect", members_StatusEffect);
    LuaSpecialInfo::registerType(mRootState, "SpecialInfo", members_SpecialInfo);

    // Make script object available to callback functions.
    lua_pushlightuserdata(mRootState, const_cast<char *>(&registryKey));
    lua_pushlightuserdata(mRootState, this);
    lua_rawset(mRootState, LUA_REGISTRYINDEX);

    // Push the error handler to first index of the stack
    lua_getglobal(mRootState, "debug");
    lua_getfield(mRootState, -1, "traceback");
    lua_remove(mRootState, 1);                  // remove the 'debug' table

    loadFile("scripts/lua/libmana.lua");
}
