#include "stdafx.h"
#include "UICarBodyWnd.h"
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../HUDManager.h"
#include "../level.h"
#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UIFrameWindow.h"
#include "UIPropertiesBox.h"
#include "../ai/monsters/BaseMonster/base_monster.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "../WeaponMagazined.h"
#include "../WeaponMagazinedWGrenade.h"
#include "../Actor.h"
#include "../eatable_item.h"
#include "../alife_registry_wrappers.h"
#include "UI3tButton.h"
#include "UIListBoxItem.h"
#include "UICheckButton.h"
#include "../InventoryBox.h"
#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../xr_3da/xr_input.h"

#include "Artifact.h"
#include "CustomOutfit.h"
#include "WeaponKnife.h"
#include "string_table.h"

constexpr auto CAR_BODY_XML = "carbody_new.xml";
constexpr auto CARBODY_ITEM_XML = "carbody_item.xml";

CUICarBodyWnd::CUICarBodyWnd()
{
    Init();
    Hide();
    m_b_need_update = false;
}

CUICarBodyWnd::~CUICarBodyWnd()
{
    m_pUIOurBagList->ClearAll(true);
    m_pUIOthersBagList->ClearAll(true);
}

void CUICarBodyWnd::Init()
{
    CUIXml uiXml;
    uiXml.Init(CONFIG_PATH, UI_PATH, CAR_BODY_XML);

    CUIXmlInit xml_init;

    xml_init.InitWindow(uiXml, "main", 0, this);

    xml_init.InitAutoStatic(uiXml, "auto_static", this);

    m_pUIOurBagList = xr_new<CUIDragDropListEx>();
    m_pUIOurBagList->SetAutoDelete(true);
    AttachChild(m_pUIOurBagList);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_list_our", 0, m_pUIOurBagList);

    m_pUIOthersBagList = xr_new<CUIDragDropListEx>();
    m_pUIOthersBagList->SetAutoDelete(true);
    AttachChild(m_pUIOthersBagList);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_list_other", 0, m_pUIOthersBagList);

    m_pUIPropertiesBox = xr_new<CUIPropertiesBox>();
    m_pUIPropertiesBox->SetAutoDelete(true);
    AttachChild(m_pUIPropertiesBox);
    m_pUIPropertiesBox->Init(0, 0, 300, 300);
    m_pUIPropertiesBox->Hide();

    SetCurrentItem(NULL);

    BindDragDropListEvents(m_pUIOurBagList);
    BindDragDropListEvents(m_pUIOthersBagList);

    // Load sounds
    if (uiXml.NavigateToNode("action_sounds", 0))
    {
        XML_NODE* stored_root = uiXml.GetLocalRoot();
        uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));

        ::Sound->create(sounds[eInvSndOpen], uiXml.Read("snd_open", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvSndClose], uiXml.Read("snd_close", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvProperties], uiXml.Read("snd_properties", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvDropItem], uiXml.Read("snd_drop_item", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvDetachAddon], uiXml.Read("snd_detach_addon", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvMoveItem], uiXml.Read("snd_move_item", 0, NULL), st_Effect, sg_SourceType);

        uiXml.SetLocalRoot(stored_root);
    }
}

void CUICarBodyWnd::InitCarBody(CInventoryOwner* pOur, IInventoryBox* pInvBox)
{
    m_pActorInventoryOwner = pOur;
    m_pOtherInventoryOwner = NULL;
    m_pOtherInventoryBox = pInvBox;
    m_pOtherInventoryBox->m_in_use = true;

    m_pActorGO = smart_cast<CGameObject*>(m_pActorInventoryOwner);
    m_pOtherGO = smart_cast<CGameObject*>(m_pOtherInventoryBox);

    m_pUIPropertiesBox->Hide();

    UpdateLists(eBoth);

    if (auto obj = smart_cast<CInventoryBox*>(pInvBox))
        obj->callback(GameObject::eOnInvBoxOpen)();
}

