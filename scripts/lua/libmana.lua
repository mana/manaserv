-------------------------------------------------------------
-- Mana Support Library                                    --
--                                                         --
-- Functions which are called by the game engine and       --
-- helper functions useful for writing other scripts.      --
--                                                         --
----------------------------------------------------------------------------------
--  Copyright 2008 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana Server.                                       --
--                                                                              --
--  The Mana Server is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

require "scripts/lua/libmana-constants"


-- Table that associates to each NPC pointer the handler function that is
-- called when a player starts talking to an NPC.
local npc_talk_functs = {}
local npc_update_functs = {}

-- Table that associates to each Character pointer its state with respect to
-- NPCs (only one at a time). A state is an array with four fields:
-- . 1: pointer to the NPC the player is currently talking to.
-- . 2: coroutine running the NPC handler.
-- . 3: next event the NPC expects from the server.
--      (1 = npc_next, 2 = npc_choose, 3 = quest_reply, 4 = 1+3)
-- . 4: countdown (in minutes) before the state is deleted.
-- . 5: name of the expected quest variable. (optional)
local states = {}

-- Array containing the function registered by atinit.
local init_fun = {}

-- Tick timer used during update to clean obsolete states.
local timer

-- Creates an NPC and associates the given handler.
-- Note: Cannot be called until map initialization has started.
function create_npc(name, id, x, y, talkfunct, updatefunct)
  local npc = mana.npc_create(name, id, x, y)
  if talkfunct then npc_talk_functs[npc] = talkfunct end
  if updatefunct then npc_update_functs[npc] = updatefunct end
  return npc
end

-- Waits for the player to acknowledge the previous message, if any.
function do_wait()
  coroutine.yield(0)
end

-- Sends an npc message to a player.
-- Note: Does not wait for the player to acknowledge the message.
function do_message(npc, ch, msg)
  -- Wait for the arrival of a pending acknowledgment, if any.
  coroutine.yield(0)
  mana.npc_message(npc, ch, msg)
  -- An acknowledgment is pending, but do not wait for its arrival.
  coroutine.yield(1)
end

-- Sends an NPC question to a player and waits for its answer.
function do_choice(npc, ch, ...)
  -- Wait for the arrival of a pending acknowledgment, if any.
  coroutine.yield(0)
  mana.npc_choice(npc, ch, ...)
  -- Wait for player choice.
  return coroutine.yield(2)
end

-- Sends an NPC integer ask to a player and waits for its answer.
function do_ask_integer(npc, ch, min_num, max_num, ...)
  -- Wait for the arrival of a pending acknowledgment, if any.
  coroutine.yield(0)
  mana.npc_ask_integer(npc, ch, min_num, max_num, ...)
  -- Wait for player answer.
  return coroutine.yield(2)
end

-- Sends an NPC string ask to a player and waits for its answer.
function do_ask_string(npc, ch)
  -- Wait for the arrival of a pending acknowledgment, if any.
  coroutine.yield(0)
  mana.npc_ask_string(npc, ch)
  -- Wait for player answer.
  return coroutine.yield(2)
end

-- Sends an NPC request to send letter to a player and waits for them to
-- send the letter.
function do_post(npc, ch)
  coroutine.yield(0)
  mana.npc_post(npc, ch)
  return coroutine.yield(1)
end

-- Gets the value of a quest variable.
-- Calling this function while an acknowledment is pending is desirable, so
-- that lag cannot be perceived by the player.
function get_quest_var(ch, name)
  -- Query the server and return immediatly if a value is available.
  local value = mana.chr_get_quest(ch, name)
  if value then return value end
  -- Wait for database reply.
  return coroutine.yield(3, name)
end

-- Gets the post for a user.
function getpost(ch)
  mana.chr_get_post(ch)
  return coroutine.yield(3)
end

