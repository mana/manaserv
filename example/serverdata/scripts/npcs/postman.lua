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
  do_message(npc, ch, "Hello " .. mana.being_get_name(ch))
  local strength = mana.being_get_attribute(ch, ATTR_STRENGTH)
  do_message(npc, ch, "You have " .. tostring(strength) .. " strength")
  do_message(npc, ch, "What would you like to do?")
  local answer = do_choice(npc, ch, "View Mail", "Send Mail", "Nothing")
  if answer == 1 then
    local sender, post = getpost(ch)
    if sender == "" then
      do_message(npc, ch, "No Post right now, sorry")
    else
      do_message(npc, ch, tostring(sender) .. " sent you " .. tostring(post))
    end
  end
  if answer == 2 then
    do_post(npc, ch)
  end
  do_npc_close(npc, ch)
end
