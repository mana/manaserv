-------------------------------------------------------------
-- This status jumps from being to being                   --
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
        target:say("I have the jumping bug!")
    end
    
    if (target:status_time(2) < 2000) then
        target:set_status_time(2, 6000)
    end
    
    if (ticknumber % 50 ~= 0) then return end
    
    local victims = get_beings_in_circle(target, 64)
    local count = #victims

    if i == 0 then return end
    
    local i
    local remaining = 1000
    local victim = nil
    
    repeat
        remaining = remaining - 1
        i = math.random(count)
        victim = victims[i]
        if (victim == target) then
            victim = nil
            i = -1
        else
            i = victim:type()
        end
    until (i == TYPE_MONSTER or i == TYPE_CHARACTER or remaining == 0)

    if (victim == nil) then return end
    
    target:remove_status(2)

    victim:apply_status(2, 6000)
    victim:say("Now I have the jumping bug")
end

get_status_effect("jumping status"):on_tick(tick)
