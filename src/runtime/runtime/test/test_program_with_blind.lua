-- luacheck: globals INITIALIZED GV
---@diagnostic disable: undefined-global
if not INITIALIZED then
	INITIALIZED = true

	BLIND_1 = Blind.new(BlindConfigFromMillis(500, 30000, 30000))
end

GV.outputs.output_up, GV.outputs.output_down =
	BLIND_1:execute(NOW, GV.inputs.input_up, GV.inputs.input_down)
