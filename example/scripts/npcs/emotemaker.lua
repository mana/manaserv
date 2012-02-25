----------------------------------------------------------
-- Emote use Function Sample                                      --
----------------------------------------------------------------------------------
--  Copyright 2009-2010 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

emo_count = 0
emo_state = EMOTE_SURPRISE

function emote_talk(npc, ch)
  if emo_state     == EMOTE_SURPRISE then
    state = "confused"
  elseif emo_state == EMOTE_SAD then
    state = "sad"
  elseif emo_state == EMOTE_HAPPY then
    state = "happy"
  end
  do_message(npc, ch, string.format("The emotional palm seems %s.", state))
  v = do_choice(npc, ch,
    "Stupid palm, you are ugly and everyone hates you!",
    "You are such a nice palm, let me give you a hug.",
    "Are you a cocos nucifera or a syagrus romanzoffiana?")

  if (v     == 1) then
    emo_state = EMOTE_SAD
  elseif (v == 2) then
    emo_state = EMOTE_HAPPY
  elseif (v == 3) then
    emo_state = EMOTE_SURPRISE
  end
end

function emote_update(npc)
  emo_count = emo_count + 1
  if emo_count > 50 then
    emo_count = 0
    mana.effect_create(emo_state, npc)
  end
end
