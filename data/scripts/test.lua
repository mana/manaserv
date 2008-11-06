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

require "data/scripts/libs/npclib"

atinit(function()
  create_npc("Test NPC", 200, 50 * TILESIZE + 16, 19 * TILESIZE + 16, npc1_talk, npclib.walkaround_small)
  create_npc("Teleporter", 201, 51 * TILESIZE + 16, 25 * TILESIZE + 16, npc4_talk, npclib.walkaround_wide)
  create_npc("Scorpion Tamer", 126, 45 * TILESIZE + 16, 25 * TILESIZE + 16, npc5_talk, nil)
  create_npc("Guard", 122, 58 * TILESIZE + 16, 15 * TILESIZE + 16, npc6_talk, npc6_update)
  create_npc("Fire Demon", 202, 58 * TILESIZE + 16, 35 * TILESIZE + 16, firedemon_talk, firedemon_update)
  create_npc("Post Box", 158, 45 * TILESIZE + 16, 22 * TILESIZE + 16, post_talk)
  create_npc("Fireworker", 158, 43 * TILESIZE, 23 * TILESIZE, fireworker_talk, npclib.walkaround_small)
  create_npc("Axe Trainer", 126, 65 * TILESIZE, 18 * TILESIZE, axetrainer_talk, nil)

  tmw.trigger_create(56 * TILESIZE, 32 * TILESIZE, 64, 64, "patrol_waypoint", 1, true)
  tmw.trigger_create(63 * TILESIZE, 32 * TILESIZE, 64, 64, "patrol_waypoint", 2, true)

  schedule_every(1 * HOURS + 30 * MINUTES, function()
    print("One and a half hour has passed on map 1-1")
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
                               "Slowly count from one to ten.",
                               "Tablepush Test")
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
      local g = tonumber(get_quest_var(ch, "001_donation"))
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
  elseif v == 7 then
    local t1, t2, t3, t4 = tmw.test_tableget();
    print("---------------");
    print ("Table 1:"); 
    for k,v in pairs(t1) do
      print (k, ":", v)
    end
    
    print ("Table 2:"); 
    for k,v in pairs(t2) do
      print (k, ":", v)
    end
    
    print ("Table 3:"); 
    for k,v in pairs(t3) do
      print (k, ":", v)
    end
    
    print ("Table 4:"); 
    for k,v in pairs(t4) do
      print (k, ":", v)
    end
    print("---------------");
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
        tmw.chr_warp(ch, nil, 60 * TILESIZE, 50 * TILESIZE)
      else
        tmw.chr_warp(ch, 3, 25 * TILESIZE, 25 * TILESIZE)
      end
    end
  end
end

function npc5_talk(npc, ch)
  do_message(npc, ch, "I am the scorpion tamer. Do you want me to spawn some scorpions?")
  local answer = do_choice(npc, ch, "Yes", "No");
  if answer == 1 then
    local x = tmw.posX(npc)
    local y = tmw.posY(npc)
    m1 = tmw.monster_create(1, x + TILESIZE, y + TILESIZE)
    m2 = tmw.monster_create(1, x - TILESIZE, y + TILESIZE)
    m3 = tmw.monster_create(1, x + TILESIZE, y - TILESIZE)
    m4 = tmw.monster_create(1, x - TILESIZE, y - TILESIZE)

    on_death(m1, function() tmw.being_say(npc, "NOOO!") end)
    on_death(m2, function() tmw.being_say(npc, "Please stop this violence!") end)
    on_death(m3, function() tmw.being_say(npc, "Stop slaughtering my scorpions!") end)
    on_death(m4, function() tmw.being_say(npc, "Leave my scorpions alone!") end)
    on_death(m4, function() tmw.being_say(m4, "AAARGH!") end)

  end
end

local guard_position = 1

function npc6_talk(npc, ch)

  if guard_position == 1 then
    tmw.being_walk(npc, 61 * TILESIZE + 16, 15 * TILESIZE + 16, 400)
    guard_position = 2
  else
    tmw.being_walk(npc, 55 * TILESIZE + 16, 15 * TILESIZE + 16, 400)
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

function post_talk(npc, ch)
  do_message(npc, ch, "Hello " .. tmw.being_get_name(ch))
  local strength = tmw.being_get_attribute(ch, ATTR_STRENGTH)
  do_message(npc, ch, "You have " .. tostring(strength) .. " strength")
  do_message(npc, ch, "Would you like to see your mail?")
  local answer = do_choice(npc, ch, "Yes", "No")
  if answer == 1 then
    local sender, post = getpost(ch)
    if sender == "" then
      do_message(npc, ch, "No Post right now, sorry")
    else
      do_message(npc, ch, tostring(sender) .. " sent you " .. tostring(post))
    end
  end
end

function fireworker_talk(npc, ch)
  do_message(npc, ch, "Do you want some fireworks?")
  local answer = do_choice(npc, ch, "Wheee! Fireworks", "Nah, thanks.")
  if answer == 1 then
    local x = tmw.posX(npc)
    local y = tmw.posY(npc)
      for c = 0, 25 do
        schedule_in (c, function()
          tmw.effect_create(c, x + math.random(-200, 200), y + math.random(-200, 200))
        end)
      end
  end
end

function axetrainer_talk(npc, ch)
  do_message(npc, ch, "I am the axe trainer. Do you want to get better at using axes?")
  local answer = do_choice(npc, ch, "Please train me, master.", "I am good enough with axes.")
  if answer == 1 then
    local newexp = tmw.chr_get_exp(ch, SKILL_WEAPON_AXE) + 100
    local nextlevel = tmw.exp_for_level(tmw.being_get_attribute(ch, SKILL_WEAPON_AXE) + 1)
    tmw.chr_give_exp(ch, SKILL_WEAPON_AXE, 100)
    local message = "I gave you 100 axe exp."
    if newexp > nextlevel then
      message = message.." This should be enough to reach the next level."
    else
      message = message.." You will still need "..tostring(nextlevel - newexp).." exp to reach the next level."
    end
    message = message.." I should really stop doing this when the server goes live."
    do_message(npc, ch, message);
  end
end
