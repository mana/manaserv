------------------
-- Support code --
------------------

-- NOTE: Could be put into a separate library

function create_npc(id, x, y, handler)
  local npc = tmw.obj_create_npc(id, x, y)
  npcs[npc] = handler
end

function do_message(npc, ch, msg)
  tmw.msg_npc_message(npc, ch, msg)
  coroutine.yield(1)
end

function do_choice(npc, ch, ...)
  tmw.msg_npc_choice(npc, ch, ...)
  return coroutine.yield(2)
end

-- Called whenever a player starts talking to an NPC.
-- Creates a coroutine based on the register NPC handler.
function npc_start(npc, ch)
  local h = npcs[npc]
  if not h then return end
  local co = coroutine.create(h)
  local b, v = coroutine.resume(co, npc, ch)
  if b and v then
    states[ch] = {npc, co, v, 5}
    if not timer then
      timer = 600
    end
  end
end

-- Called whenever a player keeps talking to an NPC.
-- Checks that the NPC expects it, and resumes the respective coroutine.
function npc_next(npc, ch)
  local w = states[ch]
  if w and w[1] == npc and w[3] == 1 then
    local b, v = coroutine.resume(w[2])
    if b and v then
      w[3] = v
      w[4] = 5
    else
      states[ch] = nil
    end
  end
end

-- Called whenever a player selects a particular reply.
-- Checks that the NPC expects it, and resumes the respective coroutine.
function npc_choose(npc, ch, u)
  local w = states[ch]
  if w and w[1] == npc and w[3] == 2 then
    local b, v = coroutine.resume(w[2], u)
    if b and v then
      w[3] = v
      w[4] = 5
    else
      states[ch] = nil
    end
  end
end

function npc_update(npc)
end

function update()
  -- Run every minute only, in order not to overload the server.
  if not timer then return end
  timer = timer - 10
  if timer ~= 0 then return end
  -- Free connections that have been inactive for 3-4 minutes.
  for k, w in pairs(states) do
    local t = w[4] - 1
    if t == 0 then
      states[k] = nil
    else
      w[4] = t
    end
  end
  -- Restart timer if there are still some pending states.
  if next(states) then
    timer = 600
  else
    timer = nil
  end
end

npcs = {}
states = {}

--------------
-- Map code --
--------------

function initialize()
  create_npc(110, 50 * 32 + 16, 19 * 32 + 16, my_npc1)
  create_npc(107, 53 * 32 + 16, 21 * 32 + 16, my_npc2)
  create_npc(107, 53 * 32 + 16, 23 * 32 + 16, my_npc3)
  create_npc(108, 51 * 32 + 16, 25 * 32 + 16, my_npc4)
end

function my_npc1(npc, ch)
  do_message(npc, ch, "Hello! I am the testing NPC")
  do_message(npc, ch, "This message is just here for testing intertwined connections.")
  do_message(npc, ch, "What do you want?")
  local v = do_choice(npc, ch, "Guns! Lots of guns!", "A christmas party!", "Nothing.")
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
  end
end

npc2_times = 1

function my_npc2(npc, ch)
  do_message(npc, ch, "You know what?")
  do_message(npc, ch, string.format("I have already asked this question %d times today.", npc2_times))
  npc2_times = npc2_times + 1
end

function my_npc3(npc, ch)
  do_message(npc, ch, "Don't you think the guy behind me is my evil twin?")
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
