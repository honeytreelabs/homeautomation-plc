---@diagnostic disable-next-line: unused-local
function Init(gv)
	---@diagnostic disable-next-line: undefined-global
	BLIND_1 = Blind.new(BlindConfigFromMillis(500, 50000, 50000))
	---@diagnostic disable-next-line: undefined-global
	BLIND_2 = Blind.new(BlindConfigFromMillis(500, 50000, 50000))
	---@diagnostic disable-next-line: undefined-global
	LIGHT_A = Light.new("A")
end

function Cycle(gv, now)
	gv.outputs.blind_1_up, gv.outputs.blind_1_down =
		BLIND_1:execute(now, gv.inputs.button_1_up, gv.inputs.button_1_down)

	gv.outputs.blind_2_up, gv.outputs.blind_2_down =
		BLIND_2:execute(now, gv.inputs.button_2_up, gv.inputs.button_2_down)

	if gv.inputs.light_remote then gv.outputs.light_a = LIGHT_A:toggle() end
end
