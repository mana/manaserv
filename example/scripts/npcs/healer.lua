--[[

 Healer NPC example

--]]

function Healer(npc, ch)
    say("Do you need healing?")
    local c = ask("Heal me fully", "Heal 100 HP", "Don't heal me")
    if c == 1 then
        ch:heal()
    elseif c == 2 then
        ch:heal(100)
    end
end