void CUICarBodyWnd::InitCarBody(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{
    m_pActorInventoryOwner = pOur;
    m_pOtherInventoryOwner = pOthers;
    m_pOtherInventoryBox = NULL;

    m_pActorGO = smart_cast<CGameObject*>(m_pActorInventoryOwner);
    m_pOtherGO = smart_cast<CGameObject*>(m_pOtherInventoryOwner);

    u16 our_id = m_pActorGO->ID();
    u16 other_id = m_pOtherGO->ID();

    m_pUIPropertiesBox->Hide();

    UpdateLists(eBoth);

    if (!smart_cast<CBaseMonster*>(m_pOtherInventoryOwner))
    {
        CInfoPortionWrapper* known_info_registry = xr_new<CInfoPortionWrapper>();
        known_info_registry->registry().init(other_id);
        KNOWN_INFO_VECTOR& known_info = known_info_registry->registry().objects();

        KNOWN_INFO_VECTOR_IT it = known_info.begin();
        for (int i = 0; it != known_info.end(); ++it, ++i)
        {
            NET_Packet P;
            CGameObject::u_EventGen(P, GE_INFO_TRANSFER, our_id);
            P.w_u16(0); // not used
            P.w_stringZ((*it).info_id); // сообщение
            P.w_u8(1); // добавление сообщения
            CGameObject::u_EventSend(P);
        }
        known_info.clear();
        xr_delete(known_info_registry);
    }
}

void CUICarBodyWnd::UpdateLists_delayed() { m_b_need_update = true; }

void CUICarBodyWnd::Hide()
{
    InventoryUtilities::SendInfoToActor("ui_car_body_hide");
    m_pUIOurBagList->ClearAll(true);
    m_pUIOthersBagList->ClearAll(true);
    inherited::Hide();
    if (m_pOtherInventoryBox)
        m_pOtherInventoryBox->m_in_use = false;

    PlaySnd(eInvSndClose);
}

void CUICarBodyWnd::ActivatePropertiesBox()
{
    m_pUIPropertiesBox->RemoveAll();

    auto pWeapon = smart_cast<CWeapon*>(CurrentIItem());
    auto pAmmo = smart_cast<CWeaponAmmo*>(CurrentIItem());
    auto pEatableItem = smart_cast<CEatableItem*>(CurrentIItem());

    LPCSTR detach_tip = CurrentIItem()->GetDetachMenuTip();

    bool b_actor_inv = CurrentItem()->OwnerList() == m_pUIOurBagList;
    const auto inv = &m_pActorInventoryOwner->inventory();
    string1024 temp;

    bool b_many = CurrentItem()->ChildsCount();
    LPCSTR _many = b_many ? "•" : "";
    LPCSTR _addon_name{};

    const char* _addon_sect{};

    if (pAmmo)
    {
        if (pAmmo->IsBoxReloadable())
        {
            // reload AmmoBox
            bool is_full = pAmmo->m_boxCurr == pAmmo->m_boxSize;
            for (const auto& type : pAmmo->m_ammoTypes)
            {
                if (inv->GetAmmoByLimit(type.c_str(), true))
                {
                    if (is_full && type == pAmmo->m_ammoSect)
                        continue;
                    auto _str = (type == pAmmo->m_ammoSect || !pAmmo->m_boxCurr) ? "st_load_ammo_type" : "st_reload_ammo_type";
                    sprintf(temp, "%s%s %s", _many, CStringTable().translate(_str).c_str(), CStringTable().translate(pSettings->r_string(type, "inv_name_short")).c_str());
                    m_pUIPropertiesBox->AddItem(temp, (void*)type.c_str(), INVENTORY_RELOAD_AMMO_BOX);
                }
            }
            // unload AmmoBox
            if (pAmmo->m_boxCurr)
            {
                sprintf(temp, "%s%s", _many, CStringTable().translate("st_unload_magazine").c_str());
                m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_UNLOAD_AMMO_BOX);
            }
        }
    }

    if (pWeapon)
    {
        if (inv->InSlot(pWeapon))
        {
            for (u32 i = 0; i < pWeapon->m_ammoTypes.size(); ++i)
            {
                if (pWeapon->CanBeReloaded() && pWeapon->TryToGetAmmo(i))
                {
                    auto ammo_sect = pSettings->r_string(pWeapon->m_ammoTypes[i].c_str(), "inv_name_short");
                    sprintf(temp, "%s %s", CStringTable().translate("st_load_ammo_type").c_str(), CStringTable().translate(ammo_sect).c_str());
                    m_pUIPropertiesBox->AddItem(temp, (void*)(__int64)i, INVENTORY_RELOAD_MAGAZINE);
                }
            }
        }
        // addons detach
        for (u32 i = 0; i < eMagazine; i++)
        {
            if (pWeapon->AddonAttachable(i) && pWeapon->IsAddonAttached(i) && pWeapon->CanDetach(pWeapon->GetAddonName(i).c_str()))
            {
                _addon_sect = pWeapon->GetAddonName(i).c_str();
                _addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
                sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
                m_pUIPropertiesBox->AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
            }
        }
        //
        if (pWeapon)
        {
            bool b = pWeapon->CanBeUnloaded();

            if (!b)
            {
                CUICellItem* itm = CurrentItem();
                for (u32 i = 0; i < itm->ChildsCount(); ++i)
                {
                    auto pWeaponChild = static_cast<CWeaponMagazined*>(itm->Child(i)->m_pData);
                    if (pWeaponChild->CanBeUnloaded())
                    {
                        b = true;
                        break;
                    }
                }
            }

            if (b)
            {
                sprintf(temp, "%s%s", _many, CStringTable().translate("st_unload_magazine").c_str());
                m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_UNLOAD_MAGAZINE);
            }
        }
    }

    if (pEatableItem)
    {
        m_pUIPropertiesBox->AddItem(pEatableItem->GetUseMenuTip(), NULL, INVENTORY_EAT_ACTION);
    }

    if (b_actor_inv)
        m_pUIPropertiesBox->CheckCustomActionsItem(CurrentIItem()->object().lua_game_object());

    if (b_actor_inv && !CurrentIItem()->IsQuestItem())
    {
        sprintf(temp, "%s%s", _many, CStringTable().translate("st_drop").c_str());
        m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DROP_ACTION);
    }

    if (CanMoveToOther(CurrentIItem(), b_actor_inv ? m_pOtherGO : m_pActorGO))
    {
        sprintf(temp, "%s%s", _many, CStringTable().translate("st_move").c_str());
        m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_MOVE_ACTION);
    }

    if (m_pUIPropertiesBox->GetItemsCount() > 0)
    {
        m_pUIPropertiesBox->AutoUpdateSize();
        m_pUIPropertiesBox->BringAllToTop();

        Fvector2 cursor_pos;
        Frect vis_rect;

        GetAbsoluteRect(vis_rect);
        cursor_pos = GetUICursor()->GetCursorPosition();
        cursor_pos.sub(vis_rect.lt);
        m_pUIPropertiesBox->Show(vis_rect, cursor_pos);

        PlaySnd(eInvProperties);
    }
}

void CUICarBodyWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == m_pUIPropertiesBox && msg == PROPERTY_CLICKED)
    {
        if (m_pUIPropertiesBox->GetClickedItem())
        {
            bool for_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
            auto itm = CurrentItem();
            auto item = CurrentIItem();
            switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
            {
            case INVENTORY_EAT_ACTION: // съесть объект
                EatItem();
                break;
            case INVENTORY_RELOAD_MAGAZINE: {
                void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
                auto Wpn = smart_cast<CWeapon*>(item);
                Wpn->m_set_next_ammoType_on_reload = (u32)(__int64)d;
                Wpn->ReloadWeapon();
            }
            break;
            case INVENTORY_UNLOAD_MAGAZINE: {
                auto wpn = smart_cast<CWeaponMagazined*>(item);
                wpn->UnloadWeaponFull();
                for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i)
                {
                    auto wpn = static_cast<CWeaponMagazined*>(itm->Child(i)->m_pData);
                    wpn->UnloadWeaponFull();
                }
            }
            break;
            case INVENTORY_RELOAD_AMMO_BOX: {
                auto sect_to_load = (LPCSTR)m_pUIPropertiesBox->GetClickedItem()->GetData();
                auto ammobox = smart_cast<CWeaponAmmo*>(item);
                ammobox->ReloadBox(sect_to_load);
                for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i)
                {
                    auto ammobox = static_cast<CWeaponAmmo*>(itm->Child(i)->m_pData);
                    ammobox->ReloadBox(sect_to_load);
                }
            }
            break;
            case INVENTORY_UNLOAD_AMMO_BOX: {
                auto ammobox = smart_cast<CWeaponAmmo*>(item);
                ammobox->UnloadBox();
                for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i)
                {
                    auto ammobox = static_cast<CWeaponAmmo*>(itm->Child(i)->m_pData);
                    ammobox->UnloadBox();
                }
            }
            break;
            case INVENTORY_DETACH_ADDON: {
                DetachAddon((const char*)(m_pUIPropertiesBox->GetClickedItem()->GetData()), for_all);
            }
            break;
            case INVENTORY_DROP_ACTION: {
                DropItems(for_all);
            }
            break;
            case INVENTORY_MOVE_ACTION: {
                MoveItems(itm, for_all);
            }
            break;
            case INVENTORY_CUSTOM_ACTION: {
                m_pUIPropertiesBox->ProcessCustomActionsItem(CurrentIItem()->object().lua_game_object());
            }
            break;
            }

            // refresh if nessesary
            switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
            {
            case INVENTORY_EAT_ACTION:
            case INVENTORY_RELOAD_MAGAZINE:
            case INVENTORY_UNLOAD_MAGAZINE:
            case INVENTORY_RELOAD_AMMO_BOX:
            case INVENTORY_UNLOAD_AMMO_BOX:
            case INVENTORY_DETACH_ADDON: {
                SetCurrentItem(nullptr);
            }
            break;
            }
        }
    }

    inherited::SendMessage(pWnd, msg, pData);
}

