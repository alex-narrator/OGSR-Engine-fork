////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_container.cpp
//	Created 	: 12.11.2014
//  Modified 	: 12.12.2014
//	Author		: Alexander Petrov
//	Description : Mobile container class, based on inventory item
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Level.h"
#include "InventoryContainer.h"

#include "Actor.h"
#include "Artifact.h"
#include "Inventory.h"

void CInventoryContainer::Load(LPCSTR section)
{
    inherited::Load(section);
    m_bQuickDrop = READ_IF_EXISTS(pSettings, r_bool, section, "quick_drop", false);
    m_iItemsLimit = READ_IF_EXISTS(pSettings, r_u32, section, "items_limit", 0);
    if (pSettings->line_exist(section, "allowed_classes"))
    {
        LPCSTR str = pSettings->r_string(section, "allowed_classes");
        for (int i = 0; i < _GetItemCount(str); ++i)
        {
            string128 item_class;
            _GetItem(str, i, item_class);
            m_allowed_classes.push_back(item_class);
        }
    }
}

u32 CInventoryContainer::Cost() const
{
    u32 res = inherited::Cost();
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        if (itm)
        {
            res += itm->Cost();
        }
    }
    if (m_pCurrentInventory)
    {
        if (auto actor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner()))
        {
            if (this == actor->GetBackpack())
            {
                for (const auto& item : m_pCurrentInventory->m_ruck)
                {
                    if (item)
                        res += item->Cost();
                }
            }
        }
    }
    return res;
}

float CInventoryContainer::Weight() const
{
    float res = inherited::Weight();
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        if (itm)
        {
            res += itm->Weight();
        }
    }
    return res;
}

bool CInventoryContainer::CanTrade() const
{
    if (!IsEmpty()) // продавать можно только пустым
        return false;
    if (m_pCurrentInventory)
    {
        if (auto actor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner()))
        {
            if (this == actor->GetBackpack())
            {
                return false;
            }
        }
    }
    return inherited::CanTrade();
}

void CInventoryContainer::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);
    UpdateDropTasks();
}

void CInventoryContainer::UpdateDropTasks()
{
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        VERIFY(itm);
        UpdateDropItem(itm);
    }
}

void CInventoryContainer::UpdateDropItem(PIItem pIItem)
{
    if (pIItem->GetDropManual())
    {
        pIItem->SetDropManual(FALSE);
        pIItem->object().Position().set(Position()); // щоб реджектнутий об'єкт з'являвся на позиції батьківського контейнера
        NET_Packet P;
        pIItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
        P.w_u16(pIItem->object().ID());
        pIItem->object().u_EventSend(P);
    } // dropManual
}

float CInventoryContainer::GetItemEffect(int effect) const
{
    bool for_rad = effect == eRadiationRestoreSpeed;
    float res = inherited::GetItemEffect(effect);
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        if (itm)
        {
            if (for_rad)
            {
                float rad = itm->GetItemEffect(effect);
                rad *= (1.f - GetHitTypeProtection(ALife::eHitTypeRadiation));
                res += rad;
            }
            else if (smart_cast<CArtefact*>(itm))
                res += itm->GetItemEffect(effect);
        }
    }
    return res;
}

bool CInventoryContainer::NeedForcedDescriptionUpdate() const
{
    for (const auto& item_id : m_items)
    {
        auto item = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        if (item)
        {
            if (auto artefact = smart_cast<CArtefact*>(item))
            {
                if (artefact->GetCondition() && artefact->m_fTTLOnDecrease)
                    return true;
            }
        }
    }
    return inherited::NeedForcedDescriptionUpdate();
}

float CInventoryContainer::GetContainmentArtefactEffect(int effect) const
{
    float res{};
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        if (itm && smart_cast<CArtefact*>(itm))
        {
            res += itm->GetItemEffect(effect);
        }
    }
    return res;
}

float CInventoryContainer::GetContainmentArtefactProtection(int hit_type) const
{
    float res{};
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        if (itm && smart_cast<CArtefact*>(itm))
        {
            res += itm->GetHitTypeProtection(hit_type);
        }
    }
    return res;
}

