#include "stdafx.h"
#include "inventory.h"
#include "actor.h"
#include "trade.h"
#include "weapon.h"

#include "ui/UIInventoryUtilities.h"

#include "eatable_item.h"
#include "script_engine.h"
#include "xrmessages.h"
// #include "game_cl_base.h"
#include "xr_level_controller.h"
#include "level.h"
#include "ai_space.h"
#include "entitycondition.h"
#include "game_base_space.h"
#include "clsid_game.h"
#include "CustomOutfit.h"
#include "Vest.h"
#include "Warbelt.h"
#include "HudItem.h"
#include "Grenade.h"

#include "UIGameSP.h"
#include "HudManager.h"
#include "ui/UIInventoryWnd.h"

using namespace InventoryUtilities;

// what to block
u32 INV_STATE_BLOCK_ALL = 0xffffffff;
u32 INV_STATE_BLOCK_2H = 1 << FIRST_WEAPON_SLOT | 1 << SECOND_WEAPON_SLOT | 1 << ARTEFACT_SLOT; // вважаємо що саме у цих слотах будуть "дворучні" предмети та блокуємо їх
u32 INV_STATE_INV_WND = INV_STATE_BLOCK_2H;
u32 INV_STATE_BUY_MENU = INV_STATE_BLOCK_ALL;
u32 INV_STATE_LADDER = INV_STATE_BLOCK_2H;
u32 INV_STATE_CAR = INV_STATE_BLOCK_2H;
u32 INV_STATE_PDA = INV_STATE_BLOCK_ALL;

bool CInventorySlot::CanBeActivated() const { return (m_bVisible && !IsBlocked()); };

bool CInventorySlot::IsBlocked() const { return (m_blockCounter > 0); }

bool CInventorySlot::maySwitchFast() const { return m_maySwitchFast; }

void CInventorySlot::setSwitchFast(bool value) { m_maySwitchFast = value; }

CInventory::CInventory()
{
    m_fTakeDist = pSettings->r_float("inventory", "take_dist");
    m_fMaxWeight = pSettings->r_float("inventory", "max_weight");

    m_slots.resize(SLOTS_TOTAL);

    string256 temp;
    for (u32 i = 0; i < m_slots.size(); ++i)
    {
        sprintf_s(temp, "slot_persistent_%d", i + 1);
        if (pSettings->line_exist("inventory", temp))
            m_slots[i].m_bPersistent = !!pSettings->r_bool("inventory", temp);
        sprintf_s(temp, "slot_switch_fast_%d", i + 1);
        m_slots[i].setSwitchFast(READ_IF_EXISTS(pSettings, r_bool, "inventory", temp, false));
    }

    m_slots[PDA_SLOT].m_bVisible = false;
    m_slots[OUTFIT_SLOT].m_bVisible = false;
    m_slots[TORCH_SLOT].m_bVisible = false;
    m_slots[HELMET_SLOT].m_bVisible = false;
    m_slots[DETECTOR_SLOT].m_bVisible = false; // KRodin: это очень важно! Слот для зп-стайл детекторов должен быть НЕ активируемым!

    for (u32 i = 0; i < m_slots.size(); ++i)
    {
        sprintf_s(temp, "slot_visible_%d", i + 1);
        if (pSettings->line_exist("inventory", temp))
            m_slots[i].m_bVisible = !!pSettings->r_bool("inventory", temp);
    }
}

void CInventory::Clear()
{
    m_all.clear();
    m_ruck.clear();
    m_belt.clear();
    m_vest.clear();

    for (auto& slot : m_slots)
        slot.m_pIItem = nullptr;

    m_pOwner = nullptr;

    CalcTotalWeight();
    InvalidateState();
}

void CInventory::Take(CGameObject* pObj, bool bNotActivate, bool strict_placement)
{
    CInventoryItem* pIItem = smart_cast<CInventoryItem*>(pObj);
    VERIFY(pIItem);

    if (pIItem->m_pCurrentInventory)
    {
        Msg("! ERROR CInventory::Take but object has m_pCurrentInventory");
        Msg("! Inventory Owner is [%d]", GetOwner()->object_id());
        Msg("! Object Inventory Owner is [%d]", pIItem->m_pCurrentInventory->GetOwner()->object_id());

        CObject* p = pObj->H_Parent();
        if (p)
            Msg("! object parent is [%s] [%d]", p->cName().c_str(), p->ID());
    }

    R_ASSERT(CanTakeItem(pIItem));

    pIItem->m_pCurrentInventory = this;
    pIItem->SetDropManual(FALSE);

    m_all.push_back(pIItem);

    if (!strict_placement)
        pIItem->m_eItemPlace = eItemPlaceUndefined;

    TryAmmoCustomPlacement(pIItem);

    bool result = false;
    switch (pIItem->m_eItemPlace)
    {
    case eItemPlaceBelt:
        result = Belt(pIItem);
        if (!result)
        {
            Msg("!![%s] cant put in belt item [%s], moving to ruck...", __FUNCTION__, pIItem->object().cName().c_str());
            pIItem->m_eItemPlace = eItemPlaceRuck;
            R_ASSERT(Ruck(pIItem));
        }
        break;
    case eItemPlaceVest:
        result = Vest(pIItem);
        if (!result)
        {
            Msg("!![%s] cant put in vest item [%s], moving to ruck...", __FUNCTION__, pIItem->object().cName().c_str());
            pIItem->m_eItemPlace = eItemPlaceRuck;
            R_ASSERT(Ruck(pIItem));
        }
        break;
    case eItemPlaceRuck: result = Ruck(pIItem);
#ifdef DEBUG
        if (!result)
            Msg("cant put in ruck item %s", *pIItem->object().cName());
#endif

        break;
    case eItemPlaceSlot:
        if (smart_cast<CActor*>(m_pOwner) && Device.dwPrecacheFrame && m_iActiveSlot == NO_ACTIVE_SLOT && m_iNextActiveSlot == NO_ACTIVE_SLOT)
            bNotActivate = true;
        result = Slot(pIItem, bNotActivate);
#ifdef DEBUG
        if (!result)
            Msg("cant put in slot item %s", *pIItem->object().cName());
#endif

        break;
    default:
        bool force_move_to_slot{}, force_ruck_default{};
        if (!m_pOwner->m_tmp_next_item_slot)
        {
            force_ruck_default = true;
            m_pOwner->m_tmp_next_item_slot = NO_ACTIVE_SLOT;
        }
        else if (m_pOwner->m_tmp_next_item_slot != NO_ACTIVE_SLOT)
        {
            pIItem->SetSlot(m_pOwner->m_tmp_next_item_slot);
            force_move_to_slot = true;
            m_pOwner->m_tmp_next_item_slot = NO_ACTIVE_SLOT;
        }

        auto pActor = smart_cast<CActor*>(m_pOwner);
        const bool def_to_slot = pActor ? !pIItem->RuckDefault() : true;
        if ((!force_ruck_default && def_to_slot && CanPutInSlot(pIItem)) || force_move_to_slot)
        {
            if (pActor && Device.dwPrecacheFrame)
                bNotActivate = true;
            result = Slot(pIItem, bNotActivate);
            VERIFY(result);
        }
        else if (!force_ruck_default && !pIItem->RuckDefault() && CanPutInBelt(pIItem))
        {
            result = Belt(pIItem);
            VERIFY(result);
        }
        else if (!force_ruck_default && !pIItem->RuckDefault() && CanPutInVest(pIItem))
        {
            result = Vest(pIItem);
            VERIFY(result);
        }
        else
        {
            result = Ruck(pIItem);
            VERIFY(result);
        }
    }

    m_pOwner->OnItemTake(pIItem);

    CalcTotalWeight();
    InvalidateState();

    pIItem->object().processing_deactivate();
    VERIFY(pIItem->m_eItemPlace != eItemPlaceUndefined);
}

