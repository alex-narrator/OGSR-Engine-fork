#pragma once
#include "UIWindow.h"
#include "../UIStaticItem.h"
#include "UIIconParams.h"

class CUIXml;
// belt panel
class CUIBeltPanel : public CUIWindow
{
public:
    CUIBeltPanel() = default;
    ~CUIBeltPanel() = default;

    virtual void Draw();
    void InitFromXML(CUIXml& xml, LPCSTR path, int index);

protected:
    float m_fScale, m_fIndent;
    u32 u_color;
    CUIStatic m_st;
    bool m_bGroupSimilar{};
    Fvector2 m_counter_offset;
    bool m_bVertical{};
};
// quick slot panel
class CUISlotPanel : public CUIWindow
{
public:
    CUISlotPanel() = default;
    ~CUISlotPanel() = default;

    virtual void Draw();
    void InitFromXML(CUIXml& xml, LPCSTR path, int index);

protected:
    float m_fScale, m_fIndent;
    u32 u_color;
    CUIStatic m_st;
    Fvector2 m_counter_offset;
    xr_vector<int> m_slots_list;
    bool m_bVertical{};
};
// active boosters panel
class CUIBoosterPanel : public CUIWindow
{
public:
    CUIBoosterPanel() = default;
    ~CUIBoosterPanel() = default;

    virtual void Draw();
    void InitFromXML(CUIXml& xml, LPCSTR path, int index);

protected:
    float m_fScale, m_fIndent;
    u32 u_color;
    CUIStatic m_st;
    Fvector2 m_timer_offset;
    bool m_bVertical{};
};