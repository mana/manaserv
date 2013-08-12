--[[

 Basic stroll ai

--]]

local STROLL_TIMEOUT = 20
local STROLL_TIMEOUT_RANDOMNESS = 10

local TARGET_SEARCH_DELAY = 10

local mob_stati = {}
local angerlist = {}

local function create_mob_status()
    return {
        angerlist = {},
    }
end

function Entity:change_anger(target, amount)
    local mob_status = mob_stati[self]
    if not mob_status then
        mob_status = create_mob_status()
        mob_stati[self] = mob_status
    end

    local anger = mob_status.angerlist[target] or 0
    mob_status.angerlist[target] = anger + amount
    mob_stati[self].update_target_timer = 0 -- Enforce looking for new target
end

local mob_config = require "scripts/monster/settings"

local function calculate_position_priority(x1, y1, x2, y2, anger, range)
    if math.floor(x1 / TILESIZE) == math.floor(x2 / TILESIZE) and
       math.floor(y1 / TILESIZE) == math.floor(y2 / TILESIZE)
    then
        -- Both on the same tile
        return anger * range
    end

    local path_length = get_path_length(x1, y1, x2, y2, range, "w")
    return (range - path_length) * anger
end

local function update_attack_ai(mob, tick)
    local config = mob_config[mob:name()]

    local target
    local target_priority
    local attack_x, attack_y

    local mob_status = mob_stati[mob]
    local timer = mob_status.update_target_timer
    if timer and timer > tick then
        return false
    end

    for _, being in ipairs(get_beings_in_circle(mob, config.trackrange)) do
        if being:type() == TYPE_CHARACTER
           and being:action() ~= ACTION_DEAD
        then
            local anger = mob_status.angerlist[being] or 0
            if anger == 0 and config.aggressive then
                anger = 1
            end

            local possible_attack_positions = {
                {
                    x = being:x() - config.attack_distance,
                    y = being:y(),
                },
                {
                    x = being:x(),
                    y = being:y() - config.attack_distance,
                },
                {
                    x = being:x() + config.attack_distance,
                    y = being:y(),
                },
                {
                    x = being:x(),
                    y = being:y() + config.attack_distance,
                },
            }
            for _, point in ipairs(possible_attack_positions) do
                local priority = calculate_position_priority(mob:x(),
                                                             mob:y(),
                                                             point.x,
                                                             point.y,
                                                             anger,
                                                             config.trackrange)

                if priority > 0 and (not target or priority > target_priority)
                then
                    target = being
                    target_priority = priority
                    attack_x, attack_y = point.x, point.y
                end
            end
        end
    end

    mob_status.update_target_timer = tick + TARGET_SEARCH_DELAY

    if not target then
        return false
    end

    local x, y = mob:position()
    if x == attack_x and y == attack_y then
        mob:use_ability(config.ability_id, target)
    else
        mob:walk(attack_x, attack_y)
    end
    return true
end

local function update_stroll_timer(mob_status, tick)

    mob_status.stroll_timer = tick + STROLL_TIMEOUT
                                   + math.random(STROLL_TIMEOUT_RANDOMNESS)
end

local function update_stroll(mob, tick)
    local mobconfig = mob_config[mob:name()]

    local mob_status = mob_stati[mob]
    local strollrange = mobconfig and mobconfig.strollrange or nil
    if (not mob_status.stroll_timer or mob_status.stroll_timer <= tick) and
        strollrange
    then
        local x, y = mob:position()
        local destination_x = math.random(x - strollrange, x + strollrange)
        local destination_y = math.random(y - strollrange, y + strollrange)
        if is_walkable(destination_x, destination_y) then
            mob:walk(destination_x, destination_y)
        end
        update_stroll_timer(mob_status, tick)
    end
end

local function remove_mob(mob)
    mob_stati[mob] = nil
end

local function update(mob, tick)
    local mob_status = mob_stati[mob]
    if not mob_status then
        mob_status = create_mob_status()
        mob_stati[mob] = mob_status
        on_remove(mob, remove_mob)
    end

    local stop_stroll = update_attack_ai(mob, tick)
    if stop_stroll then
        update_stroll_timer(mob_status, tick)
    else
        update_stroll(mob, tick)
    end
end

local function mob_attack(mob, target, ability_id)
    local config = mob_config[mob:name()]
    target:damage(mob, config.damage)
    mob:set_ability_cooldown(ability_id, 10)
end

local function mob_recharged(mob, ability_id)
    mob_stati[mob].update_target_timer = 0 -- Enforce looking for new target
end

local mob_attack_ability =
        get_ability_info("Monster attack/Basic Monster strike")
mob_attack_ability:on_use(mob_attack)
mob_attack_ability:on_recharged(mob_recharged)

-- Register all update functions for the ai
for _, monsterclass in pairs(get_monster_classes()) do
    monsterclass:on_update(update)
end
