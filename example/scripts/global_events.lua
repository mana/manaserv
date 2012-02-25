-------------------------------------------------------------
-- Global event script file                                --
--                                                         --
-- This file allows you to modify how certain events which --
-- happen frequently in the game on different maps are     --
-- supposed to be handled. It is a collection of script    --
-- functions which are always called when certain events   --
-- happen, regardless on which map. Script execution is    --
-- done in the context of the map the event happens on.    --
----------------------------------------------------------------------------------
--  Copyright 2010 Manasource Development Team                                  --
--                                                                              --
--  This file is part of Manasource.                                            --
--                                                                              --
--  Manasource is free software; you can redistribute  it and/or modify it      --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------


-- This function is called when the hit points of a character reach zero.
function on_chr_death(ch)
	mana.being_say(ch, "Noooooo!!!")
end

-- This function is called when the player clicks on the �OK� button after
-- the death message appeared. It should be used to implement the respawn
-- mechanic (for example: warp the character to the respawn location and
-- bring HP above zero in some way)
function on_chr_death_accept(ch)
	mana.being_heal(ch)	-- restores to full hp
	-- mana.being_heal(ch, 1)	--restores 1 hp (in case you want to be less nice)
	mana.chr_warp(ch, 1, 815, 100) --warp the character to the respawn location
end

-- This function is called after chr_death_accept. The difference is that
-- it is called in the context of the map the character is spawned on after
-- the respawn logic has happened.
function on_chr_respawn(ch)
	-- calls the local_respawn_function of the map the character respawned
	-- on when the script of the map has one
	if local_respawn_function ~= nil then
		local_respawn_function(ch)
	end
end


-- This function is called when a new character enters the world for the
-- first time. This can, for example, be used to give starting equipment
-- to the character and/or initialize a tutorial quest.
function on_chr_birth(ch)
	-- this message is shown on first login.
	mana.chat_message(0, ch, "And so your adventure begins...")
end

-- This function is called when a character logs into the game. This can,
-- for example, be utilized for a message-of-the-day or for various
-- handlings of offline processing mechanics.
function on_chr_login(ch)
	mana.chat_message(0, ch, "Welcome to Manasource")
end


-- This function is called when a character is disconnected. This could
-- be useful for various handling of offline processing mechanics.
function on_chr_logout(ch)
	-- notifies nearby players of logout
	local around = mana.get_beings_in_circle(
		posX(ch),
		posY(ch),
		1000)
	local msg = mana.being_get_name(ch).." left the game."
	for b in pairs(around) do
		if mana.being_type(b) == TYPE_CHARACTER then
			mana.chat_message(0, b, msg)
		end
	end
end
