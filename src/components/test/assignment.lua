gv:getInput('i'):set_int(gv:getInput('i'):as_int() + 1)
gv:getInput('b'):set_bool(not gv:getInput('b'):as_bool())
gv:getOutput('i'):set_int(gv:getOutput('i'):as_int() - 1)
gv:getOutput('b'):set_bool(not gv:getOutput('b'):as_bool())
