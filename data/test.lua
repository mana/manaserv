--------------
-- Map code --
--------------

require "data/scripts/npclib"

atinit(function()
  create_npc(110, 50 * 32 + 16, 19 * 32 + 16, npc1_talk, npclib.walkaround_small)
  create_npc(108, 51 * 32 + 16, 25 * 32 + 16, npc4_talk, npclib.walkaround_wide)
  create_npc(126, 45 * 32 + 16, 25 * 32 + 16, npc5_talk, npclib.walkaround_map)
  create_npc(122, 58 * 32 + 16, 15 * 32 + 16, npc6_talk, nil)
end)

function npc1_talk(npc, ch)
  do_message(npc, ch, "Hello! I am the testing NPC.")
  do_message(npc, ch, "This message is just here for testing intertwined connections.")
  do_message(npc, ch, "What do you want?")
  local v = do_choice(npc, ch, "Guns! Lots of guns!",
                               "A Christmas party!",
                               "To buy.",
                               "To sell.",
                               "To make a donation.")
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