bool CInventory::DropItem(CGameObject* pObj)
{
    CInventoryItem* pIItem = smart_cast<CInventoryItem*>(pObj);
    VERIFY(pIItem);
    if (!pIItem)
        return false;

    ASSERT_FMT(pIItem->m_pCurrentInventory, "CInventory::DropItem: [%s]: pIItem->m_pCurrentInventory", pObj->cName().c_str());
    ASSERT_FMT(pIItem->m_pCurrentInventory == this, "CInventory::DropItem: [%s]: this = %s, pIItem->m_pCurrentInventory = %s", pObj->cName().c_str(),
               smart_cast<const CGameObject*>(this)->cName().c_str(), smart_cast<const CGameObject*>(pIItem->m_pCurrentInventory)->cName().c_str());
    VERIFY(pIItem->m_eItemPlace != eItemPlaceUndefined);

    pIItem->object().processing_activate();

    switch (pIItem->m_eItemPlace)
    {
    case eItemPlaceBelt: {
        ASSERT_FMT(InBelt(pIItem), "CInventory::DropItem: InBelt(pIItem): %s, owner [%s]", pObj->cName().c_str(), m_pOwner->Name());
        m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));
        pIItem->object().processing_deactivate();
    }
    break;
    case eItemPlaceVest: {
        ASSERT_FMT(InVest(pIItem), "CInventory::DropItem: InVest(pIItem): %s, owner [%s]", pObj->cName().c_str(), m_pOwner->Name());
        m_vest.erase(std::find(m_vest.begin(), m_vest.end(), pIItem));
        pIItem->object().processing_deactivate();
    }
    break;
    case eItemPlaceRuck: {
        ASSERT_FMT(InRuck(pIItem), "CInventory::DropItem: InRuck(pIItem): %s, owner [%s]", pObj->cName().c_str(), m_pOwner->Name());
        m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
    }
    break;
    case eItemPlaceSlot: {
        ASSERT_FMT(InSlot(pIItem), "CInventory::DropItem: InSlot(pIItem): [%s], id: [%u], owner [%s]", pObj->cName().c_str(), pObj->ID(), m_pOwner->Name());
        if (m_iActiveSlot == pIItem->GetSlot())
            Activate(NO_ACTIVE_SLOT);

        m_slots[pIItem->GetSlot()].m_pIItem = nullptr;

        pIItem->object().processing_deactivate();
    }
    break;
    default: NODEFAULT;
    };

    TIItemContainer::iterator it = std::find(m_all.begin(), m_all.end(), pIItem);
    if (it != m_all.end())
        m_all.erase(it);
    else
        Msg("! CInventory::Drop item not found in inventory!!!");

    pIItem->OnMoveOut(pIItem->m_eItemPlace);

    pIItem->m_pCurrentInventory = nullptr;

    m_pOwner->OnItemDrop(smart_cast<CInventoryItem*>(pObj));

    CalcTotalWeight();
    InvalidateState();
    m_drop_last_frame = true;
    return true;
}

// положить вещь в слот
bool CInventory::Slot(PIItem pIItem, bool bNotActivate)
{
    VERIFY(pIItem);

    if (!CanPutInSlot(pIItem))
    {
        if (m_slots[pIItem->GetSlot()].m_pIItem == pIItem && !bNotActivate)
            if (activate_slot(pIItem->GetSlot()))
                Activate(pIItem->GetSlot());

        return false;
    }

    /*
    Вещь была в слоте. Да, такое может быть :).
    Тут необходимо проверять именно так, потому что
    GetSlot вернет новый слот, а не старый. Real Wolf.
    */
    for (u32 i = 0; i < m_slots.size(); i++)
        if (m_slots[i].m_pIItem == pIItem)
        {
            if (i == m_iActiveSlot)
                Activate(NO_ACTIVE_SLOT);
            m_slots[i].m_pIItem = NULL;
            break;
        }

    m_slots[pIItem->GetSlot()].m_pIItem = pIItem;

    // удалить из рюкзака или пояса
    if (InRuck(pIItem))
        m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
    if (InBelt(pIItem))
        m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));
    if (InVest(pIItem))
        m_vest.erase(std::find(m_vest.begin(), m_vest.end(), pIItem));

    if ((m_iActiveSlot == pIItem->GetSlot() || (m_iActiveSlot == NO_ACTIVE_SLOT && m_iNextActiveSlot == NO_ACTIVE_SLOT)) && !bNotActivate)
        if (activate_slot(pIItem->GetSlot()))
            Activate(pIItem->GetSlot());

    auto PrevPlace = pIItem->m_eItemPlace;
    pIItem->m_eItemPlace = eItemPlaceSlot;
    m_pOwner->OnItemSlot(pIItem, PrevPlace);
    pIItem->OnMoveToSlot(PrevPlace);

    pIItem->object().processing_activate();

    return true;
}

