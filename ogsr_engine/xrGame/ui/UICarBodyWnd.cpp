#include "stdafx.h"
#include "UICarBodyWnd.h"
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../HUDManager.h"
#include "../level.h"
#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UIFrameWindow.h"
#include "UIItemInfo.h"
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
#include "Vest.h"
#include "GasMask.h"
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

    m_pUIStaticTop = xr_new<CUIStatic>();
    m_pUIStaticTop->SetAutoDelete(true);
    AttachChild(m_pUIStaticTop);
    xml_init.InitStatic(uiXml, "top_background", 0, m_pUIStaticTop);

    m_pUIStaticBottom = xr_new<CUIStatic>();
    m_pUIStaticBottom->SetAutoDelete(true);
    AttachChild(m_pUIStaticBottom);
    xml_init.InitStatic(uiXml, "bottom_background", 0, m_pUIStaticBottom);

    m_pUIOurIcon = xr_new<CUIStatic>();
    m_pUIOurIcon->SetAutoDelete(true);
    AttachChild(m_pUIOurIcon);
    xml_init.InitStatic(uiXml, "static_icon", 0, m_pUIOurIcon);

    m_pUIOthersIcon = xr_new<CUIStatic>();
    m_pUIOthersIcon->SetAutoDelete(true);
    AttachChild(m_pUIOthersIcon);
    xml_init.InitStatic(uiXml, "static_icon", 1, m_pUIOthersIcon);

    m_pUICharacterInfoLeft = xr_new<CUICharacterInfo>();
    m_pUICharacterInfoLeft->SetAutoDelete(true);
    m_pUIOurIcon->AttachChild(m_pUICharacterInfoLeft);
    m_pUICharacterInfoLeft->Init(0, 0, m_pUIOurIcon->GetWidth(), m_pUIOurIcon->GetHeight(), "trade_character.xml");

    m_pUICharacterInfoRight = xr_new<CUICharacterInfo>();
    m_pUICharacterInfoRight->SetAutoDelete(true);
    m_pUIOthersIcon->AttachChild(m_pUICharacterInfoRight);
    m_pUICharacterInfoRight->Init(0, 0, m_pUIOthersIcon->GetWidth(), m_pUIOthersIcon->GetHeight(), "trade_character.xml");

    m_pUIOurBagWnd = xr_new<CUIStatic>();
    m_pUIOurBagWnd->SetAutoDelete(true);
    AttachChild(m_pUIOurBagWnd);
    xml_init.InitStatic(uiXml, "our_bag_static", 0, m_pUIOurBagWnd);

    m_pUIOurWeightWnd = xr_new<CUIStatic>();
    m_pUIOurWeightWnd->SetAutoDelete(true);
    m_pUIOurBagWnd->AttachChild(m_pUIOurWeightWnd);
    xml_init.InitStatic(uiXml, "our_weight_static", 0, m_pUIOurWeightWnd);

    m_pUIOthersBagWnd = xr_new<CUIStatic>();
    m_pUIOthersBagWnd->SetAutoDelete(true);
    AttachChild(m_pUIOthersBagWnd);
    xml_init.InitStatic(uiXml, "others_bag_static", 0, m_pUIOthersBagWnd);

    m_pUIOthersLimitWnd = xr_new<CUIStatic>();
    m_pUIOthersLimitWnd->SetAutoDelete(true);
    m_pUIOthersBagWnd->AttachChild(m_pUIOthersLimitWnd);
    xml_init.InitStatic(uiXml, "others_limit_static", 0, m_pUIOthersLimitWnd);

    m_pUIOurBagList = xr_new<CUIDragDropListEx>();
    m_pUIOurBagList->SetAutoDelete(true);
    m_pUIOurBagWnd->AttachChild(m_pUIOurBagList);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_list_our", 0, m_pUIOurBagList);

    m_pUIOthersBagList = xr_new<CUIDragDropListEx>();
    m_pUIOthersBagList->SetAutoDelete(true);
    m_pUIOthersBagWnd->AttachChild(m_pUIOthersBagList);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_list_other", 0, m_pUIOthersBagList);

    // информация о предмете
    m_pUIDescWnd = xr_new<CUIFrameWindow>();
    m_pUIDescWnd->SetAutoDelete(true);
    AttachChild(m_pUIDescWnd);
    xml_init.InitFrameWindow(uiXml, "frame_window", 0, m_pUIDescWnd);

    m_pUIStaticDesc = xr_new<CUIStatic>();
    m_pUIStaticDesc->SetAutoDelete(true);
    m_pUIDescWnd->AttachChild(m_pUIStaticDesc);
    xml_init.InitStatic(uiXml, "descr_static", 0, m_pUIStaticDesc);
    delay = uiXml.ReadAttribInt("descr_static", 0, "show_delay", 1);
    info_offset.x = uiXml.ReadAttribFlt("descr_static", 0, "show_x");
    info_offset.y = uiXml.ReadAttribFlt("descr_static", 0, "show_y");

    m_pUIItemInfo = xr_new<CUIItemInfo>();
    m_pUIItemInfo->SetAutoDelete(true);
    m_pUIDescWnd->AttachChild(m_pUIItemInfo);
    m_pUIItemInfo->Init(0, 0, m_pUIDescWnd->GetWidth(), m_pUIDescWnd->GetHeight(), CARBODY_ITEM_XML);

    xml_init.InitAutoStatic(uiXml, "auto_static", this);

    m_pUIPropertiesBox = xr_new<CUIPropertiesBox>();
    m_pUIPropertiesBox->SetAutoDelete(true);
    AttachChild(m_pUIPropertiesBox);
    m_pUIPropertiesBox->Init(0, 0, 300, 300);
    m_pUIPropertiesBox->Hide();

    SetCurrentItem(NULL);
    m_pUIStaticDesc->SetText(NULL);

    m_pUITakeAll = xr_new<CUI3tButton>();
    m_pUITakeAll->SetAutoDelete(true);
    AttachChild(m_pUITakeAll);
    xml_init.Init3tButton(uiXml, "take_all_btn", 0, m_pUITakeAll);

    m_pUIExitButton = xr_new<CUI3tButton>();
    m_pUIExitButton->SetAutoDelete(true);
    AttachChild(m_pUIExitButton);
    xml_init.Init3tButton(uiXml, "exit_button", 0, m_pUIExitButton);

    m_pUIOrganizeButton = xr_new<CUI3tButton>();
    m_pUIOrganizeButton->SetAutoDelete(true);
    AttachChild(m_pUIOrganizeButton);
    xml_init.Init3tButton(uiXml, "organize_button", 0, m_pUIOrganizeButton);

    m_pUIMoveAllFromRuckButton = xr_new<CUI3tButton>();
    m_pUIMoveAllFromRuckButton->SetAutoDelete(true);
    AttachChild(m_pUIMoveAllFromRuckButton);
    xml_init.Init3tButton(uiXml, "move_all_from_ruck_button", 0, m_pUIMoveAllFromRuckButton);

    m_pUIShowAllInv = xr_new<CUICheckButton>();
    m_pUIShowAllInv->SetAutoDelete(true);
    AttachChild(m_pUIShowAllInv);
    xml_init.InitCheck(uiXml, "show_all_inv", 0, m_pUIShowAllInv);

    BindDragDropListEnents(m_pUIOurBagList);
    BindDragDropListEnents(m_pUIOthersBagList);

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

    m_pUICharacterInfoLeft->InitCharacter(m_pActorGO->ID());
    m_pUIOthersIcon->Show(false);
    m_pUICharacterInfoRight->ClearInfo();
    m_pUIPropertiesBox->Hide();
    EnableAll();
    UpdateLists();

    m_pUIOthersLimitWnd->Show(m_pOtherInventoryBox->GetItemsLimit());

    if (auto obj = smart_cast<CInventoryBox*>(pInvBox))
    {
        obj->callback(GameObject::eOnInvBoxOpen)();
    }
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

    m_pUICharacterInfoLeft->InitCharacter(our_id);
    m_pUIOthersIcon->Show(true);
    m_pUIOthersLimitWnd->Show(false);

    CBaseMonster* monster = NULL;
    if (m_pOtherInventoryOwner)
    {
        monster = smart_cast<CBaseMonster*>(m_pOtherInventoryOwner);
        if (monster || m_pOtherInventoryOwner->use_simplified_visual())
        {
            m_pUICharacterInfoRight->ClearInfo();
            if (monster)
            {
                shared_str monster_tex_name = pSettings->r_string(monster->cNameSect(), "icon");
                m_pUICharacterInfoRight->UIIcon().InitTexture(monster_tex_name.c_str());
                m_pUICharacterInfoRight->UIIcon().SetStretchTexture(true);
            }
        }
        else
        {
            m_pUICharacterInfoRight->InitCharacter(other_id);
        }
    }

    m_pUIPropertiesBox->Hide();
    EnableAll();
    UpdateLists();

    if (!monster)
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

    if (const auto& actor = Actor())
    {
        //actor->SetWeaponHideState(INV_STATE_INV_WND, false);
        actor->SetRuckAmmoPlacement(false); // сбросим флаг перезарядки из рюкзака
        actor->EnableUIDOF(false);
        actor->TryInventoryCrouch(false);
        actor->EnableInvEffector(false);
    }
    m_bShowAllInv = false;
    PlaySnd(eInvSndClose);
}

