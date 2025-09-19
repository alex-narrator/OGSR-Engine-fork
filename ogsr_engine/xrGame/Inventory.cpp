#include "stdafx.h"
#include "inventory.h"
#include "actor.h"
#include "weapon.h"

#include "eatable_item.h"
#include "script_engine.h"
#include "xrmessages.h"
//#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "level.h"
#include "ai_space.h"
#include "entitycondition.h"
#include "game_base_space.h"
#include "clsid_game.h"
#include "CustomOutfit.h"
#include "HudItem.h"
#include "PDA.h"

#include "UIGameSP.h"
#include "HudManager.h"
#include "ui/UIPDAWnd.h"

// what to block
u32 INV_STATE_BLOCK_ALL = 0xffffffff;
u32 INV_STATE_INV_WND = INV_STATE_BLOCK_ALL;
u32 INV_STATE_BUY_MENU = INV_STATE_BLOCK_ALL;
u32 INV_STATE_LADDER = INV_STATE_BLOCK_ALL;
u32 INV_STATE_CAR = INV_STATE_BLOCK_ALL;

bool CInventorySlot::CanBeActivated() const { return (m_bVisible && !IsBlocked()); };

bool CInventorySlot::IsBlocked() const { return (m_blockCounter > 0); }

bool CInventorySlot::maySwitchFast() const { return m_maySwitchFast; }

void CInventorySlot::setSwitchFast(bool value) { m_maySwitchFast = value; }

CInventory::CInventory()
{
    m_fTakeDist = pSettings->r_float("inventory", "take_dist");
    m_fMaxWeight = pSettings->r_float("inventory", "max_weight");

    m_iMaxBelt = READ_IF_EXISTS(pSettings, r_u32, "inventory", "max_belt", 0);

    m_slots.resize(SLOTS_TOTAL);

    string256 temp;
    for (u32 i = 0; i < m_slots.size(); ++i)
    {
        sprintf_s(temp, "slot_persistent_%d", i);
        m_slots[i].m_bPersistent = READ_IF_EXISTS(pSettings, r_bool, "inventory", temp, false);
        sprintf_s(temp, "slot_switch_fast_%d", i);
        m_slots[i].setSwitchFast(READ_IF_EXISTS(pSettings, r_bool, "inventory", temp, false));
        sprintf_s(temp, "slot_visible_%d", i);
        m_slots[i].m_bVisible = READ_IF_EXISTS(pSettings, r_bool, "inventory", temp, true);
        sprintf_s(temp, "slot_need_unlock_%d", i);
        m_slots[i].m_bNeedUnlock = READ_IF_EXISTS(pSettings, r_bool, "inventory", temp, false);
    }
}

CInventory::~CInventory() {}

void CInventory::Clear()
{
    /*m_allMap.clear();*/
    m_all.clear();
    m_ruck.clear();
    m_belt.clear();

    for (u32 i = 0; i < m_slots.size(); i++)
    {
        m_slots[i].m_pIItem = nullptr;
    }

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
    /*m_allMap.emplace(pIItem->object().cNameSect(), pIItem);*/

    if (!strict_placement)
        pIItem->m_eItemPlace = eItemPlaceUndefined;

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
            Msg("cant slot in ruck item %s", *pIItem->object().cName());
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
        if (!InBelt(pIItem))
        {
            Msg("!!CInventory::DropItem: InBelt(pIItem): [%s]", pObj->cName().c_str());
            pIItem->m_eItemPlace = eItemPlaceUndefined;
        }
        else
        {
            m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));
        }

        pIItem->object().processing_deactivate();
    }
    break;
    case eItemPlaceRuck: {
        if (!InRuck(pIItem))
        {
            Msg("!!CInventory::DropItem: InRuck(pIItem): [%s]", pObj->cName().c_str());
            pIItem->m_eItemPlace = eItemPlaceUndefined;
        }
        else
        {
            m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
        }
    }
    break;
    case eItemPlaceSlot: {
        if (!InSlot(pIItem))
        {
            Msg("!!CInventory::DropItem: InSlot(pIItem): [%s], id: [%u]", pObj->cName().c_str(), pObj->ID());
            pIItem->m_eItemPlace = eItemPlaceUndefined;
        }
        else
        {
            if (m_iActiveSlot == pIItem->GetSlot())
                Activate(NO_ACTIVE_SLOT);

            m_slots[pIItem->GetSlot()].m_pIItem = nullptr;
        }

        pIItem->object().processing_deactivate();
    }
    break;
    default: NODEFAULT;
    }

    //bool removed = false;
   
    //const auto map_pair = m_allMap.equal_range(pIItem->object().cNameSect());

    //for (auto it2 = map_pair.first; it2 != map_pair.second; ++it2)
    //{
    //    if (it2->second == pIItem)
    //    {
    //        m_allMap.erase(it2);

    //        const auto it = std::find(m_all.begin(), m_all.end(), pIItem);
    //        if (it != m_all.end())
    //        {
    //            m_all.erase(it);
    //        }
    //        removed = true;
    //        break;
    //    }
    //}

    //if (!removed)
    //    Msg("! CInventory::Drop item not found in inventory!!!");

    TIItemContainer::iterator it = std::find(m_all.begin(), m_all.end(), pIItem);
    if (it != m_all.end())
        m_all.erase(it);
    else
        Msg("! CInventory::Drop item not found in inventory!!!");

    pIItem->OnMoveOut(pIItem->m_eItemPlace);

    pIItem->m_pCurrentInventory = nullptr;

    m_pOwner->OnItemDrop(smart_cast<CInventoryItem*>(pObj), pIItem->m_eItemPlace);

    CalcTotalWeight();
    InvalidateState();
    m_drop_last_frame = true;
    return true;
}

