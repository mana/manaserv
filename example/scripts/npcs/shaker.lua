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
  
    local center_x, center_y = npc:position()
    tremor(center_x, center_y, 300)
  end
end

-- function which causes a screen shake effect for all players near a
-- certain point with an intensity and direction relative to said point
function square(x)
    return x * x
end

function tremor(center_x, center_y, intensity)
    for dummy, being in ipairs(get_beings_in_circle(center_x, center_y, intensity)) do
        if being:type() == TYPE_CHARACTER then
            local being_x, being_y = being:position()
            local dist_x = being_x - center_x
            local dist_y = being_y - center_y
            local dist = math.sqrt(square(dist_x) + square(dist_y))
            local intensity = intensity - dist
            local intensity_x = intensity * (dist_x / dist) / 5
            local intensity_y = intensity * (dist_y / dist) / 5
            being:shake_screen(intensity_x, intensity_y)
        end
    end
end