bool CInventory::Belt(PIItem pIItem)
{
    if (!CanPutInBelt(pIItem))
        return false;

    // вещь была в слоте
    bool in_slot = InSlot(pIItem);
    if (in_slot)
    {
        if (m_iActiveSlot == pIItem->GetSlot())
            Activate(NO_ACTIVE_SLOT);
        m_slots[pIItem->GetSlot()].m_pIItem = NULL;
    }
    if (InRuck(pIItem))
        m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
    if (InVest(pIItem))
        m_vest.erase(std::find(m_vest.begin(), m_vest.end(), pIItem));

    m_belt.push_back(pIItem);

    CalcTotalWeight();
    InvalidateState();

    auto PrevPlace = pIItem->m_eItemPlace;
    pIItem->m_eItemPlace = eItemPlaceBelt;
    m_pOwner->OnItemBelt(pIItem, PrevPlace);
    pIItem->OnMoveToBelt(PrevPlace);

    if (in_slot)
        pIItem->object().processing_deactivate();

    pIItem->object().processing_activate();

    return true;
}

bool CInventory::Vest(PIItem pIItem)
{
    if (!CanPutInVest(pIItem))
        return false;

    // вещь была в слоте
    bool in_slot = InSlot(pIItem);
    if (in_slot)
    {
        if (m_iActiveSlot == pIItem->GetSlot())
            Activate(NO_ACTIVE_SLOT);
        m_slots[pIItem->GetSlot()].m_pIItem = NULL;
    }

    if (InRuck(pIItem))
        m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
    if (InBelt(pIItem))
        m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));

    m_vest.push_back(pIItem);

    CalcTotalWeight();
    InvalidateState();

    auto PrevPlace = pIItem->m_eItemPlace;
    pIItem->m_eItemPlace = eItemPlaceVest;
    m_pOwner->OnItemVest(pIItem, PrevPlace);
    pIItem->OnMoveToVest(PrevPlace);

    if (in_slot)
        pIItem->object().processing_deactivate();

    pIItem->object().processing_activate();

    return true;
}

bool CInventory::Ruck(PIItem pIItem)
{
    if (!CanPutInRuck(pIItem))
        return false;

    bool in_slot = InSlot(pIItem);
    // вещь была в слоте
    if (in_slot)
    {
        if (m_iActiveSlot == pIItem->GetSlot())
            Activate(NO_ACTIVE_SLOT);
        m_slots[pIItem->GetSlot()].m_pIItem = NULL;
    }
    if (InBelt(pIItem))
        m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));
    if (InVest(pIItem))
        m_vest.erase(std::find(m_vest.begin(), m_vest.end(), pIItem));

    m_ruck.push_back(pIItem);

    CalcTotalWeight();
    InvalidateState();

    EItemPlace prevPlace = pIItem->m_eItemPlace;
    m_pOwner->OnItemRuck(pIItem, prevPlace);
    pIItem->m_eItemPlace = eItemPlaceRuck;

    pIItem->OnMoveToRuck(prevPlace);

    if (in_slot)
        pIItem->object().processing_deactivate();

    return true;
}

void CInventory::Activate_deffered(u32 slot, u32 _frame)
{
    m_iLoadActiveSlot = slot;
    m_iLoadActiveSlotFrame = _frame;
}

bool CInventory::Activate(u32 slot, EActivationReason reason, bool bForce, bool now)
{
    if (m_ActivationSlotReason == eKeyAction && reason == eImportUpdate)
        return false;

    bool res = false;

    if (Device.dwFrame == m_iLoadActiveSlotFrame)
    {
        if ((m_iLoadActiveSlot == slot) && m_slots[slot].m_pIItem)
            m_iLoadActiveSlotFrame = u32(-1);
        else
        {
            res = false;
            goto _finish;
        }
    }

    if ((slot != NO_ACTIVE_SLOT && m_slots[slot].IsBlocked()) && !bForce)
    {
        res = false;
        goto _finish;
    }

    ASSERT_FMT(slot == NO_ACTIVE_SLOT || slot < m_slots.size(), "wrong slot number: [%u]", slot);

    if (slot != NO_ACTIVE_SLOT && !m_slots[slot].m_bVisible)
    {
        res = false;
        goto _finish;
    }

    if (m_iActiveSlot == slot && m_iActiveSlot != NO_ACTIVE_SLOT && m_slots[m_iActiveSlot].m_pIItem)
    {
        m_slots[m_iActiveSlot].m_pIItem->Activate();
    }

    if (m_iActiveSlot == slot ||
        (m_iNextActiveSlot == slot && m_iActiveSlot != NO_ACTIVE_SLOT && m_slots[m_iActiveSlot].m_pIItem && m_slots[m_iActiveSlot].m_pIItem->cast_hud_item() &&
         m_slots[m_iActiveSlot].m_pIItem->cast_hud_item()->IsHiding()))
    {
        res = false;
        goto _finish;
    }

    // активный слот не выбран
    if (m_iActiveSlot == NO_ACTIVE_SLOT)
    {
        if (m_slots[slot].m_pIItem)
        {
            m_iNextActiveSlot = slot;
            m_ActivationSlotReason = reason;
            res = true;
            goto _finish;
        }
        else
        {
            res = false;
            goto _finish;
        }
    }
    // активный слот задействован
    else if (slot == NO_ACTIVE_SLOT || m_slots[slot].m_pIItem)
    {
        if (m_slots[m_iActiveSlot].m_pIItem)
        {
            m_slots[m_iActiveSlot].m_pIItem->Deactivate(now || (slot != NO_ACTIVE_SLOT && m_slots[slot].maySwitchFast()));
        }

        m_iNextActiveSlot = slot;
        m_ActivationSlotReason = reason;

        res = true;
        goto _finish;
    }

_finish:

    if (res)
        m_ActivationSlotReason = reason;
    return res;
}

PIItem CInventory::ItemFromSlot(u32 slot) const
{
    VERIFY(NO_ACTIVE_SLOT != slot);
    return m_slots[slot].m_pIItem;
}

