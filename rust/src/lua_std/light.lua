Light = {}

function Light:new(name)
  return setmetatable({
    name = name or "",
    state = false,
  }, { __index = self })
end

function Light:toggle()
  self.state = not self.state
  return self.state
end

function Light:getState()
  return self.state
end
