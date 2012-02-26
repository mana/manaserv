--[[

 Example item script.

 Makes the player character say "*munch*munch*munch*" when using this item.
 The HP regeneration effect is handled separately based on the heal value in
 items.xml.

--]]

function use_candy(user)
    mana.being_say(user, "*munch*munch*munch*")
end
