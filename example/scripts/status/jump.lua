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
        being_say(target, "I have the jumping bug!")
    end
    
    if (being_get_status_time(target, 2) < 2000) then
        being_set_status_time(target, 2, 6000)
    end
    
    if (ticknumber % 50 ~= 0) then return end
    
    local victims = get_beings_in_circle(posX(target), posY(target), 64)
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
            i = being_type(victim)
        end
    until (i == TYPE_MONSTER or i == TYPE_CHARACTER or remaining == 0)

    if (victim == nil) then return end
    
    being_remove_status(target, 2)

    being_apply_status(victim, 2, 6000)
    being_say(victim, "Now I have the jumping bug")
end

get_status_effect("jumping status"):on_tick(tick)
