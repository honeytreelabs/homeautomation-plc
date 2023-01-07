---@diagnostic disable-next-line: unused-local
function Init(gv)
	---@diagnostic disable-next-line: undefined-global
	BLIND_SR = Blind.new(BlindConfigFromMillis(500, 50000, 50000))
	---@diagnostic disable-next-line: undefined-global
	BLIND_KIZI2 = Blind.new(BlindConfigFromMillis(500, 50000, 50000))
	---@diagnostic disable-next-line: undefined-global
	LIGHT_GROUND_OFFICE = Light.new("Ground Office")
end

function Cycle(gv, now)
	gv.outputs.sr_raff_up, gv.outputs.sr_raff_down =
		BLIND_SR:execute(now, gv.inputs.sr_raff_up, gv.inputs.sr_raff_down)

	gv.outputs.kizi_2_raff_up, gv.outputs.kizi_2_raff_down =
		BLIND_KIZI2:execute(now, gv.inputs.kizi_2_raff_up, gv.inputs.kizi_2_raff_down)

	if gv.inputs.ground_office_light then
		gv.outputs.ground_office_light = LIGHT_GROUND_OFFICE:toggle()
	end
end
