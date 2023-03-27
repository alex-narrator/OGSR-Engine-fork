#include "StdAfx.h"
#include "UIPanels.h"
#include "UIInventoryUtilities.h"
#include "UIXmlInit.h"
#include "inventory_item.h"
#include "Actor.h"
#include "Inventory.h"
#include "string_table.h"

void SetCounter(int count, float pos_x, float pos_y) {
	CUIStatic st_count;
	string256 str_count{};
	sprintf(str_count, "x%d", count);
	st_count.SetText(str_count);
	st_count.SetWndPos(pos_x, pos_y);
	st_count.Draw();
}

void TryAddToShowList(TIItemContainer& list, PIItem item, bool group) {
	if (!group) {
		list.push_back(item);
		return;
	}

	bool b_add{ true };
	for (const auto& _itm : list) {
		if (item->object().cNameSect() == _itm->object().cNameSect()) {
			b_add = false;
			break;
		}
	}
	if (b_add)
		list.push_back(item);
}

void CUIBeltPanel::InitFromXML	(CUIXml& xml, LPCSTR path, int index){
	CUIXmlInit::InitWindow		(xml, path, index, this);
	m_cell_size.x				= xml.ReadAttribFlt(path, index, "cell_width");
	m_cell_size.y				= xml.ReadAttribFlt(path, index, "cell_height");
	m_fScale					= xml.ReadAttribFlt(path, index, "scale");
	m_bGroupSimilar				= xml.ReadAttribFlt(path, index, "group_similar", 0);
	m_counter_offset.x			= xml.ReadAttribFlt(path, index, "counter_x", 0);
	m_counter_offset.y			= xml.ReadAttribFlt(path, index, "counter_y", 0);
}

void CUIBeltPanel::Update(){
	m_si.SetShader(InventoryUtilities::GetEquipmentIconsShader());
	m_vRects.clear();
	m_count.clear();

	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	auto& inv = pActor->inventory();

	TIItemContainer items_to_show{};
	for (const auto& itm : inv.m_belt) {
		TryAddToShowList(items_to_show, itm, m_bGroupSimilar);
	}

	for(const auto& _itm : items_to_show){
		if (_itm) {
			m_vRects.push_back( &(_itm->m_icon_params ) );
			auto item_sect = _itm->object().cNameSect().c_str();
			m_count.push_back(inv.GetSameItemCount(item_sect, false));
		}
	}
}

void CUIBeltPanel::Draw(){
	const float iIndent = 1.0f;
	      float x{};
		  float y{};
		  float iHeight;
		  float iWidth;

	Frect				rect;
	GetAbsoluteRect		(rect);
	x					= rect.left;
	y					= rect.top;	
	
	float _s			= m_cell_size.x/m_cell_size.y;

	for (int i = 0; i < m_vRects.size(); i++){
		const auto& params = m_vRects[i];

		params->set_shader( &m_si );
		const auto& r = params->original_rect();
		iHeight = m_fScale*(r.bottom - r.top);
		iWidth  = _s*m_fScale*(r.right - r.left);

		m_si.SetRect(0, 0, iWidth, iHeight);

		m_si.SetPos(x, y);
		if (m_count[i] > 1 && m_bGroupSimilar) {
			float pos_x = m_counter_offset.x + x;
			float pos_y = m_counter_offset.y + (y + iHeight);
			SetCounter(m_count[i], pos_x, pos_y);
		}
		x = x + iIndent + iWidth;

        m_si.Render();
	}

	CUIWindow::Draw();
}

/////////////////////////////////////////////////
/////////////////SLOT PANEL//////////////////////
/////////////////////////////////////////////////
void CUISlotPanel::InitFromXML(CUIXml& xml, LPCSTR path, int index){
	CUIXmlInit::InitWindow(xml, path, index, this);
	m_cell_size.x		= xml.ReadAttribFlt(path, index, "cell_width");
	m_cell_size.y		= xml.ReadAttribFlt(path, index, "cell_height");
	m_fScale			= xml.ReadAttribFlt(path, index, "scale");
	m_counter_offset.x	= xml.ReadAttribFlt(path, index, "counter_x", 0);
	m_counter_offset.y	= xml.ReadAttribFlt(path, index, "counter_y", 0);
}

