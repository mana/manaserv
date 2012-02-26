--[[

 Example monster script.

 Makes the monster boost about his abilities and makes both the monster and
 the player talkative during battle.

--]]

function update_monster(mob)
    local r = math.random(0, 200);
    if r == 0 then
        mana.being_say(mob, "Roar! I am a boss")
    end
end

function on_maggot_strike(mob, victim, hit)
    if hit > 0 then
        mana.being_say(mob, "Take this! "..hit.." damage!")
        mana.being_say(victim, "Oh Noez!")
    else
        mana.being_say(mob, "Oh no, my attack missed!")
        mana.being_say(victim, "Whew...")
    end
end
