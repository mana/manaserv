--[[

 This file demonstrates how attributes are getting calculated and how they can
 be linked to each other.

 See http://doc.manasource.org/attributes.xml for more info.

--]]

local ATTR_EXP = 22
local ATTR_LEVEL = 23

local function recalculate_base_attribute(being, attribute)
    local old_base = being:base_attribute(attribute)
    local new_base = old_base
    if attribute == ATTR_ACCURACY then
        -- Provisional
        new_base = being:modified_attribute(ATTR_DEX)
    elseif attribute == ATTR_DEFENSE then
        new_base = 0.3 * being:modified_attribute(ATTR_VIT)
    elseif attribute == ATTR_DODGE then
        -- Provisional
        new_base = being:modified_attribute(ATTR_AGI)
    elseif attribute == ATTR_MAGIC_DODGE then
        -- TODO
        new_base = 1
    elseif attribute == ATTR_MAGIC_DEFENSE then
        -- TODO
        new_base = 0
    elseif attribute == ATTR_BONUS_ASPD then
        -- TODO
        new_base = 0
    elseif attribute == ATTR_HP_REGEN then
        local hp_per_sec = being:modified_attribute(ATTR_VIT) * 0.05
        new_base = hp_per_sec * TICKS_PER_HP_REGENERATION / 10
    elseif attribute == ATTR_HP then
        local hp = being:modified_attribute(ATTR_HP)
        local max_hp = being:modified_attribute(ATTR_MAX_HP)

        if hp > max_hp then
            new_base = new_base - hp - max_hp
        end
    elseif attribute == ATTR_MAX_HP then
        local vit = being:modified_attribute(ATTR_VIT)
        new_base = (vit + 3) * (vit + 20) * 0.125
    elseif attribute == ATTR_MOVE_SPEED_TPS then
        -- Provisional
        new_base = 3.0 + being:modified_attribute(ATTR_AGI) * 0.08
    elseif attribute == ATTR_INV_CAPACITY then
        -- Provisional
        new_base = 2000 + being:modified_attribute(ATTR_STR) * 180
    elseif attribute == ATTR_ABILITY_COOLDOWN then
        -- Provisional
        new_base = 100 - being:modified_attribute(ATTR_WIL)
    elseif attribute == ATTR_LEVEL then
        -- Provisional
        --new_base = 100 - 100 * math.pow(0.99999, being:base_attribute(ATTR_EXP))
        new_base = being:base_attribute(ATTR_EXP) / 20
    end

    if new_base ~= old_base then
        being:set_base_attribute(attribute, new_base)
    end

end

local function update_derived_attributes(being, attribute)
    if attribute == ATTR_STR then
        recalculate_base_attribute(being, ATTR_INV_CAPACITY)
    elseif attribute == ATTR_AGI then
        recalculate_base_attribute(being, ATTR_DODGE)
    elseif attribute == ATTR_VIT then
        recalculate_base_attribute(being, ATTR_MAX_HP)
        recalculate_base_attribute(being, ATTR_HP_REGEN)
        recalculate_base_attribute(being, ATTR_DEFENSE)
    elseif attribute == ATTR_INT then
        -- unimplemented
    elseif attribute == ATTR_WIL then
        recalculate_base_attribute(being, ATTR_ABILITY_COOLDOWN)
    elseif attribute == ATTR_EXP then
        recalculate_base_attribute(being, ATTR_LEVEL)
    end
end

on_recalculate_base_attribute(recalculate_base_attribute)
on_update_derived_attribute(update_derived_attributes)

function Entity:level()
    return math.floor(self:base_attribute(ATTR_LEVEL))
end

function Entity:give_experience(experience)
    local old_experience = self:base_attribute(ATTR_EXP)
    local old_level = self:level()
    self:set_base_attribute(ATTR_EXP, old_experience + experience)
    if self:level() > old_level then
        self:say("LEVELUP!!! " .. self:level())
        self:set_attribute_points(self:attribute_points() + 1)
        self:set_correction_points(self:correction_points() + 1)
    end
end

local mobs_config = require "scripts/monster/settings"

local exp_receiver = {}

-- Give EXP for monster kills
local function monster_damaged(mob, source, damage)

    local receiver = exp_receiver[mob] or { chars = {}, total = 0 }
    exp_receiver[mob] = receiver

    if source and source:type() == TYPE_CHARACTER then
        mob:change_anger(source, damage)

        local current_damage = receiver.chars[source]
        if not current_damage then
            on_remove(source, function(removed_being)
                receiver[removed_being] = nil
            end)
            on_death(source, function(removed_being)
                receiver[removed_being] = nil
            end)
        end

        current_damage = (current_damage or 0) + damage
        receiver.chars[source] = current_damage
        receiver.total = receiver.total + damage
    end

    if mob:base_attribute(ATTR_HP) == 0 then
        local mob_config = mobs_config[mob:name()]
        local experience = mob_config.experience or 0
        for char, damage in pairs(receiver.chars) do
            local gained_exp = damage / receiver.total * experience
            char:give_experience(gained_exp)
        end
    end
end

for _, monsterclass in pairs(get_monster_classes()) do
    monsterclass:on_damaged(monster_damaged)
end