void CUISlotPanel::Update(){
	m_st.SetShader(InventoryUtilities::GetEquipmentIconsShader());
	m_vRects.clear();
	m_action_key.clear();
	m_count.clear();

	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	auto& inv = pActor->inventory();

	for (u32 i = 0; i < inv.m_slots.size(); ++i) {
		const auto& _itm = inv.m_slots[i].m_pIItem;
		if (!_itm) continue;

		switch (i)
		{
		case QUICK_SLOT_0:
		case QUICK_SLOT_1:
		case QUICK_SLOT_2:
		case QUICK_SLOT_3:
		{
			if (_itm) {
				string16	slot_key{};
				sprintf_s(slot_key, "ui_use_slot_%d", _itm->GetSlot());
				m_action_key.push_back(CStringTable().translate(slot_key).c_str());
				m_vRects.push_back(&(_itm->m_icon_params));
				auto item_sect = _itm->object().cNameSect().c_str();
				m_count.push_back(inv.GetSameItemCount(item_sect, false));
			}
		}
		default:
			break;
		}
	}
}

void CUISlotPanel::Draw() {
	const float iIndent = 1.0f;
	float x{};
	float y{};
	float iHeight;
	float iWidth;

	Frect rect;
	GetAbsoluteRect(rect);
	x = rect.left;
	y = rect.top;

	float _s = m_cell_size.x / m_cell_size.y;

	for (int i = 0; i < m_vRects.size(); i++){
		const auto& params = m_vRects[i];

		params->set_shader(&m_st);
		const auto& r = params->original_rect();
		iHeight = m_fScale * (r.bottom - r.top);
		iWidth = _s * m_fScale * (r.right - r.left);

		m_st.SetWndRect(0, 0, iWidth, iHeight);

		m_st.SetWndPos(x, y);
		if (m_count[i] > 1) {
			float pos_x = m_counter_offset.x + x;
			float pos_y = m_counter_offset.y + (y + iHeight);
			SetCounter(m_count[i], pos_x, pos_y);
		}
		x = x + iIndent + iWidth;

		m_st.SetText(m_action_key[i].c_str());

		m_st.Draw();

	}

	CUIWindow::Draw();
}

/////////////////////////////////////////////////
/////////////////VEST PANEL//////////////////////
/////////////////////////////////////////////////
void CUIVestPanel::InitFromXML(CUIXml& xml, LPCSTR path, int index){
	CUIXmlInit::InitWindow(xml, path, index, this);
	m_cell_size.x		= xml.ReadAttribFlt(path, index, "cell_width");
	m_cell_size.y		= xml.ReadAttribFlt(path, index, "cell_height");
	m_fScale			= xml.ReadAttribFlt(path, index, "scale");
	m_bGroupSimilar		= xml.ReadAttribFlt(path, index, "group_similar", 0);
	m_counter_offset.x	= xml.ReadAttribFlt(path, index, "counter_x", 0);
	m_counter_offset.y	= xml.ReadAttribFlt(path, index, "counter_y", 0);
}

void CUIVestPanel::Update(){
	m_st.SetShader(InventoryUtilities::GetEquipmentIconsShader());
	m_vRects.clear();
	m_count.clear();

	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	auto& inv = pActor->inventory();

	TIItemContainer items_to_show{};
	for (const auto& itm : inv.m_vest) {
		TryAddToShowList(items_to_show, itm, m_bGroupSimilar);
	}

	for (const auto& _itm : items_to_show) {
		if (_itm) {
			m_vRects.push_back(&(_itm->m_icon_params));
			auto item_sect = _itm->object().cNameSect().c_str();
			m_count.push_back(inv.GetSameItemCount(item_sect, false));
		}
	}
}

void CUIVestPanel::Draw() {
	const float iIndent = 1.0f;
	float x{};
	float y{};
	float iHeight;
	float iWidth;

	Frect rect;
	GetAbsoluteRect(rect);
	x = rect.left;
	y = rect.top;

	float _s = m_cell_size.x / m_cell_size.y;

	for (int i = 0; i < m_vRects.size(); i++){
		const auto& params = m_vRects[i];

		params->set_shader(&m_st);
		const auto& r = params->original_rect();
		iHeight = m_fScale * (r.bottom - r.top);
		iWidth = _s * m_fScale * (r.right - r.left);

		m_st.SetRect(0, 0, iWidth, iHeight);

		m_st.SetPos(x, y);
		if (m_count[i] > 1 && m_bGroupSimilar) {
			float pos_x = m_counter_offset.x + x;
			float pos_y = m_counter_offset.y + (y + iHeight);
			SetCounter(m_count[i], pos_x, pos_y);
		}
		x = x + iIndent + iWidth;

		m_st.Render();
	}

	CUIWindow::Draw();
}