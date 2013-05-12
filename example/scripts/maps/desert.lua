--[[

  Example script for the Desert map

--]]

-- From scripts/
require "scripts/lua/npclib"
-- From example/scripts
require "scripts/npcs/banker"
require "scripts/npcs/barber"
require "scripts/npcs/merchant"
require "scripts/npcs/shaker"

atinit(function()
    -- Barber examples
    npc_create("Barber Twin", 1, GENDER_MALE, 14 * TILESIZE + TILESIZE / 2, 9 * TILESIZE + TILESIZE / 2, Barber)
    npc_create("Barber Twin", 1, GENDER_MALE, 20 * TILESIZE + TILESIZE / 2, 11 * TILESIZE + TILESIZE / 2, npclib.talk(Barber, {14, 15, 16}, {}))

    -- A simple banker
    npc_create("Banker", 8, GENDER_MALE, 35 * TILESIZE + TILESIZE / 2, 24 * TILESIZE + TILESIZE / 2, Banker)

    -- A simple merchant.
    merchant_buy_table = { {"Candy", 10, 20}, {"Regenerative trinket", 10, 30}, {"Minor health potion", 10, 50}, {11, 10, 60}, {12, 10, 40} }
    merchant_sell_table = { {"Candy", 10, 19}, {"Sword", 10, 30}, {"Bow", 10, 200}, {"Leather shirt", 10, 300} }
    npc_create("Merchant", 3, GENDER_MALE, 4 * TILESIZE + TILESIZE / 2, 16 * TILESIZE + TILESIZE / 2, npclib.talk(Merchant, merchant_buy_table, merchant_sell_table))

    -- Another Merchant, selling some equipment, and buying everything...
    smith_buy_table = { {"Sword", 10, 50}, {7, 10, 70}, {10, 10, 20} }
    npc_create("Smith", 5, GENDER_MALE, 15 * TILESIZE + TILESIZE / 2, 16 * TILESIZE + TILESIZE / 2, npclib.talk(Smith, smith_buy_table))

    -- The most simple NPC - Welcoming new ones around.
    npc_create("Harmony", 11, GENDER_FEMALE, 4 * TILESIZE + TILESIZE / 2, 25 * TILESIZE + TILESIZE / 2, npclib.talk(Harmony, "Welcome in the template world!\nI hope you'll find here whatever you were searching for.", "Do look around to find some interesting things coming along!"), Harmony_update)

    -- Creates a Monster an let it talk for testing purpose.
    npc_create("Tamer", 9, GENDER_UNSPECIFIED, 28 * TILESIZE + TILESIZE / 2, 21 * TILESIZE + TILESIZE / 2, Tamer)
end)

function Smith(npc, ch, list)
    local sword_count = ch:inv_count(true, true, "Sword")
    if sword_count > 0 then
        say("Ah! I can see you already have a sword.")
    end
    Merchant(npc, ch, list)
end

function possessions_table(npc, ch)
    local item_message = "Inventory:"..
                         "\nSlot id, item id, item name, amount:"..
                         "\n----------------------"
    local inventory_table = ch:inventory()
    for i = 1, #inventory_table do
        item_message = item_message.."\n"..inventory_table[i].slot..", "
            ..inventory_table[i].id..", "..inventory_table[i].name..", "
            ..inventory_table[i].amount
    end
    say(item_message)

    item_message = "Equipment:"..
                   "\nSlot id, item id, item name:"..
                   "\n----------------------"
    local equipment_table = ch:equipment()
    for i = 1, #equipment_table do
        item_message = item_message.."\n"..equipment_table[i].slot..", "
            ..equipment_table[i].id..", "..equipment_table[i].name
    end
    say(item_message)

end

-- Global variable used to know whether Harmony talked to someone.
harmony_have_talked_to_someone = false
function Harmony(npc, ch, list)
    ch:apply_status(1, 99999)
    -- Say all the messages in the messages list.
    for i = 1, #list do
        say(list[i])
    end
    --- Give the player 100 units of money the first time.
    if  harmony_have_talked_to_someone == false then
        say("Here is some money for you to find some toys to play with.\nEh Eh!")
        ch:change_money(100)
        say(string.format("You now have %d shiny coins!", ch:money()))
        harmony_have_talked_to_someone = true
        say(string.format("Try to come back with a better level than %i.", ch:level()))
    else
        say("Let me see what you've got so far... Don't be afraid!")
        effect_create(EMOTE_WINK, npc)
        possessions_table(npc, ch)
    end
    say("Have fun!")
    effect_create(EMOTE_HAPPY, npc)
    -- Make Harmony disappear for a while... with a small earthquake effect!
    local shakeX, shakeY = npc:position()
    npc_disable(npc)
    tremor(shakeX, shakeY, 300)

    -- 20 seconds later, Harmony comes back
    schedule_in(20, function() npc_enable(npc)  end)
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
        npc:say("Hey! You're new! Come here...")
      end
    end
end

function Tamer(npc, ch, list)
    npc:say(string.format("You have %s Sword(s).",
                          ch:inv_count(true, true, "Sword")))
    npc:say(string.format("You are %s pixel away.",
                          get_distance(npc, ch)))
    npc:say("I will now spawn a monster for your training session.")

    -- Remove monsters in the area
    for i, b in ipairs(get_beings_in_rectangle(npc:x() - 3 * TILESIZE,
                                               npc:y() - 3 * TILESIZE,
                                               6 * TILESIZE, 6 * TILESIZE)) do
        if b:type() == TYPE_MONSTER then
            b:remove()
        end
    end

    local m1 = monster_create("Maggot", ch:position())

    -- (The following is not safe, since the being might have been removed by
    --  the time this function gets executed (especially with the above code))
    --
    --schedule_in(0.5, function()
    --            m1:say("Roaaarrrr!!!")
    --            end)
end
