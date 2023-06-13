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

void CGasMask::InitAddons()
{
    inherited::InitAddons();
    auto section = IsPowerSourceAttachable() && IsPowerSourceAttached() ? GetPowerSourceName() : cNameSect();
    m_fPowerLoss = READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 0.f);
}

bool CGasMask::IsPowerOn() const
{
    auto pActor = smart_cast<const CActor*>(H_Parent());
    if (pActor && pActor->GetGasMask() == this)
        return GetPowerLevel();
    return false;
}

float CGasMask::GetHitTypeProtection(int hit_type) const 
{ 
    switch (hit_type)
    {
    case ALife::eHitTypeRadiation:
    case ALife::eHitTypeChemicalBurn:
        if (!GetPowerLevel())
            return 0.f;
        break;
    }
    return inherited::GetHitTypeProtection(hit_type);
}