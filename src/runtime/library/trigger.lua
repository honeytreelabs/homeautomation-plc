local result = {R_TRIG = {}, F_TRIG = {}}

function result.R_TRIG:new()
	return setmetatable({state = false}, {__index = self})
end

function result.R_TRIG:execute(input)
	local retval = false
	if not self.state and input then retval = true end
	self.state = input
	return retval
end

function result.F_TRIG:new()
	return setmetatable({state = false}, {__index = self})
end

function result.F_TRIG:execute(input)
	local retval = false
	if self.state and not input then retval = true end
	self.state = input
	return retval
end

return result
