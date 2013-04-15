/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
 *  Copyright (C) 2010-2013  The Mana Developers
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
#include "game-server/combatcomponent.h"
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
#include "game-server/triggerareacomponent.h"
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
 * When you add or change a script binding please run the update script in the
 * docs repository!
 *
 * http://doc.manasource.org/scripting
 */

/** LUA_CATEGORY Callbacks (callbacks)
 * **Note:** You can only assign a **single** function as callback.
 * When setting a new function the old one will not be called anymore.
 * Some of this callbacks are already used for the libmana.lua. Be careful when
 * using those since they will most likely break your code in other places.
 */

/** LUA on_update_derived_attribute (callbacks)
 * on_update_derived_attribute(function ref)
 **
 * Will call the function ''ref'' when an attribute changed and other attributes
 * need recalculation. The function is expected to recalculate those then.
 *
 * **See:** [[attributes.xml]] for more info.
 *
 */
static int on_update_derived_attribute(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    BeingComponent::setUpdateDerivedAttributesCallback(getScript(s));
    return 0;
}


/** LUA on_recalculate_base_attribute (callbacks)
 * on_recalculate_base_attribute(function ref)
 **
 * Will call the function ''ref'' when an attribute base needs to be recalculated.
 * The function is expected to do this recalculation then. The engine only
 * triggers this for characters. However you can use the same function for
 * recalculating derived attributes in the
 * [[scripting#on_update_derived_attribute|on_update_derived_attribute]] callback.
 *
 * **See:** [[attributes.xml]] for more info.
 */
static int on_recalculate_base_attribute(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    BeingComponent::setRecalculateBaseAttributeCallback(getScript(s));
    return 0;
}

/** LUA on_character_death (callbacks)
 * on_character_death(function ref)
 **
 * on_character_death( function(Character*) ): void
 * Sets a listener function to the character death event.
 */
static int on_character_death(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    CharacterComponent::setDeathCallback(getScript(s));
    return 0;
}

/** LUA on_character_death_accept (callbacks)
 * on_character_death_accept(function ref)
 **
 * Will make sure that the function ''ref'' gets called with the character
 * as argument as soon a character either pressed the ok dialouge in the death
 * message or left the game while being dead.
 */
static int on_character_death_accept(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    CharacterComponent::setDeathAcceptedCallback(getScript(s));
    return 0;
}

/** LUA on_character_login (callbacks)
 * on_character_login(function ref)
 **
 * Will make sure that function ''ref'' gets called with the character
 * as argument as soon a character logged in.
 */
static int on_character_login(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    CharacterComponent::setLoginCallback(getScript(s));
    return 0;
}

/** LUA on_being_death (callbacks)
 * on_being_death(function ref)
 **
 * Will make sure that the function ''ref'' gets called with the being
 * as argument as soon a being dies.
 */
static int on_being_death(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    LuaScript::setDeathNotificationCallback(getScript(s));
    return 0;
}

/** LUA on_entity_remove (callbacks)
 * on_entity_remove(function ref)
 **
 * Will make sure that the function ''ref'' gets called with the being
 * as argument as soon a being gets removed from a map.
 */
static int on_entity_remove(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    LuaScript::setRemoveNotificationCallback(getScript(s));
    return 0;
}

/** LUA on_update (callbacks)
 * on_update(function ref)
 **
 * Will make sure that the function ''ref'' gets called every game tick.
 */
static int on_update(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    Script::setUpdateCallback(getScript(s));
    return 0;
}

/** LUA on_create_npc_delayed (callbacks)
 * on_create_npc_delayed(function ref)
 **
 * Will make sure that the function ''ref'' gets called with the
 * name, id, gender, x and y values as arguments of the npc when a npc should
 * be created at map init (Npcs defined directly in the map files use this).
 */
static int on_create_npc_delayed(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    Script::setCreateNpcDelayedCallback(getScript(s));
    return 0;
}

/** LUA on_map_initialize (callbacks)
 * on_map_initialize(function ref)
 **
 * Will make sure that the function ''ref'' gets called with the initialized
 * map as current map when the map is initialized.
 */
static int on_map_initialize(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    MapComposite::setInitializeCallback(getScript(s));
    return 0;
}

/** LUA on_craft (callbacks)
 * on_craft(function ref)
 **
 * Will make sure that the function ''ref'' gets called with the the crafting
 * character and a table with the recipes {(id, amount}) when a character
 * performs crafting.
 */
static int on_craft(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    ScriptManager::setCraftCallback(getScript(s));
    return 0;
}

/** LUA on_mapupdate (callbacks)
 * on_mapupdate(function ref)
 **
 * Will make sure that the function ''ref'' gets called with the the map id
 * as argument for each game tick and map.
 */
static int on_mapupdate(lua_State *s)
{
    luaL_checktype(s, 1, LUA_TFUNCTION);
    MapComposite::setUpdateCallback(getScript(s));
    return 0;
}


/** LUA_CATEGORY Creation and removal of stuff (creation)
 */

/** LUA npc_create (creation)
 * npc_create(string name, int spriteID, int gender, int x, int y,
 *            function talkfunct, function updatefunct)
 **
 * **Return value:** A handle to the created NPC.
 *
 * Creates a new NPC with the name ''name'' at the coordinates ''x'':''y''
 * which appears to the players with the appearence listed in their npcs.xml
 * under ''spriteID'' and the gender ''gender''. Every game tick the function
 * ''updatefunct'' is called with the handle of the NPC. When a character talks
 * to the NPC the function ''talkfunct'' is called with the NPC handle and the
 * character handle.
 *
 * For setting the gender you can use the constants defined in the
 * libmana-constants.lua:
 *
 * | 0 | GENDER_MALE         |
 * | 1 | GENDER_FEMALE       |
 * | 2 | GENDER_UNSPECIFIED  |
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

    NpcComponent *npcComponent = new NpcComponent(id);

    Entity *npc = new Entity(OBJECT_NPC);
    auto *actorComponent = new ActorComponent(*npc);
    npc->addComponent(actorComponent);
    auto *beingComponent = new BeingComponent(*npc);
    npc->addComponent(beingComponent);
    npc->addComponent(npcComponent);
    // some health so it doesn't spawn dead
    beingComponent->setAttribute(*npc, ATTR_MAX_HP, 100);
    beingComponent->setAttribute(*npc, ATTR_HP, 100);
    beingComponent->setName(name);
    beingComponent->setGender(getGender(gender));

    actorComponent->setWalkMask(Map::BLOCKMASK_WALL | Map::BLOCKMASK_MONSTER |
                                Map::BLOCKMASK_CHARACTER);
    npc->setMap(m);
    actorComponent->setPosition(*npc, Point(x, y));

    if (lua_isfunction(s, 6))
    {
        lua_pushvalue(s, 6);
        npcComponent->setTalkCallback(luaL_ref(s, LUA_REGISTRYINDEX));
    }

    if (lua_isfunction(s, 7))
    {
        lua_pushvalue(s, 7);
        npcComponent->setUpdateCallback(luaL_ref(s, LUA_REGISTRYINDEX));
    }

    GameState::enqueueInsert(npc);
    push(s, npc);
    return 1;
}

/** LUA npc_enable (creation)
 * npc_disable(handle npc)
 **
 * Re-enables an NPC that got disabled before.
 */
static int npc_enable(lua_State *s)
{
    Entity *npc = checkNpc(s, 1);
    npc->getComponent<NpcComponent>()->setEnabled(true);
    GameState::enqueueInsert(npc);
    return 0;
}

/** LUA npc_disable (creation)
 * npc_disable(handle npc)
 **
 * Disable an NPC.
 */
static int npc_disable(lua_State *s)
{
    Entity *npc = checkNpc(s, 1);
    npc->getComponent<NpcComponent>()->setEnabled(false);
    GameState::remove(npc);
    return 0;
}

/** LUA monster_create (creation)
 * monster_create(int monsterID, int x, int y)
 * monster_create(string monstername, int x, int y)
 **
 * **Return value:** A handle to the created monster.
 *
 * Spawns a new monster of type ''monsterID'' or ''monstername'' on the current
 * map on the pixel coordinates ''x'':''y''.
 */
static int monster_create(lua_State *s)
{
    MonsterClass *monsterClass = checkMonsterClass(s, 1);
    const int x = luaL_checkint(s, 2);
    const int y = luaL_checkint(s, 3);
    MapComposite *m = checkCurrentMap(s);

    Entity *monster = new Entity(OBJECT_MONSTER);
    auto *actorComponent = new ActorComponent(*monster);
    monster->addComponent(actorComponent);
    monster->addComponent(new BeingComponent(*monster));
    monster->addComponent(new MonsterComponent(*monster, monsterClass));
    monster->setMap(m);
    actorComponent->setPosition(*monster, Point(x, y));
    GameState::enqueueInsert(monster);

    push(s, monster);
    return 1;
}

/** LUA entity:remove (creation)
 * entity:remove()
 **
 * Removes the entity from its current map.
 */
static int entity_remove(lua_State *s)
{
    GameState::remove(LuaEntity::check(s, 1));
    return 0;
}

/** LUA trigger_create (creation)
 * trigger_create(int x, int y, int width, int height,
 *                function trigger_function, int arg, bool once)
 **
 * Creates a new trigger area with the given ''height'' and ''width'' in pixels
 * at the map position ''x'':''y'' in pixels. When a being steps into this area
 * the function  ''trigger_function'' is called with the being handle and
 * ''arg'' as arguments. When ''once'' is false the function is called every
 * game tick the being is inside the    area. When ''once'' is true it is only
 * called again when the being leaves and reenters the area.
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
        luaL_error(s, "trigger_create called with incorrect parameters.");
        return 0;
    }

    Script *script = getScript(s);
    MapComposite *m = checkCurrentMap(s, script);

    const bool once = lua_toboolean(s, 7);

    Script::Ref function;
    lua_pushvalue(s, 5);
    script->assignCallback(function);
    lua_pop(s, 1);

    Entity *triggerEntity = new Entity(OBJECT_OTHER, m);

    ScriptAction *action = new ScriptAction(script, function, id);
    Rectangle r = { x, y, width, height };
    TriggerAreaComponent *area = new TriggerAreaComponent(r, action, once);

    triggerEntity->addComponent(area);

    LOG_INFO("Created script trigger at " << x << "," << y
             << " (" << width << "x" << height << ") id: " << id);

    bool ret = GameState::insertOrDelete(triggerEntity);
    lua_pushboolean(s, ret);
    return 1;
}

/** LUA effect_create (creation)
 * effect_create(int id, int x, int y)
 * effect_create(int id, being b)
 **
 * Triggers the effect ''id'' from the clients effects.xml
 * (particle and/or sound) at map location ''x'':''y'' or on being ''b''.
 * This has no effect on gameplay.
 *
 * **Warning:** Remember that clients might switch off particle effects for
 * performance reasons. Thus you should not use this for important visual
 * input.
 */
