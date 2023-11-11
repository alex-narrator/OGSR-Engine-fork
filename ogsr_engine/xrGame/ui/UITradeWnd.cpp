#include "stdafx.h"
#include "UITradeWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "Entity.h"
#include "HUDManager.h"
#include "WeaponAmmo.h"
#include "Actor.h"
#include "Trade.h"
#include "UIGameSP.h"
#include "UIInventoryUtilities.h"
#include "inventoryowner.h"
#include "eatable_item.h"
#include "Weapon.h"
#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "inventory.h"
#include "level.h"
#include "string_table.h"
#include "../character_info.h"
#include "UIMultiTextStatic.h"
#include "UI3tButton.h"
#include "UIItemInfo.h"

#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIPropertiesBox.h"
#include "UIListBoxItem.h"
#include "UICheckButton.h"

#include "string_table.h"

#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../xr_3da/xr_input.h"

constexpr auto TRADE_XML = "trade.xml";
constexpr auto TRADE_CHARACTER_XML = "trade_character.xml";
constexpr auto TRADE_ITEM_XML = "trade_item.xml";

struct CUITradeInternal
{
    CUIDragDropListEx UIOurBagList;
    CUIDragDropListEx UIOthersBagList;

    CUIMultiTextStatic UIOurPriceCaption;
    CUIMultiTextStatic UIOthersPriceCaption;
    CUIDragDropListEx UIOurTradeList;
    CUIDragDropListEx UIOthersTradeList;

    SDrawStaticStruct* UIDealMsg{};
};

bool others_zero_trade;
const char* money_name;

CUITradeWnd::CUITradeWnd() : m_bDealControlsVisible(false), m_pTrade(NULL), m_pOthersTrade(NULL), bStarted(false)
{
    m_uidata = xr_new<CUITradeInternal>();
    Init();
    Hide();
    SetCurrentItem(NULL);

    others_zero_trade = !!READ_IF_EXISTS(pSettings, r_bool, "trade", "others_zero_trade", false);
    money_name = CStringTable().translate("ui_st_money_regional").c_str();
}

CUITradeWnd::~CUITradeWnd()
{
    m_uidata->UIOurBagList.ClearAll(true);
    m_uidata->UIOurTradeList.ClearAll(true);
    m_uidata->UIOthersBagList.ClearAll(true);
    m_uidata->UIOthersTradeList.ClearAll(true);
    xr_delete(m_uidata);
}

void CUITradeWnd::Init()
{
    CUIXml uiXml;
    bool xml_result = uiXml.Init(CONFIG_PATH, UI_PATH, TRADE_XML);
    R_ASSERT3(xml_result, "xml file not found: trade.xml", TRADE_XML);
    CUIXmlInit xml_init;

    xml_init.InitWindow(uiXml, "main", 0, this);

    xml_init.InitAutoStatic(uiXml, "auto_static", this);

    AttachChild(&m_uidata->UIOurPriceCaption);
    xml_init.InitMultiTextStatic(uiXml, "price_mt_static_our", 0, &m_uidata->UIOurPriceCaption);

    AttachChild(&m_uidata->UIOthersPriceCaption);
    xml_init.InitMultiTextStatic(uiXml, "price_mt_static_other", 0, &m_uidata->UIOthersPriceCaption);

    // Списки Drag&Drop
    AttachChild(&m_uidata->UIOurBagList);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_list_our_bag", 0, &m_uidata->UIOurBagList);

    AttachChild(&m_uidata->UIOthersBagList);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_list_other_bag", 0, &m_uidata->UIOthersBagList);

    AttachChild(&m_uidata->UIOurTradeList);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_list_our_trade", 0, &m_uidata->UIOurTradeList);

    AttachChild(&m_uidata->UIOthersTradeList);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_list_other_trade", 0, &m_uidata->UIOthersTradeList);

    m_pUIPropertiesBox = xr_new<CUIPropertiesBox>();
    m_pUIPropertiesBox->SetAutoDelete(true);
    AttachChild(m_pUIPropertiesBox);
    m_pUIPropertiesBox->Init(0, 0, 300, 300);
    m_pUIPropertiesBox->Hide();

    m_uidata->UIDealMsg = NULL;

    BindDragDropListEvents(&m_uidata->UIOurBagList);
    BindDragDropListEvents(&m_uidata->UIOthersBagList);
    BindDragDropListEvents(&m_uidata->UIOurTradeList);
    BindDragDropListEvents(&m_uidata->UIOthersTradeList);

    // Load sounds
    if (uiXml.NavigateToNode("action_sounds", 0))
    {
        XML_NODE* stored_root = uiXml.GetLocalRoot();
        uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));

        ::Sound->create(sounds[eInvSndOpen], uiXml.Read("snd_open", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvProperties], uiXml.Read("snd_properties", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvDropItem], uiXml.Read("snd_drop_item", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvMoveItem], uiXml.Read("snd_move_item", 0, NULL), st_Effect, sg_SourceType);
        ::Sound->create(sounds[eInvDetachAddon], uiXml.Read("snd_detach_addon", 0, NULL), st_Effect, sg_SourceType);

        uiXml.SetLocalRoot(stored_root);
    }
}