void CUICarBodyWnd::UpdateLists()
{
    TIItemContainer ruck_list;
    m_pUIOurBagList->ClearAll(true);
    m_pUIOthersBagList->ClearAll(true);

    ruck_list.clear();
    if (m_bShowAllInv)
        m_pActorInventoryOwner->inventory().AddAvailableItems(ruck_list, false);
    else
    {
        for (const auto& item : m_pActorInventoryOwner->inventory().m_ruck)
            ruck_list.push_back(item);
    }

    std::sort(ruck_list.begin(), ruck_list.end(), InventoryUtilities::GreaterRoomInRuck);

    // Наш рюкзак
    for (const auto& inv_item : ruck_list)
    {
        CUICellItem* itm = create_cell_item(inv_item);
        if (inv_item->m_highlight_equipped)
        {
            itm->m_select_equipped = true;
            itm->SetColor(reinterpret_cast<CInventoryItem*>(itm->m_pData)->ClrEquipped);
        }
        m_pUIOurBagList->SetItem(itm);
    }

    ruck_list.clear();
    if (m_pOtherInventoryOwner)
        m_pOtherInventoryOwner->inventory().AddAvailableItems(ruck_list, false);
    else
        m_pOtherInventoryBox->AddAvailableItems(ruck_list);

    std::sort(ruck_list.begin(), ruck_list.end(), InventoryUtilities::GreaterRoomInRuck);

    // Чужой рюкзак
    for (const auto& inv_item : ruck_list)
    {
        CUICellItem* itm = create_cell_item(inv_item);
        m_pUIOthersBagList->SetItem(itm);
    }

    UpdateWeight();
    m_b_need_update = false;
}

