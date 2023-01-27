local blind_1, blind_2, light_a

---@diagnostic disable-next-line: unused-local
function Init(gv)
	---@diagnostic disable-next-line: undefined-global
	blind_1 = Blind.new(BlindConfigFromMillis(500, 50000, 50000))
	---@diagnostic disable-next-line: undefined-global
	blind_2 = Blind.new(BlindConfigFromMillis(500, 50000, 50000))
	---@diagnostic disable-next-line: undefined-global
	light_a = Light.new("A")
end

function Cycle(gv, now)
	gv.outputs.blind_1_up, gv.outputs.blind_1_down =
		blind_1:execute(now, gv.inputs.button_1_up, gv.inputs.button_1_down)

	gv.outputs.blind_2_up, gv.outputs.blind_2_down =
		blind_2:execute(now, gv.inputs.button_2_up, gv.inputs.button_2_down)

	if gv.inputs.light_remote then gv.outputs.light_a = light_a:toggle() end
end