-- Processes as much of an NPC handler as possible.
local function process_npc(w, ...)
  local co = w[2]
  local pending = (w[3] == 4)
  local first = true
  while true do
    local b, v, u
    if first then
      -- First time, resume with the arguments the coroutine was waiting for.
      b, v, u = coroutine.resume(co, ...)
      first = false
    else
      -- Otherwise, simply resume.
      b, v, u = coroutine.resume(co)
    end

    if not b then print("LUA error: ", v)end

    if not b or not v then
      -- Either there was an error, or the handler just finished its work.
      return
    elseif v == 2 then
      -- The coroutine needs a user choice from the server, so wait for it.
      w[3] = 2
      break
    elseif v == 3 then
      -- The coroutine needs the value of a quest variable from the server.
      w[5] = u
      if pending then
        -- The coroutine has also sent a message to the user, so do not
        -- forget about it, as it would flood the user with new messages.
        w[3] = 4
      else
        w[3] = 3
      end
      break
    elseif pending then
      -- The coroutine is about to interact with the user. But the previous
      -- action has not been acknowledged by the user yet, so wait for it.
      w[3] = 1
      break
    elseif v == 1 then
      -- A message has just been sent. But the coroutine can keep going in case
      -- there is still some work to do while waiting for user acknowledgment.
      pending = true
    end
  end
  -- Restore the countdown, as there was some activity.
  w[4] = 5
  return true
end

-- Called by the game whenever a player starts talking to an NPC.
-- Creates a coroutine based on the registered NPC handler.
function npc_start(npc, ch)
  states[ch] = nil
  local h = npc_talk_functs[npc]
  if not h then return end
  local w = { npc, coroutine.create(h) }
  if process_npc(w, npc, ch) then
    states[ch] = w
    if not timer then
      timer = 600
    end
  end
  -- coroutine.resume(w)
  -- do_npc_close(npc, ch)
end

function do_npc_close(npc, ch)
    mana.npc_end(npc, ch)
end

-- Called by the game whenever a player keeps talking to an NPC.
-- Checks that the NPC expects it, and processes the respective coroutine.
function npc_next(npc, ch)
  local w = states[ch]
  if w then
    local w3 = w[3]
    if w3 == 4 then
      w[3] = 3
      return
    elseif w3 == 1 and process_npc(w) then
      return
    end
  end
  states[ch] = nil
end

-- Called by the game whenever a player selects a particular reply.
-- Checks that the NPC expects it, and processes the respective coroutine.
function npc_choose(npc, ch, u)
  local w = states[ch]
  if not (w and w[1] == npc and w[3] == 2 and process_npc(w, u)) then
    states[ch] = nil
  end
end

function npc_integer(npc, ch, u)
  local w = states[ch]
  if not (w and w[1] == npc and w[3] == 2 and process_npc(w, u)) then
    states[ch] = nil
  end
end

function npc_string(npc, ch, u)
  local w = states[ch]
  if not (w and w[1] == npc and w[3] == 2 and process_npc(w, u)) then
    states[ch] = nil
  end
end

-- Called by the game when a player sends a letter.
function npc_post(npc, ch, sender, letter)
  local w = states[ch]
  if not (w and w[1] == npc and w[3] == 1 and process_npc(w, sender, letter)) then
    states[ch] = nil
  end
end

-- Called by the game whenever the value of a quest variable is known.
-- Checks that the NPC expects it, and processes the respective coroutine.
-- Note: the check for NPC correctness is missing, but it should never matter.
function quest_reply(ch, name, value)
  local w = states[ch]
  if w then
    local w3 = w[3]
    if (w3 == 3 or w3 == 4) and w[5] == name then
      w[5] = nil
      if process_npc(w, value) then
        return
      end
    end
  end
  states[ch] = nil
end

function post_reply(ch, sender, letter)
  local w = states[ch]
  if w then
    local w3 = w[3]
    if (w3 == 3 or w3 == 4) then
      if process_npc(w, sender, letter) then
        return
      end
    end
  end
  states[ch] = nil
end

