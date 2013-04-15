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

local function strike(mob, victim, hit)
    if hit > 0 then
        mob:say("Take this! "..hit.." damage!")
        victim:say("Oh Noez!")
    else
        mob:say("Oh no, my attack missed!")
        victim:say("Whew...")
    end
end

local maggot = get_monster_class("maggot")
maggot:on_update(update)
local attacks = maggot:attacks();
for i, attack in ipairs(attacks) do
    attack:on_attack(strike)
end
