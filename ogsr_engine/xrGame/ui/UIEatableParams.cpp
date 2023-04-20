#include "stdafx.h"
#include "UIEatableParams.h"
#include "UIXmlInit.h"

#include "string_table.h"

#include "eatable_item.h"

constexpr auto EATABLE_PARAMS = "eatable_params.xml";

LPCSTR effect_names[] = {
	//instant
	"ui_inv_health",
	"ui_inv_power",
	"ui_inv_max_power",
	"ui_inv_satiety",
	"ui_inv_radiation",
	"ui_inv_psy_health",
	"ui_inv_alcohol",
	"ui_inv_wounds_heal",
	//boost
	"ui_inv_health_boost",
	"ui_inv_power",
	"ui_inv_max_power",
	"ui_inv_satiety",
	"ui_inv_radiation",
	"ui_inv_psy_health",
	"ui_inv_alcohol",
	"ui_inv_wounds_heal",
	//
	"ui_inv_sprint",
	"ui_inv_jump",
	//
	"ui_inv_weight",
	//
	"ui_inv_burn_protection",			// "(burn_imm)",
	"ui_inv_shock_protection",			// "(shock_imm)",
	"ui_inv_strike_protection",			// "(strike_imm)",
	"ui_inv_wound_protection",			// "(wound_imm)",
	"ui_inv_radiation_protection",		// "(radiation_imm)",
	"ui_inv_telepatic_protection",		// "(telepatic_imm)",
	"ui_inv_chemical_burn_protection",	// "(chemical_burn_imm)",
	"ui_inv_explosion_protection",		// "(explosion_imm)",
	"ui_inv_fire_wound_protection",		// "(fire_wound_imm)",

	"ui_inv_boost_time",
};

LPCSTR static_names[] = {
	//instant
	"eat_health",
	"eat_power",
	"eat_max_power",
	"eat_satiety",
	"eat_radiation",
	"eat_psyhealth",
	"eat_alcohol",
	"eat_wounds_heal",
	//boost
	"boost_health",
	"boost_power",
	"boost_max_power",
	"boost_satiety",
	"boost_radiation",
	"boost_psyhealth",
	"boost_alcohol",
	"boost_wounds_heal",
	//
	"boost_sprint",
	"boost_jump",
	//
	"boost_max_weight",
	//
	"burn_boost",
	"strike_boost",
	"shock_boost",
	"wound_boost",
	"radiation_boost",
	"telepatic_boost",
	"chemical_burn_boost",
	"explosion_boost",
	"fire_wound_boost",

	"boost_time",
};

CUIEatableParams::CUIEatableParams() {
}

CUIEatableParams::~CUIEatableParams() {
	for (u32 i = 0; i < _max_item_index; ++i){
		CUIStatic* _s = m_info_items[i];
		xr_delete(_s);
	}
	xr_delete(influence_header);
	xr_delete(boost_header);
}

void CUIEatableParams::Init() {
	CUIXml uiXml;
	uiXml.Init(CONFIG_PATH, UI_PATH, EATABLE_PARAMS);

	LPCSTR _base = "eatable_params";
	if (!uiXml.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow(uiXml, _base, 0, this);

	for (u32 i = 0; i < _max_item_index; ++i){
		strconcat(sizeof(_buff), _buff, _base, ":static_", static_names[i]);
		if (uiXml.NavigateToNode(_buff, 0)){
			m_info_items[i] = xr_new<CUIStatic>();
			CUIStatic* _s = m_info_items[i];
			_s->SetAutoDelete(false);
			CUIXmlInit::InitStatic(uiXml, _buff, 0, _s);
		}
	}

	strconcat(sizeof(_buff), _buff, _base, ":influence_header");
	influence_header = xr_new<CUIStatic>();
	influence_header->SetAutoDelete(false);
	influence_header->Show(false);
	CUIXmlInit::InitStatic(uiXml, _buff, 0, influence_header);

	strconcat(sizeof(_buff), _buff, _base, ":boost_header");
	boost_header = xr_new<CUIStatic>();
	boost_header->SetAutoDelete(false);
	boost_header->Show(false);
	CUIXmlInit::InitStatic(uiXml, _buff, 0, boost_header);

}

bool CUIEatableParams::Check(CInventoryItem* obj) {
	return smart_cast<CEatableItem*>(obj);
}

void CUIEatableParams::SetInfo(CInventoryItem* obj) {
	string128					text_to_show{};
	float						_h = 0.f;
	DetachAll();

	if (!obj) return;

	auto pEatable = smart_cast<CEatableItem*>(obj);

	if (pEatable->IsInfluencer()) {
		influence_header->Show(true);
		influence_header->SetWndPos(influence_header->GetWndPos().x, _h);
		_h += influence_header->GetWndSize().y;
		AttachChild(influence_header);
	}

	for (int i = 0; i < _max_influence_index; ++i) {
		CUIStatic* _s = m_info_items[i];

		float _val = pEatable->GetItemInfluence(i);
		if (fis_zero(_val)) continue;

		auto effect_name = CStringTable().translate(effect_names[i]).c_str();

		_val *= 100.0f;
        if (i == _max_power_influence)
            _val *= -1.f;
		LPCSTR _sn = "%";
		LPCSTR _color = (_val > 0) ? "%c[green]" : "%c[red]";
		if (i == _radiation_influence || i == _max_power_influence)
			_color = (_val > 0) ? "%c[red]" : "%c[green]";

		sprintf_s(text_to_show, "%s %s %+.1f %s",
			effect_name,
			_color,
			_val,
			_sn);

		_s->SetText(text_to_show);
		_s->SetWndPos(_s->GetWndPos().x, _h);
		_h += _s->GetWndSize().y;
		AttachChild(_s);
	}

	if (pEatable->IsBooster()) {
		boost_header->Show(true);
		boost_header->SetWndPos(boost_header->GetWndPos().x, _h);
		_h += boost_header->GetWndSize().y;
		AttachChild(boost_header);
	}

	for (u32 i = _max_influence_index; i < _max_item_index; ++i) {
		CUIStatic* _s = m_info_items[i];

		bool b_for_time = (i == _boost_time);
		float _val = b_for_time ? pEatable->GetItemBoostTime() : pEatable->GetItemBoost(i - _max_influence_index);
		if (fis_zero(_val)) continue;

		auto boost_name = CStringTable().translate(effect_names[i]).c_str();

		LPCSTR _sn{};
		LPCSTR _color{};

		if (i != _boost_time) {
			_val *= 100.0f;
			_sn = "%";
			_color = (_val > 0) ? "%c[green]" : "%c[red]";
			if (i == _radiation_boost || i == _max_power_boost)
				_color = (_val > 0) ? "%c[red]" : "%c[green]";

			sprintf_s(text_to_show, "%s %s %+.1f %s",
				boost_name,
				_color,
				_val,
				_sn);
		}else{
			_sn = CStringTable().translate("st_time_minute").c_str();
			_color = "%c[default]";

			sprintf_s(text_to_show, "%s %s %.1f %s",
				boost_name,
				_color,
				_val,
				_sn);
		}

		_s->SetText(text_to_show);
		_s->SetWndPos(_s->GetWndPos().x, _h);
		_h += _s->GetWndSize().y;
		AttachChild(_s);
	}

	SetHeight(_h);
}