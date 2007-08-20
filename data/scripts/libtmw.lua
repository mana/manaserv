------------------
-- Support code --
------------------

local npcs = {}
local states = {}
local init_fun = {}
local timer

-- Creates an NPC and associates the given handler.
-- Note: Cannot be called until map initialization has started.
function create_npc(id, x, y, handler)
  local npc = tmw.obj_create_npc(id, x, y)
  npcs[npc] = handler
end

-- Sends an npc message to a player.
-- Note: Does not wait for the player to acknowledge the message.
function do_message(npc, ch, msg)
  -- Wait for the arrival of a pending acknowledgment, if any.
  coroutine.yield(0)
  tmw.msg_npc_message(npc, ch, msg)
  -- An acknowledgment is pending, but do not wait for its arrival.
  coroutine.yield(1)
end

-- Sends an NPC question to a player and waits for its answer.
function do_choice(npc, ch, ...)
  -- Wait for the arrival of a pending acknowledgment, if any.
  coroutine.yield(0)
  tmw.msg_npc_choice(npc, ch, ...)
  -- Wait for player choice.
  return coroutine.yield(2)
end

-- Processes as much of an NPC handler as possible.
local function process_npc(co, ...)
  -- First, resume with the arguments the coroutine was waiting for.
  local b, v = coroutine.resume(co, ...)
  if not b or not v then
    return
  end
  if v == 2 then
    return 2
  end
  -- Then, keep resuming until the coroutine expects the result of a choice
  -- or an acknowledgment to a message.
  local pending = (v == 1)
  while true do
    b, v = coroutine.resume(co)
    if not b or not v then
      return
    end
    if v == 2 then
      return 2
    end
    if pending then
      return 1
    end
    if v == 1 then
      pending = true
    end
  end
end

-- Called by the game whenever a player starts talking to an NPC.
-- Creates a coroutine based on the registered NPC handler.
function npc_start(npc, ch)
  states[ch] = nil
  local h = npcs[npc]
  if not h then return end
  local co = coroutine.create(h)
  local v = process_npc(co, npc, ch)
  if v then
    states[ch] = {npc, co, v, 5}
    if not timer then
      timer = 600
    end
  end
end

-- Called by the game whenever a player keeps talking to an NPC.
-- Checks that the NPC expects it, and processes the respective coroutine.
function npc_next(npc, ch)
  local w = states[ch]
  if w and w[1] == npc and w[3] == 1 then
    local v = process_npc(w[2])
    if v then
      w[3] = v
      w[4] = 5
      return
    end
  end
  states[ch] = nil
end

-- Called by the game whenever a player selects a particular reply.
-- Checks that the NPC expects it, and processes the respective coroutine.
function npc_choose(npc, ch, u)
  local w = states[ch]
  if w and w[1] == npc and w[3] == 2 then
    local v = process_npc(w[2], u)
    if v then
      w[3] = v
      w[4] = 5
      return
    end
  end
  states[ch] = nil
end

-- Called by the game every tick for each NPC.
function npc_update(npc)
end

-- Called by the game every tick.
-- Cleans obsolete connections.
function update()
  -- Run every minute only, in order not to overload the server.
  if not timer then return end
  timer = timer - 1
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

-- Registers a function so that is is executed during map initialization.
function atinit(f)
  init_fun[#init_fun + 1] = f
end

-- Called by the game for creating NPCs embedded into maps.
-- Delays the creation until map initialization is performed.
-- Note: Assumes that the "npc_handler" global field contains the NPC handler.
function create_npc_delayed(id, x, y)
  -- Bind the name to a local variable first, as it will be reused.
  local h = npc_handler
  atinit(function() create_npc(id, x, y, h) end)
  npc_handler = nil
end

-- Called during map initialization.
-- Executes all the functions registered by atinit.
function initialize()
  for i,f in ipairs(init_fun) do
    f()
  end
  init_fun = nil
end

-- Below are some convenience methods added to the engine API

tmw.chr_money_change = function(ch, amount)
  return tmw.chr_inv_change(ch, 0, amount)
end

tmw.chr_money = function(ch)
  return tmw.chr_inv_count(ch, 0)
end
