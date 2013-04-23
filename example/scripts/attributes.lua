--[[

 This file demonstrates how attributes are getting calculated and how they can
 be linked to each other.

 See http://doc.manasource.org/attributes.xml for more info.

--]]

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
    end
end

on_recalculate_base_attribute(recalculate_base_attribute)
on_update_derived_attribute(update_derived_attributes)
