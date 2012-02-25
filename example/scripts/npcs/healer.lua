----------------------------------------------------------
-- Healer Function Sample                                      --
----------------------------------------------------------------------------------
--  Copyright 2009-2010 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

function Healer(npc, ch)
	do_message(npc, ch, "Do you need healing?")
	local c = do_choice(npc, ch, "Heal me fully", "Heal 100 HP", "Don't heal me")
	if c == 1 then
		mana.being_heal(ch)
	elseif c == 2 then
		mana.being_heal(ch, 100)
	end
end
