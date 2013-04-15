--[[

 Special action script file

 This file allows you to implement your special action system. The system can
 for example implement magic, physical attack or also such mundane things as
 showing emoticons over the characters heads.

--]]

local spell1 = get_special_info("Magic_Test Spell 1")
spell1:on_use(function(user, target, specialid)
    target = target or user
    target:say("Kaaame...Haaame... HAAAAAA!")
    user:set_special_mana(specialid, 0)
end)
spell1:on_recharged(function(ch) ch:say("Hoooooooo...") end)

local spell2 = get_special_info(2)
spell2:on_use(function(user) user:say("HAA-DOKEN!") end)

get_special_info(3):on_use(function(user) user:say("Sonic BOOM") end)