void CUICarBodyWnd::ActivatePropertiesBox()
{
    m_pUIPropertiesBox->RemoveAll();

    auto pWeapon = smart_cast<CWeapon*>(CurrentIItem());
    auto pAmmo = smart_cast<CWeaponAmmo*>(CurrentIItem());
    auto pEatableItem = smart_cast<CEatableItem*>(CurrentIItem());
    auto pVest = smart_cast<CVest*>(CurrentIItem());
    auto pGasmask = smart_cast<CGasMask*>(CurrentIItem());

    LPCSTR detach_tip = CurrentIItem()->GetDetachMenuTip();

    bool b_actor_inv = CurrentItem()->OwnerList() == m_pUIOurBagList;
    const auto inv = &m_pActorInventoryOwner->inventory();
    string1024 temp;

    bool b_show = false;

    bool b_many = CurrentItem()->ChildsCount();
    LPCSTR _many = b_many ? "•" : "";
    LPCSTR _addon_name{};

    const char* _addon_sect{};

    if (pVest && pVest->CanDetach(pVest->GetPlateName().c_str()))
    {
        _addon_sect = pVest->GetPlateName().c_str();
        _addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
        sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
        m_pUIPropertiesBox->AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
        b_show = true;
    }

    if (pGasmask && pGasmask->IsFilterInstalled() && pGasmask->CanDetach(pGasmask->GetFilterName().c_str()))
    {
        _addon_sect = pGasmask->GetFilterName().c_str();
        _addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
        sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
        m_pUIPropertiesBox->AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
        b_show = true;
    }

    if (CurrentIItem()->IsPowerSourceAttachable() && CurrentIItem()->IsPowerSourceAttached() && CurrentIItem()->CanDetach(CurrentIItem()->GetPowerSourceName().c_str()))
    {
        _addon_sect = CurrentIItem()->GetPowerSourceName().c_str();
        _addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
        sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
        m_pUIPropertiesBox->AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
        b_show = true;
    }

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
                    b_show = true;
                }
            }
            // unload AmmoBox
            if (pAmmo->m_boxCurr)
            {
                sprintf(temp, "%s%s", _many, CStringTable().translate("st_unload_magazine").c_str());
                m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_UNLOAD_AMMO_BOX);
                b_show = true;
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
                    b_show = true;
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
                b_show = true;
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
                b_show = true;
            }
        }
    }

    if (pEatableItem)
    {
        m_pUIPropertiesBox->AddItem(pEatableItem->GetUseMenuTip(), NULL, INVENTORY_EAT_ACTION);
        b_show = true;
    }

    if (CurrentIItem()->CanBeDisassembled())
    {
        sprintf(temp, "%s%s", _many, CStringTable().translate(CurrentIItem()->GetDisassembleMenuTip()).c_str());
        m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DISASSEMBLE);
        b_show = true;
    }

    if ((CheckMonsterAndKnife() || b_actor_inv) && !CurrentIItem()->IsQuestItem())
    {
        sprintf(temp, "%s%s", _many, CStringTable().translate("st_drop").c_str());
        m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DROP_ACTION);
        b_show = true;
    }

    if (CanMoveToOther(CurrentIItem(), b_actor_inv ? m_pOtherGO : m_pActorGO))
    {
        sprintf(temp, "%s%s", _many, CStringTable().translate("st_move").c_str());
        m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_MOVE_ACTION);
        b_show = true;
    }

    if (b_show)
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
    if (BUTTON_CLICKED == msg)
    {
        if (m_pUITakeAll == pWnd)
        {
            TakeAll();
        }
        else if (m_pUIExitButton == pWnd)
        {
            GetHolder()->StartStopMenu(this, true);
        }
        else if (m_pUIOrganizeButton == pWnd)
        {
            RepackAmmo();
            UpdateLists_delayed();
        }
        else if (m_pUIMoveAllFromRuckButton == pWnd)
        {
            MoveAllFromRuck();
        }
        else if (m_pUIShowAllInv == pWnd)
        {
            m_bShowAllInv = !m_bShowAllInv;
            UpdateLists_delayed();
        }
    }
    else if (pWnd == m_pUIPropertiesBox && msg == PROPERTY_CLICKED)
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
            case INVENTORY_DISASSEMBLE: {
                DisassembleItem(for_all);
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
                UpdateLists_delayed();
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
    if (m_b_need_update || m_pActorInventoryOwner->inventory().ModifyFrame() == Device.dwFrame ||
        (m_pOtherInventoryOwner && m_pOtherInventoryOwner->inventory().ModifyFrame() == Device.dwFrame))
    {
        if (m_pUIOurBagList && m_pUIOthersBagList)
        {
            int our_scroll = m_pUIOurBagList->ScrollPos();
            int other_scroll = m_pUIOthersBagList->ScrollPos();
            UpdateLists();
            m_pUIOurBagList->SetScrollPos(our_scroll);
            m_pUIOthersBagList->SetScrollPos(other_scroll);
        }
    }

    if (m_pOtherGO &&
        m_pActorGO->Position().distance_to(m_pOtherGO->Position()) - m_pOtherGO->Radius() - m_pActorGO->Radius() > m_pActorInventoryOwner->inventory().GetTakeDist() + 0.5f)
    {
        GetHolder()->StartStopMenu(this, true);
    }
    CheckForcedWeightUpdate();
    Actor()->EnableUIDOF(true);
    Actor()->UpdateCameraDirection(m_pOtherGO);
    UpdateFloatingItemDescription();
    inherited::Update();
}