static int effect_create(lua_State *s)
{
    const int id = luaL_checkint(s, 1);

    if (lua_isuserdata(s, 2))
    {
        // being mode
        Entity *b = checkBeing(s, 2);
        Effects::show(id, b);
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

/** LUA drop_item (creation)
 * drop_item(int x, int y, int id [, int number])
 * drop_item(int x, int y, string name[, int number])
 **
 * Creates an item stack on the floor.
 */
static int item_drop(lua_State *s)
{
    const int x = luaL_checkint(s, 1);
    const int y = luaL_checkint(s, 2);
    ItemClass *ic = checkItemClass(s, 3);
    const int number = luaL_optint(s, 4, 1);
    MapComposite *map = checkCurrentMap(s);

    Entity *item = Item::create(map, Point(x, y), ic, number);
    GameState::enqueueInsert(item);
    return 0;
}

/** LUA_CATEGORY Input and output (input)
 */

/** LUA say (input)
 * say(string message)
 **
 * **Warning:** May only be called from an NPC talk function.
 *
 * Shows an NPC dialog box on the screen of displaying the string ''message''.
 * Idles the current thread until the user click "OK".
 */
static int say(lua_State *s)
{
    const char *m = luaL_checkstring(s, 1);

    Script::Thread *thread = checkCurrentThread(s);
    Entity *npc = thread->getContext().npc;
    Entity *character = thread->getContext().character;
    if (!(npc && character))
        luaL_error(s, "not in npc conversation");

    MessageOut msg(GPMSG_NPC_MESSAGE);
    msg.writeInt16(npc->getComponent<ActorComponent>()->getPublicID());
    msg.writeString(m);
    gameHandler->sendTo(character, msg);

    thread->mState = Script::ThreadPaused;
    return lua_yield(s, 0);
}

/** LUA ask (input)
 * ask(item1, item2, ... itemN)
 **
 * **Return value:** Number of the option the player selected (starting with 1).
 *
 * **Warning:** May only be called from an NPC talk function.
 *
 * Shows an NPC dialog box on the users screen with a number of dialog options
 * to choose from. Idles the current thread until the user selects one or
 * aborts the current thread when the user clicks "cancel".
 *
 * Items are either strings or tables of strings (indices are ignored,
 * but presumed to be taken in order). So,
 * ''ask("A", {"B", "C", "D"}, "E")'' is the same as
 * ''ask("A", "B", "C", "D", "E")''.
 */
static int ask(lua_State *s)
{
    Script::Thread *thread = checkCurrentThread(s);
    Entity *npc = thread->getContext().npc;
    Entity *character = thread->getContext().character;
    if (!(npc && character))
        luaL_error(s, "not in npc conversation");

    MessageOut msg(GPMSG_NPC_CHOICE);
    msg.writeInt16(npc->getComponent<ActorComponent>()->getPublicID());
    for (int i = 1, i_end = lua_gettop(s); i <= i_end; ++i)
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
                    luaL_error(s, "ask called with incorrect parameters.");
                    return 0;
                }
                lua_pop(s, 1);
            }
        }
        else
        {
            luaL_error(s, "ask called with incorrect parameters.");
            return 0;
        }
    }
    gameHandler->sendTo(character, msg);

    thread->mState = Script::ThreadExpectingNumber;
    return lua_yield(s, 0);
}

/** LUA ask_number (input)
 * ask_number(min_num, max_num, [default_num])
 **
 * **Return value:** The number the player entered into the field.
 *
 * **Warning:** May only be called from an NPC talk function.
 *
 * Shows a dialog box to the user which allows him to choose a number between
 * ''min_num'' and ''max_num''. If ''default_num'' is set this number will be
 * uses as default.  Otherwise ''min_num'' will be the default.
 */
static int ask_number(lua_State *s)
{
    int min = luaL_checkint(s, 1);
    int max = luaL_checkint(s, 2);
    int defaultValue = luaL_optint(s, 3, min);

    Script::Thread *thread = checkCurrentThread(s);
    Entity *npc = thread->getContext().npc;
    Entity *character = thread->getContext().character;
    if (!(npc && character))
        luaL_error(s, "not in npc conversation");

    MessageOut msg(GPMSG_NPC_NUMBER);
    msg.writeInt16(npc->getComponent<ActorComponent>()->getPublicID());
    msg.writeInt32(min);
    msg.writeInt32(max);
    msg.writeInt32(defaultValue);
    gameHandler->sendTo(character, msg);

    thread->mState = Script::ThreadExpectingNumber;
    return lua_yield(s, 0);
}

/** LUA ask_string (input)
 * ask_string()
 **
 * **Return value:** The string the player entered.
 *
 * **Warning:** May only be called from an NPC talk function.
 *
 * Shows a dialog box to a user which allows him to enter a text.
 */
static int ask_string(lua_State *s)
{
    Script::Thread *thread = checkCurrentThread(s);
    Entity *npc = thread->getContext().npc;
    Entity *character = thread->getContext().character;
    if (!(npc && character))
        luaL_error(s, "not in npc conversation");

    MessageOut msg(GPMSG_NPC_STRING);
    msg.writeInt16(npc->getComponent<ActorComponent>()->getPublicID());
    gameHandler->sendTo(character, msg);

    thread->mState = Script::ThreadExpectingString;
    return lua_yield(s, 0);
}

/** LUA npc_post (input)
 * npc_post()
 **
 * Starts retrieving post. Better not try to use it so far.
 */
static int npc_post(lua_State *s)
{
    const Script::Context *context = getScript(s)->getContext();
    Entity *npc = context->npc;
    Entity *character = context->character;

    MessageOut msg(GPMSG_NPC_POST);
    msg.writeInt16(npc->getComponent<ActorComponent>()->getPublicID());
    gameHandler->sendTo(character, msg);

    return 0;
}

/** LUA entity:say (input)
 * entity:say(string message)
 **
 * Makes this entity (which can be a character, monster or NPC), speak the
 * string ''message'' as if it was entered by a player in the chat bar.
 */
static int entity_say(lua_State *s)
{
    Entity *actor = checkActor(s, 1);
    const char *message = luaL_checkstring(s, 2);
    GameState::sayAround(actor, message);
    return 0;
}

/** LUA entity:message (input)
 * entity:message(string message)
 **
 * Delivers the string ''message'' to this entity (which needs to be a
 * character). It will appear in the chatlog as a private message from
 * "Server".
 */
static int entity_message(lua_State *s)
{
    Entity *character = checkCharacter(s, 1);
    const char *message = luaL_checkstring(s, 2);

    GameState::sayTo(character, nullptr, message);
    return 0;
}

/** LUA announce (input)
 * announce(string message [, string sender])
 **
 * Sends a global announce with the given ''message'' and ''sender''. If no
 * ''sender'' is passed "Server" will be used as sender.
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


/** LUA_CATEGORY Inventory interaction (inventory)
 */

/** LUA trade (inventory)
 * trade(bool mode,
 *       { int item1id, int item1amount, int item1cost }, ...,
 *       { int itemNid, int itemNamount, int itemNcost })
 * trade(bool mode,
 *       { string item1name, int item1amount, int item1cost }, ...,
 *       { string itemNname, int itemNamount, int itemNcost })
 **
 * FIXME: Move into a seperate file
 * Opens a trade window from an NPC conversation. ''mode''
 * is true for selling and false for buying. You have to set each items the NPC
 * is buying/selling,    the cost and the maximum amount in {}.
 *
 * **Note:** If the fourth parameters (table type) is omitted or invalid, and
 * the mode set to sell (true),
 * the whole player inventory is then sellable.
 *
 * **N.B.:** Be sure to put a ''value'' (item cost) parameter in your items.xml
 * to permit the player to sell it when using this option.
 *
 * **Return values:**
 *   * **0** when a trade has been started
 *   * **1** when there is no buy/sellable items
 *   * **2** in case of errors.
 *
 * **Examples:**
 * <code lua trade.lua>
 *     -- "A buy sample."
 *     local buycase = trade(false, {
 *         {"Sword", 10, 20},
 *         {"Bow", 10, 30},
 *         {"Dagger", 10, 50}
 *     })
 *     if buycase == 0 then
 *       say("What do you want to buy?")
 *     elseif buycase == 1 then
 *       say("I've got no items to sell.")
 *     else
 *       say("Hmm, something went wrong... Ask a scripter to
 *       fix the buying mode!")
 *     end
 *
 * -- ...
 *
 *    -- "Example: Let the player sell only pre-determined items."
 *    local sellcase = trade(true, {
 *                      {"Sword", 10, 20},
 *                      {"Bow", 10, 30},
 *                      {"Dagger", 10, 200},
 *                      {"Knife", 10, 300},
 *                      {"Arrow", 10, 500},
 *                      {"Cactus Drink", 10, 25}
 *     })
 *     if sellcase == 0 then
 *       say("Here we go:")
 *     elseif sellcase == 1 then
 *       say("I'm not interested by your items.")
 *     else
 *       say("Hmm, something went wrong...")
 *       say("Ask a scripter to fix me!")
 *     end
 *
 * -- ...
 *
 *     -- "Example: Let the player sell every item with a 'value' parameter in
 *     the server's items.xml file
 *     local sellcase = trade(true)
 *     if sellcase == 0 then
 *       say("Ok, what do you want to sell:")
 *     elseif sellcase == 1 then
 *       say("I'm not interested by any of your items.")
 *     else
 *       say("Hmm, something went wrong...")
 *       say("Ask a scripter to fix me!")
 *     end
 * </code>
 */
