--[[

 Basic stroll ai

--]]

local STROLL_TIMEOUT = 20
local STROLL_TIMEOUT_RANDOMNESS = 10

-- Wrapping the monster update callback in order to do the stroll ai here
local old_on_update = MonsterClass.on_update
local update_functions = {}
function MonsterClass:on_update(callback)
    update_functions[self] = callback
end

local stroll_timer = {}
local angerlist = {}

local mob_config = require "scripts/monster/settings"

local function find_target(mob, config)
    local target
    local target_priority
    local attack_x, attack_y

    for _, being in ipairs(get_beings_in_circle(mob, config.trackrange)) do
        if being:type() == OBJECT_CHARACTER
           and being:action() ~= ACTION_DEAD
        then
            local anger = angerlist[being] or 0
            if anger == 0 and config.aggressive then
                anger = 1
            end

            local possible_attack_positions = {
                {
                    x = being:x() - config.attack_distance or TILESIZE,
                    y = being:y()
                },
                {

                    x = being:x()
                    y = being:y() - config.attack_distance or TILESIZE,
                },
                {

                    x = being:x() + config.attack_distance or TILESIZE,
                    y = being:y(),
                },
                {

                    x = being:x()
                    y = being:y() + config.attack_distance or TILESIZE,
                },
            }
            for _, point in ipairs(possible_attack_positions) do
                local priority = calculate_position_priority(mob:position(), point.x, point.y)
            end
            
           

        end
    end
end

local function stroll_update(mob, tick)
    local stroll_tick = stroll_timer[mob]
    local mobconfig = mob_config[mob:name()]


    local trackrange = mobconfig and mobconfig.trackrange or nil


    local strollrange = mobconfig and mobconfig.strollrange or nil
    if (not stroll_tick or stroll_tick <= tick) and strollrange then
        local x, y = mob:position()
        local destination_x = math.random(x - strollrange, x + strollrange)
        local destination_y = math.random(y - strollrange, y + strollrange)
        if is_walkable(destination_x, destination_y) then
            mob:walk(destination_x, destination_y)
        end
        stroll_timer[mob] = tick + STROLL_TIMEOUT
                            + math.random(STROLL_TIMEOUT_RANDOMNESS)
    end

    local monsterclass = get_monster_class(mob:monster_id())
    local update_function = update_functions[monsterclass]
    if update_function then
        return update_function(mob, tick)
    end
end

-- Register all update functions for strolling
for _, monsterclass in ipairs(get_monster_classes()) do
    old_on_update(monsterclass, stroll_update)
end