void CUICarBodyWnd::Show()
{
    InventoryUtilities::SendInfoToActor("ui_car_body");
    inherited::Show();
    SetCurrentItem(NULL);
    UpdateWeight();

    if (const auto& actor = Actor())
    {
        if (auto act_item = smart_cast<CHudItem*>(actor->inventory().ActiveItem()); act_item && act_item->IsZoomed())
            act_item->OnZoomOut();
        //actor->SetWeaponHideState(INV_STATE_INV_WND, true);
        actor->SetRuckAmmoPlacement(true);
        RepackAmmo();
        actor->TryInventoryCrouch(true);
        actor->EnableInvEffector(true);
        TryActivateKnife();
    }
    PlaySnd(eInvSndOpen);
}

void CUICarBodyWnd::DisableAll()
{
    m_pUIOurBagWnd->Enable(false);
    m_pUIOthersBagWnd->Enable(false);
}

void CUICarBodyWnd::EnableAll()
{
    m_pUIOurBagWnd->Enable(true);
    m_pUIOthersBagWnd->Enable(true);
}

CUICellItem* CUICarBodyWnd::CurrentItem() { return m_pCurrentCellItem; }

PIItem CUICarBodyWnd::CurrentIItem() { return (m_pCurrentCellItem) ? (PIItem)m_pCurrentCellItem->m_pData : NULL; }