static int trade(lua_State *s)
{
    const Script::Context *context = getScript(s)->getContext();
    Entity *npc = context->npc;
    Entity *character = context->character;

    luaL_argcheck(s, lua_isboolean(s, 1), 1, "boolean expected");
    bool sellMode = lua_toboolean(s, 1);

    BuySell *t = new BuySell(character, sellMode);
    if (!lua_istable(s, 2))
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

            if (t->start(npc))
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
            raiseWarning(s, "trade[Buy] called with invalid "
                         "or empty items table parameter.");
            t->cancel();
            lua_pushinteger(s, 2);
            return 1;
        }
    }

    int nbItems = 0;

    lua_pushnil(s);
    while (lua_next(s, 2))
    {
        if (!lua_istable(s, -1))
        {
            raiseWarning(s, "trade called with invalid "
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
                    raiseWarning(s, "trade called with incorrect "
                                 "item id or name.");
                    t->cancel();
                    lua_pushinteger(s, 2);
                    return 1;
                }
                v[0] = it->getDatabaseID();
            }
            else if (!lua_isnumber(s, -1))
            {
                raiseWarning(s, "trade called with incorrect parameters "
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
    if (t->start(npc))
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

/** LUA entity:inv_count (inventory)
 * entity:inv_count(bool inInventory, bool inEquipment,
 *                  int id1, ..., int idN)
 * entity:inv_count(bool inInventory, bool inEquipment,
 *                  string name1, ..., string nameN)
 **
 * Valid only for character entities.
 *
 * The boolean values ''inInventory'' and ''inEquipment'' make possible to
 * select whether equipped or carried items must be counted.
 *
 * **Return values:** A number of integers with the amount of items ''id'' or
 * ''name'' carried or equipped by the character.
 */
static int entity_inv_count(lua_State *s)
{
    Entity *q = checkCharacter(s, 1);
    if (!lua_isboolean(s, 2) || !lua_isboolean(s, 3))
    {
        luaL_error(s, "inv_count called with incorrect parameters.");
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

/** LUA entity:inv_change (inventory)
 * entity:inv_change(int id1, int number1, ..., int idN, numberN)
 * entity:inv_change(string name1, int number1, ..., string nameN, numberN)
 **
 * Valid only for character entities.
 *
 * Changes the number of items with the item ID ''id'' or ''name'' owned by
 * this character by ''number''. You can change any number of items with this
 * function by passing multiple ''id'' or ''name'' and ''number'' pairs.
 * A failure can be caused by trying to take items the character doesn't possess.
 *
 * **Return value:** Boolean true on success, boolean false on failure.
 *
 * **Warning:** When one of the operations fails the following operations are
 * ignored but these before are executed. For that reason you should always
 * check if the character possesses items you are taking away using
 * entity:inv_count.
 */
static int entity_inv_change(lua_State *s)
{
    Entity *q = checkCharacter(s, 1);
    int nb_items = (lua_gettop(s) - 1) / 2;
    Inventory inv(q);
    for (int i = 0; i < nb_items; ++i)
    {
        if (!lua_isnumber(s, i * 2 + 3))
        {
            luaL_error(s, "inv_change called with "
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
                LOG_WARN("inv_change removed more items than owned: "
                     << "character: "
                     << q->getComponent<BeingComponent>()->getName()
                     << " item id: " << id);
            }
        }
        else
        {
            nb = inv.insert(id, nb);
            if (nb)
            {
                const Point &position =
                        q->getComponent<ActorComponent>()->getPosition();
                Entity *item = Item::create(q->getMap(), position, ic, nb);
                GameState::enqueueInsert(item);
            }
        }
    }
    lua_pushboolean(s, 1);
    return 1;
}

/** LUA entity:inventory (inventory)
 * entity:inventory(): table[]{slot, item id, name, amount}
 **
 * Valid only for character entities.
 *
 * Used to get a full view of a character's inventory.
 * This is not the preferred way to know whether an item is in the character's
 * inventory:
 * Use entity:inv_count for simple cases.
 *
 * **Return value:** A table containing all the info about the character's
 * inventory. Empty slots are not listed.
 *
 * **Example of use:**
 * <code lua>
 * local inventory_table = ch:inventory()
 * for i = 1, #inventory_table do
 *     item_message = item_message.."\n"..inventory_table[i].slot..", "
 *         ..inventory_table[i].id..", "..inventory_table[i].name..", "
 *         ..inventory_table[i].amount
 * end
 * </code>
 */
static int entity_get_inventory(lua_State *s)
{
    Entity *q = checkCharacter(s, 1);

    // Create a lua table with the inventory ids.
    const InventoryData invData = q->getComponent<CharacterComponent>()
            ->getPossessions().getInventory();

    lua_newtable(s);
    int firstTableStackPosition = lua_gettop(s);
    int tableIndex = 1;

    for (InventoryData::const_iterator it = invData.begin(),
        it_end = invData.end(); it != it_end; ++it)
    {
        if (!it->second.itemId || !it->second.amount)
            continue;

        // Create the sub-table (value of the main one)
        lua_createtable(s, 0, 4);
        int subTableStackPosition = lua_gettop(s);
        // Stores the item info in it.
        lua_pushliteral(s, "slot");
        lua_pushinteger(s, it->first); // The slot id
        lua_settable(s, subTableStackPosition);

        lua_pushliteral(s, "id");
        lua_pushinteger(s, it->second.itemId);
        lua_settable(s, subTableStackPosition);

        lua_pushliteral(s, "name");
        push(s, itemManager->getItem(it->second.itemId)->getName());
        lua_settable(s, subTableStackPosition);

        lua_pushliteral(s, "amount");
        lua_pushinteger(s, it->second.amount);
        lua_settable(s, subTableStackPosition);

        // Add the sub-table as value of the main one.
        lua_rawseti(s, firstTableStackPosition, tableIndex);
        ++tableIndex;
    }

    return 1;
}

/** LUA entity:equipment (inventory)
 * entity:equipment(): table[](slot, item id, name)}
 **
 * Valid only for character entities.
 *
 * Used to get a full view of a character's equipment.
 * This is not the preferred way to know whether an item is equipped:
 * Use entity:inv_count for simple cases.
 *
 * **Return value:** A table containing all the info about the character's
 * equipment. Empty slots are not listed.
 *
 * **Example of use:**
 * <code lua>
 * local equipment_table = ch:equipment()
 * for i = 1, #equipment_table do
 *     item_message = item_message.."\n"..equipment_table[i].slot..", "
 *         ..equipment_table[i].id..", "..equipment_table[i].name
 * end
 * </code>
 */
static int entity_get_equipment(lua_State *s)
{
    Entity *q = checkCharacter(s, 1);

    // Create a lua table with the inventory ids.
    const EquipData equipData = q->getComponent<CharacterComponent>()
            ->getPossessions().getEquipment();

    lua_newtable(s);
    int firstTableStackPosition = lua_gettop(s);
    int tableIndex = 1;

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
        lua_pushliteral(s, "slot");
        lua_pushinteger(s, it->first); // The slot id
        lua_settable(s, subTableStackPosition);

        lua_pushliteral(s, "id");
        lua_pushinteger(s, it->second.itemId);
        lua_settable(s, subTableStackPosition);

        lua_pushliteral(s, "name");
        push(s, itemManager->getItem(it->second.itemId)->getName());
        lua_settable(s, subTableStackPosition);

        // Add the sub-table as value of the main one.
        lua_rawseti(s, firstTableStackPosition, tableIndex);
        ++tableIndex;
    }

    return 1;
}

/** LUA entity:equip_slot (inventory)
 * entity:equip_slot(int slot)
 **
 * Valid only for character entities.
 *
 * Makes the character equip the item in the given inventory slot.
 */
static int entity_equip_slot(lua_State *s)
{
    Entity *ch = checkCharacter(s, 1);
    int inventorySlot = luaL_checkint(s, 2);

    Inventory inv(ch);
    lua_pushboolean(s, inv.equip(inventorySlot));
    return 1;
}

/** LUA entity:equip_item (inventory)
 * entity:equip_item(int item_id)
 * entity:equip_item(string item_name)
 **
 * Valid only for character entities.
 *
 * Makes the character equip the item id when it exists in the player's
 * inventory.
 *
 * **Return value:** true if equipping suceeded. false otherwise.
 */
static int entity_equip_item(lua_State *s)
{
    Entity *ch = checkCharacter(s, 1);
    ItemClass *it = checkItemClass(s, 2);

    Inventory inv(ch);

    int inventorySlot = inv.getFirstSlot(it->getDatabaseID());
    bool success = false;

    if (inventorySlot > -1)
        success = inv.equip(inventorySlot);

    lua_pushboolean(s, success);
    return 1;
}

/** LUA entity:unequip_slot (inventory)
 * entity:unequip_slot(int slot)
 **
 * Valid only for character entities.
 *
 * Makes the character unequip the item in the given equipment slot.
 *
 * **Return value:** true upon success. false otherwise.
 */
static int entity_unequip_slot(lua_State *s)
{
    Entity *ch = checkCharacter(s, 1);
    int equipmentSlot = luaL_checkint(s, 2);

    Inventory inv(ch);

    lua_pushboolean(s, inv.unequip(inv.getSlotItemInstance(equipmentSlot)));
    return 1;
}

/** LUA entity:unequip_item (inventory)
 * entity:unequip_item(int item_id)
 * entity:unequip_item(string item_name)
 **
 * Valid only for character entities.
 *
 * Makes the character unequip the item(s) corresponding to the id when it
 * exists in the player's equipment.
 *
 * **Return value:** true when every item were unequipped from equipment.
 */
static int entity_unequip_item(lua_State *s)
{
    Entity *ch = checkCharacter(s, 1);
    ItemClass *it = checkItemClass(s, 2);

    Inventory inv(ch);
    lua_pushboolean(s, inv.unequipItem(it->getDatabaseID()));
    return 1;
}

/** LUA_CATEGORY Character and being interaction (being)
 */

/** LUA chr_get_quest (being)
 * chr_get_quest(handle character, string name)
 **
 * **Return value:** The quest variable named ''name'' for the given character.
 *
 * **Warning:** May only be called from an NPC talk function.
 *
 */
static int chr_get_quest(lua_State *s)
{
    Entity *q = checkCharacter(s, 1);
    const char *name = luaL_checkstring(s, 2);
    luaL_argcheck(s, name[0] != 0, 2, "empty variable name");

    Script::Thread *thread = checkCurrentThread(s);

    std::string value;
    bool res = getQuestVar(q, name, value);
    if (res)
    {
        push(s, value);
        return 1;
    }
    QuestCallback *f = new QuestThreadCallback(&LuaScript::getQuestCallback,
                                               getScript(s));
    recoverQuestVar(q, name, f);

    thread->mState = Script::ThreadExpectingString;
    return lua_yield(s, 0);
}

/** LUA chr_set_quest (being)
 * chr_set_quest(handle character, string name, string value)
 **
 * Sets the quest variable named ''name'' for the given  character to the value
 * ''value''.
 */
static int chr_set_quest(lua_State *s)
{
    Entity *q = checkCharacter(s, 1);
    const char *name = luaL_checkstring(s, 2);
    const char *value = luaL_checkstring(s, 3);
    luaL_argcheck(s, name[0] != 0, 2, "empty variable name");

    setQuestVar(q, name, value);
    return 0;
}

/** LUA entity:set_special_recharge_speed (being)
 * entity:set_special_recharge_speed(int specialid, int new_speed)
 * entity:set_special_recharge_speed(string specialname, int new_speed)
 **
 * Valid only for character entities.
 *
 * Sets the recharge speed of the special to a new value for the character.
 *
 * **Note:** When passing the ''specialname'' as parameter make sure that it is
 * formatted in this way: <setname>_<specialname> (for eg. "Magic_Healingspell").
 */
static int entity_set_special_recharge_speed(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);
    const int speed = luaL_checkint(s, 3);

    if (!c->getComponent<CharacterComponent>()
            ->setSpecialRechargeSpeed(special, speed))
    {
        luaL_error(s,
                   "set_special_recharge_speed called with special "
                   "that is not owned by character.");
    }
    return 0;
}

/** LUA entity:special_recharge_speed (being)
 * entity:special_recharge_speed(int specialid)
 * entity:special_recharge_speed(string specialname)
 **
 * Valid only for character entities.
 *
 * **Return value:** The current recharge speed of the special that is owned by
 * the character.
 *
 * **Note:** When passing the ''specialname'' as parameter make sure that it is
 * formatted in this way: <setname>_<specialname> (for eg. "Magic_Healingspell").
 */
static int entity_get_special_recharge_speed(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);

    auto *characterComponent = c->getComponent<CharacterComponent>();

    SpecialMap::iterator it = characterComponent->findSpecial(special);

    luaL_argcheck(s, it != characterComponent->getSpecialEnd(), 2,
                  "character does not have special");

    lua_pushinteger(s, it->second.rechargeSpeed);
    return 1;
}

/** LUA entity:set_special_mana (being)
 * entity:set_special_mana(int specialid, int new_mana)
 * entity:set_special_mana(string specialname, int new_mana)
 **
 * Valid only for character entities.
 *
 * Sets the mana (recharge status) of the special to a new value for the
 * character.
 *
 * **Note:** When passing the ''specialname'' as parameter make sure that it is
 * formatted in this way: <setname>_<specialname> (for eg. "Magic_Healingspell").
 */
static int entity_set_special_mana(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);
    const int mana = luaL_checkint(s, 3);
    if (!c->getComponent<CharacterComponent>()->setSpecialMana(special, mana))
    {
        luaL_error(s,
                   "set_special_mana called with special "
                   "that is not owned by character.");
    }
    return 0;
}

/** LUA entity:special_mana (being)
 * entity:special_mana(int specialid)
 * entity:special_mana(string specialname)
 **
 * **Return value:** The mana (recharge status) of the special that is owned by
 * the character.
 *
 * **Note:** When passing the ''specialname'' as parameter make sure that it is
 * formatted in this way: <setname>_<specialname> (for eg. "Magic_Healingspell").
 */
static int entity_get_special_mana(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    auto *characterComponent = c->getComponent<CharacterComponent>();
    const int special = checkSpecial(s, 2);
    SpecialMap::iterator it = characterComponent->findSpecial(special);
    luaL_argcheck(s, it != characterComponent->getSpecialEnd(), 2,
                  "character does not have special");
    lua_pushinteger(s, it->second.currentMana);
    return 1;
}

/** LUA entity:walk (being)
 * entity:walk(int pixelX, int pixelY [, int walkSpeed])
 **
 * Valid only for being entities.
 *
 * Set the desired destination in pixels for the being.
 *
 * The optional **'WalkSpeed'** is to be given in tiles per second. The average
 * speed is 6.0 tiles per second. If no speed is given the default speed of the
 * being is used.
 */
static int entity_walk(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    const int x = luaL_checkint(s, 2);
    const int y = luaL_checkint(s, 3);

    auto *beingComponent = being->getComponent<BeingComponent>();

    beingComponent->setDestination(*being, Point(x, y));

    if (lua_gettop(s) >= 4)
    {
        const double speedTps = luaL_checknumber(s, 4);
        beingComponent->setAttribute(*being, ATTR_MOVE_SPEED_TPS, speedTps);
        const double modifiedSpeedTps =
                beingComponent->getModifiedAttribute(ATTR_MOVE_SPEED_TPS);
        beingComponent->setAttribute(*being, ATTR_MOVE_SPEED_RAW,
                                     utils::tpsToRawSpeed(modifiedSpeedTps));
    }

    return 0;
}

/** LUA entity:damage (being)
 * entity:damage(int damage, int delta,
 *               int accuracy, int type, int element)
 * entity:damage(int damage, int delta, int accuracy,
 *               int type, int element, handle source)
 * entity:damage(int damage, int delta, int accuracy,
 *               int type, int element, handle source, int skill)
 **
 * Valid only for being entities.
 *
 * Inflicts damage to the being. The severity of the attack is between
 * ''damage'' and (''damage'' + ''delta'') and is calculated using the normal
 * [[damage calculation]] rules. The being has a chance to
 * [[hitting and dodging|dodge the attack]] with its
 * [[attributes|agility attribute]]. The ''accuracy'' decides how hard this is.
 *
 * If ''source'' is provided the attack is handled as if the ''source''
 * triggered the damage.
 * ''type'' affects which kind of armor and character
 * attributes reduce the damage. It can be one of the following values:
>>>>>>> Removed skills
 *
 * | 0 | DAMAGE_PHYSICAL  |
 * | 1 | DAMAGE_MAGICAL  |
 * | 2 | DAMAGE_OTHER  |
 *
 * ''element'' decides how the [[element system]] changes the damage. The
 * following values are possible:
 * | 0 | ELEMENT_NEUTRAL  |
 * | 1 | ELEMENT_FIRE  |
 * | 2 | ELEMENT_WATER  |
 * | 3 | ELEMENT_EARTH  |
 * | 4 | ELEMENT_AIR  |
 * | 5 | ELEMENT_LIGHTNING  |
 * | 6 | ELEMENT_METAL  |
 * | 7 | ELEMENT_WOOD  |
 * | 8 | ELEMENT_ICE  |
 *
 * **Return Value**: Actual HP reduction resulting from the attack.
 */
static int entity_damage(lua_State *s)
{
    Entity *being = checkBeing(s, 1);

    if (!being->canFight())
    {
        luaL_error(s, "damage called with victim that cannot fight");
        return 0;
    }

    Damage dmg;
    dmg.base = luaL_checkint(s, 2);
    dmg.delta = luaL_checkint(s, 3);
    dmg.cth = luaL_checkint(s, 4);
    dmg.type = (DamageType)luaL_checkint(s, 5);
    dmg.element = (Element)luaL_checkint(s, 6);
    Entity *source = 0;
    if (lua_gettop(s) >= 7)
    {
        source = checkBeing(s, 7);

        if (!source->canFight())
        {
            luaL_error(s, "damage called with source that cannot fight");
            return 0;
        }
    }
    being->getComponent<CombatComponent>()->damage(*being, source, dmg);

    return 0;
}

/** LUA entity:heal (being)
 * entity:heal([int value])
 **
 * Valid only for being entities.
 *
 * Restores ''value'' lost hit points to the being. Value can be omitted to
 * restore the being to full hit points.
 *
 * While you can (ab)use this function to hurt a being by using a negative
 * value you should rather use entity:damage for this purpose.
 */
static int entity_heal(lua_State *s)
{
    Entity *being = checkBeing(s, 1);

    if (lua_gettop(s) == 1) // when there is only one argument
        being->getComponent<BeingComponent>()->heal(*being);
    else
        being->getComponent<BeingComponent>()->heal(*being, luaL_checkint(s, 2));

    return 0;
}

/** LUA entity:name (being)
 * entity:name()
 **
 * Valid only for being entities.
 *
 * **Return value:** Name of the being.
 */
static int entity_get_name(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    push(s, being->getComponent<BeingComponent>()->getName());
    return 1;
}

/** LUA entity:type (being)
 * entity:type()
 **
 * **Return value:** Type of the given entity. These type constants are defined
 * in libmana-constants.lua:
 *
 * | 0 | TYPE_ITEM      |
 * | 1 | TYPE_ACTOR     |
 * | 2 | TYPE_NPC       |
 * | 3 | TYPE_MONSTER   |
 * | 4 | TYPE_CHARACTER |
 * | 5 | TYPE_EFFECT    |
 * | 6 | TYPE_OTHER     |
*/
static int entity_get_type(lua_State *s)
{
    Entity *entity = LuaEntity::check(s, 1);
    lua_pushinteger(s, entity->getType());
    return 1;
}

/** LUA entity:action (being)
 * entity:action()
 **
 * Valid only for being entities.
 *
 * **Return value:** Current action of the being. These action constants are
 * defined in libmana-constants.lua:
 *
 * | 0 | ACTION_STAND  |
 * | 1 | ACTION_WALK   |
 * | 2 | ACTION_ATTACK |
 * | 3 | ACTION_SIT    |
 * | 4 | ACTION_DEAD   |
 * | 5 | ACTION_HURT   |
 */
static int entity_get_action(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    lua_pushinteger(s, being->getComponent<BeingComponent>()->getAction());
    return 1;
}

/** LUA entity:set_action (being)
 * entity:set_action(int action)
 **
 * Valid only for being entities.
 *
 * Sets the current action for the being.
 */
static int entity_set_action(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    BeingAction act = static_cast<BeingAction>(luaL_checkint(s, 2));
    being->getComponent<BeingComponent>()->setAction(*being, act);
    return 0;
}

/** LUA entity:direction (being)
 * entity:direction()
 **
 * Valid only for being entities.
 *
 * **Return value:** Current direction of the being. These direction constants
 * are defined in libmana-constants.lua:
 *
 * | 0 | DIRECTION_DEFAULT |
 * | 1 | DIRECTION_UP      |
 * | 2 | DIRECTION_DOWN    |
 * | 3 | DIRECTION_LEFT    |
 * | 4 | DIRECTION_RIGHT   |
 * | 5 | DIRECTION_INVALID |
 */
static int entity_get_direction(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    lua_pushinteger(s, being->getComponent<BeingComponent>()->getDirection());
    return 1;
}

/** LUA entity:set_direction (being)
 * entity:set_direction(int direction)
 **
 * Valid only for being entities.
 *
 * Sets the current direction of the given being. Directions are same as in
 * ''entity:direction''.
 */
static int entity_set_direction(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    BeingDirection dir = static_cast<BeingDirection>(luaL_checkint(s, 2));
    being->getComponent<BeingComponent>()->setDirection(*being, dir);
    return 0;
}

/** LUA entity:set_walkmask (being)
 * entity:set_walkmask(string mask)
 **
 * Valid only for actor entities.
 *
 * Sets the walkmasks of an actor. The mask is a set of characters which stand
 * for different collision types.
 *
 * | w | Wall      |
 * | c | Character |
 * | m | Monster   |
 *
 * This means entity:set_walkmask("wm") will prevent the being from walking
 * over walls and monsters.
 */
static int entity_set_walkmask(lua_State *s)
{
   Entity *being = checkActor(s, 1);
   const char *stringMask = luaL_checkstring(s, 2);
   unsigned char mask = 0x00;
   if (strchr(stringMask, 'w'))
       mask |= Map::BLOCKMASK_WALL;
   else if (strchr(stringMask, 'c'))
       mask |= Map::BLOCKMASK_CHARACTER;
   else if (strchr(stringMask, 'm'))
       mask |= Map::BLOCKMASK_MONSTER;
   being->getComponent<ActorComponent>()->setWalkMask(mask);
   return 0;
}

/** LUA entity:walkmask (being)
 * entity:walkmask()
 **
 * Valid only for actor entities.
 *
 * **Return value:** The walkmask of the actor formatted as string. (See
 * [[scripting#entityset_walkmask|entity:set_walkmask]])
 */
static int entity_get_walkmask(lua_State *s)
{
   Entity *being = checkBeing(s, 1);
   const unsigned char mask =
           being->getComponent<ActorComponent>()->getWalkMask();
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

/** LUA entity:warp (being)
 * entity:warp(int mapID, int posX, int posY)
 * entity:warp(string mapName, int posX, int posY)
 **
 * Valid only for character entities.
 *
 * Teleports the character to the position ''posX'':''posY'' on the map
 * with the ID number ''mapID'' or name ''mapName''. The ''mapID'' can be
 * substituted by ''nil'' to warp the character to a new position on the
 * current map.
 */
static int entity_warp(lua_State *s)
{
    Entity *character = checkCharacter(s, 1);
    int x = luaL_checkint(s, 3);
    int y = luaL_checkint(s, 4);

    bool b = lua_isnil(s, 2);
    if (!(b || lua_isnumber(s, 2) || lua_isstring(s, 2)))
    {
        luaL_error(s, "warp called with incorrect parameters.");
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
        LOG_INFO("warp called with a non-walkable place.");
        do
        {
            x = rand() % map->getWidth();
            y = rand() % map->getHeight();
        } while (!map->getWalk(x, y) && --c);
        x *= map->getTileWidth();
        y *= map->getTileHeight();
    }
    GameState::enqueueWarp(character, m, Point(x, y));

    return 0;
}

/** LUA entity:position (being)
 * entity:position()
 **
 * Valid only for actor entities.
 *
 * **Return value:** The x and y position of the actor in pixels, measured from
 * the top-left corner of the map it is currently on.
 */
static int entity_get_position(lua_State *s)
{
    Entity *being = checkActor(s, 1);
    const Point &p = being->getComponent<ActorComponent>()->getPosition();
    lua_pushinteger(s, p.x);
    lua_pushinteger(s, p.y);
    return 2;
}

/** LUA entity:x (being)
 * entity:x()
 **
 * Valid only for actor entities.
 *
 * **Return value:** The x position of the actor in pixels, measured from
 * the left border of the map it is currently on.
 */
static int entity_get_x(lua_State *s)
{
    Entity *being = checkActor(s, 1);
    const Point &p = being->getComponent<ActorComponent>()->getPosition();
    lua_pushinteger(s, p.x);
    return 1;
}

/** LUA entity:y (being)
 * entity:y()
 **
 * Valid only for actor entities.
 *
 * **Return value:** The y position of the actor in pixels, measured from
 * the top border of the map it is currently on.
 */
static int entity_get_y(lua_State *s)
{
    Entity *being = checkActor(s, 1);
    const Point &p = being->getComponent<ActorComponent>()->getPosition();
    lua_pushinteger(s, p.y);
    return 1;
}

/** LUA entity:base_attribute (being)
 * entity:base_attribute(int attribute_id)
 **
 * Valid only for being entities.
 *
 * **Return value:** Returns the value of the being's ''base attribute''.
 */
static int entity_get_base_attribute(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    int attr = luaL_checkint(s, 2);
    luaL_argcheck(s, attr > 0, 2, "invalid attribute id");

    lua_pushinteger(s, being->getComponent<BeingComponent>()->getAttributeBase(attr));
    return 1;
}

/** LUA entity:set_base_attribute (being)
 * entity:set_base_attribute(int attribute_id, double new_value)
 **
 * Valid only for being entities.
 *
 * Set the value of the being's ''base attribute'' to the 'new_value' parameter
 * given. (It can be negative).
 */
static int entity_set_base_attribute(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    int attr = luaL_checkint(s, 2);
    double value = luaL_checknumber(s, 3);

    being->getComponent<BeingComponent>()->setAttribute(*being, attr, value);
    return 0;
}

/** entity:modified_attribute (being)
 * entity:modified_attribute(int attribute_id)
 **
 * Valid only for being entities.
 *
 * *Return value:** Returns the value of the being's ''modified attribute''.
 *
 * The modified attribute is equal to the base attribute + currently applied
 * modifiers.
 *
 * To get to know how to configure and create modifiers, you can have a look at
 * the [[attributes.xml]] file and at the [[#entityapply_attribute_modifier]]()
 * and [[#entityremove_attribute_modifier]]() lua functions.
 *
 * Note also that items, equipment, and monsters attacks can cause attribute
 * modifiers.
 *
 * FIXME: This functions about applying and removing modifiers are still WIP,
 * because some simplifications and renaming could occur.
 */
static int entity_get_modified_attribute(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    int attr = luaL_checkint(s, 2);
    luaL_argcheck(s, attr > 0, 2, "invalid attribute id");

    const double value =
            being->getComponent<BeingComponent>()->getModifiedAttribute(attr);
    lua_pushinteger(s, value);
    return 1;
}

/** LUA entity:apply_attribute_modifier (being)
 * entity:apply_attribute_modifier(int attribute_id, double value,
 *                                 unsigned int layer, [unsigned short duration,
 *                                 [unsigned int effect_id]])
 **
 * Valid only for being entities.
 *
 * **Parameters description:** \\
 *   * **value** (double): The modifier value (can be negative).
 *   * **layer** (unsigned int): The layer or level of the modifier.
 *     As modifiers are stacked on an attribute, the layer determines
 *     where the modifier will be inserted. Also, when adding a modifier,
 *     all the modifiers with an higher ayer value will also be recalculated.
 *   * **duration** (unsigned short): The modifier duration in ticks((A tick is
 *     equal to 100ms.)). If set to 0, the modifier is permanent.
 *   * **effect_id** (unsigned int): Set and keep that parameter when you want
 *     to retrieve the exact layer later. (FIXME: Check this.)
 */
static int entity_apply_attribute_modifier(lua_State *s)
{
    Entity *being   = checkBeing(s, 1);
    int attr        = luaL_checkint(s,2);
    double value    = luaL_checknumber(s, 3);
    int layer       = luaL_checkint(s, 4);
    int duration    = luaL_optint(s, 5, 0);
    int effectId    = luaL_optint(s, 6, 0);

    being->getComponent<BeingComponent>()->applyModifier(*being, attr, value,
                                                         layer, duration,
                                                         effectId);
    return 0;
}

/** LUA entity:remove_attribute_modifier (being)
 * entity:remove_attribute_modifier(int attribute_id,
 *                                  double value, unsigned int layer)
 **
 * Valid only for being entities.
 *
 * Permits to remove an attribute modifier by giving its value and its layer.
 */
static int entity_remove_attribute_modifier(lua_State *s)
{
    Entity *being   = checkBeing(s, 1);
    int attr        = luaL_checkint(s, 2);
    double value    = luaL_checknumber(s, 3);
    int layer       = luaL_checkint(s, 4);
    int effectId    = luaL_optint(s, 5, 0);

    being->getComponent<BeingComponent>()->removeModifier(*being, attr, value,
                                                          layer, effectId);
    return 0;
}

/** LUA entity:gender (being)
 * entity:gender()
 **
 * Valid only for being entities.
 *
 * **Return value:** The gender of the being. These gender constants are
 * defined in libmana-constants.lua:
 *
 * | 0 | GENDER_MALE        |
 * | 1 | GENDER_FEMALE      |
 * | 2 | GENDER_UNSPECIFIED |
 */
static int entity_get_gender(lua_State *s)
{
    Entity *b = checkBeing(s, 1);
    lua_pushinteger(s, b->getComponent<BeingComponent>()->getGender());
    return 1;
}

/** LUA entity:set_gender (being)
 * entity:set_gender(int gender)
 **
 * Valid only for being entities.
 *
 * Sets the gender of the being.
 *
 * The gender constants are defined in libmana-constants.lua:
 *
 * | 0 | GENDER_MALE        |
 * | 1 | GENDER_FEMALE      |
 * | 2 | GENDER_UNSPECIFIED |
 */
static int entity_set_gender(lua_State *s)
{
    Entity *b = checkBeing(s, 1);
    const int gender = luaL_checkinteger(s, 2);
    b->getComponent<BeingComponent>()->setGender(getGender(gender));
    return 0;
}

/** LUA entity:hair_color (being)
 * entity:hair_color()
 **
 * Valid only for character entities.
 *
 * **Return value:** The hair color ID of the character.
 */
static int entity_get_hair_color(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);

    lua_pushinteger(s, c->getComponent<CharacterComponent>()->getHairColor());
    return 1;
}

/** LUA entity:set_hair_color (being)
 * entity:set_hair_color(int color)
 **
 * Valid only for character entities.
 *
 * Sets the hair color ID of the character to ''color''.
 */
static int entity_set_hair_color(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const int color = luaL_checkint(s, 2);
    luaL_argcheck(s, color >= 0, 2, "invalid color id");

    c->getComponent<CharacterComponent>()->setHairColor(color);
    c->getComponent<ActorComponent>()->raiseUpdateFlags(
            UPDATEFLAG_LOOKSCHANGE);

    return 0;
}

/** LUA entity:hair_style (being)
 * entity:hair_style()
 **
 * Valid only for character entities.
 *
 * **Return value:** The hair style ID of the character.
 */
static int entity_get_hair_style(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);

    lua_pushinteger(s, c->getComponent<CharacterComponent>()->getHairStyle());
    return 1;
}

