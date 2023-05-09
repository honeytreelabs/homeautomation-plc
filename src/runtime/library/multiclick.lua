local trigger = require("trigger")

local result = {MultiClick = {}}

function result.MultiClick:new(period)
	return setmetatable({period = period, trigger = trigger.R_TRIG:new()},
	                    {__index = self})
end

function result.MultiClick.execute(self, now, input)
	if not self.start then
		if not self.trigger:execute(input) then return 0 end
		self.start = now
		self.cnt = 1
		return 0
	end
	if now - self.start < self.period then
		if self.trigger:execute(input) then self.cnt = self.cnt + 1 end
		return 0
	end
	self.start = nil
	return self.cnt
end

return result
