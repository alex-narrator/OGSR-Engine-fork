#include "stdafx.h"
#include "UICellCustomItems.h"
#include "UIInventoryUtilities.h"
#include "Weapon.h"
#include "WeaponMagazined.h"
#include "UIDragDropListEx.h"

#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../Actor.h"
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

void CUIInventoryCellItem::init_add()
{
    static CUIXml uiXml;
    static bool is_xml_ready = false;
    if (!is_xml_ready)
    {
        bool xml_result = uiXml.Init(CONFIG_PATH, UI_PATH, "inventory_new.xml");
        R_ASSERT3(xml_result, "file parsing error ", uiXml.m_xml_file_name);
        is_xml_ready = true;
    }

    m_text_add = xr_new<CUIStatic>();
    m_text_add->SetAutoDelete(true);
    AttachChild(m_text_add);
    CUIXmlInit::InitStatic(uiXml, "cell_item_text_add", 0, m_text_add);
    m_text_add->Show(false);
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

    return (fsimilar(object()->GetCondition(), ci->object()->GetCondition(), 0.01f) && fsimilar(object()->Weight(), ci->object()->Weight(), 0.01f) &&
            fsimilar(object()->GetPowerLevel(), ci->object()->GetPowerLevel(), 0.01f) &&
            fsimilar(object()->GetItemEffect(CInventoryItem::eRadiationRestoreSpeed), ci->object()->GetItemEffect(CInventoryItem::eRadiationRestoreSpeed), 0.01f) &&
            object()->object().cNameSect() == ci->object()->object().cNameSect() && object()->m_eItemPlace == ci->object()->m_eItemPlace &&
            object()->Cost() == ci->object()->Cost());
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
    CUIStatic* s = NULL;
    if (!b_auto_drag_childs)
        return i;

    for (auto it = this->m_ChildWndList.begin(); it != this->m_ChildWndList.end(); ++it)
    {
        if (auto s_child = smart_cast<CUIStatic*>(*it))
        {
            if (Core.Features.test(xrCore::Feature::show_inv_item_condition))
                if (s_child == m_text)
                    continue;

            if (s_child == m_text_add)
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
    if (Core.Features.test(xrCore::Feature::show_inv_item_condition))
        UpdateConditionProgressBar();
}

CUIAmmoCellItem::CUIAmmoCellItem(CWeaponAmmo* itm) : inherited(itm)
{
    if (itm->IsBoxReloadable() || itm->IsBoxReloadableEmpty())
    {
        init_add();
    }
}

bool CUIAmmoCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIAmmoCellItem* ci = smart_cast<CUIAmmoCellItem*>(itm);
    if (!ci)
        return false;

    return ((object()->cNameSect() == ci->object()->cNameSect()));
}

void CUIAmmoCellItem::Update()
{
    inherited::Update();
    if (object()->IsBoxReloadable() || object()->IsBoxReloadableEmpty())
        UpdateItemTextCustom();
    else
        UpdateItemText();
}

void CUIAmmoCellItem::UpdateItemTextCustom()
{
    inherited::UpdateItemText();

    if (!m_text_add)
        return;

    string32 str;
    sprintf_s(str, "%d/%d", object()->m_boxCurr, object()->m_boxSize);

    Fvector2 pos{GetWidth() - m_text_add->GetWidth(), GetHeight() - m_text_add->GetHeight()};

    m_text_add->SetWndPos(pos);
    m_text_add->SetText(str);
    m_text_add->Show(true);
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

        if (Core.Features.test(xrCore::Feature::show_inv_item_condition))
        {
            m_text->SetText(str);
            m_text->Show(true);
        }
        else
            SetText(str);
    }
    else
    {
        if (Core.Features.test(xrCore::Feature::show_inv_item_condition))
        {
            m_text->SetText("");
            m_text->Show(false);
        }
        else
            SetText("");
    }
}