/** LUA entity:set_hair_style (being)
 * entity:set_hair_style(int style)
 **
 * Valid only for character entities.
 *
 * Sets the hair style ID of the character to ''style''.
 */
static int entity_set_hair_style(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const int style = luaL_checkint(s, 2);
    luaL_argcheck(s, style >= 0, 2, "invalid style id");

    c->getComponent<CharacterComponent>()->setHairStyle(style);
    c->getComponent<ActorComponent>()->raiseUpdateFlags(
            UPDATEFLAG_LOOKSCHANGE);
    return 0;
}

/** LUA entity:kill_count (being)
 * entity:kill_count(int monsterId)
 * entity:kill_count(string monsterName)
 * entity:kill_count(MonsterClass monsterClass)
 **
 * Valid only for character entities.
 *
 * **Return value:** The total number of monsters of the specy (passed either
 * as monster id, monster name or monster class) the character has killed
 * during its career.
 */
static int entity_get_kill_count(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    MonsterClass *monster = checkMonsterClass(s, 2);

    lua_pushinteger(s, c->getComponent<CharacterComponent>()->getKillCount(monster->getId()));
    return 1;
}

/** LUA entity:rights (being)
 * entity:rights()
 **
 * Valid only for character entities.
 *
 * **Return value:** The access level of the account of the character.
 */
