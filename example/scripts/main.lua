--[[

  This is the main script file loaded by the server, as configured in
  manaserv.xml. It defines how certain global events should be handled.

--]]

-- At the moment the event handlers are split up over the following files:
require "scripts/global_events"
require "scripts/abilities"
require "scripts/crafting"
require "scripts/attributes"

require "scripts/items/candy"

require "scripts/monster/basic_ai"
require "scripts/monster/testmonster"

require "scripts/status/jump"
require "scripts/status/plague"
