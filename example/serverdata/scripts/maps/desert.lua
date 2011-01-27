----------------------------------------------------------
-- Template script for the Desert map                   --
----------------------------------------------------------------------------------
--  Copyright 2011 The Mana Development Team                                    --
--                                                                              --
--  This file is part of Manasource Project.                                    --
--                                                                              --
--  Manasource is free software; you can redistribute  it and/or modify it      --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

-- From scripts/
require "scripts/lua/npclib"
-- From example/serverdata/scripts
require "scripts/npcs/banker"
require "scripts/npcs/barber"

atinit(function()
    create_npc("Barber", 100, 14 * TILESIZE + TILESIZE / 2, 9 * TILESIZE + TILESIZE / 2, Barber, nil)
    create_npc("Barber 2", 100, 20 * TILESIZE + TILESIZE / 2, 11 * TILESIZE + TILESIZE / 2, npclib.talk(Barber, {14, 15, 16}, {}), nil)
    create_npc("Banker", 149, 35 * TILESIZE + TILESIZE / 2, 24 * TILESIZE + TILESIZE / 2, Banker, nil)

    create_npc("Test", 102, 4 * TILESIZE + TILESIZE / 2, 25 * TILESIZE + TILESIZE / 2, npclib.talk(Test, "String1", "String2", "String3", "Etc"), nil)
end)

function Test(npc, ch, list)
    for i = 1, #list do
        do_message(npc, ch, list[i])
    end
    do_npc_close(npc, ch)
end