static int entity_get_rights(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    lua_pushinteger(s, c->getComponent<CharacterComponent>()->getAccountLevel());
    return 1;
}

/** LUA entity:kick (being)
 * entity:kick()
 **
 * Valid only for character entities.
 *
 * Kicks the character.
 */
static int entity_kick(lua_State *s)
{
    Entity *ch = checkCharacter(s, 1);
    MessageOut kickmsg(GPMSG_CONNECT_RESPONSE);
    kickmsg.writeInt8(ERRMSG_ADMINISTRATIVE_LOGOFF);
    ch->getComponent<CharacterComponent>()->getClient()->disconnect(kickmsg);
    return 0;
}

/** LUA entity:mapid (being)
 * entity:mapid()
 **
 * **Return value:** the id of the map where the entity is located or nil if
 * there is none.
 */
static int entity_get_mapid(lua_State *s)
{
    Entity *entity = LuaEntity::check(s, 1);
    if (MapComposite *map = entity->getMap())
        lua_pushinteger(s, map->getID());
    else
        lua_pushnil(s);

    return 1;
}

/** LUA chr_request_quest (being)
 * chr_request_quest(handle character, string questvariable, Ref function)
 **
 * Requests the questvar from the account server. This will make it available in
 * the quest cache after some time. The passed function will be called back as
 * soon the quest var is available.
 */
static int chr_request_quest(lua_State *s)
{
    Entity *ch = checkCharacter(s, 1);
    const char *name = luaL_checkstring(s, 2);
    luaL_argcheck(s, name[0] != 0, 2, "empty variable name");
    luaL_checktype(s, 3, LUA_TFUNCTION);

    std::string value;
    bool res = getQuestVar(ch, name, value);
    if (res)
    {
        // Already cached, call passed callback immediately
        Script *script = getScript(s);
        Script::Ref callback;
        script->assignCallback(callback);

        script->prepare(callback);
        script->push(ch);
        script->push(name);
        script->push(value);
        script->execute(ch->getMap());

        return 0;
    }

    QuestCallback *f = new QuestRefCallback(getScript(s), name);
    recoverQuestVar(ch, name, f);

    return 0;
}

/** LUA chr_try_get_quest (being)
 * chr_try_get_quest(handle character, string questvariable)
 **
 * Callback for checking if a quest variable is available in cache.
 *
 * **Return value:** It will return the variable if it is or nil
 * if it is not in cache.
 */
static int chr_try_get_quest(lua_State *s)
{
    Entity *q = checkCharacter(s, 1);
    const char *name = luaL_checkstring(s, 2);
    luaL_argcheck(s, name[0] != 0, 2, "empty variable name");

    std::string value;
    bool res = getQuestVar(q, name, value);
    if (res)
        push(s, value);
    else
        lua_pushnil(s);
    return 1;
}

/** LUA get_character_by_name (being)
 * get_character_by_name(string name)
 **
 * Tries to find an online character by name.
 *
 * **Return value** the character handle or nil if there is none.
 */
static int get_character_by_name(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    push(s, gameHandler->getCharacterByNameSlow(name));
    return 1;
}

/** LUA chr_get_post (being)
 * chr_get_post(handle character)
 **
 * Gets the post for the character.
 */
static int chr_get_post(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);

    Script *script = getScript(s);
    Script::Thread *thread = checkCurrentThread(s, script);

    PostCallback f = { &LuaScript::getPostCallback, script };
    postMan->getPost(c, f);

    thread->mState = Script::ThreadExpectingTwoStrings;
    return lua_yield(s, 0);
}

/** LUA entity:register (being)
 * entity:register()
 **
 * Makes the server call the on_being_death and on_entity_remove callbacks
 * when the being dies or the entity is removed from the map.
 *
 * **Note:** You should never need to call this in most situations. It is
 * handeled by the libmana.lua
 */
static int entity_register(lua_State *s)
{
    Entity *entity = LuaEntity::check(s, 1);
    Script *script = getScript(s);

    entity->signal_removed.connect(sigc::mem_fun(script, &Script::processRemoveEvent));

    if (BeingComponent *bc = entity->findComponent<BeingComponent>())
        bc->signal_died.connect(sigc::mem_fun(script, &Script::processDeathEvent));

    return 0;
}

/** LUA entity:shake_screen (being)
 * entity:shake_screen(int x, int y[, float strength, int radius])
 **
 * Valid only for character entities.
 *
 * Shakes the screen for a given character.
 */
static int entity_shake_screen(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const int x = luaL_checkint(s, 2);
    const int y = luaL_checkint(s, 3);

    MessageOut msg(GPMSG_SHAKE);
    msg.writeInt16(x);
    msg.writeInt16(y);

    if (lua_isnumber(s, 4))
        msg.writeInt16((int) (lua_tonumber(s, 4) * 10000));
    if (lua_isnumber(s, 5))
        msg.writeInt16(lua_tointeger(s, 5));

    c->getComponent<CharacterComponent>()->getClient()->send(msg);

    return 0;
}

/** LUA entity:show_text_particle (being)
 * entity:show_text_particle(string text)
 **
 * Valid only for character entities.
 *
 * Shows a text particle on a client. This effect is only visible for the
 * character.
 */
static int entity_show_text_particle(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const char *text = luaL_checkstring(s, 2);

    MessageOut msg(GPMSG_CREATE_TEXT_PARTICLE);
    msg.writeString(text);
    c->getComponent<CharacterComponent>()->getClient()->send(msg);

    return 0;
}

/** LUA entity:give_special (being)
 * entity:give_special(int special)
 **
 * Valid only for character entities.
 *
 * Enables a special for a character.
 */
static int entity_give_special(lua_State *s)
{
    // cost_type is ignored until we have more than one cost type
    Entity *c = checkCharacter(s, 1);
    const int special = checkSpecial(s, 2);
    const int currentMana = luaL_optint(s, 3, 0);

    c->getComponent<CharacterComponent>()->giveSpecial(special, currentMana);
    return 0;
}

/** LUA entity:has_special (being)
 * entity:has_special(int special)
 **
 * Valid only for character entities.
 *
 * **Return value:** True if the character has the special, false otherwise.
 */
static int entity_has_special(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const int special = luaL_checkint(s, 2);

    lua_pushboolean(s, c->getComponent<CharacterComponent>()->hasSpecial(special));
    return 1;
}

/** LUA entity:take_special (being)
 * entity:take_special(int special)
 **
 * Valid only for character entities.
 *
 * Removes a special from a character.
 *
 * **Return value:** True if removal was successful, false otherwise (in case
 * the character did not have the special).
 */
static int entity_take_special(lua_State *s)
{
    Entity *c = checkCharacter(s, 1);
    const int special = luaL_checkint(s, 2);

    CharacterComponent *cc = c->getComponent<CharacterComponent>();
    lua_pushboolean(s, cc->hasSpecial(special));
    cc->takeSpecial(special);
    return 1;
}


/** LUA_CATEGORY Monster (monster)
 */

/** LUA entity:monster_id (monster)
 * entity:monster_id()
 **
 * Valid only for monster entities.
 *
 * **Return value:** The id of the monster class.
 */
