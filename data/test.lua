------------------
-- Support code --
------------------

-- NOTE: Could be put into a separate library

function do_message(npc, ch, msg)
  tmw.msg_npc_message(npc, ch, msg)
  coroutine.yield(1)
end

function do_choice(npc, ch, msg)
  tmw.msg_npc_choice(npc, ch, msg)
  return coroutine.yield(2)
end

function npc_start(npc, ch)
  -- TODO: choose the handler depending on the npc type
  local co = coroutine.create(my_npc_handler)
  local b, v = coroutine.resume(co, npc, ch)
  if b and v then
    npcs[ch] = {npc, co, v}
  end
end

function npc_next(npc, ch)
  local w = npcs[ch]
  if w and w[1] == npc and w[3] == 1 then
    local co = w[2]
    local b, v = coroutine.resume(co)
    if b and v then
      npcs[ch] = {npc, co, v}
    else
      npcs[ch] = nil
    end
  end
end

function npc_choose(npc, ch, u)
  local w = npcs[ch]
  if w and w[1] == npc and w[3] == 2 then
    local co = w[2]
    local b, v = coroutine.resume(co, u)
    if b and v then
      npcs[ch] = {npc, co, v}
    else
      npcs[ch] = nil
    end
  end
end

function npc_update(npc)
end

function update()
  -- TODO: delete obsolete entries of the npcs table
end

npcs = {}

--------------
-- Map code --
--------------

function initialize()
  tmw.obj_create_npc(110, 50 * 32 + 16, 19 * 32 + 16)
end

function my_npc_handler(npc, ch)
  do_message(npc, ch, "Hello! I am the testing NPC")
  do_message(npc, ch, "This message is just here for testing intertwined connections.")
  do_message(npc, ch, "What do you want?")
  local v = do_choice(npc, ch, "Guns! Lots of guns!:Nothing")
  if v == 1 then
    do_message(npc, ch, "Sorry, this is a heroic-fantasy game, I do not have any gun.")
  end
end

