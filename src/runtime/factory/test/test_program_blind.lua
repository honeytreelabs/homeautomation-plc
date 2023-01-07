---@diagnostic disable: unused-local
---@diagnostic disable-next-line: undefined-global
function Init(gv) BLIND_SR = Blind.new(BlindConfigFromMillis(500, 30000, 30000)) end

function Cycle(gv, now) BLIND_SR.execute(now, false, false) end
