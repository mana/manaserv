--[[

 Allows to assign multiple functions to some callbacks

--]]

local monsterclass_update_old_callback = MonsterClass.on_update
local monsterclass_update_callbacks = {}

local function on_monsterclass_update(monsterclass, entity, tick)
    for _, func in ipairs(monsterclass_update_callbacks[monsterclass]) do
        func(entity, tick)
    end
end

function MonsterClass:on_update(func)
    if not monsterclass_update_callbacks[self] then
        monsterclass_update_old_callback(self, function(entity, tick)
            on_monsterclass_update(self, entity, tick)
        end)
        monsterclass_update_callbacks[self] = {}
    end
    table.insert(monsterclass_update_callbacks[self], func)
end
