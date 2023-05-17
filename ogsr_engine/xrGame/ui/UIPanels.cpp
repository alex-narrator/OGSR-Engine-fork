#include "StdAfx.h"
#include "UIPanels.h"
#include "UIInventoryUtilities.h"
#include "UIXmlInit.h"
#include "inventory_item.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "Inventory.h"
#include "string_table.h"

using namespace InventoryUtilities;

void SetCounter(u32 color, int count, float pos_x, float pos_y)
{
    CUIStatic st_count;
    string256 str_count{};
    sprintf(str_count, "x%d", count);
    st_count.SetText(str_count);
    st_count.SetTextColor(color);
    st_count.SetWndPos(pos_x, pos_y);
    st_count.Draw();
}

void TryAddToShowList(TIItemContainer& list, PIItem item, bool group)
{
    if (!group)
    {
        list.push_back(item);
        return;
    }

    bool b_add{true};
    for (const auto& _itm : list)
    {
        if (item->object().cNameSect() == _itm->object().cNameSect())
        {
            b_add = false;
            break;
        }
    }
    if (b_add)
        list.push_back(item);
}

void SetTimer(u32 color, float time, float pos_x, float pos_y)
{
    CUIStatic st_timer;
    string256 str_time{};
    sprintf(str_time, "%.f %s", time, CStringTable().translate("st_time_minute").c_str());
    st_timer.SetText(str_time);
    st_timer.SetTextColor(color);
    st_timer.SetWndPos(pos_x, pos_y);
    st_timer.Draw();
}

void CUIBeltPanel::InitFromXML(CUIXml& xml, LPCSTR path, int index)
{
    CUIXmlInit::InitWindow(xml, path, index, this);
    m_fScale = xml.ReadAttribFlt(path, index, "scale");
    m_fIndent = xml.ReadAttribFlt(path, index, "indent", 1.f);
    m_bGroupSimilar = xml.ReadAttribFlt(path, index, "group_similar", 0);
    m_counter_offset.x = xml.ReadAttribFlt(path, index, "counter_x", 0);
    m_counter_offset.y = xml.ReadAttribFlt(path, index, "counter_y", 0);
    u_color = CUIXmlInit::GetColor(xml, path, index, color_argb(255, 255, 255, 255));
}

void CUIBeltPanel::Draw()
{
    m_st.SetShader(GetEquipmentIconsShader());

    Fvector2 pos{}, size{};

    Frect rect;
    GetAbsoluteRect(rect);
    pos.set(rect.left, rect.top);

    float k_x{UI()->get_current_kx()};

    auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    auto& inv = pActor->inventory();

    TIItemContainer items_to_show{};
    for (const auto& itm : inv.m_vest)
        TryAddToShowList(items_to_show, itm, m_bGroupSimilar);
    for (const auto& itm : inv.m_belt)
        TryAddToShowList(items_to_show, itm, m_bGroupSimilar);

    for (const auto& _itm : items_to_show)
    {
        auto& params = _itm->m_icon_params;

        params.set_shader(&m_st);
        const auto& r = params.original_rect();
        size.set(r.width(), r.height());
        size.mul(m_fScale);
        size.x *= k_x;

        m_st.SetWndRect(0, 0, size.x, size.y);

        m_st.SetWndPos(pos.x, pos.y);

        auto count = inv.GetSameItemCount(_itm->object().cNameSect().c_str(), false);
        if (count > 1)
        {
            float pos_x = m_counter_offset.x + pos.x;
            float pos_y = m_counter_offset.y + (pos.y + size.y);
            SetCounter(u_color, count, pos_x, pos_y);
        }
        pos.x = pos.x + m_fIndent + size.x;

        TryAttachIcons(&m_st, _itm, m_fScale);

        m_st.Draw();
    }

    CUIWindow::Draw();
}

