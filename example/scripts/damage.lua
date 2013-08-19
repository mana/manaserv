--[[

 Offers damage functions

--]]

local monster_class_damage_functions = {}

function MonsterClass:on_damaged(func)
    monster_class_damage_functions[self] = func
end

-- damage is a table with these keys:
-- base, delta, chance_to_hit
function Entity:damage(source, damage)
    local hp_loss = math.random(damage.base, damage.base + damage.delta)
    local dodge = self:modified_attribute("Dodge")

    if dodge > 0 and math.random(dodge) > math.random(damage.chance_to_hit) or
       damage.chance_to_hit == 0
    then
        hp_loss = 0 -- attack missed
        self:say("HAHA MISSED")
    else
        local defense = self:modified_attribute("Defense")
        local randomness = hp_loss > 16 and math.random(hp_loss / 16) or 0
        hp_loss = hp_loss * (1 - (0.0159375 * defense) / (1 + 0.017 * defense))
                          + randomness
        self:say("UH NO")
    end

    if hp_loss > 0 then
        local hp = self:base_attribute("HP")
        hp_loss = math.min(hp, hp_loss)
        self:add_hit_taken(hp_loss)
        self:set_base_attribute("HP", hp - hp_loss)
        self:say("I GOT DAMAGED " .. hp - hp_loss)

        if self:type() == TYPE_MONSTER then
            local class = get_monster_class(self:monster_id())
            local callback = monster_class_damage_functions[class]
            if callback then
                return callback(self, source, hp_loss)
            end
        end
    else
        hp_loss = 0
    end
end
