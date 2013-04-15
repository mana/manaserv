--[[

 This file provides an example of a simple crafting system.

--]]

-- a primitive example crafting system which cares about item order and exact amount
local function craft_strict(ch, recipe)
    if (recipe[1].id == 8 and recipe[1].amount == 2 and -- has two iron
        recipe[2].id == 9 and recipe[2].amount == 1)    -- and one wood
        then
        ch:inv_change(8, -2, --take away the iron
                      9, -1, --take away the wood
                      5, 1 ) -- give a sword
        ch:message("You've crafted a sword")
        return
    end
    ch:message("This wouldn't create anything useful")
end

-- this turns multiple occurences of the same item into one by adding up
-- their amounts and sorts the recipe by item ID.
-- This makes stuff a lot easier when your crafting system isn't supposed to
-- care about the order items are in.
local function make_condensed_and_sorted_item_list(recipe)
    local condensed = {}
    for index, item in pairs(recipe) do
        if condensed[item.id] == nil then
            condensed[item.id] = item.amount
        else
            condensed[item.id] = condensed[item.id] + item.amount
        end
    end

    local sorted = {}
    for id, amount in pairs(condensed) do
        local item = {}
        item.id = id
        item.amount = amount
        table.insert(sorted, item)
    end

    table.sort(sorted, function(item1, item2)
        return (item1.id < item2.id)
    end)

    return sorted
end

-- a primitive example crafting system which doesn't care about item order
-- and amount. It even allows to mention the same item multiple times.
local function craft_lax(ch, recipe)
    recipe = make_condensed_and_sorted_item_list(recipe)

    if (recipe[1].id == 8 and recipe[1].amount >= 2 and -- has at least two iron
        recipe[2].id == 9 and recipe[2].amount >= 1)    -- and at least one wood
        then
        ch:inv_change(8, -2, -- take away the iron
                      9, -1, -- take away the wood
                      5, 1 ) -- give a sword
        ch:message("You've crafted a sword")
        return
    end
    ch:message("This wouldn't create anything useful")
end

-- This function is registered with the game engine to use when a character
-- tries to craft something from items in its inventory.
local function craft(ch, recipe)
    -- ch is the crafting character
    --
    -- recipe is a table with the ingredients.
    -- it is a common 1-based array. each element of this array is a table with
    -- the two keys "id" and "amount".
    -- The engine has already checked that the character owns enough of those
    -- things, so you needn't do this again.

    -- uncomment one (but not both!) of the following three lines to enable the
    -- example crafting systems

    ch:message("There is no crafting in this game world.")
    --craft_strict(ch, recipe)
    --craft_lax(ch, recipe)
end

on_craft(craft)