CUIWarbeltCellItem::CUIWarbeltCellItem(CWarbelt* itm) : inherited(itm) { init_add(); }
void CUIWarbeltCellItem::Update()
{
    inherited::Update();
    UpdateItemText();
}
void CUIWarbeltCellItem::UpdateItemText()
{
    inherited::UpdateItemText();

    string32 str;
    sprintf_s(str, "[%dx%d]", object()->GetBeltWidth(), object()->GetBeltHeight());

    Fvector2 pos{GetWidth() - m_text_add->GetWidth(), GetHeight() - m_text_add->GetHeight()};

    m_text_add->SetWndPos(pos);
    m_text_add->SetText(str);
    m_text_add->Show(true);
}

CUIVestCellItem::CUIVestCellItem(CVest* itm) : inherited(itm) { init_add(); }
void CUIVestCellItem::Update()
{
    inherited::Update();
    UpdateItemText();
}
void CUIVestCellItem::UpdateItemText()
{
    inherited::UpdateItemText();

    string32 str;
    sprintf_s(str, "[%dx%d]", object()->GetVestWidth(), object()->GetVestHeight());

    Fvector2 pos{GetWidth() - m_text_add->GetWidth(), GetHeight() - m_text_add->GetHeight()};

    m_text_add->SetWndPos(pos);
    m_text_add->SetText(str);
    m_text_add->Show(true);
}

CUIContainerCellItem::CUIContainerCellItem(CInventoryContainer* itm) : inherited(itm)
{
    if (!fis_zero(object()->GetItemEffect(CInventoryItem::eAdditionalWeight)))
    {
        init_add();
    }
}
void CUIContainerCellItem::Update()
{
    inherited::Update();
    UpdateItemText();
}
void CUIContainerCellItem::UpdateItemText()
{
    inherited::UpdateItemText();

    if (!m_text_add)
        return;

    string32 str;

    float add_weight = object()->GetItemEffect(CInventoryItem::eAdditionalWeight);
    auto measure_weight = CStringTable().translate("st_kg").c_str();

    if (!fis_zero(add_weight))
        sprintf_s(str, "%.0f%s", add_weight, measure_weight);

    Fvector2 pos{GetWidth() - m_text_add->GetWidth(), GetHeight() - m_text_add->GetHeight()};

    m_text_add->SetWndPos(pos);
    m_text_add->SetText(str);
    m_text_add->Show(true);
}

CUIEatableCellItem::CUIEatableCellItem(CEatableItem* itm) : inherited(itm)
{
    if (itm->GetStartPortionsNum() > 1)
    {
        init_add();
    }
}

bool CUIEatableCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIEatableCellItem* ci = smart_cast<CUIEatableCellItem*>(itm);
    if (!ci)
        return false;

    return object()->GetPortionsNum() == ci->object()->GetPortionsNum();
}

void CUIEatableCellItem::Update()
{
    inherited::Update();
    UpdateItemText();
}