void CUICarBodyWnd::SetCurrentItem(CUICellItem* itm)
{
    if (m_pCurrentCellItem == itm)
        return;
    m_pCurrentCellItem = itm;
    if (!m_pCurrentCellItem)
        itm_to_descr = nullptr;
    
    if (!Core.Features.test(xrCore::Feature::floating_description_window))
        m_pUIItemInfo->InitItem(CurrentIItem());

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
    TryPlayStabbing(CurrentIItem(), m_pOtherGO);
    CurrentIItem()->Drop();
    old_owner->RemoveItem(ci, b_all);

    SetCurrentItem(NULL);

    UpdateWeight();

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
    TryPlayStabbing(itm, owner_from);
    itm->Transfer(owner_from->ID(), owner_to->ID());
    return true;
}

void CUICarBodyWnd::BindDragDropListEnents(CUIDragDropListEx* lst)
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
    if (!CheckMonsterAndKnife())
        return false;
    bool can_move{};
    if (auto owner = smart_cast<CInventoryOwner*>(owner_to))
    {
        can_move = owner->inventory().CanTakeItem(pItem);
    }
    else if (auto box = smart_cast<IInventoryBox*>(owner_to))
    {
        can_move = box->CanTakeItem(pItem);
    }
    return can_move;
}

void CUICarBodyWnd::UpdateWeight() 
{ 
    InventoryUtilities::UpdateWeight(*m_pUIOurWeightWnd);
    if (m_pOtherInventoryBox)
        InventoryUtilities::UpdateLimit(*m_pUIOthersLimitWnd, m_pOtherGO);
}

#include "InventoryContainer.h"
void CUICarBodyWnd::CheckForcedWeightUpdate()
{
    bool need_update{};
    auto place_to_search = m_pActorInventoryOwner->inventory().GetActiveArtefactPlace();
    for (const auto& item : place_to_search)
    {
        auto artefact = smart_cast<CArtefact*>(item);
        if (artefact && !fis_zero(artefact->m_fTTLOnDecrease) && !fis_zero(artefact->GetCondition()) && !fis_zero(artefact->GetItemEffect(CInventoryItem::eAdditionalWeight)))
        {
            need_update = true;
            break;
        }
        if (auto container = smart_cast<CInventoryContainer*>(item))
        {
            if (!fis_zero(container->GetContainmentArtefactEffect(CInventoryItem::eAdditionalWeight)))
            {
                need_update = true;
                break;
            }
        }
    }
    if (need_update)
        UpdateWeight();
}