static int entity_get_monster_id(lua_State *s)
{
    Entity *monster = checkMonster(s, 1);
    MonsterComponent *monsterComponent = monster->getComponent<MonsterComponent>();
    lua_pushinteger(s, monsterComponent->getSpecy()->getId());
    return 1;
}

/** LUA entity:change_anger (monster)
 * entity:change_anger(handle being, int anger, int element)
 **
 * Valid only for monster entities.
 *
 * Makes the monster more angry about the ''being'' by adding ''anger'' to the
 * being.
 */
static int entity_change_anger(lua_State *s)
{
    Entity *monster = checkMonster(s, 1);
    Entity *being = checkBeing(s, 2);
    const int anger = luaL_checkint(s, 3);
    const int element = luaL_checkint(s, 4);
    monster->getComponent<MonsterComponent>()->changeAnger(being, (Element)element, anger);
    return 0;
}

/** LUA entity:drop_anger (monster)
 * entity:drop_anger(handle target)
 **
 * Valid only for monster entities.
 *
 * Will drop all anger against the ''target''.
 */
static int entity_drop_anger(lua_State *s)
{
    Entity *monster = checkMonster(s, 1);
    Entity *being = checkBeing(s, 2);
    monster->getComponent<MonsterComponent>()->forgetTarget(being);
    return 0;
}

/** LUA entity:get_angerlist (monster)
 * entity:get_angerlist()
 **
 * Valid only for monster entities.
 *
 * **Return value:** A table with the beings as key and the anger against them
 * as values.
 */
static int entity_get_angerlist(lua_State *s)
{
    Entity *monster = checkMonster(s, 1);
    MonsterComponent *monsterComponent =
            monster->getComponent<MonsterComponent>();
    pushSTLContainer(s, monsterComponent->getAngerList());
    return 1;
}


/** LUA_CATEGORY Status effects (statuseffects)
 */

/** LUA entity:apply_status (statuseffects)
 * entity:apply_status(int status_id, int time)
 **
 * Valid only for being entities.
 *
 * Gives a being a status effect ''status_id'', status effects don't work on
 * NPCs. ''time'' is in game ticks.
 */
static int entity_apply_status(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);
    const int time = luaL_checkint(s, 3);

    being->getComponent<BeingComponent>()->applyStatusEffect(id, time);
    return 0;
}

/** LUA entity:remove_status (statuseffects)
 * entity:remove_status(int status_id)
 **
 * Valid only for being entities.
 *
 * Removes a given status effect from a being.
 */
static int entity_remove_status(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);

    being->getComponent<BeingComponent>()->removeStatusEffect(id);
    return 0;
}

/** LUA entity:has_status (statuseffects)
 * entity:has_status(int status_id)
 **
 * Valid only for being entities.
 *
 * **Return value:** True if the being has a given status effect.
 */
static int entity_has_status(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);

    lua_pushboolean(s, being->getComponent<BeingComponent>()->hasStatusEffect(id));
    return 1;
}

/** LUA entity:status_time (statuseffects)
 * entity:status_time(int status_id)
 **
 * Valid only for being entities.
 *
 * **Return Value:** Number of ticks remaining on a status effect.
 */
static int entity_get_status_time(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);

    lua_pushinteger(s, being->getComponent<BeingComponent>()->getStatusEffectTime(id));
    return 1;
}

/** LUA entity:set_status_time (statuseffects)
 * entity:set_status_time(int status_id, int time)
 **
 * Valid only for being entities.
 *
 * Sets the time on a status effect a target being already has.
 */
static int entity_set_status_time(lua_State *s)
{
    Entity *being = checkBeing(s, 1);
    const int id = luaL_checkint(s, 2);
    const int time = luaL_checkint(s, 3);

    being->getComponent<BeingComponent>()->setStatusEffectTime(id, time);
    return 0;
}


/** LUA_CATEGORY Map information (mapinformation)
 */

/** LUA get_map_id (mapinformation)
 * get_map_id()
 **
 * **Return value:** The ID number of the map the script runs on.
 */
static int get_map_id(lua_State *s)
{
    Script *script = getScript(s);

    if (MapComposite *mapComposite = script->getContext()->map)
        lua_pushinteger(s, mapComposite->getID());
    else
        lua_pushnil(s);

    return 1;
}

/** LUA get_map_property (mapinformation)
 * get_map_property(string key)
 **
 * **Return value:** The value of the property ''key'' of the current map. The
 * string is empty if the property ''key'' does not exist.
 */
static int get_map_property(lua_State *s)
{
    const char *property = luaL_checkstring(s, 1);
    Map *map = checkCurrentMap(s)->getMap();

    push(s, map->getProperty(property));
    return 1;
}

/** LUA is_walkable (mapinformation)
 * is_walkable(int x, int y)
 **
 * **Return value:** True if ''x'':''y'' is a walkable pixel
 * on the current map.
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

/** LUA map_get_pvp (mapinformation)
 * map_get_pvp()
 **
 * **Return value:** The pvp situation of the map.
 *
 * There are constants for the different pvp situations in the libmana-constants.lua:
 *
 * | 0 | PVP_NONE  |
 * | 1 | PVP_FREE  |
 */
static int map_get_pvp(lua_State *s)
{
    MapComposite *m = checkCurrentMap(s);
    lua_pushinteger(s, m->getPvP());
    return 1;
}


/** LUA_CATEGORY Persistent variables (variables)
 */

/** LUA on_mapvar_changed (variables)
 * on_mapvar_changed(string key, function func)
 **
 * Registers a callback to the key. This callback will be called with the key
 * and value of the changed variable.
 *
 * **Example:**
 * <code lua>on_mapvar_changed(key, function(key, value)
 *   log(LOG_DEBUG, "mapvar " .. key .. " has new value " .. value)
 * end)</code>
 */
