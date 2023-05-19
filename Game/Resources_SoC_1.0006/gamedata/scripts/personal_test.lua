local item = db.actor:item_in_slot(6)
if item and item:is_power_consumer() and item:is_power_source_attached() then
	if item:get_power_level() < 100 then
		item:set_power_level(1)
	end
end

-- local actor = db.actor
-- local h = hit()
-- h.draftsman = actor 
-- h.type = hit.fire_wound 
-- h.direction = vector():set(0,0,0) 
-- h:bone("bip01_head")
-- h.power = 0.11
-- h.impulse = 0.0 
-- actor:hit(h)