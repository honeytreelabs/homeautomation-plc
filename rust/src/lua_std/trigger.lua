function R_TRIG(last)
  local self = { last = last or false }

  function self:execute(cur)
    cur = cur or false
    local ret = (not self.last) and cur
    self.last = cur
    return ret
  end

  return self
end

function F_TRIG(last)
  local self = { last = last or false }

  function self:execute(cur)
    cur = cur or false
    local ret = self.last and (not cur)
    self.last = cur
    return ret
  end

  return self
end
