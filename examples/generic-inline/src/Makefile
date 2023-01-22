../generic-inline.yaml: generic-inline.yaml blind_logic.lua
	yq ea ".tasks[0].programs[0].script = \"$$(cat blind_logic.lua)\"" generic-inline.yaml > ../generic-inline.yaml