void CUITradeWnd::InitTrade(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{
    VERIFY(pOur);
    VERIFY(pOthers);

    m_pInvOwner = pOur;
    m_pOthersInvOwner = pOthers;
    m_uidata->UIOthersPriceCaption.GetPhraseByIndex(0)->SetText(CStringTable().translate("ui_st_opponent_items").c_str());

    m_pInv = &m_pInvOwner->inventory();
    m_pOthersInv = pOur->GetTrade()->GetPartnerInventory();

    m_pTrade = pOur->GetTrade();
    m_pOthersTrade = pOur->GetTrade()->GetPartnerTrade();

    m_pUIPropertiesBox->Hide();

    UpdateLists(eBoth);
}

void CUITradeWnd::ActivatePropertiesBox()
{
    m_pUIPropertiesBox->RemoveAll();

    auto pWeapon = smart_cast<CWeapon*>(CurrentIItem());
    auto pAmmo = smart_cast<CWeaponAmmo*>(CurrentIItem());
    auto pEatableItem = smart_cast<CEatableItem*>(CurrentIItem());

    LPCSTR detach_tip = CurrentIItem()->GetDetachMenuTip();

    bool b_actor_inv = CurrentItem()->OwnerList() == &m_uidata->UIOurBagList;
    const auto& inv = m_pInv;

    string1024 temp;

    bool b_show = false;
    bool b_many = CurrentItem()->ChildsCount();
    LPCSTR _many = b_many ? "•" : "";
    LPCSTR _addon_name{};

    const char* _addon_sect{};

    if (b_actor_inv)
    {
        if (pAmmo && pAmmo->IsBoxReloadable())
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

        if (pWeapon)
        {
            if (inv->InSlot(pWeapon) && smart_cast<CWeaponMagazined*>(pWeapon))
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

        for (const auto& action : CurrentIItem()->m_script_actions_map)
        {
            if (luabind::functor<bool> m_functorHasAction; ai().script_engine().functor(action.second[0].c_str(), m_functorHasAction))
            {
                if (m_functorHasAction(CurrentIItem()->object().lua_game_object()))
                {
                    LPCSTR tip_text{};
                    if (luabind::functor<LPCSTR> tip_func; ai().script_engine().functor(action.first.c_str(), tip_func))
                        tip_text = tip_func(CurrentIItem()->object().lua_game_object());
                    else
                        tip_text = CStringTable().translate(action.first).c_str();

                    m_pUIPropertiesBox->AddItem(tip_text, (void*)action.first.c_str(), INVENTORY_SCRIPT_ACTION);
                    b_show = true;
                }
            }
            else
                Msg("!Item-action-condition function [%s] not exist.", action.second[0].c_str());
        }

        if (!CurrentIItem()->IsQuestItem())
        {
            sprintf(temp, "%s%s", _many, CStringTable().translate("st_drop").c_str());
            m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DROP_ACTION);
            b_show = true;
        }
    }

    if (CanMoveToOther(CurrentIItem(), b_actor_inv))
    {
        sprintf(temp, "%s%s", _many, CStringTable().translate("st_move").c_str());
        m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_MOVE_ACTION);
        b_show = true;
    }

    b_show = true;

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

void CUITradeWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
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
            case INVENTORY_SCRIPT_ACTION: {
                auto it = CurrentIItem()->m_script_actions_map.find((LPCSTR)m_pUIPropertiesBox->GetClickedItem()->GetData());
                if (luabind::functor<void> m_functorDoAction; ai().script_engine().functor(it->second[1].c_str(), m_functorDoAction))
                    m_functorDoAction(CurrentIItem()->object().lua_game_object());
                else
                    Msg("!Item-action function [%s] not exist.", it->second[1].c_str());
            }
            break;
            }
        }

        // refresh if nessesary
        switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
        {
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

    CUIWindow::SendMessage(pWnd, msg, pData);
}

