---@diagnostic disable-next-line: unused-local
function Init(gv)
	---@diagnostic disable-next-line: undefined-global
	local haspretty, pretty = pcall(require, "pl.pretty")
	if haspretty then
		local foo = {one = 'two', three = 'four'}
		pretty.dump(foo)
	end
end

---@diagnostic disable-next-line: unused-local
function Cycle(gv, now)
	if gv.inputs.foo then gv.outputs.bar = gv.outputs.bar + 1 end
end