void CUICarBodyWnd::Draw() { inherited::Draw(); }

void CUICarBodyWnd::Update()
{
    EListType et = eNone;

    if (m_b_need_update ||
        m_pActorInventoryOwner->inventory().StateInvalid() &&
            (m_pOtherInventoryOwner && m_pOtherInventoryOwner->inventory().StateInvalid() || m_pOtherInventoryBox && m_pOtherInventoryBox->StateInvalid()))
    {
        et = eBoth;
    }
    else if (m_pActorInventoryOwner->inventory().StateInvalid())
    {
        et = e1st;
    }
    else if (m_pOtherInventoryOwner && m_pOtherInventoryOwner->inventory().StateInvalid() || m_pOtherInventoryBox && m_pOtherInventoryBox->StateInvalid())
    {
        et = e2nd;
    }

    if (et != eNone)
        UpdateLists(et);

    if (m_pOtherGO &&
        m_pActorGO->Position().distance_to(m_pOtherGO->Position()) - m_pOtherGO->Radius() - m_pActorGO->Radius() > m_pActorInventoryOwner->inventory().GetTakeDist() + 0.5f)
    {
        GetHolder()->StartStopMenu(this, true);
    }

    Actor()->UpdateCameraDirection(m_pOtherGO);
    inherited::Update();
}

void CUICarBodyWnd::UpdateLists(EListType mode)
{
    TIItemContainer ruck_list;
    auto show_item = [&](const auto pIItem) -> bool { return !pIItem->m_flags.test(CInventoryItem::FIHiddenForInventory); };

    if (mode == eBoth || mode == e1st)
    {
        int our_scroll = m_pUIOurBagList->ScrollPos();
        m_pUIOurBagList->ClearAll(true);
        m_pActorInventoryOwner->inventory().RepackAmmo();

        ruck_list.clear();
        m_pActorInventoryOwner->inventory().AddAvailableItems(ruck_list, true);
        std::sort(ruck_list.begin(), ruck_list.end(), InventoryUtilities::CustomSort);
        // Наш рюкзак
        for (const auto& inv_item : ruck_list)
        {
            if (!show_item(inv_item))
                continue;
            CUICellItem* itm = create_cell_item(inv_item);
            if (inv_item->m_highlight_equipped)
            {
                itm->m_select_equipped = true;
                itm->SetColor(reinterpret_cast<CInventoryItem*>(itm->m_pData)->ClrEquipped);
            }
            m_pUIOurBagList->SetItem(itm);
        }
        m_pUIOurBagList->SetScrollPos(our_scroll);
    }

    if (mode == eBoth || mode == e2nd)
    {
        int other_scroll = m_pUIOthersBagList->ScrollPos();
        m_pUIOthersBagList->ClearAll(true);
        if (m_pOtherInventoryOwner)
            m_pOtherInventoryOwner->inventory().RepackAmmo();
        else
            m_pOtherInventoryBox->RepackAmmo();
        ruck_list.clear();
        if (m_pOtherInventoryOwner)
            m_pOtherInventoryOwner->inventory().AddAvailableItems(ruck_list, true);
        else
            m_pOtherInventoryBox->AddAvailableItems(ruck_list);
        std::sort(ruck_list.begin(), ruck_list.end(), InventoryUtilities::CustomSort);
        // Чужой рюкзак
        for (const auto& inv_item : ruck_list)
        {
            if (!show_item(inv_item))
                continue;
            CUICellItem* itm = create_cell_item(inv_item);
            m_pUIOthersBagList->SetItem(itm);
        }
        m_pUIOthersBagList->SetScrollPos(other_scroll);
    }

    m_b_need_update = false;
}