/////////////////////////////////////////////////
/////////////////SLOT PANEL//////////////////////
/////////////////////////////////////////////////
void CUISlotPanel::InitFromXML(CUIXml& xml, LPCSTR path, int index)
{
    CUIXmlInit::InitWindow(xml, path, index, this);
    m_fScale = xml.ReadAttribFlt(path, index, "scale");
    m_fIndent = xml.ReadAttribFlt(path, index, "indent", 1.f);
    m_counter_offset.x = xml.ReadAttribFlt(path, index, "counter_x", 0);
    m_counter_offset.y = xml.ReadAttribFlt(path, index, "counter_y", 0);
    u_color = CUIXmlInit::GetColor(xml, path, index, color_argb(255, 255, 255, 255));

    m_slots_list.clear();
    LPCSTR m_slots_sect = xml.ReadAttrib(path, index, "slots", nullptr);
    if (m_slots_sect)
    {
        char buf[16];
        for (int i = 0; i < _GetItemCount(m_slots_sect); ++i)
        {
            auto slot = atoi(_GetItem(m_slots_sect, i, buf));
            if (slot < SLOTS_TOTAL)
                m_slots_list.push_back(slot);
        }
    }
}

void CUISlotPanel::Draw()
{
    m_st.SetShader(GetEquipmentIconsShader());
    m_st.SetTextColor(u_color);

    Fvector2 pos{}, size{};

    Frect rect;
    GetAbsoluteRect(rect);
    pos.set(rect.left, rect.top);

    float k_x{UI()->get_current_kx()};

    auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    auto& inv = pActor->inventory();

    string16 slot_key{};

    for (const auto& slot : m_slots_list)
    {
        const auto& _itm = inv.m_slots[slot].m_pIItem;
        if (!_itm)
            continue;

        auto& params = _itm->m_icon_params;

        params.set_shader(&m_st);
        const auto& r = params.original_rect();
        size.set(r.width(), r.height());
        size.mul(m_fScale);
        size.x *= k_x;

        m_st.SetWndRect(0, 0, size.x, size.y);

        m_st.SetWndPos(pos.x, pos.y);

        auto count = inv.GetSameItemCount(_itm->object().cNameSect().c_str(), false);
        if (count > 1)
        {
            float pos_x = m_counter_offset.x + pos.x;
            float pos_y = m_counter_offset.y + (pos.y + size.y);
            SetCounter(u_color, count, pos_x, pos_y);
        }
        pos.x = pos.x + m_fIndent + size.x;

        sprintf_s(slot_key, "ui_use_slot_%d", _itm->GetSlot());
        m_st.SetText(CStringTable().translate(slot_key).c_str());

        TryAttachIcons(&m_st, _itm, m_fScale);

        m_st.Draw();
    }

    CUIWindow::Draw();
}

/////////////////////////////////////////////////
/////////////////BOOSTER PANEL//////////////////////
/////////////////////////////////////////////////
void CUIBoosterPanel::InitFromXML(CUIXml& xml, LPCSTR path, int index)
{
    CUIXmlInit::InitWindow(xml, path, index, this);
    m_fScale = xml.ReadAttribFlt(path, index, "scale");
    m_fIndent = xml.ReadAttribFlt(path, index, "indent", 1.f);
    m_timer_offset.x = xml.ReadAttribFlt(path, index, "timer_x");
    m_timer_offset.y = xml.ReadAttribFlt(path, index, "timer_y");
    u_color = CUIXmlInit::GetColor(xml, path, index, color_argb(255, 255, 255, 255));
}

void CUIBoosterPanel::Draw()
{
    m_st.SetShader(GetEquipmentIconsShader());

    Fvector2 pos{}, size{};

    Frect rect;
    GetAbsoluteRect(rect);
    pos.set(rect.left, rect.top);
    float k_x{UI()->get_current_kx()};

    auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());

    LPCSTR _sect_to_draw{};
    for (const auto& item : pActor->conditions().GetBoosters())
    {
        auto& _b = item.second;
        auto sect = _b.s_BoostSection;
        if (sect == _sect_to_draw)
            continue;

        _sect_to_draw = sect;
        CIconParams params(sect);

        params.set_shader(&m_st);
        const auto& r = params.original_rect();
        size.set(r.width(), r.height());
        size.mul(m_fScale);
        size.x *= k_x;

        m_st.SetWndRect(0, 0, size.x, size.y);

        m_st.SetWndPos(pos.x, pos.y);

        float pos_x = m_timer_offset.x + pos.x;
        float pos_y = m_timer_offset.y + (pos.y + size.y);
        SetTimer(u_color, _b.f_BoostTime, pos_x, pos_y);
        pos.x = pos.x + m_fIndent + size.x;

        m_st.Draw();
    }

    CUIWindow::Draw();
}