void CUITradeWnd::Draw()
{
    inherited::Draw();
    if (m_uidata->UIDealMsg)
        m_uidata->UIDealMsg->Draw();
}

void CUITradeWnd::Update()
{
    EListType et = eNone;

    if (m_b_need_update || m_pInv->StateInvalid() && m_pOthersInv->StateInvalid())
    {
        et = eBoth;
    }
    else if (m_pInv->StateInvalid())
    {
        et = e1st;
    }
    else if (m_pOthersInv->StateInvalid())
    {
        et = e2nd;
    }
    if (et != eNone)
        UpdateLists(et);

    inherited::Update();
    Actor()->UpdateCameraDirection(smart_cast<CGameObject*>(m_pOthersInvOwner));

    if (m_uidata->UIDealMsg)
    {
        m_uidata->UIDealMsg->Update();
        if (!m_uidata->UIDealMsg->IsActual())
        {
            HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money");
            m_uidata->UIDealMsg = NULL;
        }
    }
}

#include "UIInventoryUtilities.h"
void CUITradeWnd::Show()
{
    InventoryUtilities::SendInfoToActor("ui_trade");
    inherited::Show(true);
    inherited::Enable(true);

    SetCurrentItem(NULL);
    ResetAll();
    m_uidata->UIDealMsg = NULL;

    UpdateLists(eBoth);

    PlaySnd(eInvSndOpen);
}

void CUITradeWnd::Hide()
{
    InventoryUtilities::SendInfoToActor("ui_trade_hide");
    inherited::Show(false);
    inherited::Enable(false);
    if (bStarted)
        StopTrade();

    m_uidata->UIDealMsg = NULL;

    if (HUD().GetUI()->UIGame())
        HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money");

    m_uidata->UIOurBagList.ClearAll(true);
    m_uidata->UIOurTradeList.ClearAll(true);
    m_uidata->UIOthersBagList.ClearAll(true);
    m_uidata->UIOthersTradeList.ClearAll(true);
}

void CUITradeWnd::StartTrade()
{
    if (m_pTrade)
        m_pTrade->TradeCB(true);
    if (m_pOthersTrade)
        m_pOthersTrade->TradeCB(true);
    bStarted = true;
}

void CUITradeWnd::StopTrade()
{
    if (m_pTrade)
        m_pTrade->TradeCB(false);
    if (m_pOthersTrade)
        m_pOthersTrade->TradeCB(false);
    bStarted = false;
}

#include "../trade_parameters.h"
bool CUITradeWnd::CanMoveToOther(PIItem pItem, bool our)
{
    if (pItem->m_flags.test(CInventoryItem::FIAlwaysUntradable) || !pItem->CanTrade())
        return false;
    if (!our)
        return true;

    float r1 = CalcItemsWeight(&m_uidata->UIOurTradeList); // our
    float r2 = CalcItemsWeight(&m_uidata->UIOthersTradeList); // other

    float itmWeight = pItem->Weight();
    float otherInvWeight = m_pOthersInv->CalcTotalWeight();
    float otherMaxWeight = m_pOthersInv->GetMaxWeight();

    if (!m_pOthersInvOwner->trade_parameters().enabled(CTradeParameters::action_buy(0), pItem->object().cNameSect()))
        return (false);

    if (pItem->GetCondition() < m_pOthersInvOwner->trade_parameters().factors(CTradeParameters::action_buy(0), pItem->object().cNameSect()).min_condition())
        return false;

    if (otherInvWeight - r2 + r1 + itmWeight > otherMaxWeight)
        return false;

    return true;
}

void move_item(CUICellItem* itm, CUIDragDropListEx* from, CUIDragDropListEx* to)
{
    CUICellItem* _itm = from->RemoveItem(itm, false);
    to->SetItem(_itm);
}

bool CUITradeWnd::ToOurTrade(CUICellItem* itm)
{
    if (!CanMoveToOther((PIItem)m_pCurrentCellItem->m_pData, true))
        return false;

    move_item(itm, &m_uidata->UIOurBagList, &m_uidata->UIOurTradeList);
    UpdatePrices();
    return true;
}

bool CUITradeWnd::ToOthersTrade(CUICellItem* itm)
{
    if (!CanMoveToOther((PIItem)m_pCurrentCellItem->m_pData, false))
        return false;

    move_item(itm, &m_uidata->UIOthersBagList, &m_uidata->UIOthersTradeList);
    UpdatePrices();

    return true;
}

