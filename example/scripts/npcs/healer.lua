--[[

 Healer NPC example

--]]

function Healer(npc, ch)
    npc_message(npc, ch, "Do you need healing?")
    local c = npc_choice(npc, ch, "Heal me fully", "Heal 100 HP", "Don't heal me")
    if c == 1 then
        being_heal(ch)
    elseif c == 2 then
        being_heal(ch, 100)
    end
end
