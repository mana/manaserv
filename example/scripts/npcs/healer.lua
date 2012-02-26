--[[

 Healer NPC example

--]]

function Healer(npc, ch)
    do_message(npc, ch, "Do you need healing?")
    local c = do_choice(npc, ch, "Heal me fully", "Heal 100 HP", "Don't heal me")
    if c == 1 then
        mana.being_heal(ch)
    elseif c == 2 then
        mana.being_heal(ch, 100)
    end
end
