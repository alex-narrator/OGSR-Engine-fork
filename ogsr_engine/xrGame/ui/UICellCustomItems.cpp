#include "stdafx.h"
#include "UICellCustomItems.h"
#include "UIInventoryUtilities.h"
#include "Weapon.h"
#include "WeaponMagazined.h"
#include "UIDragDropListEx.h"

#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "Actor.h"
#include "Inventory.h"
#include "UIInventoryWnd.h"
#include "UICursor.h"
#include "UIXmlInit.h"
#include "string_table.h"

constexpr auto INV_GRID_WIDTHF = 50.0f;
constexpr auto INV_GRID_HEIGHTF = 50.0f;

CUIInventoryCellItem::CUIInventoryCellItem(CInventoryItem* itm)
{
    m_pData = (void*)itm;
    itm->m_cell_item = this;

    itm->m_icon_params.set_shader(this);

    m_grid_size.set(itm->GetGridWidth(), itm->GetGridHeight());
    b_auto_drag_childs = true;
}

bool CUIInventoryCellItem::EqualTo(CUICellItem* itm)
{
    CUIInventoryCellItem* ci = smart_cast<CUIInventoryCellItem*>(itm);
    if (!itm)
        return false;

    // Real Wolf: Колбек на группировку и само регулирование группировкой предметов. 12.08.2014.
    auto item1 = (CInventoryItem*)m_pData;
    auto item2 = (CInventoryItem*)itm->m_pData;

    if (item1->m_always_ungroupable || item2->m_always_ungroupable)
        return false;
    if (item1->m_flags.test(CInventoryItem::FIUngroupable) || item2->m_flags.test(CInventoryItem::FIUngroupable))
        return false;

    g_actor->callback(GameObject::eUIGroupItems)(item1->object().lua_game_object(), item2->object().lua_game_object());

    auto fl1 = item1->m_flags;
    auto fl2 = item2->m_flags;

    item1->m_flags.set(CInventoryItem::FIUngroupable, false);
    item2->m_flags.set(CInventoryItem::FIUngroupable, false);

    if (fl1.test(CInventoryItem::FIUngroupable) || fl2.test(CInventoryItem::FIUngroupable))
        return false;

    bool b_script_equal{true};
    if (pSettings->line_exist("engine_callbacks", "is_cell_items_equal"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "is_cell_items_equal");
        if (luabind::functor<bool> lua_function; ai().script_engine().functor(callback, lua_function))
            b_script_equal = lua_function(item1->object().lua_game_object(), item2->object().lua_game_object());
    }

    return (fsimilar(object()->GetCondition(), ci->object()->GetCondition(), 0.01f) && 
            fsimilar(object()->Weight(), ci->object()->Weight(), 0.01f) &&
            fsimilar(object()->GetItemEffect(CInventoryItem::eRadiationRestoreSpeed), ci->object()->GetItemEffect(CInventoryItem::eRadiationRestoreSpeed), 0.01f) &&
            object()->object().cNameSect() == ci->object()->object().cNameSect() && 
            object()->m_eItemPlace == ci->object()->m_eItemPlace &&
            object()->Cost() == ci->object()->Cost() && 
            object()->GetMarked() == ci->object()->GetMarked() && 
            b_script_equal
        );
}

CUIInventoryCellItem::~CUIInventoryCellItem()
{
    if (auto item = object())
        item->m_cell_item = NULL;
}

void CUIInventoryCellItem::OnFocusReceive()
{
    m_selected = true;

    if (auto InvWnd = smart_cast<CUIInventoryWnd*>(this->OwnerList()->GetTop()))
    {
        InvWnd->HideSlotsHighlight();
        InvWnd->ShowSlotsHighlight(object());
    }

    inherited::OnFocusReceive();

    GetMessageTarget()->SendMessage(this, DRAG_DROP_ITEM_FOCUS_RECEIVED, nullptr);

    if (object()->object().m_spawned)
    {
        auto script_obj = object()->object().lua_game_object();
        g_actor->callback(GameObject::eCellItemFocus)(script_obj);
    }
}

void CUIInventoryCellItem::OnFocusLost()
{
    m_selected = false;

    if (auto InvWnd = smart_cast<CUIInventoryWnd*>(this->OwnerList()->GetTop()))
    {
        auto CellPos = this->m_pParentList->m_container->PickCell(GetUICursor()->GetCursorPosition());
        if (!this->m_pParentList->m_container->ValidCell(CellPos) || this->m_pParentList->m_container->GetCellAt(CellPos).Empty())
            InvWnd->HideSlotsHighlight();
    }

    inherited::OnFocusLost();

    GetMessageTarget()->SendMessage(this, DRAG_DROP_ITEM_FOCUS_LOST, nullptr);

    if (object()->object().m_spawned)
    {
        auto script_obj = object()->object().lua_game_object();
        g_actor->callback(GameObject::eCellItemFocusLost)(script_obj);
    }
}

