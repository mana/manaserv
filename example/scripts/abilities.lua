--[[

 Abilities script file

 This file allows you to implement your ability action system. The system can
 for example implement magic, physical attack or also such mundane things as
 showing emoticons over the characters heads.

--]]

local spell1 = get_ability_info("Magic_Test Spell 1")
spell1:on_use(function(user, x, y, abilityId)
    target = target or user
    target:say("Kaaame...Haaame... HAAAAAA! " .. x .. " " .. y)
    user:set_ability_mana(abilityId, 0)
end)
spell1:on_recharged(function(ch) ch:say("Hoooooooo...") end)

local spell2 = get_ability_info(2)
spell2:on_use(function(user) user:say("HAA-DOKEN!") end)

get_ability_info(3):on_use(function(user) user:say("Sonic BOOM") end)