bool CInventory::Action(s32 cmd, u32 flags)
{
    CActor* pActor = smart_cast<CActor*>(m_pOwner);

    if (pActor)
    {
        switch (cmd)
        {
        case kWPN_FIRE: {
            pActor->SetShotRndSeed();
        }
        break;
        case kWPN_ZOOM: {
            pActor->SetZoomRndSeed();
        }
        break;
        };
    };

    if (m_iActiveSlot < m_slots.size() && m_slots[m_iActiveSlot].m_pIItem && m_slots[m_iActiveSlot].m_pIItem->Action(cmd, flags))
        return true;
    bool b_send_event = false;
    switch (cmd)
    {
    case kWPN_1:
    case kWPN_2:
    case kWPN_3:
    case kWPN_4:
    case kWPN_5:
    case kWPN_6: {
        if (flags & CMD_START)
        {
            if ((int)m_iActiveSlot == cmd - kWPN_1 && m_slots[m_iActiveSlot].m_pIItem)
                b_send_event = Activate(NO_ACTIVE_SLOT);
            else
                b_send_event = Activate(cmd - kWPN_1, eKeyAction);
        }
    }
    break;
    case kWPN_7: {
        if (flags & CMD_START)
        {
            if ((int)m_iActiveSlot == ARTEFACT_SLOT && m_slots[m_iActiveSlot].m_pIItem)
            {
                b_send_event = Activate(NO_ACTIVE_SLOT);
            }
            else
            {
                b_send_event = Activate(ARTEFACT_SLOT, eKeyAction);
            }
        }
    }
    break;
    case kUSE_QUICK_SLOT_0:
    case kUSE_QUICK_SLOT_1:
    case kUSE_QUICK_SLOT_2:
    case kUSE_QUICK_SLOT_3: {
        PIItem itm = NULL;

        switch (cmd)
        {
        case kUSE_QUICK_SLOT_0: itm = m_slots[QUICK_SLOT_0].m_pIItem; break;
        case kUSE_QUICK_SLOT_1: itm = m_slots[QUICK_SLOT_1].m_pIItem; break;
        case kUSE_QUICK_SLOT_2: itm = m_slots[QUICK_SLOT_2].m_pIItem; break;
        case kUSE_QUICK_SLOT_3: itm = m_slots[QUICK_SLOT_3].m_pIItem; break;
        }

        if (flags & CMD_START)
        {
            auto pHudItem = smart_cast<CHudItem*>(itm);
            // если в слоте предмет без худа то и активировать его не нужно
            if (!pHudItem)
                return false;

            if (m_iActiveSlot == itm->GetSlot())
                b_send_event = Activate(NO_ACTIVE_SLOT);
            else
                b_send_event = Activate(itm->GetSlot(), eKeyAction);
        }
    }
    break;
    }

    return false;
}

void CInventory::Update()
{
    // Да, KRodin писал это в здравом уме и понимает, что это полная хуйня. Но ни одного нормального решения придумать не удалось. Может потом какие-то мысли появятся.
    // А проблема вся в том, что арты и костюм выходят в онлайн в хаотичном порядке. И получается, что арты на пояс уже пытаются залезть, а костюма вроде как ещё нет,
    // соотв. и слотов под арты как бы нет. Вот поэтому до первого апдейта CInventory актора считаем, что все слоты та пояс доступны.
    // По моим наблюдениям на момент первого апдейта CInventory, все предметы в инвентаре актора уже вышли в онлайн.
    m_bUpdated = true;

    bool bActiveSlotVisible;

    if (m_iActiveSlot == NO_ACTIVE_SLOT || !m_slots[m_iActiveSlot].m_pIItem || !m_slots[m_iActiveSlot].m_pIItem->cast_hud_item() ||
        m_slots[m_iActiveSlot].m_pIItem->cast_hud_item()->IsHidden())
    {
        bActiveSlotVisible = false;
    }
    else
    {
        bActiveSlotVisible = true;
    }

    if (m_iNextActiveSlot != m_iActiveSlot && !bActiveSlotVisible)
    {
        if (m_iNextActiveSlot != NO_ACTIVE_SLOT && m_slots[m_iNextActiveSlot].m_pIItem)
            m_slots[m_iNextActiveSlot].m_pIItem->Activate();

        m_iActiveSlot = m_iNextActiveSlot;
    }
    UpdateDropTasks();
}

void CInventory::UpdateDropTasks()
{
    for (const auto& item : m_all)
        UpdateDropItem(item);

    if (m_drop_last_frame)
    {
        m_drop_last_frame = false;
        m_pOwner->OnItemDropUpdate();
    }
}

void CInventory::UpdateDropItem(PIItem pIItem)
{
    if (pIItem->GetDropManual())
    {
        pIItem->SetDropManual(FALSE);
        NET_Packet P;
        pIItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
        P.w_u16(u16(pIItem->object().ID()));
        pIItem->object().u_EventSend(P);
    } // dropManual
}

// ищем на поясе гранату такоже типа
PIItem CInventory::Same(const PIItem pIItem, bool bSearchRuck) const
{
    if (bSearchRuck)
    {
        for (const auto& item : m_ruck)
        {
            if ((item != pIItem) && !xr_strcmp(item->object().cNameSect(), pIItem->object().cNameSect()))
                return item;
        }
    }
    else
    {
        for (const auto& item : m_vest)
        {
            if ((item != pIItem) && !xr_strcmp(item->object().cNameSect(), pIItem->object().cNameSect()))
                return item;
        }
        for (const auto& item : m_belt)
        {
            if ((item != pIItem) && !xr_strcmp(item->object().cNameSect(), pIItem->object().cNameSect()))
                return item;
        }
        for (const auto& _slot : m_slots)
        {
            const auto _item = _slot.m_pIItem;
            if (!_item)
                continue;

            if ((_item != pIItem) && !xr_strcmp(_item->object().cNameSect(), pIItem->object().cNameSect()))
            {
                return _item;
            }
        }
    }
    return nullptr;
}

// ищем на поясе гранату для слота
PIItem CInventory::SameSlot(const u32 slot, PIItem pIItem, bool bSearchRuck) const
{
    if (slot == NO_ACTIVE_SLOT)
        return nullptr;

    if (bSearchRuck)
    {
        for (const auto& _item : m_ruck)
        {
            if (_item != pIItem && _item->GetSlot() == slot)
            {
                return _item;
            }
        }
    }
    else
    {
        for (const auto& _item : m_vest)
        {
            if (_item != pIItem && _item->GetSlot() == slot)
            {
                return _item;
            }
        }
        for (const auto& _item : m_belt)
        {
            if (_item != pIItem && _item->GetSlot() == slot)
            {
                return _item;
            }
        }
        for (const auto& _slot : m_slots)
        {
            const auto _item = _slot.m_pIItem;
            if (!_item)
                continue;

            if (_item != pIItem && _item->GetSlot() == slot)
            {
                return _item;
            }
        }
    }
    return nullptr;
}

