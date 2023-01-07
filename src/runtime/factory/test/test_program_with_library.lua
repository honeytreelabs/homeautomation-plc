---@diagnostic disable-next-line: unused-local
function Init(gv)
	---@diagnostic disable-next-line: undefined-global
	TRIGGER_1 = R_TRIG.new()
	---@diagnostic disable-next-line: undefined-global
	LIGHT_1 = Light.new()
end

---@diagnostic disable-next-line: unused-local
function Cycle(gv, now)
	if TRIGGER_1:execute(gv.inputs.input_1) then
		gv.outputs.output_1 = LIGHT_1:toggle()
	end
end
