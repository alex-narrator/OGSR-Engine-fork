#include "stdafx.h"
#include "UIPropertiesBox.h"
#include "../hudmanager.h"
#include "../level.h"
#include "UIListBoxItem.h"
#include "UIXmlInit.h"

#include "string_table.h"

constexpr auto OFFSET_X = 5;
constexpr auto OFFSET_Y = 5;
constexpr auto FRAME_BORDER_WIDTH = 20;
constexpr auto FRAME_BORDER_HEIGHT = 22;

#define ITEM_HEIGHT (GetFont()->CurrentHeight() + 2.0f)

CUIPropertiesBox::CUIPropertiesBox()
{
    SetFont(HUD().Font().pFontArial14);
    m_UIListWnd.SetImmediateSelection(true);
    SetWindowName("property_box");
    // custom script actions for properties box
    constexpr LPCSTR custom_action_sect = "custom_properties_box_action";
    if (pSettings->section_exist(custom_action_sect))
    {
        u32 action_count = pSettings->line_count(custom_action_sect);
        LPCSTR name, value;
        string128 str{};
        for (u32 i = 0; i < action_count; ++i)
        {
            pSettings->r_line(custom_action_sect, i, &name, &value);
            xr_vector<shared_str> vect{_GetItem(value, 1, str), _GetItem(value, 2, str)};
            m_custom_actions_map.emplace(std::move(_GetItem(value, 0, str)), std::move(vect));
        }
    }
}

CUIPropertiesBox::~CUIPropertiesBox() {}

void CUIPropertiesBox::Init(float x, float y, float width, float height)
{
    inherited::Init(x, y, width, height);

    AttachChild(&m_UIListWnd);

    CUIXml xml_doc;
    xml_doc.Init(CONFIG_PATH, UI_PATH, "inventory_new.xml");

    LPCSTR t = xml_doc.Read("properties_box:texture", 0, "");
    R_ASSERT(t);
    InitTexture(t);

    CUIXmlInit::InitListBox(xml_doc, "properties_box:list", 0, &m_UIListWnd);

    m_UIListWnd.Init(OFFSET_X, OFFSET_Y, width - OFFSET_X * 2, height - OFFSET_Y * 2);
}

void CUIPropertiesBox::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == &m_UIListWnd)
    {
        if (msg == LIST_ITEM_CLICKED)
        {
            GetMessageTarget()->SendMessage(this, PROPERTY_CLICKED, pData);
            Hide();
        }
    }
    inherited::SendMessage(pWnd, msg, pData);
}

bool CUIPropertiesBox::AddItem(const char* str, void* pData, u32 tag_value)
{
    CUIListBoxItem* itm = m_UIListWnd.AddItem(str);
    itm->SetTAG(tag_value);
    itm->SetData(pData);

    return true;
}
void CUIPropertiesBox::RemoveItemByTAG(u32 tag) { m_UIListWnd.RemoveWindow(m_UIListWnd.GetItemByTAG(tag)); }

void CUIPropertiesBox::RemoveAll() { m_UIListWnd.Clear(); }

void CUIPropertiesBox::Show(const Frect& parent_rect, const Fvector2& point)
{
    Fvector2 prop_pos{};
    Fvector2 prop_size = GetWndSize();

    if (point.x - prop_size.x > parent_rect.x1 && point.y + prop_size.y < parent_rect.y2)
    {
        prop_pos.set(point.x - prop_size.x, point.y);
    }
    else if (point.x - prop_size.x > parent_rect.x1 && point.y - prop_size.y > parent_rect.y1)
    {
        prop_pos.set(point.x - prop_size.x, point.y - prop_size.y);
    }
    else if (point.x + prop_size.x < parent_rect.x2 && point.y - prop_size.y > parent_rect.y1)
    {
        prop_pos.set(point.x, point.y - prop_size.y);
    }
    else
        prop_pos.set(point.x, point.y);

    SetWndPos(prop_pos);

    inherited::Show(true);
    inherited::Enable(true);

    ResetAll();

    GetParent()->SetMouseCapture(this, true);
    m_pOrignMouseCapturer = this;
    m_UIListWnd.Reset();
}

void CUIPropertiesBox::Hide()
{
    CUIWindow::Show(false);
    CUIWindow::Enable(false);
    CUIWindow::Reset();

    if (GetParent()->GetMouseCapturer() == this)
        GetParent()->SetMouseCapture(this, false);
}

bool CUIPropertiesBox::OnMouse(float x, float y, EUIMessages mouse_action)
{
    bool cursor_on_box;

    if (x >= 0 && x < GetWidth() && y >= 0 && y < GetHeight())
        cursor_on_box = true;
    else
        cursor_on_box = false;

    if (mouse_action == WINDOW_LBUTTON_DOWN && !cursor_on_box)
    {
        Hide();
        return true;
    }

    return inherited::OnMouse(x, y, mouse_action);
}

void CUIPropertiesBox::AutoUpdateSize()
{
    SetHeight(m_UIListWnd.GetItemHeight() * m_UIListWnd.GetSize() + m_UIListWnd.GetVertIndent());
    m_UIListWnd.SetHeight(GetHeight());
    float f = float(m_UIListWnd.GetLongestLength() + m_UIListWnd.GetHorizIndent()) + 2;
    SetWidth(_max(20.0f, f));
    m_UIListWnd.SetWidth(_max(20.0f, f));
    m_UIListWnd.UpdateChildrenLenght();
}

CUIListBoxItem* CUIPropertiesBox::GetClickedItem() { return m_UIListWnd.GetSelectedItem(); }
void CUIPropertiesBox::Update() { inherited::Update(); }
void CUIPropertiesBox::Draw() { inherited::Draw(); }

bool CUIPropertiesBox::OnKeyboard(int dik, EUIMessages keyboard_action)
{
    if (dik != get_action_dik(kADDITIONAL_ACTION))
        Hide();
    return true;
}

bool CUIPropertiesBox::CheckCustomActions(CScriptGameObject* obj)
{
    bool res{};
    for (const auto& action : m_custom_actions_map)
    {
        if (luabind::functor<bool> m_functorHasAction; ai().script_engine().functor(action.second[0].c_str(), m_functorHasAction))
        {
            if (m_functorHasAction(obj))
            {
                LPCSTR tip_text{};
                if (luabind::functor<LPCSTR> tip_func; ai().script_engine().functor(action.first.c_str(), tip_func))
                    tip_text = tip_func(obj);
                else
                    tip_text = CStringTable().translate(action.first).c_str();

                AddItem(tip_text, (void*)action.first.c_str(), INVENTORY_CUSTOM_ACTION);
                res = true;
            }
        }
        else
            Msg("!Item-action-condition function [%s] not exist.", action.second[0].c_str());
    }
    return res;
}

void CUIPropertiesBox::ProcessCustomActions(CScriptGameObject* obj)
{
    auto it = m_custom_actions_map.find((LPCSTR)GetClickedItem()->GetData());
    if (luabind::functor<void> m_functorDoAction; ai().script_engine().functor(it->second[1].c_str(), m_functorDoAction))
        m_functorDoAction(obj);
    else
        Msg("!Item-action function [%s] not exist.", it->second[1].c_str());
}