// ищем на поясе гранату для слота
PIItem CInventory::SameGrenade(PIItem pIItem, bool bSearchRuck) const
{
    if (bSearchRuck)
    {
        for (const auto& _item : m_ruck)
        {
            if (_item != pIItem && smart_cast<CGrenade*>(_item))
            {
                return _item;
            }
        }
    }
    else
    {
        for (const auto& _item : m_vest)
        {
            if (_item != pIItem && smart_cast<CGrenade*>(_item))
            {
                return _item;
            }
        }
        for (const auto& _item : m_belt)
        {
            if (_item != pIItem && smart_cast<CGrenade*>(_item))
            {
                return _item;
            }
        }
        for (const auto& _slot : m_slots)
        {
            const auto _item = _slot.m_pIItem;
            if (!_item)
                continue;

            if (_item != pIItem && smart_cast<CGrenade*>(_item))
            {
                return _item;
            }
        }
    }
    return nullptr;
}

// найти в инвенторе вещь с указанным именем
PIItem CInventory::Get(const char* name, bool bSearchRuck) const
{
    const TIItemContainer& list = bSearchRuck ? m_ruck : m_belt;

    for (const auto& item : list)
    {
        if (item && !xr_strcmp(item->object().cNameSect(), name) && item->Useful())
            return item;
    }

    return NULL;
}

PIItem CInventory::Get(CLASS_ID cls_id, bool bSearchRuck) const
{
    const TIItemContainer& list = bSearchRuck ? m_ruck : m_belt;

    for (const auto& item : list)
    {
        if (item && item->object().CLS_ID == cls_id && item->Useful())
            return item;
    }
    return NULL;
}

PIItem CInventory::Get(const u16 id, bool bSearchRuck) const
{
    const TIItemContainer& list = bSearchRuck ? m_ruck : m_belt;

    for (const auto& item : list)
    {
        if (item && item->object().ID() == id)
            return item;
    }
    return NULL;
}

// search both (ruck and belt)
PIItem CInventory::GetAny(const char* name) const
{
    PIItem itm = Get(name, false);
    if (!itm)
        itm = Get(name, true);
    return itm;
}

PIItem CInventory::item(CLASS_ID cls_id) const
{
    for (const auto& item : m_all)
    {
        if (item && item->object().CLS_ID == cls_id && item->Useful())
            return item;
    }
    return NULL;
}

float CInventory::TotalWeight() const
{
    VERIFY(m_fTotalWeight >= 0.f);
    return m_fTotalWeight;
}

float CInventory::CalcTotalWeight()
{
    float weight{};
    for (const auto& item : m_all)
    {
        weight += item->Weight();
    }

    m_fTotalWeight = weight;
    return m_fTotalWeight;
}

bool CInventory::bfCheckForObject(ALife::_OBJECT_ID tObjectID)
{
    for (const auto& item : m_all)
    {
        if (item && item->object().ID() == tObjectID)
            return (true);
    }
    return (false);
}

CInventoryItem* CInventory::get_object_by_id(ALife::_OBJECT_ID tObjectID)
{
    for (const auto& item : m_all)
    {
        if (item && item->object().ID() == tObjectID)
            return (item);
    }
    return (0);
}

// скушать предмет
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
bool CInventory::Eat(PIItem pIItem, CInventoryOwner* eater)
{
    // устанаовить съедобна ли вещь
    CEatableItem* pItemToEat = smart_cast<CEatableItem*>(pIItem);
    R_ASSERT(pItemToEat);

    if (!eater)
        eater = m_pOwner;

    CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(eater);
    R_ASSERT(entity_alive);

    if (Actor()->m_inventory == this)
        Actor()->callback(GameObject::eOnBeforeUseItem)((smart_cast<CGameObject*>(pIItem))->lua_game_object());

    pItemToEat->UseBy(entity_alive);

    if (Actor()->m_inventory == this)
        Actor()->callback(GameObject::eUseObject)((smart_cast<CGameObject*>(pIItem))->lua_game_object());

    if (pItemToEat->Empty() && entity_alive->Local())
    {
        auto object = pIItem->cast_game_object();

        NET_Packet P;
        CGameObject::u_EventGen(P, GE_OWNERSHIP_REJECT, object->H_Parent()->ID());
        P.w_u16(pIItem->object().ID());
        CGameObject::u_EventSend(P);

        CGameObject::u_EventGen(P, GE_DESTROY, object->ID());
        CGameObject::u_EventSend(P);

        return false;
    }
    return true;
}

bool CInventory::InSlot(PIItem pIItem) const
{
    if (pIItem->GetSlot() < m_slots.size() && m_slots[pIItem->GetSlot()].m_pIItem == pIItem)
        return true;
    return false;
}
bool CInventory::InBelt(PIItem pIItem) const
{
    if (Get(pIItem->object().ID(), false))
        return true;
    return false;
}
bool CInventory::InVest(PIItem pIItem) const { return (std::find(m_vest.begin(), m_vest.end(), pIItem) != m_vest.end()); }
bool CInventory::InRuck(PIItem pIItem) const
{
    if (Get(pIItem->object().ID(), true))
        return true;
    return false;
}

bool CInventory::CanPutInSlot(PIItem pIItem) const
{
    if (!m_bSlotsUseful)
        return false;

    if (!GetOwner()->CanPutInSlot(pIItem, pIItem->GetSlot()))
        return false;

    if (!IsSlotAllowed(pIItem->GetSlot()))
        return false;

    if (pIItem->GetSlot() < m_slots.size() && !m_slots[pIItem->GetSlot()].m_pIItem && IsSlotAllowed(pIItem->GetSlot()))
        return true;

    return false;
}

// KRodin: добавлено специально для равнозначных слотов.
bool CInventory::CanPutInSlot(PIItem pIItem, u8 slot) const
{
    if (!m_bSlotsUseful)
        return false;

    if (!GetOwner()->CanPutInSlot(pIItem, slot))
        return false;

    if (!IsSlotAllowed(slot))
        return false;

    if (slot < m_slots.size() && !m_slots[slot].m_pIItem)
        return true;

    return false;
}

