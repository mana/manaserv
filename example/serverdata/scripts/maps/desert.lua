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

atinit(function()

    -- Barber examples
    create_npc("Barber Twin", 1, 14 * TILESIZE + TILESIZE / 2, 9 * TILESIZE + TILESIZE / 2, Barber, nil)
    create_npc("Barber Twin", 1, 20 * TILESIZE + TILESIZE / 2, 11 * TILESIZE + TILESIZE / 2, npclib.talk(Barber, {14, 15, 16}, {}), nil)

    -- A simple banker
    create_npc("Banker", 8, 35 * TILESIZE + TILESIZE / 2, 24 * TILESIZE + TILESIZE / 2, Banker, nil)

    -- A simple merchant.
    merchant_buy_table = { {1, 10, 20}, {2, 10, 30}, {3, 10, 50} }
    merchant_sell_table = { {1, 10, 19}, {5, 10, 30}, {6, 10, 200}, {7, 10, 300} }
    create_npc("Merchant", 3, 4 * TILESIZE + TILESIZE / 2, 16 * TILESIZE + TILESIZE / 2, npclib.talk(Merchant, merchant_buy_table, merchant_sell_table), nil)

    -- Another Merchant, selling some equipment, and buying everything...
    smith_buy_table = { {5, 10, 50}, {7, 10, 70} }
    create_npc("Smith", 5, 15 * TILESIZE + TILESIZE / 2, 16 * TILESIZE + TILESIZE / 2, npclib.talk(Merchant, smith_buy_table), nil)


    -- The most simple NPC - Welcoming new ones around.
    create_npc("Harmony", 11, 4 * TILESIZE + TILESIZE / 2, 25 * TILESIZE + TILESIZE / 2, npclib.talk(Harmony, "Welcome in the template world!\nI hope you'll find here whatever you were searching for.", "Do look around to find some interesting things coming along!"), Harmony_update)
end)

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
    end
    do_message(npc, ch, "Have fun!")
    mana.effect_create(EMOTE_HAPPY, npc)
    do_npc_close(npc, ch)
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