bool CUITradeWnd::ToOurBag(CUICellItem* itm)
{
    move_item(itm, &m_uidata->UIOurTradeList, &m_uidata->UIOurBagList);
    UpdatePrices();

    return true;
}

bool CUITradeWnd::ToOthersBag(CUICellItem* itm)
{
    move_item(itm, &m_uidata->UIOthersTradeList, &m_uidata->UIOthersBagList);
    UpdatePrices();

    return true;
}

float CUITradeWnd::CalcItemsWeight(CUIDragDropListEx* pList)
{
    float res = 0.0f;

    for (u32 i = 0; i < pList->ItemsCount(); ++i)
    {
        CUICellItem* itm = pList->GetItemIdx(i);
        PIItem iitem = (PIItem)itm->m_pData;
        res += iitem->Weight();
        for (u32 j = 0; j < itm->ChildsCount(); ++j)
        {
            PIItem jitem = (PIItem)itm->Child(j)->m_pData;
            res += jitem->Weight();
        }
    }
    return res;
}

u32 CUITradeWnd::CalcItemsPrice(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying)
{
    u32 iPrice = 0;

    for (u32 i = 0; i < pList->ItemsCount(); ++i)
    {
        CUICellItem* itm = pList->GetItemIdx(i);
        PIItem iitem = (PIItem)itm->m_pData;
        iPrice += pTrade->GetItemPrice(iitem, bBuying);

        for (u32 j = 0; j < itm->ChildsCount(); ++j)
        {
            PIItem jitem = (PIItem)itm->Child(j)->m_pData;
            iPrice += pTrade->GetItemPrice(jitem, bBuying);
        }
    }

    return iPrice;
}

void CUITradeWnd::PerformTrade()
{
    if (m_uidata->UIOurTradeList.ItemsCount() == 0 && m_uidata->UIOthersTradeList.ItemsCount() == 0)
        return;

    int our_money = (int)m_pInvOwner->get_money();
    int others_money = (int)m_pOthersInvOwner->get_money();

    int delta_price = int(m_iOurTradePrice - m_iOthersTradePrice);

    our_money += delta_price;
    others_money -= delta_price;

    if (our_money >= 0 && others_money >= 0 && (m_iOurTradePrice >= 0 || m_iOthersTradePrice > 0))
    {
        m_pOthersTrade->OnPerformTrade(m_iOthersTradePrice, m_iOurTradePrice);

        TransferItems(&m_uidata->UIOurTradeList, &m_uidata->UIOthersBagList, m_pOthersTrade, true);
        TransferItems(&m_uidata->UIOthersTradeList, &m_uidata->UIOurBagList, m_pOthersTrade, false);
        if (others_zero_trade)
        {
            m_pOthersTrade->pThis.inv_owner->set_money(others_money, true);
            m_pOthersTrade->pPartner.inv_owner->set_money(our_money, true);
        }
    }
    else
    {
        string256 deal_refuse_text; // строка с текстом сообщения-отказа при невозмжности совершить торговую сделку
        auto trader_name = others_money < 0 ? m_pOthersInvOwner->Name() : m_pInvOwner->Name();
        m_uidata->UIDealMsg = HUD().GetUI()->UIGame()->AddCustomStatic("not_enough_money", true);
        sprintf(deal_refuse_text, "%s: %s", trader_name, CStringTable().translate("st_not_enough_money_to_trade").c_str());
        m_uidata->UIDealMsg->wnd()->SetText(deal_refuse_text);

        m_uidata->UIDealMsg->m_endTime = Device.fTimeGlobal + 1.0f; // sec
    }
    SetCurrentItem(NULL);
}

#include "../xr_level_controller.h"

bool CUITradeWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
    if (m_b_need_update)
        return true;

    if (m_pUIPropertiesBox->GetVisible())
        m_pUIPropertiesBox->OnKeyboard(dik, keyboard_action);

    if (inherited::OnKeyboard(dik, keyboard_action))
        return true;

    return false;
}

bool CUITradeWnd::OnMouse(float x, float y, EUIMessages mouse_action)
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

    CUIWindow::OnMouse(x, y, mouse_action);

    return true; // always returns true, because ::StopAnyMove() == true;
}

