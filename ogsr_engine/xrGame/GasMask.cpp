#include "stdafx.h"
#include "GasMask.h"
#include "Actor.h"
#include "Inventory.h"
#include "xrserver_objects_alife_items.h"

CGasMask::CGasMask() 
{ 
    SetSlot(GASMASK_SLOT); 
}

bool CGasMask::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    return pA ? (pA->GetGasMask() == this) : true;
}

float CGasMask::HitThruArmour(SHit* pHDS)
{
    float hit_power = pHDS->power;
    auto actor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
    if (!actor || !actor->IsHitToHead(pHDS))
        return hit_power;

    hit_power *= (1.0f - GetHitTypeProtection(pHDS->type()));

    Hit(pHDS);

    return hit_power;
};