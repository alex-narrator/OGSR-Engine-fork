////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_container.cpp
//	Created 	: 12.11.2014
//  Modified 	: 12.12.2014
//	Author		: Alexander Petrov
//	Description : Mobile container class, based on inventory item
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "InventoryContainer.h"

#include "Actor.h"
#include "Artifact.h"

u32 CInventoryContainer::Cost() const
{
    u32 res = inherited::Cost();
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        if (itm)
            res += itm->Cost();
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
            res += itm->Weight();
    }
    return res;
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

float CInventoryContainer::GetContainmentArtefactEffect(int effect) const
{
    float res{};
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        auto art = smart_cast<CArtefact*>(itm);
        if (art && art->CanAffect())
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