-- luacheck: globals INITIALIZED GV
---@diagnostic disable: undefined-global
if not INITIALIZED then
	INITIALIZED = true
	BLIND_SR = Blind.new(BlindConfigFromMillis(500, 30000, 30000))
end

BLIND_SR.execute(NOW, false, false)
