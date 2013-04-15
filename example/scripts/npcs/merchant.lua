----------------------------------------------------------
-- Merchant Function Sample                                      --
----------------------------------------------------------------------------------
--  Copyright 2009-2010 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

function Merchant(npc, ch, buy_sell_table)

    -- Important note: You can add two tables made of trinoms here when calling the
    -- merchant function. E.g.: Merchant(npc, ch, buy_table, sell_table)
    -- Even though, the function here will see only one table:
    -- buy_sell_table[1] will corresponds to the first table (used to list
    -- boughtable items, and buy_sell_table[2] listing sellable items.

    local rights = ch:rights()

    if (rights >= 128) then
        announce(ch:name() .. " the big administrator was at my shop!",
                 npc:name())
        say "Oh mighty server administrator, how can I avoid your wrath?"
    elseif (rights >= 8) then
        say "How can I be of assistance, sir gamemaster?"
    elseif (rights >= 4) then
        say "What feature would you like to debug, developer?"
    elseif (rights >= 2) then
        say "How can I assist you in your testing duties?"
    elseif (rights >= 1) then
        if ch:gender() == GENDER_FEMALE then
            say "What do you want, Madam?"
        else
            say "What do you want, Sir?"
        end
    else
        say "...Aren't you supposed to be banned??"
    end

    -- Constructing the choice list
    local choice_table = {}
    table.insert (choice_table, "To Buy...")

    if (buy_sell_table[2] == nil) then
        table.insert (choice_table, "To sell stuff...")
    else
        table.insert (choice_table, "Can you make me a price for what I have?")
    end
    table.insert (choice_table, "Tell me about the objects on this map")
    table.insert (choice_table, "Nevermind...")

    local v = ask(choice_table)

    --Debug and learning purpose
    --for i,k in ipairs(choice_table) do print(i,k) end
    -- The buy table first line content
    --print (((buy_sell_table[1])[1])[1], ((buy_sell_table[1])[1])[2], ((buy_sell_table[1])[1])[3])
    -- The sell table first line content
    --print (((buy_sell_table[2])[1])[1], ((buy_sell_table[2])[1])[2], ((buy_sell_table[2])[1])[3])

    if v == 1 then
    -- "To buy."
    local buycase = trade(false, buy_sell_table[1])
    if buycase == 0 then
        say "What do you want to buy?"
    elseif buycase == 1 then
        say "I've got no items to sell."
    else
        say "Hmm, something went wrong... Ask a scripter to fix the buying mode!"
    end

    elseif v == 2 then

        if buy_sell_table[2] == nil then
            -- "To sell stuff..."
            local sellcase = trade(true)
            if sellcase == 0 then
                say "Ok, what do you want to sell?"
            elseif sellcase == 1 then
                say "I'm not interested by any of your items."
            else
                say "Hmm, something went wrong... Ask a scripter to fix this!"
            end
        else
            -- "Can you make me a price for what I have?"
            local sellcase = trade(true, buy_sell_table[2])
            if sellcase == 0 then
                say "Here we go:"
            elseif sellcase == 1 then
                say "I'm not that interested in any of your items."
            else
                say "Hmm, something went wrong... Ask a scripter to fix me!"
            end
        end

    elseif v == 3 then

        local objects = map_get_objects()
        say("There are " .. #objects .. " objects on this map, their names are:")
        for i=1,#objects do
            say(tostring(i) .. ": " .. objects[i]:name())
        end

    end

    say "See you later!"
end
