----------------------------------------------------------
-- Test Scripts                                         --
--                                                      --
-- Provisorical NPC scripts currently included on map   --
-- new_1-1.tmx for demonstrating and testing variouse   --
-- features of the scripting engine.                    --
--                                                      --
----------------------------------------------------------------------------------
--  Copyright 2008 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

require "data/scripts/npclib"

atinit(function()
  create_npc("Test NPC", 200, 50 * 32 + 16, 19 * 32 + 16, npc1_talk, npclib.walkaround_small)
  create_npc("Teleporter", 201, 51 * 32 + 16, 25 * 32 + 16, npc4_talk, npclib.walkaround_wide)
  create_npc("Spider Tamer", 126, 45 * 32 + 16, 25 * 32 + 16, npc5_talk, npclib.walkaround_map)
  create_npc("Guard", 122, 58 * 32 + 16, 15 * 32 + 16, npc6_talk, npc6_update)
  create_npc("Fire Demon", 202, 58 * 32 + 16, 35 * 32 + 16, firedemon_talk, firedemon_update)
  
  tmw.trigger_create(56 * 32, 32 * 32, 64, 64, "patrol_waypoint", 1, true)
  tmw.trigger_create(63 * 32, 32 * 32, 64, 64, "patrol_waypoint", 2, true)
  
  schedule_every(3600, function()
    print("One hour has passed on map 1-1")
  end)
end)


function patrol_waypoint(obj, id)
	if (id == 1) then
		tmw.chatmessage(obj, "you've reached patrol point 1")
		tmw.being_say(obj, "I have reached patrol point 1")
	end
	if (id == 2) then
		tmw.chatmessage(obj, "you've reached patrol point 2")
		tmw.being_say(obj, "I have reached patrol point 2")
	end
end


function npc1_talk(npc, ch)
  do_message(npc, ch, "Hello! I am the testing NPC.")
  do_message(npc, ch, "This message is just here for testing intertwined connections.")
  do_message(npc, ch, "What do you want?")
  local v = do_choice(npc, ch, "Guns! Lots of guns!",
                               "A Christmas party!",
                               "To buy.",
                               "To sell.",
                               "To make a donation.",
                               "Slowly count from one to ten.")
  if v == 1 then
    do_message(npc, ch, "Sorry, this is a heroic-fantasy game, I do not have any gun.")
  elseif v == 2 then
    local n1, n2 = tmw.chr_inv_count(ch, 524, 511)
    if n1 == 0 or n2 ~= 0 then
      do_message(npc, ch, "Yeah right...")
    else
      do_message(npc, ch, "I can't help you with the party. But I see you have a fancy hat. I could change it into Santa's hat. Not much of a party, but it would get you going.")
      v = do_choice(npc, ch, "Please do.", "No way! Fancy hats are classier.")
      if v == 1 then
        tmw.chr_inv_change(ch, 524, -1, 511, 1)
      end
    end
  elseif v == 3 then
    tmw.npc_trade(npc, ch, false, { {533, 10, 20}, {535, 10, 30}, {537, 10, 50} })
  elseif v == 4 then
    tmw.npc_trade(npc, ch, true, { {511, 10, 200}, {524, 10, 300}, {508, 10, 500}, {537, 10, 25} })
  elseif v == 5 then
    if tmw.chr_money_change(ch, -100) then
      do_message(npc, ch, string.format("Thank you for you patronage! You are left with %d gil.", tmw.chr_money(ch)))
      local g = tonumber(get_quest_var(npc, ch, "001_donation"))
      if not g then g = 0 end
      g = g + 100
      tmw.chr_set_quest(ch, "001_donation", g)
      do_message(npc, ch, string.format("As of today, you have donated %d gil.", g))
    else
      do_message(npc, ch, "I would feel bad taking money from someone that poor.")
    end
  elseif v == 6 then
    tmw.being_say(npc, "As you wish...")
    schedule_in(2, function() tmw.being_say(npc, "One") end)
    schedule_in(4, function() tmw.being_say(npc, "Two") end)
    schedule_in(6, function() tmw.being_say(npc, "Three") end)
    schedule_in(8, function() tmw.being_say(npc, "Four") end)
    schedule_in(10, function() tmw.being_say(npc, "Five") end)
    schedule_in(12, function() tmw.being_say(npc, "Six") end)
    schedule_in(14, function() tmw.being_say(npc, "Seven") end)
    schedule_in(16, function() tmw.being_say(npc, "Eight") end)
    schedule_in(18, function() tmw.being_say(npc, "Nine") end)
    schedule_in(20, function() tmw.being_say(npc, "Ten") end)
  end
end

function npc4_talk(npc, ch)
  do_message(npc, ch, "Where do you want to go?")
  local v = do_choice(npc, ch, "Map 1", "Map 3")
  if v >= 1 and v <= 2 then
    do_message(npc, ch, "Are you really sure?")
    local w = do_choice(npc, ch, "Yes, I am.", "I still have a few things to do around here.")
    if w == 1 then
      if v == 1 then
        tmw.chr_warp(ch, nil, 60 * 32, 50 * 32)
      else
        tmw.chr_warp(ch, 3, 25 * 32, 25 * 32)
      end
    end
  end
end

function npc5_talk(npc, ch)
  do_message(npc, ch, "I am the spider tamer. Do you want me to spawn some spiders?")
  local answer = do_choice(npc, ch, "Yes", "No");
  if answer == 1 then
    local x = tmw.posX(npc)
    local y = tmw.posY(npc)
    tmw.monster_create(1012, x + 32, y + 32)
    tmw.monster_create(112, x - 32, y + 32)
    tmw.monster_create(1012, x + 32, y - 32)
    tmw.monster_create(1012, x - 32, y - 32)
  end
end

local guard_position = 1

function npc6_talk(npc, ch)

  if guard_position == 1 then
    tmw.being_walk(npc, 61 * 32 + 16, 15 * 32 + 16, 400)
    guard_position = 2
  else
    tmw.being_walk(npc, 55 * 32 + 16, 15 * 32 + 16, 400)
    guard_position = 1
  end
end

function npc6_update(npc)
  local r = math.random(0, 100)
  if (r == 0) then
    tmw.being_say(npc, "*humhumhum*")
  end
  if (r == 1) then
    tmw.being_say(npc, "guarding the city gate is so much fun *sigh*")
  end
  if (r == 2) then
    tmw.being_say(npc, "can't someone order me to walk to the other side of the gate?")
  end
end


function firedemon_talk(npc, ch)
  do_message(npc, ch, "Burn, puny mortals! BURN! BUUUURN!!!")
end

local firedemon_timer = 0;

function firedemon_update(npc)
	firedemon_timer = firedemon_timer + 1
	if (firedemon_timer == 5) then
	  firedemon_timer = 0
	  local victims = tmw.get_beings_in_circle(tmw.posX(npc), tmw.posY(npc), 64)
	  local i = 1;
	  while (victims[i]) do
	    tmw.being_damage(victims[i], 20, 10, 32000, DAMAGE_MAGICAL, ELEMENT_FIRE)
		i = i + 1
	  end
	end
	
	npclib.walkaround_map(npc)
end
