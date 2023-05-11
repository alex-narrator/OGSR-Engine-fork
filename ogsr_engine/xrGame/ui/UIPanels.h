#pragma once
#include "UIWindow.h"
#include "../UIStaticItem.h"
#include "UIIconParams.h"

class CUIXml;
// belt & vest panel
class CUIBeltPanel : public CUIWindow
{
public:
    CUIBeltPanel() = default;
    ~CUIBeltPanel() = default;

    virtual void Update();
    virtual void Draw();
    void InitFromXML(CUIXml& xml, LPCSTR path, int index);

protected:
    float m_fScale;
    CUIStatic m_st;
    bool m_bGroupSimilar{};
    Fvector2 m_counter_offset;
};
// quick slot panel
class CUISlotPanel : public CUIWindow
{
public:
    CUISlotPanel() = default;
    ~CUISlotPanel() = default;

    virtual void Update();
    virtual void Draw();
    void InitFromXML(CUIXml& xml, LPCSTR path, int index);

protected:
    float m_fScale;
    CUIStatic m_st;
    Fvector2 m_counter_offset;
    xr_vector<int> m_slots_list;
};