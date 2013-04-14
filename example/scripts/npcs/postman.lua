----------------------------------------------------------
-- Postman Function Sample                                      --
----------------------------------------------------------------------------------
--  Copyright 2009-2010 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

function post_talk(npc, ch)
  say("Hello " .. being_get_name(ch))
  local strength = being_get_attribute(ch, ATTR_STRENGTH)
  say("You have " .. tostring(strength) .. " strength")
  say("What would you like to do?")
  local answer = ask("View Mail", "Send Mail", "Nothing")
  if answer == 1 then
    local sender, post = chr_get_post(ch)
    if sender == "" then
      say("No Post right now, sorry")
    else
      say(tostring(sender) .. " sent you " .. tostring(post))
    end
  end
  if answer == 2 then
    npc_post(npc, ch)
  end
end