bool CInventoryContainer::can_be_attached() const
{
    const auto actor = smart_cast<const CActor*>(H_Parent());
    return actor ? (actor->GetBackpack() == this) : true;
}

void CInventoryContainer::Hit(SHit* pHDS)
{
    inherited::Hit(pHDS);

    auto actor = smart_cast<CActor*>(H_Parent());
    if (actor && actor->GetBackpack() == this)
    {
        HitItemsInBackPack(pHDS);
        return;
    }
    HitItemsInContainer(pHDS);
}

void CInventoryContainer::HitItemsInBackPack(SHit* pHDS)
{
    TIItemContainer ruck = m_pCurrentInventory->m_ruck;
    if (ruck.empty())
        return;
    pHDS->power *= (1.0f - GetHitTypeProtection(pHDS->type()));

    switch (pHDS->type())
    {
    case ALife::eHitTypeFireWound:
    case ALife::eHitTypeWound:
    case ALife::eHitTypeWound_2: {
        u32 random_item = ::Random.randI(0, ruck.size());
        auto item = ruck[random_item];
        if (item)
            item->Hit(pHDS);
    }
    break;
    default: {
        for (const auto& item : ruck)
        {
            item->Hit(pHDS);
        }
    }
    break;
    }
}

void CInventoryContainer::HitItemsInContainer(SHit* pHDS)
{
    if (IsEmpty())
        return;
    pHDS->power *= (1.0f - GetHitTypeProtection(pHDS->type()));
    PIItem item{};

    switch (pHDS->type())
    {
    case ALife::eHitTypeFireWound:
    case ALife::eHitTypeWound:
    case ALife::eHitTypeWound_2: {
        u32 random_item = ::Random.randI(0, m_items.size());
        item = smart_cast<PIItem>(Level().Objects.net_Find(random_item));
        if (item)
            item->Hit(pHDS);
    }
    break;
    default: {
        for (const auto& item_id : m_items)
        {
            item = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
            if (item)
            {
                item->Hit(pHDS);
            }
        }
    }
    break;
    }
}

void CInventoryContainer::OnMoveToSlot(EItemPlace prevPlace)
{
    inherited::OnMoveToSlot(prevPlace);
    auto& inv = m_pCurrentInventory;
    if (inv)
    {
        auto pActor = smart_cast<CActor*>(inv->GetOwner());
        if (pActor)
        {
            inv->BackpackItemsTransfer(this, false);
        }
    }
}

void CInventoryContainer::OnMoveOut(EItemPlace prevPlace)
{
    inherited::OnMoveOut(prevPlace);
    auto& inv = m_pCurrentInventory;
    if (inv && prevPlace == eItemPlaceSlot)
    {
        auto pActor = smart_cast<CActor*>(inv->GetOwner());
        if (pActor)
        {
            inv->BackpackItemsTransfer(this, true);
        }
    }
}

bool CInventoryContainer::HasQuickDrop() const
{
    for (const auto slot : m_slots)
    {
        if (slot == BACKPACK_SLOT && m_bQuickDrop)
            return true;
    }
    return false;
}

u32 CInventoryContainer::GetSameItemCount(shared_str sect) const
{
    u32 count{};
    for (const auto& item_id : m_items)
    {
        auto item = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        if (item && item->object().cNameSect() == sect)
        {
            ++count;
        }
    }
    return count;
}

void CInventoryContainer::AddUniqueItems(TIItemContainer& items_container) const
{
    auto is_unique = [](TIItemContainer& list, PIItem item) {
        bool res{true};
        for (const auto& _itm : list)
        {
            if (item->object().cNameSect() == _itm->object().cNameSect())
            {
                res = false;
                break;
            }
        }
        return res;
    };

    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        VERIFY(itm);
        if (is_unique(items_container, itm))
            items_container.push_back(itm);
    }
}

bool CInventoryContainer::CanTakeItem(CInventoryItem* inventory_item) const 
{
    bool res{true};
    if (m_iItemsLimit)
        res = (m_items.size() < m_iItemsLimit);
    if (!m_allowed_classes.empty())
        res = res && (std::find(m_allowed_classes.begin(), m_allowed_classes.end(), pSettings->r_string(inventory_item->object().cNameSect(), "class")) != m_allowed_classes.end());
    return res;
}