void CUICarBodyWnd::Show()
{
    InventoryUtilities::SendInfoToActor("ui_car_body");
    inherited::Show();
    SetCurrentItem(NULL);

    if (const auto& actor = Actor())
    {
        if (auto act_item = smart_cast<CHudItem*>(actor->inventory().ActiveItem()); act_item && act_item->IsZoomed())
            act_item->OnZoomOut();
        RepackAmmo();
    }
    PlaySnd(eInvSndOpen);
}

CUICellItem* CUICarBodyWnd::CurrentItem() { return m_pCurrentCellItem; }

PIItem CUICarBodyWnd::CurrentIItem() { return (m_pCurrentCellItem) ? (PIItem)m_pCurrentCellItem->m_pData : NULL; }

void CUICarBodyWnd::SetCurrentItem(CUICellItem* itm)
{
    if (m_pCurrentCellItem == itm)
        return;
    m_pCurrentCellItem = itm;
    
    //m_pUIItemInfo->InitItem(CurrentIItem());

    if (m_pCurrentCellItem)
    {
        m_pCurrentCellItem->m_select_armament = true;
        auto script_obj = CurrentIItem()->object().lua_game_object();
        g_actor->callback(GameObject::eCellItemSelect)(script_obj);
    }
}

void CUICarBodyWnd::TakeAll()
{
    u32 cnt = m_pUIOthersBagList->ItemsCount();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUICellItem* ci = m_pUIOthersBagList->GetItemIdx(i);
        for (u32 j = 0; j < ci->ChildsCount(); ++j)
        {
            PIItem _itm = (PIItem)(ci->Child(j)->m_pData);
            TransferItem(_itm, m_pOtherGO, m_pActorGO);
        }
        PIItem itm = (PIItem)(ci->m_pData);
        TransferItem(itm, m_pOtherGO, m_pActorGO);
    }
    PlaySnd(eInvMoveItem);
}

void CUICarBodyWnd::MoveItems(CUICellItem* itm, bool b_all)
{
    CUIDragDropListEx* owner_list = itm->OwnerList();

    bool actor_to_other = owner_list != m_pUIOthersBagList;

    if (!CanMoveToOther(CurrentIItem(), actor_to_other ? m_pOtherGO : m_pActorGO))
        return;

    if (actor_to_other)
    { // actor -> other
        if (b_all)
        {
            for (u32 j = 0; j < CurrentItem()->ChildsCount(); ++j)
            {
                PIItem _itm = (PIItem)(CurrentItem()->Child(j)->m_pData);
                TransferItem(_itm, m_pActorGO, m_pOtherGO);
            }
        }
        PIItem itm = (PIItem)(CurrentItem()->m_pData);
        TransferItem(itm, m_pActorGO, m_pOtherGO);
    }
    else
    { // other -> actor
        if (b_all)
        {
            for (u32 j = 0; j < CurrentItem()->ChildsCount(); ++j)
            {
                PIItem _itm = (PIItem)(CurrentItem()->Child(j)->m_pData);
                TransferItem(_itm, m_pOtherGO, m_pActorGO);
            }
        }
        PIItem itm = (PIItem)(CurrentItem()->m_pData);
        TransferItem(itm, m_pOtherGO, m_pActorGO);
    }

    PlaySnd(eInvMoveItem);

    owner_list->RemoveItem(itm, true);

    SetCurrentItem(NULL);
}

void CUICarBodyWnd::DropItems(bool b_all)
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!pActor)
    {
        return;
    }

    CUICellItem* ci = CurrentItem();
    if (!ci)
    {
        return;
    }

    CUIDragDropListEx* old_owner = ci->OwnerList();

    if (b_all)
    {
        u32 cnt = ci->ChildsCount();
        for (u32 i = 0; i < cnt; ++i)
        {
            CUICellItem* itm = ci->PopChild();
            PIItem iitm = (PIItem)itm->m_pData;
            iitm->Drop();
        }
    }

    CurrentIItem()->Drop();
    old_owner->RemoveItem(ci, b_all);

    SetCurrentItem(NULL);

    PlaySnd(eInvDropItem);
}

#include "../xr_level_controller.h"

bool CUICarBodyWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
    if (m_b_need_update)
        return true;

    if (keyboard_action == WINDOW_KEY_PRESSED)
    {
        if (m_pUIPropertiesBox->GetVisible())
            m_pUIPropertiesBox->OnKeyboard(dik, keyboard_action);
    }

    if (keyboard_action == WINDOW_KEY_PRESSED && is_binded(kUSE, dik))
    {
        GetHolder()->StartStopMenu(this, true);
        return true;
    }

    if (inherited::OnKeyboard(dik, keyboard_action))
        return true;

    return false;
}

