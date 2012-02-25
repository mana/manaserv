----------------------------------------------------------
-- Banker Function                                      --
----------------------------------------------------------------------------------
--  Copyright 2008 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana World.                                        --
--                                                                              --
--  The Mana World  is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

function Banker(npc, ch)
    if mana.being_get_gender(ch) == GENDER_MALE then
        do_message(npc, ch, "Welcome to the bank, sir!")
    elseif mana.being_get_gender(ch) == GENDER_FEMALE then
        do_message(npc, ch, "Welcome to the bank, madam!")
    else
        do_message(npc, ch, "Welcome to the bank... uhm... person of unspecified gender!")
    end
    local account = tonumber(get_quest_var(ch, "BankAccount"))
    local result = -1
    do_wait()

    if (account == nil) then --Initial account creation, if needed
        do_message(npc, ch, "Hello! Would you like to setup a bank account? There is a sign-on bonus right now!")
        result = do_choice(npc, ch, "Yes", "No")
        if (result == 1) then
            mana.chr_set_quest(ch, "BankAccount", 5)
            do_message(npc, ch, "Your account has been made. Your sign-on bonus is 5GP.")
            account = 5
        end
    end

    if (account ~= nil) then --If the player has an account
        local money = 0
        local input = 0
        result = 1
        while (result < 3) do --While they've choosen a valid option that isn't "Never mind"
            account = tonumber(get_quest_var(ch, "BankAccount")) --Why do I need to convert this?
            do_message(npc, ch, "Your balance: " .. account .. ".\nYour money: " .. mana.chr_money(ch) .. ".")
            result = do_choice(npc, ch, "Deposit", "Withdraw", "Never mind")
            if (result == 1) then --Deposit
                money = mana.chr_money(ch);
                if (money > 0) then --Make sure they have money to deposit
                    do_message(npc, ch, "How much would you like to deposit? (0 will cancel)")
                    input = do_ask_integer(npc, ch, 0, money, 1)
                    do_wait()
                    money = mana.chr_money(ch)
                    if (input > 0 and input <= money) then --Make sure something weird doesn't happen and they try to deposit more than they have
                        mana.chr_money_change(ch, -input)
                        mana.chr_set_quest(ch, "BankAccount", account + input)
                        do_message(npc, ch, input .. " GP deposited.")
                    elseif (input > money) then --Chosen more than they have
                        do_message(npc, ch, "You don't have that much money. But you just did....")
                    end
                else
                    do_message(npc, ch, "You don't have any money to deposit!")
                end
            elseif (result == 2) then --Withdraw
                if (account > 0) then --Make sure they have money to withdraw
                    do_message(npc, ch, "How much would you like to withdraw? (0 will cancel)")
                    input = do_ask_integer(npc, ch, 0, account, 1)
                    if (input > 0 and input <= account) then --Make sure something weird doesn't happen and they try to withdraw more than they have
                        mana.chr_money_change(ch, input)
                        mana.chr_set_quest(ch, "BankAccount", account - input)
                        do_message(npc, ch, input .. " GP withdrawn.")
                    elseif (input > account) then --Chosen more than they have
                        do_message(npc, ch, "You don't have that much in your account. But you just did....")
                    end
                else
                    do_message(npc, ch, "Your account is empty!")
                end
            end
        end --This ends the while loop
    end

    do_message(npc, ch, "Thank you. Come again!")
end
