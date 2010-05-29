----------------------------------------------------------
-- Library for commonly used NPC scripts                --
--                                                      --
--                                                      --
-- Any NPC update function or talk function which could --
-- be used for NPCs on more than one map should be      --
-- placed here.                                         --
--                                                      --
----------------------------------------------------------------------------------
--  Copyright 2008 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana Server.                                       --
--                                                                              --
--  The Mana Server is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

module("npclib", package.seeall);


-- Update function walkaround_small
-- makes the NPC walk around in a 64x64 pixel square around its start location.
-- Useful for NPCs which are supposed to stay on a specific spot but
-- move a bit from time to time.

local wasmall_timer = {}
local wasmall_startx = {}
local wasmall_starty = {}

function walkaround_small(npc)
  if not wasmall_timer[npc] then
    wasmall_timer[npc] = 1
    wasmall_startx[npc] = mana.posX(npc)
    wasmall_starty[npc] = mana.posY(npc)
  end

  wasmall_timer[npc] = wasmall_timer[npc] + 1

  if wasmall_timer[npc] == 100 then
    wasmall_timer[npc] = math.random(1, 10)
    local x = math.random(-32, 32) + wasmall_startx[npc]
    local y = math.random(-32, 32) + wasmall_starty[npc]
    mana.being_walk(npc, x, y, 2)
  end
end


-- Update function walkaround_wide
-- makes the NPC walk around in a 256x256 pixel square around its start
-- location. Useful for NPCs which are supposed to be found near a specific
-- location but not nailed to the floor.

local wawide_timer = {}
local wawide_startx = {}
local wawide_starty = {}

function walkaround_wide(npc)
  if not wawide_timer[npc] then
    wawide_timer[npc] = 1
    wawide_startx[npc] = mana.posX(npc)
    wawide_starty[npc] = mana.posY(npc)
  end

  wawide_timer[npc] = wawide_timer[npc] + 1

  if wawide_timer[npc] == 50 then
    wawide_timer[npc] = math.random(1, 10)
    local x = math.random(-128, 128) + wawide_startx[npc]
    local y = math.random(-128, 128) + wawide_starty[npc]
    mana.being_walk(npc, x, y, 2)
  end
end


-- Update function walkaround_map
-- makes the NPC wander around the whole map. Useful when the players are
-- supposed to search a bit for the NPC.

local wam_timer = {}

function walkaround_map(npc)
  if not wam_timer[npc] then
    wam_timer[npc] = 1
  end

  wam_timer[npc] = wam_timer[npc] + 1

  if wam_timer[npc] == 50 then
    wam_timer[npc] = math.random(1, 10)
    local x = math.random(-128, 128) + mana.posX(npc)
    local y = math.random(-128, 128) + mana.posY(npc)
    mana.being_walk(npc, x, y, 2)
  end
end

-- Allows passage of more information to an NPC's talk function
function talk(f, ...)
    local a = {...}
    return function(npc, ch)
        f(npc, ch, a)
    end
end
