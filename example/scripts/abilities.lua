--[[

 Abilities script file

 This file allows you to implement your ability action system. The system can
 for example implement magic, physical attack or also such mundane things as
 showing emoticons over the characters heads.

--]]

local spell1 = get_ability_info("Magic_Test Spell 1")
spell1:on_use(function(user, x, y, abilityId)
    target = target or user
    local s_x, s_y = user:position()
    -- d_x, d_y will be the relative center of the attack
    -- (it will be 1 tile in front of the attacker)
    local d_x, d_y = x - s_x, y - s_y
    local length = math.sqrt(d_x * d_x + d_y * d_y)
    d_x = d_x / length * TILESIZE
    d_y = d_x / length * TILESIZE

    local target_x, target_y = s_x + d_x, s_y + d_y

    -- Attack radius is TILESIZE
    local affected_beings = get_beings_in_circle(target_x, target_y, TILESIZE)
    for _, being in ipairs(affected_beings) do
        if being ~= user then
            local damage = {
                base = 10,
                delta = 5,
                chance_to_hit = user:modified_attribute(ATTR_STR),
            }
            being:damage(user, damage)
        end
    end
end)
--spell1:on_recharged(function(ch) ch:say("Hoooooooo...") end)
