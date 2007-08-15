------------------
-- Support code --
------------------

local npcs = {}
local states = {}
local init_fun = {}
local timer

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

function atinit(f)
  init_fun[#init_fun + 1] = f
end

function create_npc_delayed(id, x, y)
  -- Bind the name to a local variable first, as it will be reused.
  local h = npc_handler
  atinit(function() create_npc(id, x, y, h) end)
  npc_handler = nil
end

function initialize()
  for i,f in ipairs(init_fun) do
    f()
  end
  init_fun = nil
end
