--[[

 Example monster script.

 Makes the monster boost about his abilities and makes both the monster and
 the player talkative during battle.

--]]

local function update(mob)
    local r = math.random(0, 200);
    if r == 0 then
        mob:say("Roar! I am a boss")
    end
end

local maggot = get_monster_class("maggot")
maggot:on_update(update)