bool CUIInventoryCellItem::OnMouse(float x, float y, EUIMessages action)
{
    bool r = inherited::OnMouse(x, y, action);

    g_actor->callback(GameObject::eOnCellItemMouse)(object()->object().lua_game_object(), x, y, action);

    return r;
}

CUIDragItem* CUIInventoryCellItem::CreateDragItem()
{
    CUIDragItem* i = inherited::CreateDragItem();
    if (!b_auto_drag_childs)
        return i;

    CUIStatic* s{};
    for (const auto& item : m_ChildWndList)
    {
        if (auto s_child = smart_cast<CUIStatic*>(item))
        {
            if (s_child == m_text)
                continue;

            s = xr_new<CUIStatic>();
            s->SetAutoDelete(true);
            if (s_child->GetShader())
                s->SetShader(s_child->GetShader());

            s->SetWndRect(s_child->GetWndRect());
            s->SetOriginalRect(s_child->GetOriginalRect());

            s->SetStretchTexture(s_child->GetStretchTexture());
            s->SetText(s_child->GetText());

            if (auto text = s_child->GetTextureName())
                s->InitTextureEx(text, s_child->GetShaderName());

            s->SetColor(i->wnd()->GetColor());
            s->SetHeading(i->wnd()->Heading());
            i->wnd()->AttachChild(s);
        }
    }
    return i;
}

void CUIInventoryCellItem::Update()
{
    inherited::Update();
    if (CursorOverWindow())
    {
        Frect clientArea;
        m_pParentList->GetClientArea(clientArea);
        Fvector2 cp = GetUICursor()->GetCursorPosition();
        if (clientArea.in(cp))
            GetMessageTarget()->SendMessage(this, DRAG_DROP_ITEM_FOCUSED_UPDATE, nullptr);
    }
}

CUIAmmoCellItem::CUIAmmoCellItem(CWeaponAmmo* itm) : inherited(itm) {}

bool CUIAmmoCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIAmmoCellItem* ci = smart_cast<CUIAmmoCellItem*>(itm);
    if (!ci)
        return false;

    return object()->m_ammoSect == ci->object()->m_ammoSect;
}

void CUIAmmoCellItem::Update()
{
    inherited::Update();
    if (object()->IsBoxReloadable())
        inherited::UpdateItemText();
    else
        UpdateItemText();
}

void CUIAmmoCellItem::UpdateItemText()
{
    if (NULL == m_custom_draw)
    {
        xr_vector<CUICellItem*>::iterator it = m_childs.begin();
        xr_vector<CUICellItem*>::iterator it_e = m_childs.end();

        u16 total = object()->m_boxCurr;
        for (; it != it_e; ++it)
            total = total + ((CUIAmmoCellItem*)(*it))->object()->m_boxCurr;

        string32 str;
        sprintf_s(str, "%d", total);

        m_text->SetText(str);
        m_text->Show(true);
    }
    else
    {
        m_text->SetText("");
        m_text->Show(false);
    }
}

CUIEatableCellItem::CUIEatableCellItem(CEatableItem* itm) : inherited(itm) {}

bool CUIEatableCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIEatableCellItem* ci = smart_cast<CUIEatableCellItem*>(itm);
    if (!ci)
        return false;

    return object()->GetPortionsNum() == ci->object()->GetPortionsNum();
}

CUIArtefactCellItem::CUIArtefactCellItem(CArtefact* itm) : inherited(itm) {}

bool CUIArtefactCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIArtefactCellItem* ci = smart_cast<CUIArtefactCellItem*>(itm);
    if (!ci)
        return false;

    return fsimilar(object()->GetRandomKoef(), ci->object()->GetRandomKoef(), 0.01f);
}

CUIWeaponCellItem::CUIWeaponCellItem(CWeapon* itm) : inherited(itm) { b_auto_drag_childs = false; }

bool CUIWeaponCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIWeaponCellItem* ci = smart_cast<CUIWeaponCellItem*>(itm);
    if (!ci)
        return false;

    bool b_addons = ((object()->GetAddonsState() == ci->object()->GetAddonsState()));
    bool b_place = ((object()->m_eItemPlace == ci->object()->m_eItemPlace));

    return b_addons && b_place;
}