bool CUICarBodyWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
    if (m_b_need_update)
        return true;

    if (mouse_action == WINDOW_RBUTTON_DOWN)
    {
        if (m_pUIPropertiesBox->IsShown())
        {
            m_pUIPropertiesBox->Hide();
            return true;
        }
    }

    if (m_pUIPropertiesBox->IsShown())
    {
        switch (mouse_action)
        {
        case WINDOW_MOUSE_WHEEL_DOWN:
        case WINDOW_MOUSE_WHEEL_UP: return true; break;
        }
    }

    if (CUIWindow::OnMouse(x, y, mouse_action))
    {
        return true;
    }

    return false;
}

void CUICarBodyWnd::EatItem()
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!pActor)
        return;
    m_pActorInventoryOwner->inventory().Eat(CurrentIItem(), m_pActorInventoryOwner);
    SetCurrentItem(NULL);
}

bool CUICarBodyWnd::OnItemStartDrag(CUICellItem* itm)
{
    return false; // default behaviour
}

bool CUICarBodyWnd::OnItemDrop(CUICellItem* itm)
{
    CUIDragDropListEx* old_owner = itm->OwnerList();
    CUIDragDropListEx* new_owner = CUIDragDropListEx::m_drag_item->BackList();

    if (old_owner == new_owner || !old_owner || !new_owner)
        return false;

    bool b_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
    MoveItems(itm, b_all);

    return true;
}

bool CUICarBodyWnd::OnItemDbClick(CUICellItem* itm)
{
    bool b_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
    MoveItems(itm, b_all);
    return true;
}

bool CUICarBodyWnd::OnItemSelected(CUICellItem* itm)
{
    SetCurrentItem(itm);
    itm->ColorizeItems({m_pUIOurBagList, m_pUIOthersBagList});
    return false;
}

bool CUICarBodyWnd::OnItemRButtonClick(CUICellItem* itm)
{
    SetCurrentItem(itm);
    ActivatePropertiesBox();
    return false;
}

bool CUICarBodyWnd::TransferItem(PIItem itm, CGameObject* owner_from, CGameObject* owner_to)
{
    if (!CanMoveToOther(itm, owner_to))
        return false;
    itm->Transfer(owner_from->ID(), owner_to->ID());
    return true;
}

void CUICarBodyWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
    lst->m_f_item_drop = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemDrop);
    lst->m_f_item_start_drag = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemStartDrag);
    lst->m_f_item_db_click = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemDbClick);
    lst->m_f_item_selected = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemSelected);
    lst->m_f_item_rbutton_click = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemRButtonClick);
}

void CUICarBodyWnd::PlaySnd(eInventorySndAction a)
{
    if (sounds[a]._handle())
        sounds[a].play(NULL, sm_2D);
}

void CUICarBodyWnd::MoveAllFromRuck()
{
    for (const auto& item : m_pActorInventoryOwner->inventory().m_ruck)
        TransferItem(item, m_pActorGO, m_pOtherGO);
}

bool CUICarBodyWnd::CanMoveToOther(PIItem pItem, CGameObject* owner_to) const
{
    if (smart_cast<CBaseMonster*>(owner_to))
        return false;
    bool can_move{};
    if (auto owner = smart_cast<CInventoryOwner*>(owner_to))
        can_move = owner->inventory().CanTakeItem(pItem);
    else if (auto box = smart_cast<IInventoryBox*>(owner_to))
        can_move = box->CanTakeItem(pItem);
    return can_move;
}

void CUICarBodyWnd::DetachAddon(const char* addon_name, bool for_all)
{
    PlaySnd(eInvDetachAddon);

    auto itm = CurrentItem();
    for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i)
    {
        auto child_itm = itm->Child(i);
        ((PIItem)child_itm->m_pData)->Detach(addon_name, true);
    }
    CurrentIItem()->Detach(addon_name, true);

    // спрятать вещь из активного слота в инвентарь на время вызова менюшки
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (pActor && CurrentIItem() == pActor->inventory().ActiveItem())
    {
        pActor->inventory().Activate(NO_ACTIVE_SLOT);
    }
}

void CUICarBodyWnd::RepackAmmo()
{ 
    m_pActorInventoryOwner->inventory().RepackAmmo();
    if (m_pOtherInventoryOwner)
        m_pOtherInventoryOwner->inventory().RepackAmmo();
    else
        m_pOtherInventoryBox->RepackAmmo();
}