#include "stdafx.h"
#include "UIArmorParams.h"
#include "UIXmlInit.h"

#include "string_table.h"
#include "inventory_item.h"
#include "CustomOutfit.h"
#include "Helmet.h"

constexpr auto ARMOR_PARAMS = "armor_params.xml";

LPCSTR armor_zone_names[] = {
	"st_armor_head",		//голова
	"st_armor_jav",			//щелепа
	"st_armor_neck",		//шия
	"st_armor_clavicle",	//ключиці
	"st_armor_spine2",		//груди верх
	"st_armor_spine1",		//груди низ
	"st_armor_spine",		//живіт
	"st_armor_pelvis",		//таз
	"st_armor_upperarm",	//плече
	"st_armor_forearm",		//передпліччя
	"st_armor_hand",		//рука (долоня)
	"st_armor_thigh",		//стегно
	"st_armor_calf",		//литка
	"st_armor_foot",		//стопа
	"st_armor_toe",			//пальці ніг
	"st_armor_hit_fraction",//заброньова травма
};

LPCSTR armor_static_names[] = {
	"armor_head",			//голова
	"armor_jav",			//щелепа
	"armor_neck",			//шия
	"armor_clavicle",		//ключиці
	"armor_spine2",			//груди верх
	"armor_spine1",			//груди низ
	"armor_spine",			//живіт
	"armor_pelvis",			//таз
	"armor_upperarm",		//плече
	"armor_forearm",		//передпліччя
	"armor_hand",			//рука (долоня)
	"armor_thigh",			//стегно
	"armor_calf",			//литка
	"armor_foot",			//стопа
	"armor_toe",			//пальці ніг
	"armor_hit_fraction",	//заброньова травма
};

CUIArmorParams::CUIArmorParams() {
}

CUIArmorParams::~CUIArmorParams() {
	for (u32 i = 0; i < _max_item_index; ++i) {
		CUIStatic* _s = m_info_items[i];
		xr_delete(_s);
	}
	xr_delete(armor_header);
}

void CUIArmorParams::Init() {
	CUIXml uiXml;
	uiXml.Init(CONFIG_PATH, UI_PATH, ARMOR_PARAMS);

	LPCSTR _base = "armor_params";
	if (!uiXml.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow(uiXml, _base, 0, this);

	for (u32 i = 0; i < _max_item_index; ++i) {
		strconcat(sizeof(_buff), _buff, _base, ":static_", armor_static_names[i]);
		if (uiXml.NavigateToNode(_buff, 0)) {
			m_info_items[i] = xr_new<CUIStatic>();
			CUIStatic* _s = m_info_items[i];
			_s->SetAutoDelete(false);
			CUIXmlInit::InitStatic(uiXml, _buff, 0, _s);
		}
	}

	strconcat(sizeof(_buff), _buff, _base, ":armor_header");
	armor_header = xr_new<CUIStatic>();
	armor_header->SetAutoDelete(false);
	CUIXmlInit::InitStatic(uiXml, _buff, 0, armor_header);
}

bool CUIArmorParams::Check(CInventoryItem* obj) {
	return obj->HasArmorToDisplay(_hit_fraction);
}

void CUIArmorParams::SetInfo(CInventoryItem* obj) {
	string128					text_to_show{};
	float						_h = 0.f;
	DetachAll();

	if (!obj) return;

	armor_header->SetWndPos(armor_header->GetWndPos().x, _h);
	_h += armor_header->GetWndSize().y;
	AttachChild(armor_header);

	for (int i = 0; i < _max_item_index; ++i) {
		CUIStatic* _s = m_info_items[i];

		float _val = (i == _hit_fraction) ? 
			obj->GetArmorHitFraction() * 100.f : 
			obj->GetArmorByBone(i);
		
		if (fis_zero(_val)) continue;

		if (auto outfit = smart_cast<CCustomOutfit*>(obj)) {
			if (!outfit->m_bIsHelmetBuiltIn && i < _neck)
				continue;
		}
		if (auto helmet = smart_cast<CHelmet*>(obj) && i > _jav)
			continue;

		auto zone_name = CStringTable().translate(armor_zone_names[i]).c_str();

		LPCSTR _sn = (i == _hit_fraction) ? 
			"%" :
			CStringTable().translate("st_armor_class").c_str();

		sprintf_s(text_to_show, "%s %.0f %s",
			zone_name,
			_val,
			_sn);

		_s->SetText(text_to_show);
		_s->SetWndPos(_s->GetWndPos().x, _h);
		_h += _s->GetWndSize().y;
		AttachChild(_s);
	}

	SetHeight(_h);
}