// проверяет можем ли поместить вещь на пояс,
// при этом реально ничего не меняется
bool CInventory::CanPutInBelt(PIItem pIItem) const
{
    if (InBelt(pIItem))
        return false;
    if (!m_bBeltUseful)
        return false;
    if (!pIItem || !pIItem->Belt())
        return false;
    if (!IsAllItemsLoaded())
        return true;
    auto pActor = smart_cast<CActor*>(m_pOwner);
    if (pActor && !pActor->GetWarbelt())
        return false;
    if (!InVest(pIItem) && HasSameModuleEquiped(pIItem))
        return false;
    auto belt = static_cast<TIItemContainer>(m_belt);

    return FreeRoom_inBelt(belt, pIItem, BeltWidth(), BeltHeight());
}

bool CInventory::CanPutInVest(PIItem pIItem) const
{
    if (InVest(pIItem))
        return false;
    if (!m_bVestUseful)
        return false;
    if (!pIItem || !pIItem->Vest())
        return false;
    if (!IsAllItemsLoaded())
        return true;
    auto pActor = smart_cast<CActor*>(m_pOwner);
    if (pActor && !pActor->GetVest())
        return false;
    if (!InBelt(pIItem) && HasSameModuleEquiped(pIItem))
        return false;
    auto vest = static_cast<TIItemContainer>(m_vest);

    return FreeRoom_inVest(vest, pIItem, VestWidth(), VestHeight());
}
// проверяет можем ли поместить вещь в рюкзак,
// при этом реально ничего не меняется
bool CInventory::CanPutInRuck(PIItem pIItem) const
{
    if (InRuck(pIItem))
        return false;
    return true;
}

u32 CInventory::dwfGetObjectCount() { return (m_all.size()); }

CInventoryItem* CInventory::tpfGetObjectByIndex(int iIndex)
{
    if ((iIndex >= 0) && (iIndex < (int)m_all.size()))
    {
        TIItemContainer& l_list = m_all;
        int i = 0;
        for (TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it, ++i)
            if (i == iIndex)
                return (*l_it);
    }
    else
    {
        ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "invalid inventory index!");
        return (0);
    }
    R_ASSERT(false);
    return (0);
}

CInventoryItem* CInventory::GetItemFromInventory(LPCSTR SectName)
{
    auto It = std::find_if(m_all.begin(), m_all.end(), [SectName](const auto* pInvItm) { return pInvItm->object().cNameSect() == SectName; });
    if (It != m_all.end())
        return *It;

    return nullptr;
}

bool CInventory::CanTakeItem(CInventoryItem* inventory_item) const
{
    if (inventory_item->object().getDestroy())
        return false;

    if (!inventory_item->CanTake())
        return false;

    for (TIItemContainer::const_iterator it = m_all.begin(); it != m_all.end(); it++)
        if ((*it)->object().ID() == inventory_item->object().ID())
            break;
    VERIFY3(it == m_all.end(), "item already exists in inventory", *inventory_item->object().cName());

    // перевантаження не враховується для актора
    if (!OwnerIsActor() && (TotalWeight() + inventory_item->Weight() > m_pOwner->MaxCarryWeight()))
        return false;

    return true;
}

void CInventory::AddAvailableItems(TIItemContainer& items_container, bool for_trade) const
{
    for (const auto& item : m_ruck)
    {
        if (!for_trade || item->CanTrade())
            items_container.push_back(item);
    }

    if (m_bBeltUseful)
    {
        for (const auto& item : m_belt)
        {
            if (!for_trade || item->CanTrade())
                items_container.push_back(item);
        }
    }

    if (m_bVestUseful)
    {
        for (const auto& item : m_vest)
        {
            if (!for_trade || item->CanTrade())
                items_container.push_back(item);
        }
    }

    if (m_bSlotsUseful)
    {
        for (const auto& slot : m_slots)
        {
            if (slot.m_pIItem && !slot.m_bPersistent && (!for_trade || slot.m_pIItem->CanTrade()))
            {
                items_container.push_back(slot.m_pIItem);
            }
        }
    }
}

bool CInventory::isBeautifulForActiveSlot(CInventoryItem* pIItem)
{
    for (const auto& slot : m_slots)
    {
        if (slot.m_pIItem && slot.m_pIItem->IsNecessaryItem(pIItem))
            return (true);
    }
    return (false);
}

void CInventory::Items_SetCurrentEntityHud(bool current_entity)
{
    for (const auto& item : m_all)
    {
        CWeapon* pWeapon = smart_cast<CWeapon*>(item);
        if (pWeapon)
        {
            pWeapon->InitAddons();
            pWeapon->UpdateAddonsVisibility();
        }
    }
};

// call this only via Actor()->SetWeaponHideState()
void CInventory::SetSlotsBlocked(u16 mask, bool bBlock, bool now)
{
    bool bChanged = false;
    for (int i = 0; i < SLOTS_TOTAL; ++i)
    {
        if (mask & (1 << i))
        {
            bool bCanBeActivated = m_slots[i].CanBeActivated();
            if (bBlock)
            {
                ++m_slots[i].m_blockCounter;
                VERIFY2(m_slots[i].m_blockCounter < 5, "block slots overflow");
            }
            else
            {
                --m_slots[i].m_blockCounter;
                VERIFY2(m_slots[i].m_blockCounter > -5, "block slots underflow");
            }
            if (bCanBeActivated != m_slots[i].CanBeActivated())
                bChanged = true;
        }
    }
    if (bChanged)
    {
        u32 ActiveSlot = GetActiveSlot();
        u32 PrevActiveSlot = GetPrevActiveSlot();

        if (PrevActiveSlot == NO_ACTIVE_SLOT)
        {
            if (GetNextActiveSlot() != NO_ACTIVE_SLOT && m_slots[GetNextActiveSlot()].m_pIItem && m_slots[GetNextActiveSlot()].m_pIItem->cast_hud_item() &&
                m_slots[GetNextActiveSlot()].m_pIItem->cast_hud_item()->IsShowing())
            {
                ActiveSlot = GetNextActiveSlot();
                SetActiveSlot(GetNextActiveSlot());
                m_slots[ActiveSlot].m_pIItem->Activate(true);
            }
        }
        else if (m_slots[PrevActiveSlot].m_pIItem && m_slots[PrevActiveSlot].m_pIItem->cast_hud_item() && m_slots[PrevActiveSlot].m_pIItem->cast_hud_item()->IsHiding())
        {
            m_slots[PrevActiveSlot].m_pIItem->Deactivate(true);
            ActiveSlot = NO_ACTIVE_SLOT;
            SetActiveSlot(NO_ACTIVE_SLOT);
        }

        if (ActiveSlot == NO_ACTIVE_SLOT)
        { // try to restore hidden weapon
            if (PrevActiveSlot != NO_ACTIVE_SLOT && m_slots[PrevActiveSlot].CanBeActivated())
                if (Activate(PrevActiveSlot, eGeneral, false, now))
                    SetPrevActiveSlot(NO_ACTIVE_SLOT);
        }
        else
        { // try to hide active weapon
            if (!m_slots[ActiveSlot].CanBeActivated())
                if (Activate(NO_ACTIVE_SLOT, eGeneral, false, now))
                    SetPrevActiveSlot(ActiveSlot);
        }
    }
}

