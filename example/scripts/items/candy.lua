--[[

 Example item script.

 Makes the player character say "*munch*munch*munch*" when using a candy.
 The HP regeneration effect is handled separately based on the heal value in
 items.xml.

--]]

local candy = mana.get_item_class("candy")

candy:on("use", function(user)
    mana.being_say(user, "*munch*munch*munch*")
end)
