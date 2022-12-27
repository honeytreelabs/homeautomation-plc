-- luacheck: globals Library
---@diagnostic disable: undefined-global
local luaunit = require("luaunit")

function Test_R_TRIG()
	local trigger = R_TRIG.new()
	luaunit.assertEquals(trigger:execute(false), false)
	luaunit.assertEquals(trigger:execute(true), true)
	luaunit.assertEquals(trigger:execute(false), false)
	luaunit.assertEquals(trigger:execute(false), false)
	luaunit.assertEquals(trigger:execute(true), true)
end

function Test_F_TRIG()
	local trigger = F_TRIG.new()
	luaunit.assertEquals(trigger:execute(false), false)
	luaunit.assertEquals(trigger:execute(true), false)
	luaunit.assertEquals(trigger:execute(true), false)
	luaunit.assertEquals(trigger:execute(false), true)
	luaunit.assertEquals(trigger:execute(false), false)
end

function Test_Light()
	local light = Light.new("stairs")
	luaunit.assertEquals(light:getState(), false)
	luaunit.assertEquals(light:toggle(), true)
	luaunit.assertEquals(light:toggle(), false)
	luaunit.assertEquals(light:getState(), false)
end

TEST_RESULT = luaunit.LuaUnit.run()
