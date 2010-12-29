----------------------------------------------------------------------------------
--  Copyright 2009 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

function update(mob)
  local r = math.random(0, 200);
  if r == 0 then
    mana.being_say(mob, "Roar! I am a boss")
  end
end

function strike(mob, victim, hit)
  if hit > 0 then
    mana.being_say(mob, "Take this! "..hit.." damage!")
    mana.being_say(victim, "Oh Noez!")
  else
    mana.being_say(mob, "Oh no, my attack missed!")
    mana.being_say(victim, "Whew...")
  end
end
