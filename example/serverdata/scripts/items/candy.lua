-------------------------------------------------------------
-- Example use script. Makes the player character say      --
-- "*munch*munch*munch*" when using this item.             --
-- The HP regeneration effect is handled separately based  --
-- on the heal value in items.xml                          --
----------------------------------------------------------------------------------
--  Copyright 2009 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------
function use(user)
	mana.being_say(user, "*munch*munch*munch*")
end
