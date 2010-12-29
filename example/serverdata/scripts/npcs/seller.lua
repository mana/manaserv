----------------------------------------------------------
-- Seller Function Sample                                      --
----------------------------------------------------------------------------------
--  Copyright 2009-2010 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

function Seller(npc, ch)
  do_message(npc, ch, "Hello! What can I provide you today?")
  local rights = mana.chr_get_rights(ch);

  if (rights >= 128) then
    do_message(npc, ch, "Oh mighty server administrator, how can I avoid your wrath?")
  elseif (rights >= 8) then
    do_message(npc, ch, "How can I be of assistance, sir gamemaster?")
  elseif (rights >= 4) then
    do_message(npc, ch, "What feature would you like to debug, developer?")
  elseif (rights >= 2) then
    do_message(npc, ch, "How can I assist you in your testing duties?")
  elseif (rights >= 1) then
    do_message(npc, ch, "What do you want, lowly player?")
  else
    do_message(npc, ch, "...Aren't you supposed to be banned??")
  end

  local v = do_choice(npc, ch, "To buy...",
                               "To sell stuff...",
                               "Can you make me a price for what I have?")
  if v == 1 then
    -- "To buy."
    local buycase = mana.npc_trade(npc, ch, false, { {1, 10, 20}, {2, 10, 30}, {3, 10, 50} })
    if buycase == 0 then
      do_message(npc, ch, "What do you want to buy?")
    elseif buycase == 1 then
      do_message(npc, ch, "I've got no items to sell.")
    else
      do_message(npc, ch, "Hmm, something went wrong... Ask a scripter to fix the buying mode!")
    end

  elseif v == 2 then

    -- "To sell stuff..."
    local sellcase = mana.npc_trade(npc, ch, true)
    if sellcase == 0 then
      do_message(npc, ch, "Ok, what do you want to sell?")
    elseif sellcase == 1 then
      do_message(npc, ch, "I'm not interested by any of your items.")
    else
      do_message(npc, ch, "Hmm, something went wrong... Ask a scripter to fix this!")
    end

  elseif v == 3 then

    -- "Can you make me a price for what I have?"
    local sellcase = mana.npc_trade(npc, ch, true, { {4, 10, 20}, {5, 10, 30}, {6, 10, 200}, {7, 10, 300} })
    if sellcase == 0 then
      do_message(npc, ch, "Here we go:")
    elseif sellcase == 1 then
      do_message(npc, ch, "I'm not that interested in any of your items.")
    else
      do_message(npc, ch, "Hmm, something went wrong... Ask a scripter to fix me!")
    end

  end
  do_message(npc, ch, "See you later!")
  do_npc_close(npc, ch)
end

