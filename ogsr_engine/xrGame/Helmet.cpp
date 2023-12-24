#include "stdafx.h"
#include "Helmet.h"
#include "Actor.h"
#include "Inventory.h"

float CHelmet::HitThruArmour(SHit* pHDS)
{
    auto actor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
    if (!actor || !actor->IsHitToHead(pHDS))
        return pHDS->damage();

    return inherited::HitThruArmour(pHDS);
};

float CHelmet::GetHitTypeProtection(int hit_type) const { return (hit_type == ALife::eHitTypeFireWound) ? 0.f : inherited::GetHitTypeProtection(hit_type); }

bool CHelmet::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    return pA ? (pA->GetHelmet() == this) : true;
}