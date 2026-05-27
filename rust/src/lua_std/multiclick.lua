MultiClick = {}

function MultiClick:new(period)
  return setmetatable({
    period = period,
    trigger = R_TRIG(),
  }, { __index = self })
end

function MultiClick:execute(now, input)
  if not self.start then
    if not self.trigger:execute(input) then
      return 0
    end
    self.start = now
    self.count = 1
    return 0
  end

  if now - self.start < self.period then
    if self.trigger:execute(input) then
      self.count = self.count + 1
    end
    return 0
  end

  self.start = nil
  return self.count
end
