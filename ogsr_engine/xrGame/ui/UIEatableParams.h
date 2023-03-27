#pragma once
#include "UIWindow.h"
#include "UIStatic.h"

class CInventoryItem;

class CUIEatableParams : public CUIWindow
{
public:
								CUIEatableParams		();
	virtual						~CUIEatableParams		();

	void 						Init					();
	void 						SetInfo					(CInventoryItem* obj);
	bool 						Check					(CInventoryItem* obj);

protected:
	enum {
		//instant
		_health_influence,
		_power_influence,
		_max_power_influence,
		_satiety_influence,
		_radiation_influence,
		_psy_health_influence,
		_alcohol_influence,
		_wounds_heal_influence,

		_max_influence_index,

		_health_boost = _max_influence_index,
		_power_boost,
		_max_power_boost,
		_satiety_boost,
		_radiation_boost,
		_psy_health_boost,
		_alcohol_boost,
		_wounds_heal_boost,

		_sprint_boost,
		_jump_boost,
		_max_weight_boost,

		_burn_imm_boost,
		_shock_imm_boost,
		_strike_imm_boost,
		_wound_imm_boost,
		_radiation_imm_boost,
		_telepatic_imm_boost,
		_chemical_burn_imm_boost,
		_explosion_imm_boost,
		_fire_wound_imm_boost,

		_boost_time,

		_max_item_index,
	};

	CUIStatic*					m_info_items[_max_item_index]{};
	CUIStatic*					influence_header{};
	CUIStatic*					boost_header{};
};