void CUIEatableCellItem::UpdateItemText()
{
    inherited::UpdateItemText();

    if (!m_text_add)
        return;

    string32 str;
    sprintf_s(str, "%d/%d", object()->GetPortionsNum(), object()->GetStartPortionsNum());

    Fvector2 pos{GetWidth() - m_text_add->GetWidth(), GetHeight() - m_text_add->GetHeight()};

    m_text_add->SetWndPos(pos);
    m_text_add->SetText(str);
    m_text_add->Show(true);
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

CUIWeaponCellItem::CUIWeaponCellItem(CWeapon* itm) : inherited(itm)
{
    b_auto_drag_childs = false;

    m_cell_size.set(INV_GRID_WIDTHF, INV_GRID_HEIGHTF);

    if (itm->GetAmmoMagSize())
    {
        init_add();
    }
}

void CUIWeaponCellItem::UpdateItemText()
{
    inherited::UpdateItemText();

    if (!m_text_add)
        return;

    string32 str;
    auto pWeaponMag = smart_cast<CWeaponMagazined*>(object());
    sprintf_s(str, "%d/%d%s", object()->GetAmmoElapsed(), object()->GetAmmoMagSize(),
              pWeaponMag && (pWeaponMag->HasFireModes() || pWeaponMag->IsGrenadeMode()) ? pWeaponMag->GetCurrentFireModeStr() : "");

    Fvector2 pos{GetWidth() - m_text_add->GetWidth(), GetHeight() - m_text_add->GetHeight()};

    m_text_add->SetWndPos(pos);
    m_text_add->SetText(str);
    m_text_add->Show(true);
}

#include "../object_broker.h"
CUIWeaponCellItem::~CUIWeaponCellItem() {}

bool CUIWeaponCellItem::is_scope() { return object()->AddonAttachable(eScope) && object()->IsAddonAttached(eScope); }
bool CUIWeaponCellItem::is_silencer() { return object()->AddonAttachable(eSilencer) && object()->IsAddonAttached(eSilencer); }
bool CUIWeaponCellItem::is_launcher() { return object()->AddonAttachable(eLauncher) && object()->IsAddonAttached(eLauncher); }
bool CUIWeaponCellItem::is_laser() { return object()->AddonAttachable(eLaser) && object()->IsAddonAttached(eLaser); }
bool CUIWeaponCellItem::is_flashlight() { return object()->AddonAttachable(eFlashlight) && object()->IsAddonAttached(eFlashlight); }
bool CUIWeaponCellItem::is_stock() { return object()->AddonAttachable(eStock) && object()->IsAddonAttached(eStock); }
bool CUIWeaponCellItem::is_extender() { return object()->AddonAttachable(eExtender) && object()->IsAddonAttached(eExtender); }
bool CUIWeaponCellItem::is_forend() { return object()->AddonAttachable(eForend) && object()->IsAddonAttached(eForend); }
bool CUIWeaponCellItem::is_magazine() { return object()->AddonAttachable(eMagazine, true) && object()->IsAddonAttached(eMagazine) && !!object()->GetMagazineIconSect(true); }

void CUIWeaponCellItem::CreateIcon(u32 t, CIconParams& params)
{
    if (GetIcon(t))
        return;
    m_addons[t] = xr_new<CUIStatic>();
    m_addons[t]->SetAutoDelete(true);
    AttachChild(m_addons[t]);
    params.set_shader(m_addons[t]);
}

void CUIWeaponCellItem::DestroyIcon(u32 t)
{
    DetachChild(m_addons[t]);
    m_addons[t] = NULL;
}

CUIStatic* CUIWeaponCellItem::GetIcon(u32 t) { return m_addons[t]; }

void CUIWeaponCellItem::Update()
{
    bool b = Heading();
    inherited::Update();
    UpdateItemText();
    bool bForceReInitAddons = (b != Heading());

    if (object()->AddonAttachable(eSilencer))
    {
        if (object()->IsAddonAttached(eSilencer))
        {
            if (!GetIcon(eSilencer) || bForceReInitAddons)
            {
                CIconParams params(object()->GetAddonName(eSilencer));
                CreateIcon(eSilencer, params);
                InitAddon(GetIcon(eSilencer), params, object()->GetAddonOffset(eSilencer), Heading());
            }
        }
        else
        {
            if (m_addons[eSilencer])
                DestroyIcon(eSilencer);
        }
    }

    if (object()->AddonAttachable(eScope))
    {
        if (object()->IsAddonAttached(eScope))
        {
            if (!GetIcon(eScope) || bForceReInitAddons)
            {
                CIconParams params(object()->GetAddonName(eScope));
                CreateIcon(eScope, params);
                InitAddon(GetIcon(eScope), params, object()->GetAddonOffset(eScope), Heading());
            }
        }
        else
        {
            if (m_addons[eScope])
                DestroyIcon(eScope);
        }
    }

    if (object()->AddonAttachable(eLauncher))
    {
        if (object()->IsAddonAttached(eLauncher))
        {
            if (!GetIcon(eLauncher) || bForceReInitAddons)
            {
                CIconParams params(object()->GetAddonName(eLauncher));
                CreateIcon(eLauncher, params);
                InitAddon(GetIcon(eLauncher), params, object()->GetAddonOffset(eLauncher), Heading());
            }
        }
        else
        {
            if (m_addons[eLauncher])
                DestroyIcon(eLauncher);
        }
    }

    if (object()->AddonAttachable(eLaser))
    {
        if (object()->IsAddonAttached(eLaser))
        {
            if (!GetIcon(eLaser) || bForceReInitAddons)
            {
                CIconParams params(object()->GetAddonName(eLaser));
                CreateIcon(eLaser, params);
                InitAddon(GetIcon(eLaser), params, object()->GetAddonOffset(eLaser), Heading());
            }
        }
        else
        {
            if (m_addons[eLaser])
                DestroyIcon(eLaser);
        }
    }

    if (object()->AddonAttachable(eFlashlight))
    {
        if (object()->IsAddonAttached(eFlashlight))
        {
            if (!GetIcon(eFlashlight) || bForceReInitAddons)
            {
                CIconParams params(object()->GetAddonName(eFlashlight));
                CreateIcon(eFlashlight, params);
                InitAddon(GetIcon(eFlashlight), params, object()->GetAddonOffset(eFlashlight), Heading());
            }
        }
        else
        {
            if (m_addons[eFlashlight])
                DestroyIcon(eFlashlight);
        }
    }

    if (object()->AddonAttachable(eStock))
    {
        if (object()->IsAddonAttached(eStock))
        {
            if (!GetIcon(eStock) || bForceReInitAddons)
            {
                CIconParams params(object()->GetAddonName(eStock));
                CreateIcon(eStock, params);
                InitAddon(GetIcon(eStock), params, object()->GetAddonOffset(eStock), Heading());
            }
        }
        else
        {
            if (m_addons[eStock])
                DestroyIcon(eStock);
        }
    }
    if (object()->AddonAttachable(eExtender))
    {
        if (object()->IsAddonAttached(eExtender))
        {
            if (!GetIcon(eExtender) || bForceReInitAddons)
            {
                CIconParams params(object()->GetAddonName(eExtender));
                CreateIcon(eExtender, params);
                InitAddon(GetIcon(eExtender), params, object()->GetAddonOffset(eExtender), Heading());
            }
        }
        else
        {
            if (m_addons[eExtender])
                DestroyIcon(eExtender);
        }
    }
    if (object()->AddonAttachable(eForend))
    {
        if (object()->IsAddonAttached(eForend))
        {
            if (!GetIcon(eForend) || bForceReInitAddons)
            {
                CIconParams params(object()->GetAddonName(eForend));
                CreateIcon(eForend, params);
                InitAddon(GetIcon(eForend), params, object()->GetAddonOffset(eForend), Heading());
            }
        }
        else
        {
            if (m_addons[eForend])
                DestroyIcon(eForend);
        }
    }
    if (object()->AddonAttachable(eMagazine, true))
    {
        if (object()->IsAddonAttached(eMagazine) && !!object()->GetMagazineIconSect(true))
        {
            if (!GetIcon(eMagazine) || bForceReInitAddons)
            {
                CIconParams params(object()->GetMagazineIconSect(true));
                CreateIcon(eMagazine, params);
                InitAddon(GetIcon(eMagazine), params, object()->GetAddonOffset(eMagazine), Heading());
            }
        }
        else
        {
            if (m_addons[eMagazine])
                DestroyIcon(eMagazine);
        }
    }
}

void CUIWeaponCellItem::OnAfterChild(CUIDragDropListEx* parent_list)
{
    const Ivector2& cs = parent_list->CellSize();
    m_cell_size.set((float)cs.x, (float)cs.y);
    CUIStatic* s_silencer = is_silencer() ? GetIcon(eSilencer) : nullptr;
    CUIStatic* s_scope = is_scope() ? GetIcon(eScope) : nullptr;
    CUIStatic* s_launcher = is_launcher() ? GetIcon(eLauncher) : nullptr;
    CUIStatic* s_laser = is_laser() ? GetIcon(eLaser) : nullptr;
    CUIStatic* s_flashlight = is_flashlight() ? GetIcon(eFlashlight) : nullptr;
    CUIStatic* s_stock = is_stock() ? GetIcon(eStock) : nullptr;
    CUIStatic* s_extender = is_extender() ? GetIcon(eExtender) : nullptr;
    CUIStatic* s_forend = is_forend() ? GetIcon(eForend) : nullptr;
    CUIStatic* s_magazine = is_magazine() ? GetIcon(eMagazine) : nullptr;

    InitAllAddons(s_silencer, s_scope, s_launcher, s_laser, s_flashlight, s_stock, s_extender, s_forend, s_magazine, parent_list->GetVerticalPlacement());
}

void CUIWeaponCellItem::InitAllAddons(CUIStatic* s_silencer, CUIStatic* s_scope, CUIStatic* s_launcher, CUIStatic* s_laser, CUIStatic* s_flashlight, CUIStatic* s_stock,
                                      CUIStatic* s_extender, CUIStatic* s_forend, CUIStatic* s_magazine, bool b_vertical)
{
    CIconParams params;

    if (s_silencer)
    {
        params.Load(object()->GetAddonName(eSilencer));
        params.set_shader(s_silencer);
        InitAddon(s_silencer, params, object()->GetAddonOffset(eSilencer), b_vertical);
    }
    if (s_scope)
    {
        params.Load(object()->GetAddonName(eScope));
        params.set_shader(s_scope);
        InitAddon(s_scope, params, object()->GetAddonOffset(eScope), b_vertical);
    }
    if (s_launcher)
    {
        params.Load(object()->GetAddonName(eLauncher));
        params.set_shader(s_launcher);
        InitAddon(s_launcher, params, object()->GetAddonOffset(eLauncher), b_vertical);
    }
    if (s_laser)
    {
        params.Load(object()->GetAddonName(eLaser));
        params.set_shader(s_laser);
        InitAddon(s_laser, params, object()->GetAddonOffset(eLaser), b_vertical);
    }
    if (s_flashlight)
    {
        params.Load(object()->GetAddonName(eFlashlight));
        params.set_shader(s_flashlight);
        InitAddon(s_flashlight, params, object()->GetAddonOffset(eFlashlight), b_vertical);
    }
    if (s_stock)
    {
        params.Load(object()->GetAddonName(eStock));
        params.set_shader(s_stock);
        InitAddon(s_stock, params, object()->GetAddonOffset(eStock), b_vertical);
    }
    if (s_extender)
    {
        params.Load(object()->GetAddonName(eExtender));
        params.set_shader(s_extender);
        InitAddon(s_extender, params, object()->GetAddonOffset(eExtender), b_vertical);
    }
    if (s_forend)
    {
        params.Load(object()->GetAddonName(eForend));
        params.set_shader(s_forend);
        InitAddon(s_forend, params, object()->GetAddonOffset(eForend), b_vertical);
    }
    if (s_magazine)
    {
        params.Load(object()->GetMagazineIconSect(true));
        params.set_shader(s_magazine);
        InitAddon(s_magazine, params, object()->GetAddonOffset(eMagazine), b_vertical);
    }
}

void CUIWeaponCellItem::InitAddon(CUIStatic* s, CIconParams& params, Fvector2 addon_offset, bool b_rotate)
{
    Fvector2 base_scale{};
    Fvector2 inventory_size{};
    Fvector2 expected_size{};
    static int method = 1;

    inventory_size.x = INV_GRID_WIDTHF * m_grid_size.x;
    inventory_size.y = INV_GRID_HEIGHTF * m_grid_size.y;

    expected_size.x = m_cell_size.x * m_grid_size.x; // vert: cell_size = 40x50, grid_size = 5x2
    expected_size.y = m_cell_size.y * m_grid_size.y;
    if (Heading())
    { // h = 250, w = 80, i.x = 300, i.y = 100
        if (1 == method)
        {
            expected_size.x = m_cell_size.y * m_grid_size.x;
            expected_size.y = m_cell_size.x * m_grid_size.y;
        }
        if (2 == method)
        {
            expected_size.x = m_cell_size.y * m_grid_size.y;
            expected_size.y = m_cell_size.x * m_grid_size.x;
        }

        base_scale.x = GetHeight() / inventory_size.x;
        base_scale.y = GetWidth() / inventory_size.y;
    }
    else
    {
        base_scale.x = GetWidth() / inventory_size.x;
        base_scale.y = GetHeight() / inventory_size.y;
    }

    if (method > 0)
    {
        base_scale.x = expected_size.x / inventory_size.x;
        base_scale.y = expected_size.y / inventory_size.y;
    }

    Fvector2 cell_size{};
    cell_size.x = params.grid_width * INV_GRID_WIDTHF;
    cell_size.y = params.grid_height * INV_GRID_HEIGHTF;
    cell_size.mul(base_scale);

    if (b_rotate)
    {
        s->SetWndSize(Fvector2().set(cell_size.y, cell_size.x));
        Fvector2 new_offset{};
        new_offset.x = addon_offset.y * base_scale.x;
        new_offset.y = GetHeight() - addon_offset.x * base_scale.x - cell_size.x;
        addon_offset = new_offset;
        addon_offset.x *= UI()->get_current_kx();
    }
    else
    {
        s->SetWndSize(cell_size);
        addon_offset.mul(base_scale);
    }
    s->SetWndPos(addon_offset);
    s->SetStretchTexture(true);

    s->EnableHeading(b_rotate);

    if (b_rotate)
    {
        s->SetHeading(GetHeading());
        Fvector2 offs{};
        offs.set(0.0f, s->GetWndSize().y);
        s->SetHeadingPivot(Fvector2().set(0.0f, 0.0f), offs, true);
    }

    s->SetWindowName("wpn_addon");
}

CUIStatic* MakeAddonStatic(CUIDragItem* i, CIconParams& params)
{
    CUIStatic* s = xr_new<CUIStatic>();
    params.set_shader(s);
    s->SetAutoDelete(true);
    s->SetColor(i->wnd()->GetColor());
    i->wnd()->AttachChild(s);
    return s;
}

CUIDragItem* CUIWeaponCellItem::CreateDragItem()
{
    CUIDragItem* i = inherited::CreateDragItem();

    CIconParams params;

    CUIStatic* s_silencer{};
    CUIStatic* s_scope{};
    CUIStatic* s_launcher{};
    CUIStatic* s_laser{};
    CUIStatic* s_flashlight{};
    CUIStatic* s_stock{};
    CUIStatic* s_extender{};
    CUIStatic* s_forend{};
    CUIStatic* s_magazine{};

    if (GetIcon(eSilencer))
    {
        params.Load(object()->GetAddonName(eSilencer));
        s_silencer = MakeAddonStatic(i, params);
    }

    if (GetIcon(eScope))
    {
        params.Load(object()->GetAddonName(eScope));
        s_scope = MakeAddonStatic(i, params);
    }

    if (GetIcon(eLauncher))
    {
        params.Load(object()->GetAddonName(eLauncher));
        s_launcher = MakeAddonStatic(i, params);
    }

    if (GetIcon(eLaser))
    {
        params.Load(object()->GetAddonName(eLaser));
        s_laser = MakeAddonStatic(i, params);
    }

    if (GetIcon(eFlashlight))
    {
        params.Load(object()->GetAddonName(eFlashlight));
        s_flashlight = MakeAddonStatic(i, params);
    }

    if (GetIcon(eStock))
    {
        params.Load(object()->GetAddonName(eStock));
        s_stock = MakeAddonStatic(i, params);
    }

    if (GetIcon(eExtender))
    {
        params.Load(object()->GetAddonName(eExtender));
        s_extender = MakeAddonStatic(i, params);
    }

    if (GetIcon(eForend))
    {
        params.Load(object()->GetAddonName(eForend));
        s_forend = MakeAddonStatic(i, params);
    }

    if (GetIcon(eMagazine))
    {
        params.Load(object()->GetMagazineIconSect(true));
        s_magazine = MakeAddonStatic(i, params);
    }

    if (Heading())
        m_cell_size.set(m_cell_size.y, m_cell_size.x); // swap before
    InitAllAddons(s_silencer, s_scope, s_launcher, s_laser, s_flashlight, s_stock, s_extender, s_forend, s_magazine, false);
    if (Heading())
        m_cell_size.set(m_cell_size.y, m_cell_size.x); // swap after

    return i;
}

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

CBuyItemCustomDrawCell::CBuyItemCustomDrawCell(LPCSTR str, CGameFont* pFont)
{
    m_pFont = pFont;
    VERIFY(xr_strlen(str) < 16);
    strcpy_s(m_string, str);
}

void CBuyItemCustomDrawCell::OnDraw(CUICellItem* cell)
{
    Fvector2 pos;
    cell->GetAbsolutePos(pos);
    UI()->ClientToScreenScaled(pos, pos.x, pos.y);
    m_pFont->Out(pos.x, pos.y, m_string);
    m_pFont->OnRender();
}