bool CUICarBodyWnd::CheckMonsterAndKnife() const
{
    return !Core.Features.test(xrCore::Feature::knife_to_cut_parts) || !smart_cast<CBaseMonster*>(m_pOtherInventoryOwner) ||
        smart_cast<CWeaponKnife*>(m_pActorInventoryOwner->inventory().ActiveItem());
}

void CUICarBodyWnd::TryActivateKnife()
{
    if (CheckMonsterAndKnife())
        return;
    auto& inv = m_pActorInventoryOwner->inventory();
    for (const auto& slot : inv.m_slots)
    {
        if (slot.m_pIItem && slot.CanBeActivated() && smart_cast<CWeaponKnife*>(slot.m_pIItem))
        {
            inv.Activate(slot.m_pIItem->GetSlot());
            return;
        }
    }
}

void CUICarBodyWnd::TryPlayStabbing(PIItem itm, CGameObject* owner_from)
{
    auto monster = smart_cast<CBaseMonster*>(owner_from);
    if (!monster)
        return;
    bool owner_is_monster = (itm->object().H_Parent() == owner_from);
    if (Core.Features.test(xrCore::Feature::knife_to_cut_parts) && monster && owner_is_monster)
    {
        auto knife = smart_cast<CWeaponKnife*>(m_pActorInventoryOwner->inventory().ActiveItem());
        if (knife)
        {
            knife->Fire2Start(); // нанесём удар ножом
            itm->ChangeCondition(-(1 - knife->GetCondition())); // уменьшим Condition части монстра на величину износа ножа (1 - Knife->GetCondition())
            knife->ChangeCondition(-knife->GetCondDecPerShotOnHit() * monster->m_fSkinDensityK); // уменьшим Condition ножа износ за удар * коэф плотности кожи монстра
        }
    }
}

void CUICarBodyWnd::DisassembleItem(bool b_all)
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!pActor)
        return;
    if (!CurrentIItem() || CurrentIItem()->IsQuestItem())
        return;
    if (b_all)
    {
        u32 cnt = CurrentItem()->ChildsCount();
        for (u32 i = 0; i < cnt; ++i)
        {
            CUICellItem* itm = CurrentItem()->PopChild();
            PIItem iitm = (PIItem)itm->m_pData;
            iitm->Disassemble();
        }
    }
    CurrentIItem()->Disassemble();
    SetCurrentItem(nullptr);
    UpdateWeight();
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

void CUICarBodyWnd::InitFloatingDescription(CUICellItem* itm)
{
    if (!Core.Features.test(xrCore::Feature::floating_description_window) || Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION)))
        return;
    itm_to_descr = itm;
    m_pUIItemInfo->InitItem((PIItem)itm_to_descr->m_pData);
    delay_time = Device.dwTimeGlobal + delay * 1000;
    BringToTop(m_pUIDescWnd);
}
void CUICarBodyWnd::UpdateFloatingItemDescription()
{
    if (!Core.Features.test(xrCore::Feature::floating_description_window) || Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION)))
        return;
    auto cur_time = Device.dwTimeGlobal;
    m_pUIDescWnd->Show(itm_to_descr && itm_to_descr->m_selected && cur_time > delay_time && !m_pUIPropertiesBox->IsShown());
    Fvector2 v_res{1024.f, 768.f};
    Fvector2 pos{GetUICursor()->GetCursorPosition()};
    pos.add(info_offset);
    Fvector2 wnd_size{m_pUIDescWnd->GetWidth(), m_pUIDescWnd->GetHeight()};
    Fvector2 delta{pos.x + wnd_size.x - v_res.x, pos.y + wnd_size.y - v_res.y};
    if (delta.x > 0.f)
        pos.x -= delta.x;
    if (delta.y > 0.f)
        pos.y -= delta.y;
    m_pUIDescWnd->SetWndPos(pos);
}