//положить вещь в слот
bool CInventory::Slot(PIItem pIItem, bool bNotActivate)
{
    VERIFY(pIItem);
    //	Msg("To Slot %s[%d]", *pIItem->object().cName(), pIItem->object().ID());

    if (!CanPutInSlot(pIItem))
    {
        /*
        Msg("there is item %s[%d,%x] in slot %d[%d,%x]",
                *m_slots[pIItem->GetSlot()].m_pIItem->object().cName(),
                m_slots[pIItem->GetSlot()].m_pIItem->object().ID(),
                m_slots[pIItem->GetSlot()].m_pIItem,
                pIItem->GetSlot(),
                pIItem->object().ID(),
                pIItem);
        */
        if (m_slots[pIItem->GetSlot()].m_pIItem == pIItem && !bNotActivate)
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

    //удалить из рюкзака или пояса
    TIItemContainer::iterator it = std::find(m_ruck.begin(), m_ruck.end(), pIItem);
    if (m_ruck.end() != it)
        m_ruck.erase(it);
    it = std::find(m_belt.begin(), m_belt.end(), pIItem);
    if (m_belt.end() != it)
        m_belt.erase(it);

    if ((m_iActiveSlot == pIItem->GetSlot() || (m_iActiveSlot == NO_ACTIVE_SLOT && m_iNextActiveSlot == NO_ACTIVE_SLOT)) && !bNotActivate)
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

    //вещь была в слоте
    bool in_slot = InSlot(pIItem);
    if (in_slot)
    {
        if (m_iActiveSlot == pIItem->GetSlot())
            Activate(NO_ACTIVE_SLOT);
        m_slots[pIItem->GetSlot()].m_pIItem = NULL;
    }

    m_belt.insert(m_belt.end(), pIItem);

    if (!in_slot)
    {
        TIItemContainer::iterator it = std::find(m_ruck.begin(), m_ruck.end(), pIItem);
        if (m_ruck.end() != it)
            m_ruck.erase(it);
    }

    CalcTotalWeight();
    InvalidateState();

    EItemPlace p = pIItem->m_eItemPlace;
    pIItem->m_eItemPlace = eItemPlaceBelt;
    m_pOwner->OnItemBelt(pIItem, p);
    pIItem->OnMoveToBelt(p);

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
    //вещь была в слоте
    if (in_slot)
    {
        if (m_iActiveSlot == pIItem->GetSlot())
            Activate(NO_ACTIVE_SLOT);
        m_slots[pIItem->GetSlot()].m_pIItem = NULL;
    }
    else
    {
        //вещь была на поясе или вообще только поднята с земли
        TIItemContainer::iterator it = std::find(m_belt.begin(), m_belt.end(), pIItem);
        if (m_belt.end() != it)
            m_belt.erase(it);
    }

    m_ruck.insert(m_ruck.end(), pIItem);

    CalcTotalWeight();
    InvalidateState();

    EItemPlace prevPlace = pIItem->m_eItemPlace;
    m_pOwner->OnItemRuck(pIItem, prevPlace);
    pIItem->m_eItemPlace = eItemPlaceRuck;

    if (pIItem->GetSlot() != OUTFIT_SLOT || (smart_cast<CActor*>(GetOwner()) && in_slot)) //фикс сброса визуала актора при взятии в инвентарь любого костюма
        pIItem->OnMoveToRuck(prevPlace);
    else
        pIItem->CInventoryItem::OnMoveToRuck(prevPlace);

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

    //активный слот не выбран
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
    //активный слот задействован
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
    if (m_iActiveSlot < m_slots.size() && m_slots[m_iActiveSlot].m_pIItem && m_slots[m_iActiveSlot].m_pIItem->Action(cmd, flags))
        return true;

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
            if (GetActiveSlot() == cmd - kWPN_1 && ActiveItem())
                Activate(NO_ACTIVE_SLOT);
            else
                Activate(cmd - kWPN_1, eKeyAction);
        }
    }
    break;
    case kWPN_7: {
        if (flags & CMD_START)
        {
            if ((int)m_iActiveSlot == ARTEFACT_SLOT && m_slots[m_iActiveSlot].m_pIItem)
                Activate(NO_ACTIVE_SLOT);
            else
                Activate(ARTEFACT_SLOT, eKeyAction);
        }
    }
    break;
    case kACTIVE_JOBS:
    case kMAP:
    case kJOURNAL:
    case kCONTACTS: {
        if (flags & CMD_START)
        {
            auto Pda = m_pOwner->GetPDA();
            if (!Pda || !Pda->Is3DPDA() || !psActorFlags.test(AF_3D_PDA))
                break;

            extern bool g_actor_allow_pda;

            if (GetActiveSlot() == PDA_SLOT && ActiveItem())
                Activate(NO_ACTIVE_SLOT);
            else if (g_actor_allow_pda)
            {
                auto pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
                pGameSP->PdaMenu->SetActiveSubdialog(cmd == kACTIVE_JOBS ? eptQuests : (cmd == kMAP ? eptMap : (cmd == kJOURNAL ? eptDiary : eptContacts)));
                Activate(PDA_SLOT, eKeyAction);
            }
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
    // соотв. и слотов под арты как бы нет. Вот поэтому до первого апдейта CInventory актора считаем, что все слоты для артов доступны ( см. CInventory::BeltSlotsCount() )
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

//скушать предмет
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

    auto lua_go = smart_cast<CGameObject*>(pIItem)->lua_game_object();

    if (Actor()->m_inventory == this)
        Actor()->callback(GameObject::eOnBeforeUseItem)(lua_go);

    if (!pItemToEat->m_bCanBeEaten)
        return false;

    pItemToEat->UseBy(entity_alive);

    if (Actor()->m_inventory == this)
        Actor()->callback(GameObject::eUseObject)(lua_go);

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

bool CInventory::InBelt(PIItem pIItem) const { return (std::find(m_belt.begin(), m_belt.end(), pIItem) != m_belt.end()); }
bool CInventory::InRuck(PIItem pIItem) const { return (std::find(m_ruck.begin(), m_ruck.end(), pIItem) != m_ruck.end()); }

bool CInventory::CanPutInSlot(PIItem pIItem, bool b_check_all) const
{
    if (b_check_all && pIItem->GetSlotsCount() > 1)
        for (const auto slot : pIItem->GetSlots())
            if (CanPutInSlot(pIItem, slot))
            {
                pIItem->SetSlot(slot);
                return true;
            }

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

    if (slot < m_slots.size() && !m_slots[slot].m_pIItem && IsSlotAllowed(slot))
        return true;

    return false;
}

//проверяет можем ли поместить вещь на пояс,
//при этом реально ничего не меняется
bool CInventory::CanPutInBelt(PIItem pIItem)
{
    if (InBelt(pIItem))
        return false;
    if (!m_bBeltUseful)
        return false;
    if (!pIItem || !pIItem->Belt())
        return false;
    if (!IsAllItemsLoaded())
        return true;
    if (m_belt.size() >= BeltSize())
        return false;

    bool res{true};
    if (pSettings->line_exist("engine_callbacks", "can_put_in_belt"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "can_put_in_belt");
        if (luabind::functor<bool> lua_function; ai().script_engine().functor(callback, lua_function))
            res = lua_function(smart_cast<CGameObject*>(pIItem)->lua_game_object());
    }
    return res;
}
//проверяет можем ли поместить вещь в рюкзак,
//при этом реально ничего не меняется
bool CInventory::CanPutInRuck(PIItem pIItem) const
{
    if (InRuck(pIItem))
        return false;
    return true;
}

u32 CInventory::dwfGetObjectCount() const { return (m_all.size()); }

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

    CActor* pActor = smart_cast<CActor*>(m_pOwner);
    //актер всегда может взять вещь
    if (!pActor && (TotalWeight() + inventory_item->Weight() > m_pOwner->MaxCarryWeight()))
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
    m_bBlockDevice = bBlock;
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

void CInventory::IterateAmmo(std::function<bool(const PIItem)> callback) const
{
    for (const auto& item : m_all)
        if (smart_cast<CWeaponAmmo*>(item) && callback(item))
            return;
}

PIItem CInventory::GetAmmoByLimit(const char* sect, bool limit_max, xr_vector<shared_str>* magazines) const
{
    PIItem box{};
    u32 limit{};
    xr_vector<shared_str> mags{};
    if (magazines)
        mags = *magazines;

    auto callback = [&](const auto pIItem) -> bool {
        const auto* ammo = smart_cast<CWeaponAmmo*>(pIItem);

        if (!ammo->UsefulForReload() || !ammo->m_boxCurr || mags.size() && (!ammo->IsBoxReloadable() || std::find(mags.begin(), mags.end(), ammo->cNameSect()) == mags.end()))
            return false;

        shared_str sect_to_compare = mags.size() ? ammo->m_ammoSect : ammo->cNameSect();
        if (!xr_strcmp(sect_to_compare, sect))
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

    IterateAmmo(callback);
    if (mags.size() && !box) // шукали магазин та не знайшли
    {
        mags = {}; // шукаємо набої у пачках
        IterateAmmo(callback);
    }

    return box;
}

bool CInventory::IsActiveSlotBlocked() const
{
    for (const auto& slot : m_slots)
        if (slot.CanBeActivated())
            return false;
    return true;
}

bool CInventory::IsAllItemsLoaded() const { return m_bUpdated; }

bool CInventory::IsSlotAllowed(u32 slot) const
{
    if (slot >= m_slots.size())
        return false;    

    if (!smart_cast<CActor*>(m_pOwner) || !IsAllItemsLoaded())
        return true;

    return (!m_slots[slot].m_bNeedUnlock || HasUnlockForSlot(slot)) && !HasLockForSlot(slot);
}

bool CInventory::HasUnlockForSlot(u32 check_slot) const
{
    for (const auto& slot : m_slots)
    {
        auto item = slot.m_pIItem;
        if (!item)
            continue;
        auto unlock_slots = item->GetSlotsUnlocked();
        if (unlock_slots.empty())
            continue;
        if (std::find(unlock_slots.begin(), unlock_slots.end(), check_slot) != unlock_slots.end())
            return true;
    }
    for (const auto& item : m_belt)
    {
        auto unlock_slots = item->GetSlotsUnlocked();
        if (unlock_slots.empty())
            continue;
        if (std::find(unlock_slots.begin(), unlock_slots.end(), check_slot) != unlock_slots.end())
            return true;
    }
    return false;
}

bool CInventory::HasLockForSlot(u32 check_slot) const
{
    for (const auto& slot : m_slots)
    {
        auto item = slot.m_pIItem;
        if (!item)
            continue;
        auto lock_slots = item->GetSlotsLocked();
        if (lock_slots.empty())
            continue;
        if (std::find(lock_slots.begin(), lock_slots.end(), check_slot) != lock_slots.end())
            return true;
    }
    for (const auto& item : m_belt)
    {
        auto lock_slots = item->GetSlotsLocked();
        if (lock_slots.empty())
            continue;
        if (std::find(lock_slots.begin(), lock_slots.end(), check_slot) != lock_slots.end())
            return true;
    }
    return false;
}