void CInventory::Iterate(bool bSearchRuck, std::function<bool(const PIItem)> callback) const
{
    const auto& list = bSearchRuck ? m_ruck : m_belt;
    for (const auto& it : list)
        if (callback(it))
            break;
}

void CInventory::IterateAmmo(bool bSearchRuck, std::function<bool(const PIItem)> callback) const
{
    if (bSearchRuck)
    {
        for (const auto& item : m_ruck)
        {
            const auto* ammo = smart_cast<CWeaponAmmo*>(item);
            if (ammo && ammo->m_boxCurr && callback(item))
                break;
        }
    }
    else
    {
        for (const auto& item : m_vest)
        {
            const auto* ammo = smart_cast<CWeaponAmmo*>(item);
            if (ammo && ammo->m_boxCurr && callback(item))
                break;
        }
        for (const auto& item : m_belt)
        {
            const auto* ammo = smart_cast<CWeaponAmmo*>(item);
            if (ammo && ammo->m_boxCurr && callback(item))
                break;
        }
        for (const auto& _slot : m_slots)
        {
            const auto item = _slot.m_pIItem;
            const auto* ammo = smart_cast<CWeaponAmmo*>(item);
            if (ammo && ammo->m_boxCurr && callback(item))
                break;
        }
    }
}

PIItem CInventory::GetAmmoByLimit(const char* sect, bool forActor, bool limit_max, bool include_magazines) const
{
    PIItem box{};
    u32 limit{};

    auto callback = [&](const auto pIItem) -> bool {
        const auto* ammo = smart_cast<CWeaponAmmo*>(pIItem);
        shared_str sect_to_compare = include_magazines ? ammo->m_ammoSect : ammo->cNameSect();
        if (ammo->m_boxCurr && !xr_strcmp(sect_to_compare, sect))
        {
            const bool size_fits_limit = (ammo->m_boxCurr == (limit_max ? ammo->m_boxSize : 1));
            const bool update_limit = limit_max ? ammo->m_boxCurr > limit : (limit == 0 || ammo->m_boxCurr < limit);

            if (size_fits_limit)
            {
                box = pIItem;
                return true;
            }
            if (update_limit)
            {
                box = pIItem;
                limit = ammo->m_boxCurr;
            }
        }
        return false;
    };

    bool include_ruck = !forActor || Actor()->IsRuckAmmoPlacement();

    IterateAmmo(include_ruck, callback);

    return box;
}

bool CInventory::IsActiveSlotBlocked() const
{
    for (const auto& slot : m_slots)
        if (slot.CanBeActivated())
            return false;
    return true;
}

void CInventory::TryToHideWeapon(bool b_hide_state, bool b_save_prev_slot)
{
    if (b_hide_state)
    {
        if (ActiveItem() && !ActiveItem()->IsSingleHanded())
        {
            m_iPrevActiveSlot = b_save_prev_slot ? GetActiveSlot() : NO_ACTIVE_SLOT;
            Activate(NO_ACTIVE_SLOT);
        }
    }
    else
    {
        if (m_iPrevActiveSlot != NO_ACTIVE_SLOT)
        {
            Activate(m_iPrevActiveSlot);
            m_iPrevActiveSlot = NO_ACTIVE_SLOT;
        }
    }
}

// получаем айтем из всего инвентаря или с пояса
PIItem CInventory::GetSame(const PIItem pIItem, bool bSearchRuck) const
{
    if (bSearchRuck)
    {
        for (const auto& _item : m_ruck)
        {
            if ((_item != pIItem) && !xr_strcmp(_item->object().cNameSect(), pIItem->object().cNameSect()))
            {
                return _item;
            }
        }
    }
    else
    {
        for (const auto& _item : m_vest)
        {
            if ((_item != pIItem) && !xr_strcmp(_item->object().cNameSect(), pIItem->object().cNameSect()))
            {
                return _item;
            }
        }
        for (const auto& _item : m_belt)
        {
            if ((_item != pIItem) && !xr_strcmp(_item->object().cNameSect(), pIItem->object().cNameSect()))
            {
                return _item;
            }
        }
        for (const auto& _slot : m_slots)
        {
            const auto _item = _slot.m_pIItem;
            if (!_item)
                continue;

            if ((_item != pIItem) && !xr_strcmp(_item->object().cNameSect(), pIItem->object().cNameSect()))
            {
                return _item;
            }
        }
    }
    return nullptr;
}

u32 CInventory::GetSameItemCount(LPCSTR caSection, bool SearchRuck)
{
    u32 l_dwCount = 0;
    TIItemContainer& l_list = SearchRuck ? m_ruck : m_belt;
    for (const auto& item : l_list)
    {
        if (item && item->Useful() && !xr_strcmp(item->object().cNameSect(), caSection))
            ++l_dwCount;
    }
    // помимо пояса еще и в слотах поищем
    /*if (!SearchRuck)*/
    for (const auto& item : m_vest)
    {
        if (item && item->Useful() && !xr_strcmp(item->object().cNameSect(), caSection))
            ++l_dwCount;
    }

    for (const auto& _slot : m_slots)
    {
        PIItem item = _slot.m_pIItem;
        if (item && item->Useful() && !xr_strcmp(item->object().cNameSect(), caSection))
            ++l_dwCount;
    }

    return (l_dwCount);
}

