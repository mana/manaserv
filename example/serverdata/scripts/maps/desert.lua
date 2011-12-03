----------------------------------------------------------
-- Template script for the Desert map                   --
----------------------------------------------------------------------------------
--  Copyright 2011 The Mana Development Team                                    --
--                                                                              --
--  This file is part of Manasource Project.                                    --
--                                                                              --
--  Manasource is free software; you can redistribute  it and/or modify it      --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

-- From scripts/
require "scripts/lua/npclib"
-- From example/serverdata/scripts
require "scripts/npcs/banker"
require "scripts/npcs/barber"
require "scripts/npcs/merchant"
require "scripts/npcs/shaker"

atinit(function()

    -- Barber examples
    create_npc("Barber Twin", 1, GENDER_MALE, 14 * TILESIZE + TILESIZE / 2, 9 * TILESIZE + TILESIZE / 2, Barber, nil)
    create_npc("Barber Twin", 1, GENDER_MALE, 20 * TILESIZE + TILESIZE / 2, 11 * TILESIZE + TILESIZE / 2, npclib.talk(Barber, {14, 15, 16}, {}), nil)

    -- A simple banker
    create_npc("Banker", 8, GENDER_MALE, 35 * TILESIZE + TILESIZE / 2, 24 * TILESIZE + TILESIZE / 2, Banker, nil)

    -- A simple merchant.
    merchant_buy_table = { {"Candy", 10, 20}, {"Regenerative trinket", 10, 30}, {"Minor health potion", 10, 50}, {11, 10, 60}, {12, 10, 40} }
    merchant_sell_table = { {"Candy", 10, 19}, {"Sword", 10, 30}, {"Bow", 10, 200}, {"Leather shirt", 10, 300} }
    create_npc("Merchant", 3, GENDER_MALE, 4 * TILESIZE + TILESIZE / 2, 16 * TILESIZE + TILESIZE / 2, npclib.talk(Merchant, merchant_buy_table, merchant_sell_table), nil)

    -- Another Merchant, selling some equipment, and buying everything...
    smith_buy_table = { {"Sword", 10, 50}, {7, 10, 70}, {10, 10, 20} }
    create_npc("Smith", 5, GENDER_MALE, 15 * TILESIZE + TILESIZE / 2, 16 * TILESIZE + TILESIZE / 2, npclib.talk(Smith, smith_buy_table), nil)

    -- The most simple NPC - Welcoming new ones around.
    create_npc("Harmony", 11, GENDER_FEMALE, 4 * TILESIZE + TILESIZE / 2, 25 * TILESIZE + TILESIZE / 2, npclib.talk(Harmony, "Welcome in the template world!\nI hope you'll find here whatever you were searching for.", "Do look around to find some interesting things coming along!"), Harmony_update)

    -- Creates a Monster an let it talk for testing purpose.
    create_npc("Tamer", 9, GENDER_UNSPECIFIED, 28 * TILESIZE + TILESIZE / 2, 21 * TILESIZE + TILESIZE / 2, Tamer, nil)
end)

function Smith(npc, ch, list)
    local sword_count = mana.chr_inv_count(ch, true, true, "Sword")
    if sword_count > 0 then
        do_message(npc, ch, "Ah! I can see you already have a sword.")
    end
    Merchant(npc, ch, list)
end

function possessions_table(npc, ch)
    local item_message = "Inventory:"..
                         "\nSlot id, item id, item name, amount:"..
                         "\n----------------------"
    local inventory_table = mana.chr_get_inventory(ch)
    for i = 1, #inventory_table do
        item_message = item_message.."\n"..inventory_table[i].slot..", "
            ..inventory_table[i].id..", "..inventory_table[i].name..", "
            ..inventory_table[i].amount
    end
    do_message(npc, ch, item_message)

    item_message = "Equipment:"..
                   "\nSlot id, item id, item name:"..
                   "\n----------------------"
    local equipment_table = mana.chr_get_equipment(ch)
    for i = 1, #equipment_table do
        item_message = item_message.."\n"..equipment_table[i].slot..", "
            ..equipment_table[i].id..", "..equipment_table[i].name
    end
    do_message(npc, ch, item_message)

end

-- Global variable used to know whether Harmony talked to someone.
harmony_have_talked_to_someone = false
function Harmony(npc, ch, list)
    -- Say all the messages in the messages list.
    for i = 1, #list do
        do_message(npc, ch, list[i])
    end
    --- Give the player 100 units of money the first time.
    if  harmony_have_talked_to_someone == false then
        do_message(npc, ch, "Here is some money for you to find some toys to play with.\nEh Eh!")
        mana.chr_money_change(ch, 100)
        do_message(npc, ch, string.format("You now have %d shiny coins!", mana.chr_money(ch)))
        harmony_have_talked_to_someone = true
        do_message(npc, ch, string.format("Try to come back with a better level than %i.", mana.chr_get_level(ch)))
    else
        do_message(npc, ch, "Let me see what you've got so far... Don't be afraid!")
        mana.effect_create(EMOTE_WINK, npc)
        possessions_table(npc, ch)
    end
    do_message(npc, ch, "Have fun!")
    mana.effect_create(EMOTE_HAPPY, npc)
    -- Make Harmony disappear for a while... with a small earthquake effect!
    local shakeX = mana.posX(npc)
    local shakeY = mana.posY(npc)
    mana.npc_disable(npc)
    tremor(shakeX, shakeY, 300)

    -- 20 seconds later, Harmony comes back
    schedule_in(20, function() mana.npc_enable(npc)  end)
    schedule_in(20, function() tremor(shakeX, shakeY, 300)  end)
end

-- Global variable used to control Harmony's updates.
-- One tick equals to 100ms, so 100 below equals to 10000ms or 10 seconds
harmony_tick_count = 0
function Harmony_update(npc)
    if harmony_have_talked_to_someone == false then
      harmony_tick_count = harmony_tick_count + 1
      if harmony_tick_count > 100 then
        harmony_tick_count = 0
        mana.being_say(npc, "Hey! You're new! Come here...")
      end
    end
end

function Tamer(npc, ch, list)
    mana.being_say(npc, string.format("You have %s Sword(s).",
                                      mana.chr_inv_count(ch, true, true, "Sword")))
    mana.being_say(npc, string.format("You are %s pixel away.",
                                      mana.get_distance(npc, ch)))
    mana.being_say(npc, "I will now spawn a monster for your training session.")

    -- Remove monsters in the area
    for i, b in ipairs(mana.get_beings_in_rectangle(mana.posX(npc) - 3 * TILESIZE,
                                                    mana.posY(npc) - 3 * TILESIZE,
                                                    6 * TILESIZE, 6 * TILESIZE)) do
        if mana.being_type(b) == TYPE_MONSTER then
            mana.monster_remove(b)
        end
    end

    local m1 = mana.monster_create("Maggot", mana.posX(ch), mana.posY(ch))
    mana.monster_change_anger(m1, ch, 100)

    -- (The following is not safe, since the being might have been removed by
    --  the time this function gets executed (especially with the above code))
    --
    --schedule_in(0.5, function()
    --            mana.being_say(m1, "Roaaarrrr!!!")
    --            mana.monster_change_anger(m1, ch, 100)
    --            end)
end
