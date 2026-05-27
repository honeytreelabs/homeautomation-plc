function BlindConfigFromMillis(periodIdle, periodUp, periodDown)
  return {
    period_idle = periodIdle * 1000,
    period_up = periodUp * 1000,
    period_down = periodDown * 1000
  }
end

Blind = {}

function Blind:new(cfg)
  local instance = {
    cfg = cfg,
    state = "idle",
    start = 0,
    outputs_up = false,
    outputs_down = false,
    up_trigger = R_TRIG(false),
    down_trigger = R_TRIG(false)
  }

  local function enter_idle(now, button_up, button_down)
    instance.state = "idle"
    instance.start = now
    instance.outputs_up = false
    instance.outputs_down = false
    instance.up_trigger = R_TRIG(button_up)
    instance.down_trigger = R_TRIG(button_down)
  end

  local function enter_up(now, button_up, button_down)
    instance.state = "up"
    instance.start = now
    instance.outputs_up = true
    instance.outputs_down = false
    instance.up_trigger = R_TRIG(button_up)
    instance.down_trigger = R_TRIG(button_down)
  end

  local function enter_down(now, button_up, button_down)
    instance.state = "down"
    instance.start = now
    instance.outputs_up = false
    instance.outputs_down = true
    instance.up_trigger = R_TRIG(button_up)
    instance.down_trigger = R_TRIG(button_down)
  end

  function instance:execute(now, button_up, button_down)
    if instance.state == "idle" then
      local up_triggered = instance.up_trigger:execute(button_up)
      local down_triggered = instance.down_trigger:execute(button_down)

      if now - instance.start >= instance.cfg.period_idle then
        if up_triggered then
          enter_up(now, button_up, button_down)
        elseif down_triggered then
          enter_down(now, button_up, button_down)
        end
      end
    elseif instance.state == "up" then
      if now - instance.start > instance.cfg.period_up
          or instance.up_trigger:execute(button_up)
          or instance.down_trigger:execute(button_down) then
        enter_idle(now, button_up, button_down)
      end
    elseif instance.state == "down" then
      if now - instance.start > instance.cfg.period_down
          or instance.up_trigger:execute(button_up)
          or instance.down_trigger:execute(button_down) then
        enter_idle(now, button_up, button_down)
      end
    end

    return instance.outputs_up, instance.outputs_down
  end

  return instance
end
