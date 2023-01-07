---@diagnostic disable-next-line: unused-local
function Init(gv)
	---@diagnostic disable-next-line: undefined-global
	BLIND_1 = Blind.new(BlindConfigFromMillis(500, 30000, 30000))
end

function Cycle(gv, now)
	gv.outputs.output_up, gv.outputs.output_down =
		BLIND_1:execute(now, gv.inputs.input_up, gv.inputs.input_down)
end