void CUITradeWnd::UpdatePrices()
{
    m_iOurTradePrice = CalcItemsPrice(&m_uidata->UIOurTradeList, m_pOthersTrade, true);
    m_iOthersTradePrice = CalcItemsPrice(&m_uidata->UIOthersTradeList, m_pOthersTrade, false);

    if (!m_pOthersInvOwner->InfinitiveMoney())
    {
        u32 others_money = m_pOthersInvOwner->get_money();
        if (others_zero_trade && m_iOurTradePrice > others_money)
            m_iOurTradePrice = others_money;
    }

    string256 buf;
    sprintf_s(buf, "%d %s", m_iOurTradePrice, money_name);
    m_uidata->UIOurPriceCaption.GetPhraseByIndex(2)->str = buf;
    sprintf_s(buf, "%d %s", m_iOthersTradePrice, money_name);
    m_uidata->UIOthersPriceCaption.GetPhraseByIndex(2)->str = buf;
}

void CUITradeWnd::TransferItems(CUIDragDropListEx* pSellList, CUIDragDropListEx* pBuyList, CTrade* pTrade, bool bBuying)
{
    while (pSellList->ItemsCount())
    {
        CUICellItem* itm = pSellList->RemoveItem(pSellList->GetItemIdx(0), false);
        auto InvItm = (PIItem)itm->m_pData;
        if (!bBuying)
            InvItm->OnMoveOut(InvItm->m_eItemPlace);
        InvItm->m_highlight_equipped = false; // Убираем подсветку после продажи предмета
        itm->m_select_equipped = false;
        pTrade->TransferItem(InvItm, bBuying, !others_zero_trade);
        pBuyList->SetItem(itm);
    }

    if (!others_zero_trade)
    {
        pTrade->pThis.inv_owner->set_money(pTrade->pThis.inv_owner->get_money(), true);
        pTrade->pPartner.inv_owner->set_money(pTrade->pPartner.inv_owner->get_money(), true);
    }
}

void CUITradeWnd::UpdateLists(EListType mode)
{
    if (mode == eBoth || mode == e1st)
    {
        m_uidata->UIOurBagList.ClearAll(true);
        m_uidata->UIOurTradeList.ClearAll(true);
        m_pInvOwner->inventory().RepackAmmo();
    }

    if (mode == eBoth || mode == e2nd)
    {
        m_uidata->UIOthersBagList.ClearAll(true);
        m_uidata->UIOthersTradeList.ClearAll(true);
    }

    UpdatePrices();

    if (mode == eBoth || mode == e1st)
    {
        ruck_list.clear();
        m_pInv->AddAvailableItems(ruck_list, true);
        std::sort(ruck_list.begin(), ruck_list.end(), InventoryUtilities::GreaterRoomInRuck);
        FillList(ruck_list, m_uidata->UIOurBagList, true);
    }

    if (mode == eBoth || mode == e2nd)
    {
        ruck_list.clear();
        m_pOthersInv->AddAvailableItems(ruck_list, true);
        std::sort(ruck_list.begin(), ruck_list.end(), InventoryUtilities::GreaterRoomInRuck);
        FillList(ruck_list, m_uidata->UIOthersBagList, false);
    }

    m_b_need_update = false;
}

void CUITradeWnd::FillList(TIItemContainer& cont, CUIDragDropListEx& dragDropList, bool our)
{
    auto show_item = [&](const auto pIItem) -> bool { return !pIItem->m_flags.test(CInventoryItem::FIHiddenForInventory); };

    for (const auto& item : cont)
    {
        if (!show_item(item))
            continue;

        CUICellItem* itm = create_cell_item(item);
        if (item->m_highlight_equipped)
            itm->m_select_equipped = true;
        bool canTrade = CanMoveToOther(item, our);
        ColorizeItem(itm, canTrade, itm->m_select_equipped);
        dragDropList.SetItem(itm);
    }
}

bool CUITradeWnd::OnItemStartDrag(CUICellItem* itm)
{
    return false; // default behaviour
}

bool CUITradeWnd::OnItemSelected(CUICellItem* itm)
{
    SetCurrentItem(itm);
    itm->ColorizeItems({&m_uidata->UIOurTradeList, &m_uidata->UIOthersTradeList, &m_uidata->UIOurBagList, &m_uidata->UIOthersBagList});
    return false;
}

bool CUITradeWnd::OnItemRButtonClick(CUICellItem* itm)
{
    SetCurrentItem(itm);
    ActivatePropertiesBox();
    return false;
}

bool CUITradeWnd::OnItemDrop(CUICellItem* itm)
{
    CUIDragDropListEx* old_owner = itm->OwnerList();
    CUIDragDropListEx* new_owner = CUIDragDropListEx::m_drag_item->BackList();
    if (old_owner == new_owner || !old_owner || !new_owner)
        return false;

    bool b_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
    MoveItems(itm, b_all);

    return true;
}

