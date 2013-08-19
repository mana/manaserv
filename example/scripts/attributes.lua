--[[

 This file demonstrates how attributes are getting calculated and how they can
 be linked to each other.

 See http://doc.manasource.org/attributes.xml for more info.

--]]

local function recalculate_base_attribute(being, attribute_name)
    local old_base = being:base_attribute(attribute_name)
    local new_base = old_base
    if attribute == "Accuracy" then
        -- Provisional
        new_base = being:modified_attribute("Dexterity")
    elseif attribute == "Defense" then
        new_base = 0.3 * being:modified_attribute("Vitality")
    elseif attribute == "Dodge" then
        -- Provisional
        new_base = being:modified_attribute("Agility")
    elseif attribute == "M. dodge" then
        -- TODO
        new_base = 1
    elseif attribute == "M. defense" then
        -- TODO
        new_base = 0
    elseif attribute == "Bonus att. speed" then
        -- TODO
        new_base = 0
    elseif attribute == "HP regeneration" then
        local hp_per_sec = being:modified_attribute("Vitality") * 0.05
        new_base = hp_per_sec * TICKS_PER_HP_REGENERATION / 10
    elseif attribute == "HP" then
        local hp = being:modified_attribute("HP")
        local max_hp = being:modified_attribute("Max HP")

        if hp > max_hp then
            new_base = new_base - hp - max_hp
        end
    elseif attribute == "Max HP" then
        local vit = being:modified_attribute("Vitality")
        new_base = (vit + 3) * (vit + 20) * 0.125
    elseif attribute == "Movement speed" then
        -- Provisional
        new_base = 3.0 + being:modified_attribute("Agility") * 0.08
    elseif attribute == "Capacity" then
        -- Provisional
        new_base = 2000 + being:modified_attribute("Strength") * 180
    elseif attribute == "Global ability cooldown" then
        -- Provisional
        new_base = 100 - being:modified_attribute("Willpower")
    elseif attribute == "Level" then
        -- Provisional
        --new_base = 100 - 100 * math.pow(0.99999, being:base_attribute("XP"))
        new_base = being:base_attribute("XP") / 20
    end

    if new_base ~= old_base then
        being:set_base_attribute(attribute_name, new_base)
    end

end

local function update_derived_attributes(being, attribute)
    local attribute_name = attribute:name()
    if attribute_name == "Strength" then
        recalculate_base_attribute(being, "Capacity")
    elseif attribute_name == "Agility" then
        recalculate_base_attribute(being, "Dodge")
    elseif attribute_name == "Vitality" then
        recalculate_base_attribute(being, "Max HP")
        recalculate_base_attribute(being, "HP regeneration")
        recalculate_base_attribute(being, "Defense")
    elseif attribute_name == "Intelligence" then
        -- unimplemented
    elseif attribute_name == "Willpower" then
        recalculate_base_attribute(being, "Global ability cooldown")
    elseif attribute_name == "XP" then
        recalculate_base_attribute(being, "Level")
    end
end

on_recalculate_base_attribute(recalculate_base_attribute)
on_update_derived_attribute(update_derived_attributes)

function Entity:level()
    return math.floor(self:base_attribute("Level"))
end

function Entity:give_experience(experience)
    local old_experience = self:base_attribute("XP")
    local old_level = self:level()
    self:set_base_attribute("XP", old_experience + experience)
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

    if mob:base_attribute("HP") == 0 then
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
