--[[

 Offers damage functions

--]]


-- damage is a table with they keys:
-- base, delta, chance_to_hit
function Entity:damage(source, damage)
    local hp_loss = math.random(damage.base, damage.base + damage.delta)
    local dodge = self:modified_attribute(ATTR_DODGE)

    if math.random(dodge) > math.random(damage.chance_to_hit) then
        hp_loss = 0 -- attack missed
    else
        local defense = self:modified_attribute(ATTR_DEFENSE)
        local randomness = hp_loss > 16 and math.random(hp_loss / 16) or 0
        hp_loss = hp_loss * (1 - (0.0159375 * defense) / (1 + 0.017 * defense))
                          + randomness
    end

    if hp_loss > 0 then
        local hp = self:modified_attribute(ATTR_HP)
        hp_loss = math.min(hp, hp_loss)
        self:add_hit_taken(hp_loss)
        self:set_base_attribute(ATTR_HP, hp - hp_loss)
    else
        hp_loss = 0
    end
end