-- Called by the game every tick for each NPC.
function npc_update(npc)
  local h = npc_update_functs[npc];
  if h then h(npc) end;
end

-- Called by the game every tick.
-- Checks for scheduled function calls
-- Cleans obsolete connections.
function update()
  -- check the scheduler
  check_schedule()

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
function create_npc_delayed(name, id, x, y)
  -- Bind the name to a local variable first, as it will be reused.
  local h = npc_handler
  atinit(function() create_npc(name, id, x, y, h, nil) end)
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


-- SCHEDULER

-- Table of scheduled jobs. A job is an array with 3 elements:
-- 0: the UNIX timestamp when it is executed
-- 1: the function which is executed
-- 2: nil when it is a one-time job. Repetition interval is seconds when it is
--    a repeated job.
local scheduler_jobs = {}

-- compare function used to sort the scheduler_jobs table.
-- the jobs which come first are at the end of the table.
local function job_cmp(job1, job2)
  return (job1[0] > job2[0])
end

-- checks for jobs which have to be executed, executes them and reschedules
-- them when they are repeated jobs.
function check_schedule()
  if #scheduler_jobs==0 then return end
  while os.time() > scheduler_jobs[#scheduler_jobs][0] do
    -- retreive the job and remove it from the schedule
    job = scheduler_jobs[#scheduler_jobs]
	table.remove(scheduler_jobs)
	-- reschedule the job when it is a repeated job
	if job[2] then
		schedule_every(job[2], job[1])
	end
	-- execute the job
	job[1]()
  end
end

-- schedules a function call to be executed once in n seconds
function schedule_in(seconds, funct)
  local job = {}
  job[0] = os.time() + seconds
  job[1] = funct
  job[2] = nil
  table.insert(scheduler_jobs, job)
  table.sort(scheduler_jobs, job_cmp)
end

-- schedules a function call to be executed at regular intervals of n seconds
function schedule_every(seconds, funct)
  local job = {}
  job[0] = os.time() + seconds
  job[1] = funct
  job[2] = seconds
  table.insert(scheduler_jobs, job)
  table.sort(scheduler_jobs, job_cmp)
end


-- DEATH NOTIFICATIONS
local ondeath_functs = {}
local onremove_functs = {}

-- requests the gameserver to notify the script engine when the being
-- dies and adds a script function to be executed in this case.
function on_death(being, funct)
  if ondeath_functs[being] == nil then
    ondeath_functs[being] = {}
  end
  table.insert(ondeath_functs[being], funct)
  mana.being_register(being)
end

-- requests the gameserver to notify the script engine when the being
-- dies and adds a script function to be executed in this case.
function on_remove(being, funct)
  if onremove_functs[being] == nil then
    onremove_functs[being] = {}
  end
  table.insert(onremove_functs[being], funct)
  mana.being_register(being)
end

-- called by the engine when a registred being dies.
function death_notification(being)
  if type(ondeath_functs[being]) == "table" then
    for i,funct in pairs(ondeath_functs[being]) do
      funct()
    end
    ondeath_functs[being] = nil
  end
end

-- called by the engine when a registred being is removed.
function remove_notification(being)
  if type(onremove_functs[being]) == "table" then
    for i,funct in pairs(onremove_functs[being]) do
      funct()
    end
    onremove_functs[being] = nil
    ondeath_functs[being] = nil
  end
end


-- Below are some convenience methods added to the engine API

mana.chr_money_change = function(ch, amount)
  return mana.chr_inv_change(ch, 0, amount)
end

mana.chr_money = function(ch)
  return mana.chr_inv_count(ch, 0)
end



function cast(ch, arg)
	if arg == 1 then
		mana.being_say(ch, "Kaaame...Haaame... HAAAAAA!")
	end
	if arg == 2 then
		mana.being_say(ch, "HAA-DOKEN!")
	end
	if arg == 3 then
		mana.being_say(ch, "Sonic BOOM")
	end

end