bool CUITradeWnd::OnItemDbClick(CUICellItem* itm)
{
    SetCurrentItem(itm);
    bool b_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
    MoveItems(itm, b_all);
    return true;
}

void CUITradeWnd::MoveItems(CUICellItem* itm, bool b_all)
{
    if (!itm)
        return;

    auto old_owner = itm->OwnerList();

    auto MoveItemFromCell = [&](void* cellitm) {
        auto itm = static_cast<CUICellItem*>(cellitm);

        if (old_owner == &m_uidata->UIOurBagList)
            ToOurTrade(itm);
        else if (old_owner == &m_uidata->UIOurTradeList)
            ToOurBag(itm);
        else if (old_owner == &m_uidata->UIOthersBagList)
            ToOthersTrade(itm);
        else if (old_owner == &m_uidata->UIOthersTradeList)
            ToOthersBag(itm);
    };

    u32 cnt = itm->ChildsCount();
    for (u32 i = 0; i < cnt && b_all; ++i)
        MoveItemFromCell(itm);

    MoveItemFromCell(itm);

    PlaySnd(eInvMoveItem);

    UpdatePrices();

    SetCurrentItem(NULL);
}

CUICellItem* CUITradeWnd::CurrentItem() { return m_pCurrentCellItem; }

PIItem CUITradeWnd::CurrentIItem() { return (m_pCurrentCellItem) ? (PIItem)m_pCurrentCellItem->m_pData : NULL; }

void CUITradeWnd::SetCurrentItem(CUICellItem* itm)
{
    if (m_pCurrentCellItem == itm)
        return;
    m_pCurrentCellItem = itm;
    
    //m_uidata->UIItemInfo.InitItem(CurrentIItem());

    if (!m_pCurrentCellItem)
        return;

    m_pCurrentCellItem->m_select_armament = true;

    //CUIDragDropListEx* owner = itm->OwnerList();
    //bool bBuying = (owner == &m_uidata->UIOurBagList) || (owner == &m_uidata->UIOurTradeList);

    //if (m_uidata->UIItemInfo.UICost)
    //{
    //    string256 str;

    //    sprintf_s(str, "%d %s", m_pOthersTrade->GetItemPrice(CurrentIItem(), bBuying), money_name);
    //    m_uidata->UIItemInfo.UICost->SetText(str);
    //}

    auto script_obj = CurrentIItem()->object().lua_game_object();
    g_actor->callback(GameObject::eCellItemSelect)(script_obj);
}

void CUITradeWnd::SwitchToTalk() { GetMessageTarget()->SendMessage(this, TRADE_WND_CLOSED); }

void CUITradeWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
    lst->m_f_item_drop = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemDrop);
    lst->m_f_item_start_drag = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemStartDrag);
    lst->m_f_item_db_click = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemDbClick);
    lst->m_f_item_selected = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemSelected);
    lst->m_f_item_rbutton_click = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemRButtonClick);
}

void CUITradeWnd::ColorizeItem(CUICellItem* itm, bool canTrade, bool highlighted)
{
    static const bool highlight_cop_enabled = !Core.Features.test(xrCore::Feature::colorize_untradable); // Это опция для Dsh, не убирать!

    if (!canTrade)
    {
        if (highlight_cop_enabled)
            itm->m_select_untradable = true;
        itm->SetColor(reinterpret_cast<CInventoryItem*>(itm->m_pData)->ClrUntradable);
    }
    else
    {
        if (highlight_cop_enabled)
            itm->m_select_untradable = false;
        if (highlighted)
            itm->SetColor(reinterpret_cast<CInventoryItem*>(itm->m_pData)->ClrEquipped);
    }
}

void CUITradeWnd::PlaySnd(eInventorySndAction a)
{
    if (sounds[a]._handle())
        sounds[a].play(NULL, sm_2D);
}

void CUITradeWnd::EatItem()
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!pActor)
        return;
    m_pInv->Eat(CurrentIItem(), m_pInvOwner);
    SetCurrentItem(nullptr);
}

void CUITradeWnd::DropItems(bool b_all)
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

    CUIDragDropListEx* owner_list = ci->OwnerList();

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

    owner_list->RemoveItem(ci, b_all);

    SetCurrentItem(nullptr);
    PlaySnd(eInvDropItem);
}

void CUITradeWnd::DetachAddon(const char* addon_name, bool for_all)
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