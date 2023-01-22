-- called once before cyclic execution
function Init(gv) BLIND = Blind.new(BlindConfigFromMillis(500, 50000, 50000)) end

-- called every cycle
function Cycle(gv, now)
	gv.outputs.blind_up, gv.outputs.blind_down =
		BLIND:execute(now, gv.inputs.button_up, gv.inputs.button_down)
end
