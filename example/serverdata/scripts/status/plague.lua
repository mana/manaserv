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

function tick(target, ticknumber)
    if (ticknumber % 10 == 0) then
        mana.being_say(target, "I have the plague! :( = " .. ticknumber)
    end
    local victims = mana.get_beings_in_circle(mana.posX(target), mana.posY(target), 64)
    local i = 1
    while (victims[i]) do
       if (mana.being_has_status(victims[i], 1) == false) then
           mana.being_apply_status(victims[i], 1, 6000)
           mana.being_say(victims[i], "I don't feel so good")
       end
       i = i + 1
    end
end
