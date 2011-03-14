----------------------------------------------------------------------------------
--  Copyright 2009-2010 The Mana World Development Team                         --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

shake_count = 0

function shaker_update(npc)
  shake_count = shake_count + 1
  if shake_count > 20 then
    shake_count = 0
  
    center_x = mana.posX(npc)
    center_y = mana.posY(npc)
    tremor(center_x, center_y, 300)
    
  end
end

-- function which causes a screen shake effect for all players near a
-- certain point with an intensity and direction relative to said point
function square(x)
    return x * x
end

function tremor (center_x, center_y, intensity)
    for dummy, object in ipairs(mana.get_beings_in_circle(center_x, center_y, intensity)) do
        if mana.being_type(object) == TYPE_CHARACTER then
            object_x = mana.posX(object)
            object_y = mana.posY(object)
            dist_x = object_x - center_x
            dist_y = object_y - center_y
            dist = math.sqrt(square(dist_x) + square(dist_y))
            intensity_local = intensity - dist
            intensity_x = (intensity - dist) * (dist_x / dist) / 5
            intensity_y = (intensity - dist) * (dist_y / dist) / 5
            mana.chr_shake_screen(object, intensity_x, intensity_y)
        end
    end
end



