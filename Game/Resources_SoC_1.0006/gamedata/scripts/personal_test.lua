local item = db.actor:item_in_slot(6)
if item and item:is_power_consumer() and item:is_power_source_attached() then
	if item:get_power_level() < 100 then
		item:set_power_level(1)
	end
end