void CInventory::TryAmmoCustomPlacement(CInventoryItem* pIItem)
{
    auto pAmmo = smart_cast<CWeaponAmmo*>(pIItem);
    if (!pAmmo)
        return;

    auto pActor = smart_cast<CActor*>(m_pOwner);
    if (!pActor)
        return;

    if (pAmmo->m_bNeedFindPlace)
    {
        pAmmo->m_bNeedFindPlace = false; // сбрасываем флажок спавна патронов
        if (!IsAllItemsLoaded())
            return;
        if (!pActor->IsRuckAmmoPlacement())
        { // если включены патроны с пояса, то для боеприпасов актора, которые спавнятся при разрядке
            if (pAmmo->IsBoxReloadable() && !pAmmo->m_boxCurr && HasDropPouch()) // якщо пустий магазин та є сумка для скидання - кладемо до рюкзаку
                return;
            if (CanPutInVest(pAmmo))
            { // спробуємо до розгрузки
                pAmmo->m_eItemPlace = eItemPlaceVest;
                return;
            }
            else if (CanPutInBelt(pAmmo))
            { // спробуємо до поясу
                pAmmo->m_eItemPlace = eItemPlaceBelt;
                return;
            }
            else
            { // попробуем определить свободный слот и положить в него
                for (const auto& slot : pAmmo->GetSlots())
                {
                    pAmmo->SetSlot(slot);
                    if (CanPutInSlot(pAmmo))
                    {
                        pAmmo->m_eItemPlace = eItemPlaceSlot;
                        return;
                    }
                }
            }
            if (!HasDropPouch()) // нікуди не вміщається та немає сумки для скидання - кидаємо на землю
                pAmmo->SetDropManual(TRUE);
        }
    }
}

u32 CInventory::BeltWidth() const
{
    if (auto pActor = smart_cast<CActor*>(m_pOwner))
    {
        if (auto warbelt = pActor->GetWarbelt())
        {
            return warbelt->GetBeltWidth();
        }
    }
    return 0;
}

u32 CInventory::BeltHeight() const
{
    if (auto pActor = smart_cast<CActor*>(m_pOwner))
    {
        if (auto warbelt = pActor->GetWarbelt())
        {
            return warbelt->GetBeltHeight();
        }
    }
    return 0;
}

u32 CInventory::VestWidth() const
{
    if (auto pActor = smart_cast<CActor*>(m_pOwner))
    {
        if (auto vest = pActor->GetVest())
        {
            return vest->GetVestWidth();
        }
    }
    return 0;
}

u32 CInventory::VestHeight() const
{
    if (auto pActor = smart_cast<CActor*>(m_pOwner))
    {
        if (auto vest = pActor->GetVest())
        {
            return vest->GetVestHeight();
        }
    }
    return 0;
}

bool CInventory::IsAllItemsLoaded() const { return m_bUpdated; }

bool CInventory::OwnerIsActor() const { return smart_cast<CActor*>(m_pOwner); }

void CInventory::DropBeltToRuck(bool skip_volume_check)
{
    if (!OwnerIsActor() || !IsAllItemsLoaded())
        return;
    for (const auto& item : m_belt)
        Ruck(item);
}

void CInventory::DropVestToRuck(bool skip_volume_check)
{
    if (!OwnerIsActor() || !IsAllItemsLoaded())
        return;
    for (const auto& item : m_vest)
        Ruck(item);
}

void CInventory::DropSlotsToRuck(u32 min_slot, u32 max_slot)
{
    if (!OwnerIsActor() || !IsAllItemsLoaded())
        return;

    if (max_slot == NO_ACTIVE_SLOT)
        max_slot = min_slot;

    for (const auto& slot : m_slots)
    {
        if (!slot.m_pIItem)
            continue;
        auto s = slot.m_pIItem->GetSlot();
        if (min_slot <= s && s <= max_slot)
            Ruck(ItemFromSlot(s));
    }
}

#include "InventoryContainer.h"
void CInventory::BackpackItemsTransfer(CInventoryItem* container, bool move_to_container)
{
    if (!OwnerIsActor() || !IsAllItemsLoaded())
        return;
    auto actor_id = m_pOwner->object_id();
    auto container_id = container->object().ID();
    if (move_to_container)
    {
        for (const auto& item : m_ruck)
        {
            item->Transfer(actor_id, container_id);
        }
    }
    else
    {
        auto cont = smart_cast<CInventoryContainer*>(container);
        if (!cont)
            return;
        for (const auto& item_id : cont->GetItems())
        {
            auto item = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
            item->Transfer(container_id, actor_id);
        }
    }
}

bool CInventory::IsSlotAllowed(u32 slot) const
{
    auto pActor = smart_cast<CActor*>(m_pOwner);
    if (!pActor || !IsAllItemsLoaded())
        return true;
    auto outfit = pActor->GetOutfit();
    switch (slot)
    {
    case KNIFE_SLOT:
    case APPARATUS_SLOT:
    case GRENADE_SLOT:
    case ARTEFACT_SLOT: return HasModuleForSlot(slot); break;
    case HELMET_SLOT: return (!outfit || !outfit->m_bIsHelmetBuiltIn); break;
    }
    return true;
}

bool CInventory::HasModuleForSlot(u32 check_slot) const
{
    for (const auto& item : m_vest)
    {
        if (item->GetSlotEnabled() == check_slot)
            return true;
    }
    for (const auto& item : m_belt)
    {
        if (item->GetSlotEnabled() == check_slot)
            return true;
    }
    return false;
}

bool CInventory::HasSameModuleEquiped(PIItem item) const
{
    if (!item->IsModule())
        return false;
    for (const auto& _item : m_vest)
    {
        if (_item->GetSlotEnabled() == item->GetSlotEnabled())
            return true;
    }
    for (const auto& _item : m_belt)
    {
        if (_item->GetSlotEnabled() == item->GetSlotEnabled())
            return true;
    }
    return false;
}

bool CInventory::HasDropPouch() const
{
    for (const auto& item : m_vest)
    {
        if (item->IsDropPouch())
            return true;
    }
    for (const auto& item : m_belt)
    {
        if (item->IsDropPouch())
            return true;
    }
    return false;
}

bool CInventory::activate_slot(u32 slot)
{
    switch (slot)
    {
    case KNIFE_SLOT:
    case FIRST_WEAPON_SLOT:
    case SECOND_WEAPON_SLOT:
    case GRENADE_SLOT:
    case APPARATUS_SLOT:
    case BOLT_SLOT:
    case ARTEFACT_SLOT:
    case QUICK_SLOT_0:
    case QUICK_SLOT_1:
    case QUICK_SLOT_2:
    case QUICK_SLOT_3: return ItemFromSlot(slot) && ItemFromSlot(slot)->cast_hud_item();
    }

    return false;
}

TIItemContainer CInventory::GetActiveArtefactPlace() const { return Core.Features.test(xrCore::Feature::artefacts_from_all) ? m_all : m_belt; }