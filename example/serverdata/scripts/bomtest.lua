-------------------------------------------------------------------------------
-- This file verifies that an UTF-8 BOM is correctly handled by manaserv ------

function testUtf8Bom()
    -- Dummy function, the test is really about whether the hidden BOM at the
    -- start of this file is skipped before tripping the Lua parser.
end
