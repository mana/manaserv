----------------------------------------------------------
-- Seller Function Sample                                      --
----------------------------------------------------------------------------------
--  Copyright 2009-2010 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

function npc1_talk(npc, ch)
  on_remove(ch, function() print "Player has left the map." end);
  do_message(npc, ch, "Hello! I am the testing NPC.")
  local rights = mana.chr_get_rights(ch);

  if (rights >= 128) then
    do_message(npc, ch, "Oh mighty server administrator, how can I avoid your wrath?")
  elseif (rights >= 8) then
    do_message(npc, ch, "How can I be of assistance, sir gamemaster?")
  elseif (rights >= 4) then
    do_message(npc, ch, "What feature would you like to debug, developer?")
  elseif (rights >= 2) then
    do_message(npc, ch, "How can I assist you in your testing duties?")
  elseif (rights >= 1) then
    do_message(npc, ch, "What do you want, lowly player?")
  else
    do_message(npc, ch, "...aren't you supposed to be banned??")
  end

  local v = do_choice(npc, ch, "Guns! Lots of guns!",
                               "A Christmas party!",
                               "To make a donation.",
                               "Slowly count from one to ten.",
                               "Tablepush Test")
  if v == 1 then
    do_message(npc, ch, "Sorry, this is a heroic-fantasy game, I do not have any gun.")

  elseif v == 2 then
    local n1, n2 = mana.chr_inv_count(ch, 524, 511)
    if n1 == 0 or n2 ~= 0 then
      do_message(npc, ch, "Yeah right...")
    else
      do_message(npc, ch, "I can't help you with the party. But I see you have a fancy hat. I could change it into Santa's hat. Not much of a party, but it would get you going.")
      v = do_choice(npc, ch, "Please do.", "No way! Fancy hats are classier.")
      if v == 1 then
        mana.chr_inv_change(ch, 524, -1, 511, 1)
      end
    end

  elseif v == 3 then
    if mana.chr_money_change(ch, -100) then
      do_message(npc, ch, string.format("Thank you for you patronage! You are left with %d GP.", mana.chr_money(ch)))
      local g = tonumber(get_quest_var(ch, "001_donation"))
      if not g then g = 0 end
      g = g + 100
      mana.chr_set_quest(ch, "001_donation", g)
      do_message(npc, ch, string.format("As of today, you have donated %d GP.", g))
    else
      do_message(npc, ch, "I would feel bad taking money from someone that poor.")
    end

  elseif v == 4 then
    mana.being_say(npc, "As you wish...")
    schedule_in(2, function() mana.being_say(npc, "One") end)
    schedule_in(4, function() mana.being_say(npc, "Two") end)
    schedule_in(6, function() mana.being_say(npc, "Three") end)
    schedule_in(8, function() mana.being_say(npc, "Four") end)
    schedule_in(10, function() mana.being_say(npc, "Five") end)
    schedule_in(12, function() mana.being_say(npc, "Six") end)
    schedule_in(14, function() mana.being_say(npc, "Seven") end)
    schedule_in(16, function() mana.being_say(npc, "Eight") end)
    schedule_in(18, function() mana.being_say(npc, "Nine") end)
    schedule_in(20, function() mana.being_say(npc, "Ten") end)

  elseif v == 5 then
    function printTable (t)
      for k,v in pairs(t) do
        print (k, ":", v)
      end
    end
    local t1, t2, t3, t4, t5 = mana.test_tableget();
    print("---------------");
    print ("Table 1:");
    printTable (t1)
    print ("Table 2:");
    printTable (t2)
    print ("Table 3:");
    printTable (t3)
    print ("Table 4:");
    printTable (t4)
    print ("Table 5:");
    printTable (t5)
    print("---------------");
  end

  do_message(npc, ch, "See you later!")
end

