-- luacheck: globals INITIALIZED GV
---@diagnostic disable: undefined-global
if not INITIALIZED then
	INITIALIZED = true
	TRIGGER_1 = R_TRIG.new()
	LIGHT_1 = Light.new()
end

if TRIGGER_1:execute(GV.inputs.input_1) then
	GV.outputs.output_1 = LIGHT_1:toggle()
end
