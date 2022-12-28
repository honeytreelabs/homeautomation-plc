-- luacheck: globals INITIALIZED GV
---@diagnostic disable: undefined-global
if not INITIALIZED then
	INITIALIZED = true

	BLIND_SR = Blind.new(BlindConfigFromMillis(500, 50000, 50000))
	BLIND_KIZI2 = Blind.new(BlindConfigFromMillis(500, 50000, 50000))
	LIGHT_GROUND_OFFICE = Light.new("Ground Office")
end

GV.outputs.sr_raff_up, GV.outputs.sr_raff_down =
	BLIND_SR:execute(NOW, GV.inputs.sr_raff_up, GV.inputs.sr_raff_down)

GV.outputs.kizi_2_raff_up, GV.outputs.kizi_2_raff_down =
	BLIND_KIZI2:execute(NOW, GV.inputs.kizi_2_raff_up, GV.inputs.kizi_2_raff_down)

if GV.inputs.ground_office_light then
	GV.outputs.ground_office_light = LIGHT_GROUND_OFFICE:toggle()
end
