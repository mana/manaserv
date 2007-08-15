--------------
-- Map code --
--------------

atinit(function()
  create_npc(110, 50 * 32 + 16, 19 * 32 + 16, my_npc1)
  create_npc(108, 51 * 32 + 16, 25 * 32 + 16, my_npc4)
end)

function my_npc1(npc, ch)
  do_message(npc, ch, "Hello! I am the testing NPC")
  do_message(npc, ch, "This message is just here for testing intertwined connections.")
  do_message(npc, ch, "What do you want?")
  local v = do_choice(npc, ch, "Guns! Lots of guns!", "A christmas party!", "To buy.", "To sell.", "Nothing.")
  if v == 1 then
    do_message(npc, ch, "Sorry, this is a heroic-fantasy game, I do not have any gun.")
  elseif v == 2 then
    local n1, n2 = tmw.chr_inv_count(ch, 524, 511)
    if n1 == 0 or n2 ~= 0 then
      do_message(npc, ch, "Yeah right...")
    else
      do_message(npc, ch, "I can't help you with the party. But I see you have a fancy hat. I could change it into a santa hat. Not much of a party, but it would get you going.")
      v = do_choice(npc, ch, "Please do.", "No way! Fancy hats are classier.")
      if v == 1 then
        tmw.chr_inv_change(ch, 524, -1, 511, 1)
      end
    end
  elseif v == 3 then
    tmw.test_npc_buy(npc, ch)
  elseif v == 4 then
    tmw.test_npc_sell(npc, ch)
  end
end

function my_npc4(npc, ch)
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
