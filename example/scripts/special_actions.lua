-------------------------------------------------------------
-- Special action script file                              --
--                                                         --
-- This file allows you to implement your special          --
-- action system. The system can for example implement     --
-- magic, physical attack or also such mundane things      --
-- as showing emoticons over the characters heads.         --
----------------------------------------------------------------------------------
--  Copyright 2010 Manasource Development Team                                  --
--                                                                              --
--  This file is part of Manasource.                                            --
--                                                                              --
--  Manasource is free software; you can redistribute  it and/or modify it      --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

local specialCost = {}
specialCost[1] = 50
specialCost[2] = 250
specialCost[3] = 1000

function use_special(ch, id)
	-- perform whatever the special with the ID does
	if id == 1 then
		mana.being_say(ch, "Kaaame...Haaame... HAAAAAA!")
	end
	if id == 2 then
		mana.being_say(ch, "HAA-DOKEN!")
	end
	if id == 3 then
		mana.being_say(ch, "Sonic BOOM")
	end
end

function get_special_recharge_cost(id)
	-- return the recharge cost for the special with the ID
	return specialCost[id]
end
