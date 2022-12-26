-- luacheck: globals INITIALIZED GV
---@diagnostic disable: undefined-global
if not INITIALIZED then
	INITIALIZED = true

	-- use imported module
	local haspretty, pretty = pcall(require, "pl.pretty")
	if haspretty then
		local foo = {one = 'two', three = 'four'}
		pretty.dump(foo)
	end
	return
end

if GV.inputs.foo then GV.outputs.bar = GV.outputs.bar + 1 end

