local multiclick = require("multiclick")
local luaunit = require("luaunit")

function Test_MultiClick()
	local m = multiclick.MultiClick:new(500)

	luaunit.assertEquals(m:execute(1000, false), 0)

	luaunit.assertEquals(m:execute(1000, true), 0)
	luaunit.assertEquals(m:execute(1100, true), 0)
	luaunit.assertEquals(m:execute(1200, true), 0)
	luaunit.assertEquals(m:execute(1300, true), 0)
	luaunit.assertEquals(m:execute(1500, true), 1)

	luaunit.assertEquals(m:execute(1000, true), 0)
	luaunit.assertEquals(m:execute(1501, false), 0)

	luaunit.assertEquals(m:execute(1000, true), 0)
	luaunit.assertEquals(m:execute(1050, false), 0)
	luaunit.assertEquals(m:execute(1100, true), 0)
	luaunit.assertEquals(m:execute(1150, false), 0)
	luaunit.assertEquals(m:execute(1200, true), 0)
	luaunit.assertEquals(m:execute(1250, false), 0)
	luaunit.assertEquals(m:execute(1501, true), 3)
	luaunit.assertEquals(m:execute(1502, false), 0)
end

os.exit(luaunit.LuaUnit.run())