static int on_mapvar_changed(lua_State *s)
{
    const char *key = luaL_checkstring(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    luaL_argcheck(s, key[0] != 0, 2, "empty variable name");
    MapComposite *m = checkCurrentMap(s);
    m->setMapVariableCallback(key, getScript(s));
    return 0;
}

/** LUA on_worldvar_changed (variables)
 * on_worldvar_changed(string key, function func)
 **
 * Registers a callback to the key. This callback will be called with the key
 * and value of the changed variable.
 *
 * **Example:**
 * <code lua>on_worldvar_changed(key, function(key, value)
 *   log(LOG_DEBUG, "worldvar " .. key .. " has new value " .. value)
 * end)</code>
 */
static int on_worldvar_changed(lua_State *s)
{
    const char *key = luaL_checkstring(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    luaL_argcheck(s, key[0] != 0, 2, "empty variable name");
    MapComposite *m = checkCurrentMap(s);
    m->setWorldVariableCallback(key, getScript(s));
    return 0;
}

/** LUA getvar_map (variables)
 * getvar_map(string variablename)
 **
 * **Return value:** the value of a persistent map variable.
 *
 * **See:** [[scripting#map|map[]]] for an easier way to get a map variable.
 */
static int getvar_map(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    luaL_argcheck(s, name[0] != 0, 1, "empty variable name");

    MapComposite *map = checkCurrentMap(s);

    push(s, map->getVariable(name));
    return 1;
}

/** LUA setvar_map (variables)
 * setvar_map(string variablename, string value)
 **
 * Sets the value of a persistent map variable.
 *
 * **See:** [[scripting#map|map[]]] for an easier way to get a map variable.
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

/** LUA getvar_world (variables)
 * getvar_world(string variablename)
 **
 * Gets the value of a persistent global variable.
 *
 * **See:** [[scripting#world|world[]]] for an easier way to get a map variable.
 */
static int getvar_world(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    luaL_argcheck(s, name[0] != 0, 1, "empty variable name");

    push(s, GameState::getVariable(name));
    return 1;
}

/** LUA setvar_world (variables)
 * setvar_world(string variablename, string value)
 **
 * Sets the value of a persistent global variable.
 *
 * **See:** [[scripting#world|world[]]] for an easier way to get a map variable.
 */
static int setvar_world(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    const char *value = luaL_checkstring(s, 2);
    luaL_argcheck(s, name[0] != 0, 1, "empty variable name");

    GameState::setVariable(name, value);
    return 0;
}


/** LUA_CATEGORY Logging (logging)
 */

/** LUA log (logging)
 * log(int log_level, string message)
 **
 * Log something at the specified log level. The available log levels are:
 * | 0 | LOG_FATAL    |
 * | 1 | LOG_ERROR    |
 * | 2 | LOG_WARNING  |
 * | 3 | LOG_INFO     |
 * | 4 | LOG_DEBUG    |
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


/** LUA_CATEGORY Area of Effect (area)
 * In order to easily use area of effects in your items or in your scripts,
 * the following functions are available:
 */

/** LUA get_beings_in_circle (area)
 * get_beings_in_circle(int x, int y, int radius)
 * get_beings_in_circle(handle actor, int radius)
 **
 * **Return value:** This function returns a lua table of all beings in a
 * circle of radius (in pixels) ''radius'' centered either at the pixel at
 * (''x'', ''y'') or at the position of ''being''.
 */
static int get_beings_in_circle(lua_State *s)
{
    int x, y, r;
    if (lua_isuserdata(s, 1))
    {
        Entity *b = checkActor(s, 1);
        const Point &pos = b->getComponent<ActorComponent>()->getPosition();
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
        Entity *b = *i;
        char t = b->getType();
        if (t == OBJECT_NPC || t == OBJECT_CHARACTER || t == OBJECT_MONSTER)
        {
            auto *actorComponent = b->getComponent<ActorComponent>();
            if (Collision::circleWithCircle(actorComponent->getPosition(),
                                            actorComponent->getSize(),
                                            Point(x, y), r))
            {
                push(s, b);
                lua_rawseti(s, tableStackPosition, tableIndex);
                tableIndex++;
            }
        }
    }

    return 1;
}

/** LUA get_beings_in_rectangle (area)
 * get_beings_in_rectangle(int x, int y, int width, int height)
 **
 * **Return value:** An table of being entities within the rectangle.
 * All parameters have to be passed as pixels.
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
        Entity *b = *i;
        char t = b->getType();
        if ((t == OBJECT_NPC || t == OBJECT_CHARACTER || t == OBJECT_MONSTER) &&
            rect.contains(b->getComponent<ActorComponent>()->getPosition()))
        {
            push(s, b);
            lua_rawseti(s, tableStackPosition, tableIndex);
            tableIndex++;
        }
    }
     return 1;
 }

/** LUA get_distance (area)
 * get_distance(handle being1, handle being2)
 * get_distance(int x1, int y1, int x2, int y2)
 **
 * **Return value:** The distance between the two beings or the two points
 * in pixels.
 */
static int get_distance(lua_State *s)
{
    int x1, y1, x2, y2;
    if (lua_gettop(s) == 2)
    {
        Entity *being1 = checkBeing(s, 1);
        Entity *being2 = checkBeing(s, 2);

        x1 = being1->getComponent<ActorComponent>()->getPosition().x;
        y1 = being1->getComponent<ActorComponent>()->getPosition().y;
        x2 = being2->getComponent<ActorComponent>()->getPosition().x;
        y2 = being2->getComponent<ActorComponent>()->getPosition().y;
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


/** LUA_CATEGORY Special info class (specialinfo)
 * See the [[specials.xml#A script example|specials Documentation]] for a
 * script example
 */

/** LUA get_special_info (specialinfo)
 * get_special_info(int specialId)
 * get_special_info(string specialName)
 **
 * **Return value:** This function returns a object of the specialinfo class.
 * See below for usage of that object.
 *
 * **Note:** When passing the ''specialName'' as parameter make sure that it is
 * formatted in this way: <setname>_<specialname> (for eg. "Magic_Healingspell").
 */
static int get_special_info(lua_State *s)
{
    const int special = checkSpecial(s, 1);
    SpecialManager::SpecialInfo *info = specialManager->getSpecialInfo(special);
    luaL_argcheck(s, info, 1, "invalid special");
    LuaSpecialInfo::push(s, info);
    return 1;
}

/** LUA specialinfo:name (specialinfo)
 * specialinfo:name()
 **
 * ** Return value:** The name of the specialinfo object.
 *
 * **Note:** See [[scripting#get_special_info|get_special_info]] for getting a
 * specialinfo object.
 */
static int specialinfo_get_name(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    push(s, info->name);
    return 1;
}

/** LUA specialinfo:needed_mana (specialinfo)
 * specialinfo:needed_mana()
 **
 * ** Return value:** The mana that is needed to use the special
 *
 * **Note:** See [[scripting#get_special_info|get_special_info]] for getting a
 * specialinfo object.
 */
static int specialinfo_get_needed_mana(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    lua_pushinteger(s, info->neededMana);
    return 1;
}

/** LUA specialinfo:rechargeable (specialinfo)
 * specialinfo:rechargeable()
 **
 * ** Return value:** A boolean value that indicates whether the special is
 * rechargeable or usuable without recharge.
 *
 * **Note:** See [[scripting#get_special_info|get_special_info]] for getting
 * a specialinfo object.
 */
static int specialinfo_is_rechargeable(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    lua_pushboolean(s, info->rechargeable);
    return 1;
}

/** LUA specialinfo:on_use (specialinfo)
 * specialinfo:on_use(function callback)
 **
 * Assigns the ''callback'' as callback for the use event. This function will
 * be called everytime a character uses a special.
 *
 * **Note:** See [[scripting#get_special_info|get_special_info]] for getting
 * a specialinfo object.
 */
static int specialinfo_on_use(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    Script *script = getScript(s);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    script->assignCallback(info->useCallback);
    return 0;
}

/** LUA specialinfo:on_recharged (specialinfo)
 * specialinfo:on_recharged(function callback)
 **
 * Assigns the ''callback'' as callback for the recharged event. This function
 * will be called everytime when the special is fully recharged.
 *
 * **Note:** See [[scripting#get_special_info|get_special_info]] for getting
 * a specialinfo object.
 */
static int specialinfo_on_recharged(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    Script *script = getScript(s);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    script->assignCallback(info->rechargedCallback);
    return 0;
}

/** LUA specialinfo:category (specialinfo)
 * specialinfo:category(function callback)
 **
 * **Return value:** The set-name of the special as defined in the
 * [[specials.xml]]
 *
 * **Note:** See [[scripting#get_special_info|get_special_info]] for getting
 * a specialinfo object.
 */
static int specialinfo_get_category(lua_State *s)
{
    SpecialManager::SpecialInfo *info = LuaSpecialInfo::check(s, 1);
    push(s, info->setName);
    return 1;
}


/** LUA_CATEGORY Status effect class (statuseffectclass)
 */

/** LUA get_status_effect (statuseffectclass)
 * get_status_effect(string name)
 **
 * **Return value:** This function returns a object of the statuseffect class.
 * See below for usage of that object.
 */
static int get_status_effect(lua_State *s)
{
    const char *name = luaL_checkstring(s, 1);
    LuaStatusEffect::push(s, StatusManager::getStatusByName(name));
    return 1;
}

/** LUA statuseffect:on_tick (statuseffectclass)
 * statuseffect:on_tick(function callback)
 **
 * Sets the callback that gets called for every tick when the status effect
 * is active.
 *
 * **Note:** See [[scripting#get_status_effect|get_status_effect]] for getting
 * a statuseffect object.
 */
static int status_effect_on_tick(lua_State *s)
{
    StatusEffect *statusEffect = LuaStatusEffect::check(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    statusEffect->setTickCallback(getScript(s));
    return 0;
}

/** LUA_CATEGORY Monster class (monsterclass)
 */

/** LUA get_monster_class (monsterclass)
 * get_monster_class(int monsterid)
 * get_monster_class(string monstername)
 **
 * **Return value:** This function returns a object of the monster class.
 * See below for usage of that object.
 */
static int get_monster_class(lua_State *s)
{
    LuaMonsterClass::push(s, checkMonsterClass(s, 1));
    return 1;
}

/** LUA monsterclass:on_update (monsterclass)
 * monsterclass:on_update(function callback)
 **
 * Assigns the ''callback'' as callback for the monster update event. This
 * callback will be called every tick for each monster of that class.
 *
 * **Note:** See [[scripting#get_monster_class|get_monster_class]] for getting
 * a monsterclass object.
 */
static int monster_class_on_update(lua_State *s)
{
    MonsterClass *monsterClass = LuaMonsterClass::check(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    monsterClass->setUpdateCallback(getScript(s));
    return 0;
}

/** LUA monsterclass:on_damage (monsterclass)
 * monsterclass:on_damage(function callback)
 **
 * Assigns the ''callback'' as callback for the monster damage event.
 * This callback will be called every time when a monster takes damage.
 * The damage can be either invoked from scripts or from other beings such
 * as players. The parameters of the callback are: the attacked monster,
 * the being dealing the damage and the hp loss
 *
 * **Note:** See [[scripting#get_monster_class|get_monster_class]] for getting
 * a monsterclass object.
 *
 * **Example:** <code lua>
 * local function damage(mob, aggressor, hploss)
 *     mob:say("I took damage -.- ".. hploss)
 *     if aggressor then
 *         mob:say("Curse you, ".. aggressor:name())
 *     end
 * end
 * local maggot = get_monster_class("maggot")
 * maggot:on_damage(damage)</code>
 */
static int monster_class_on_damage(lua_State *s)
{
    MonsterClass *monsterClass = LuaMonsterClass::check(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    monsterClass->setDamageCallback(getScript(s));
    return 0;
}

/** LUA monsterclass:name (monsterclass)
 * monsterclass:name()
 **
 * **Return value:** The name of the monster class.
 */
static int monster_class_get_name(lua_State *s)
{
    MonsterClass *monsterClass = LuaMonsterClass::check(s, 1);
    push(s, monsterClass->getName());
    return 1;
}

/** LUA monsterclass:attacks (monsterclass)
 * monsterclass:attacks()
 **
 * **Return value:** This function returns a table with all attacks of the
 * monster. See the [[scripting#AttackInfo class|Attack Info]] section.
 */
static int monster_class_attacks(lua_State *s)
{
    MonsterClass *monsterClass = LuaMonsterClass::check(s, 1);
    pushSTLContainer(s, monsterClass->getAttackInfos());
    return 1;
}


/** LUA_CATEGORY AttackInfo class (attackinfoclass)
 * The AttackInfo class reveals info about attacks and provides functions to
 * register callbacks on attacks. See the
 * [[attackconfiguration|Attack Configuration]] for more info.
 * To get an AttackInfo use
 * [[scripting#monsterclass:attacks|monsterclass:attacks]] or
 * [[scripting#itemclass:attacks|itemclass:attacks]]
 */

/** LUA attackinfo:priority (attackinfoclass)
 * attackinfo:priority()
 **
 * **Return value:** This function returns the priority of the attack.
 */
static int attack_get_priority(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    lua_pushinteger(s, attack->getPriority());
    return 1;
}

/** LUA attackinfo:cooldowntime (attackinfoclass)
 * attackinfo:cooldowntime()
 **
 * **Return value:** This function returns the cooldowntime (time after dealing
 * damage after which a new attack can be used) of the attack.
 */
static int attack_get_cooldowntime(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    lua_pushinteger(s, attack->getCooldownTime());
    return 1;
}

/** LUA attackinfo:warmuptime (attackinfoclass)
 * attackinfo:warmuptime()
 **
 * **Return value:** This function returns the warmuptime (time before a attack
 * triggers damage after being used) of the attack.
 */
static int attack_get_warmuptime(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    lua_pushinteger(s, attack->getWarmupTime());
    return 1;
}

/** LUA attackinfo:reusetime (attackinfoclass)
 * attackinfo:reusetime()
 **
 * **Return value:** This function returns the reusetime (time after which the
 * same attack can be used again) of the attack.
 */
static int attack_get_reusetime(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    lua_pushinteger(s, attack->getReuseTime());
    return 1;
}

/** LUA attackinfo:damage (attackinfoclass)
 * >attackinfo:damage()
 **
 * **Return value:** This function returns the damage info of the attack.
 *
 * **See also:** [[scripting#Damage Class|Damage Class]]
 */
static int attack_get_damage(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    LuaDamage::push(s, &attack->getDamage());
    return 1;
}

/** LUA attackinfo:on_attack (attackinfoclass)
 * attackinfo:on_attack(function callback)
 **
 * Assigns a callback to the attack that will be called as soon the attack is
 * used. The callback will get called with the following parameters:
 * being user, being target, int damage_dealt.
 */
static int attack_on_attack(lua_State *s)
{
    AttackInfo *attack = LuaAttackInfo::check(s, 1);
    luaL_checktype(s, 2, LUA_TFUNCTION);
    attack->setCallback(getScript(s));
    return 0;
}


/** LUA_CATEGORY Damage Class (damageclass)
 * The Damage class provides info about the kind of damage attack deals.
 */

/** LUA damage:id (damageclass)
 * damage:id()
 **
 * **Return value:** This function returns the id of the attack.
 */
static int damage_get_id(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->id);
    return 1;
}

/** LUA damage:base (damageclass)
 * damage:base()
 **
 * **Return value:** This function returns the base damage of the attack.
 * It is the minimum of damage dealt.
 */
static int damage_get_base(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->base);
    return 1;
}

/** LUA damage:delta (damageclass)
 * damage:delta()
 **
 * **Return value:** This function returns the damage delta of the attack.
 * base damage + delta damage is the maximum of damage the attack can cause.
 * A number in between will be picked by random.
 */
static int damage_get_delta(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->delta);
    return 1;
}

/** LUA damage:cth (damageclass)
 * damage:cth()
 **
 * **Return value:** This function returns the chance to hit of the attack.
 * This number is not a percent value but some factor. Higher means a better
 * chance to hit. FIXME: Add info about the factor.
 */
static int damage_get_cth(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->cth);
    return 1;
}

/** LUA damage:element (damageclass)
 * damage:element()
 **
 * **Return value:** This function returns the element of the attack.
 *
 * **See:** [[scripting#entitydamage|entity:damage]] for possible values.
 */
static int damage_get_element(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->element);
    return 1;
}

/** LUA damage:type (damageclass)
 * damage:type()
 **
 * **Return value:** This function returns the type of the attack.
 *
 * **See:** [[scripting#entitydamage|entity:damage]] for possible values.
 */
static int damage_get_type(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->type);
    return 1;
}

/** LUA damage:is_truestrike (damageclass)
 * damage:is_truestrike()
 **
 * **Return value:** This function returns whether the attack is a true strike.
 * A true strike is not effected by chance of hit or anything that could
 * prevent the hit.
 */
static int damage_is_truestrike(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushboolean(s, damage->trueStrike);
    return 1;
}

/** LUA damage:range (damageclass)
 * damage:range()
 **
 * **Return value:** This function returns the range of the attack in pixels.
 */
static int damage_get_range(lua_State *s)
{
    Damage *damage = LuaDamage::check(s, 1);
    lua_pushinteger(s, damage->range);
    return 1;
}


/** LUA_CATEGORY Map object class (mapobjectclass)
 */

/** LUA map_get_objects (mapobjectclass)
 * map_get_objects()
 * map_get_objects(string type)
 **
 * **Return value:** A table of all objects or a table of all objects of the
 * given ''type''. See below for usage of these objects.
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

/** LUA mapobject:property (mapobjectclass)
 * mapobject:property(string key)
 **
 * **Return value:** The value of the property of the key ''key'' or nil if
 * the property does not exists.
 *
 * **Note:** See [[scripting#map_get_objects|map_get_objects]] for getting a
 * monsterclass object.
 */
static int map_object_get_property(lua_State *s)
{
    const char *key = luaL_checkstring(s, 2);
    MapObject *obj = LuaMapObject::check(s, 1);

    std::string property = obj->getProperty(key);
    if (!property.empty())
    {
        push(s, property);
        return 1;
    }
    else
    {
        // scripts can check for nil
        return 0;
    }
}

/** LUA mapobject:bounds (mapobjectclass)
 * mapobject:bounds()
 **
 * **Return value:** x, y position and height, width of the ''mapobject''.
 *
 * **Example use:**
 * <code lua>local x, y, width, height = my_mapobject:bounds()</code>
 *
 * **Note:** See [[scripting#map_get_objects|map_get_objects]] for getting a
 * mapobject object.
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

/** LUA mapobject:name (mapobjectclass)
 * mapobject:name()
 **
 * **Return value:** Name as set in the mapeditor of the ''mapobject''.
 *
 * **Note:** See [[scripting#map_get_objects|map_get_objects]] for getting
 * a mapobject object.
 */
static int map_object_get_name(lua_State *s)
{
    MapObject *obj = LuaMapObject::check(s, 1);
    push(s, obj->getName());
    return 1;
}

/** LUA mapobject:type (mapobjectclass)
 * mapobject:type()
 **
 * **Return value:** Type as set in the mapeditor of the ''mapobject''.
 *
 * **Note:** See [[scripting#map_get_objects|map_get_objects]] for getting
 * a mapobject object.
 */
static int map_object_get_type(lua_State *s)
{
    MapObject *obj = LuaMapObject::check(s, 1);
    push(s, obj->getType());
    return 1;
}


/** LUA_CATEGORY Item class (itemclass)
 */

/** LUA get_item_class (itemclass)
 * get_item_class(int itemid)
 * get_item_class(string itemname)
 **
 * **Return value:** This function returns a object of the item class.
 * See below for usage of that object.
 */
static int get_item_class(lua_State *s)
{
    LuaItemClass::push(s, checkItemClass(s, 1));
    return 1;
}

/** LUA itemclass:on (itemclass)
 * itemclass:on(string event, function callback)
 **
 * Assigns ''callback'' as callback for the ''event'' event.
 *
 * **Note:** See [[scripting#get_item_class|get_item_class]] for getting
 * a itemclass object.
 */
static int item_class_on(lua_State *s)
{
    ItemClass *itemClass = LuaItemClass::check(s, 1);
    const char *event = luaL_checkstring(s, 2);
    luaL_checktype(s, 3, LUA_TFUNCTION);
    itemClass->setEventCallback(event, getScript(s));
    return 0;
}

/** LUA itemclass:name (itemclass)
 * itemclass:name()
 **
 * **Return value:** The name of the item class.
 */
static int item_class_get_name(lua_State *s)
{
    ItemClass *itemClass = LuaItemClass::check(s, 1);
    push(s, itemClass->getName());
    return 1;
}

/** LUA itemclass:attacks (itemclass)
 * itemclass:attacks()
 **
 * **Return value:** Returns a list of all attacks the item offers.
 *
 * **See:** the [[scripting#AttackInfo Class|AttackInfo class]] for more info
 * about how to use the values in the list.
 */
static int item_class_attacks(lua_State *s)
{
    ItemClass *itemClass = LuaItemClass::check(s, 1);
    std::vector<AttackInfo *> attacks = itemClass->getAttackInfos();
    pushSTLContainer<AttackInfo *>(s, attacks);
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
        ivector[i - 1] = i * i;

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
        lua_pushliteral(s, "File not found");

    return 1;
}


LuaScript::LuaScript():
    nbArgs(-1)
{
    mRootState = luaL_newstate();
    mCurrentState = mRootState;
    luaL_openlibs(mRootState);

    // Register package loader that goes through the resource manager
    // package.loaders[2] = require_loader
    lua_getglobal(mRootState, "package");
#if LUA_VERSION_NUM < 502
    lua_getfield(mRootState, -1, "loaders");
#else
    lua_getfield(mRootState, -1, "searchers");
#endif
    lua_pushcfunction(mRootState, require_loader);
    lua_rawseti(mRootState, -2, 2);
    lua_pop(mRootState, 2);

    // Put the callback functions in the scripting environment.
    static luaL_Reg const callbacks[] = {
        { "on_update_derived_attribute",    on_update_derived_attribute       },
        { "on_recalculate_base_attribute",  on_recalculate_base_attribute     },
        { "on_character_death",             on_character_death                },
        { "on_character_death_accept",      on_character_death_accept         },
        { "on_character_login",             on_character_login                },
        { "on_being_death",                 on_being_death                    },
        { "on_entity_remove",               on_entity_remove                  },
        { "on_update",                      on_update                         },
        { "on_create_npc_delayed",          on_create_npc_delayed             },
        { "on_map_initialize",              on_map_initialize                 },
        { "on_craft",                       on_craft                          },
        { "on_mapvar_changed",              on_mapvar_changed                 },
        { "on_worldvar_changed",            on_worldvar_changed               },
        { "on_mapupdate",                   on_mapupdate                      },
        { "get_item_class",                 get_item_class                    },
        { "get_monster_class",              get_monster_class                 },
        { "get_status_effect",              get_status_effect                 },
        { "npc_create",                     npc_create                        },
        { "say",                            say                               },
        { "ask",                            ask                               },
        { "ask_number",                     ask_number                        },
        { "ask_string",                     ask_string                        },
        { "trade",                          trade                             },
        { "npc_post",                       npc_post                          },
        { "npc_enable",                     npc_enable                        },
        { "npc_disable",                    npc_disable                       },
        { "chr_get_quest",                  chr_get_quest                     },
        { "chr_set_quest",                  chr_set_quest                     },
        { "chr_request_quest",              chr_request_quest                 },
        { "chr_try_get_quest",              chr_try_get_quest                 },
        { "getvar_map",                     getvar_map                        },
        { "setvar_map",                     setvar_map                        },
        { "getvar_world",                   getvar_world                      },
        { "setvar_world",                   setvar_world                      },
        { "chr_get_post",                   chr_get_post                      },
        { "monster_create",                 monster_create                    },
        { "trigger_create",                 trigger_create                    },
        { "get_beings_in_circle",           get_beings_in_circle              },
        { "get_beings_in_rectangle",        get_beings_in_rectangle           },
        { "get_character_by_name",          get_character_by_name             },
        { "effect_create",                  effect_create                     },
        { "test_tableget",                  test_tableget                     },
        { "get_map_id",                     get_map_id                        },
        { "get_map_property",               get_map_property                  },
        { "is_walkable",                    is_walkable                       },
        { "map_get_pvp",                    map_get_pvp                       },
        { "item_drop",                      item_drop                         },
        { "log",                            log                               },
        { "get_distance",                   get_distance                      },
        { "map_get_objects",                map_get_objects                   },
        { "announce",                       announce                          },
        { "get_special_info",               get_special_info                  },
        { NULL, NULL }
    };
#if LUA_VERSION_NUM < 502
    lua_pushvalue(mRootState, LUA_GLOBALSINDEX);
    luaL_register(mRootState, NULL, callbacks);
#else
    lua_pushglobaltable(mRootState);
    luaL_setfuncs(mRootState, callbacks, 0);
#endif
    lua_pop(mRootState, 1);                     // pop the globals table

    static luaL_Reg const members_AttackInfo[] = {
        { "priority",                       attack_get_priority               },
        { "cooldowntime",                   attack_get_cooldowntime           },
        { "warmuptime",                     attack_get_warmuptime             },
        { "reusetime",                      attack_get_reusetime              },
        { "damage",                         attack_get_damage                 },
        { "on_attack",                      attack_on_attack                  },
        { NULL, NULL }
    };

    static luaL_Reg const members_Damage[] = {
        { "id",                             damage_get_id                     },
        { "base",                           damage_get_base                   },
        { "delta",                          damage_get_delta                  },
        { "cth",                            damage_get_cth                    },
        { "element",                        damage_get_element                },
        { "type",                           damage_get_type                   },
        { "is_truestrike",                  damage_is_truestrike              },
        { "range",                          damage_get_range                  },
        { NULL, NULL }
    };

    static luaL_Reg const members_Entity[] = {
        { "remove",                         entity_remove                     },
        { "say",                            entity_say                        },
        { "message",                        entity_message                    },
        { "inventory",                      entity_get_inventory              },
        { "inv_change",                     entity_inv_change                 },
        { "inv_count",                      entity_inv_count                  },
        { "equipment",                      entity_get_equipment              },
        { "equip_slot",                     entity_equip_slot                 },
        { "equip_item",                     entity_equip_item                 },
        { "unequip_slot",                   entity_unequip_slot               },
        { "unequip_item",                   entity_unequip_item               },
        { "set_special_recharge_speed",     entity_set_special_recharge_speed },
        { "special_recharge_speed",         entity_get_special_recharge_speed },
        { "set_special_mana",               entity_set_special_mana           },
        { "special_mana",                   entity_get_special_mana           },
        { "walk",                           entity_walk                       },
        { "damage",                         entity_damage                     },
        { "heal",                           entity_heal                       },
        { "name",                           entity_get_name                   },
        { "type",                           entity_get_type                   },
        { "action",                         entity_get_action                 },
        { "set_action",                     entity_set_action                 },
        { "direction",                      entity_get_direction              },
        { "set_direction",                  entity_set_direction              },
        { "set_walkmask",                   entity_set_walkmask               },
        { "walkmask",                       entity_get_walkmask               },
        { "warp",                           entity_warp                       },
        { "position",                       entity_get_position               },
        { "x",                              entity_get_x                      },
        { "y",                              entity_get_y                      },
        { "base_attribute",                 entity_get_base_attribute         },
        { "set_base_attribute",             entity_set_base_attribute         },
        { "modified_attribute",             entity_get_modified_attribute     },
        { "apply_attribute_modifier",       entity_apply_attribute_modifier   },
        { "remove_attribute_modifier",      entity_remove_attribute_modifier  },
        { "gender",                         entity_get_gender                 },
        { "set_gender",                     entity_set_gender                 },
        { "hair_color",                     entity_get_hair_color             },
        { "set_hair_color",                 entity_set_hair_color             },
        { "hair_style",                     entity_get_hair_style             },
        { "set_hair_style",                 entity_set_hair_style             },
        { "kill_count",                     entity_get_kill_count             },
        { "rights",                         entity_get_rights                 },
        { "kick",                           entity_kick                       },
        { "mapid",                          entity_get_mapid                  },
        { "register",                       entity_register                   },
        { "shake_screen",                   entity_shake_screen               },
        { "show_text_particle",             entity_show_text_particle         },
        { "give_special",                   entity_give_special               },
        { "has_special",                    entity_has_special                },
        { "take_special",                   entity_take_special               },
        { "monster_id",                     entity_get_monster_id             },
        { "change_anger",                   entity_change_anger               },
        { "drop_anger",                     entity_drop_anger                 },
        { "angerlist",                      entity_get_angerlist              },
        { "apply_status",                   entity_apply_status               },
        { "remove_status",                  entity_remove_status              },
        { "has_status",                     entity_has_status                 },
        { "status_time",                    entity_get_status_time            },
        { "set_status_time",                entity_set_status_time            },
        { NULL, NULL }
    };

    static luaL_Reg const members_ItemClass[] = {
        { "on",                             item_class_on                     },
        { "name",                           item_class_get_name               },
        { "attacks",                        item_class_attacks                },
        { NULL, NULL }
    };

    static luaL_Reg const members_MapObject[] = {
        { "property",                       map_object_get_property           },
        { "bounds",                         map_object_get_bounds             },
        { "name",                           map_object_get_name               },
        { "type",                           map_object_get_type               },
        { NULL, NULL }
    };

    static luaL_Reg const members_MonsterClass[] = {
        { "on_update",                      monster_class_on_update           },
        { "on_damage",                      monster_class_on_damage           },
        { "name",                           monster_class_get_name            },
        { "attacks",                        monster_class_attacks             },
        { NULL, NULL }
    };

    static luaL_Reg const members_StatusEffect[] = {
        { "on_tick",                        status_effect_on_tick             },
        { NULL, NULL }
    };

    static luaL_Reg const members_SpecialInfo[] = {
        { "name",                           specialinfo_get_name              },
        { "needed_mana",                    specialinfo_get_needed_mana       },
        { "rechargeable",                   specialinfo_is_rechargeable       },
        { "on_use",                         specialinfo_on_use                },
        { "on_recharged",                   specialinfo_on_recharged          },
        { "category",                       specialinfo_get_category          },
        { NULL, NULL}
    };

    LuaAttackInfo::registerType(mRootState, "Attack", members_AttackInfo);
    LuaDamage::registerType(mRootState, "Damage", members_Damage);
    LuaEntity::registerType(mRootState, "Entity", members_Entity);
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
