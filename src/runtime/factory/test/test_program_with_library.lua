local trigger_1, light_1

---@diagnostic disable-next-line: unused-local
function Init(gv)
	---@diagnostic disable-next-line: undefined-global
	trigger_1 = R_TRIG.new()
	---@diagnostic disable-next-line: undefined-global
	light_1 = Light.new()
end

---@diagnostic disable-next-line: unused-local
function Cycle(gv, now)
	if trigger_1:execute(gv.inputs.input_1) then
		gv.outputs.output_1 = light_1:toggle()
	end
end
