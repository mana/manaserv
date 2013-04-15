-------------------------------------------------------------
-- This when applied to a being will spread from one being --
-- to another                                              --
-- Thats all it does.                                      --
----------------------------------------------------------------------------------
--  Copyright 2009 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

local function tick(target, ticknumber)
    if (ticknumber % 10 == 0) then
        target:say("I have the plague! :( = " .. ticknumber)
    end
    local victims = get_beings_in_circle(target, 64)
    local i = 1
    while (victims[i]) do
       if (victims[i]:has_status(1) == false) then
           victims[i]:apply_status(1, 6000)
           victims[i]:say("I don't feel so good")
       end
       i = i + 1
    end
end

get_status_effect("plague"):on_tick(tick)
