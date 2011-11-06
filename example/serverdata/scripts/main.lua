----------------------------------------------------------------------------------
--  Copyright 2011 Manasource Development Team                                  --
--                                                                              --
--  This file is part of Manasource.                                            --
--                                                                              --
--  Manasource is free software; you can redistribute  it and/or modify it      --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

-- This is the main script file loaded by the server, as configured in
-- manaserv.xml. It defines how certain global events should be handled.

-- At the moment the event handlers are split up over the following files:
require "scripts/global_events"
require "scripts/special_actions"
require "scripts/crafting"
