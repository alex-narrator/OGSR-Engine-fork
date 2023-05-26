#include "stdafx.h"
#include "WarBelt.h"

#include "Inventory.h"
#include "Actor.h"

CWarbelt::CWarbelt() { SetSlot(WARBELT_SLOT); }

CWarbelt::~CWarbelt() {}

void CWarbelt::Load(LPCSTR section)
{
    inherited::Load(section);
    m_iBeltWidth = READ_IF_EXISTS(pSettings, r_u32, section, "belt_width", 1);
    m_iBeltHeight = READ_IF_EXISTS(pSettings, r_u32, section, "belt_height", 1);
}

void CWarbelt::OnMoveToSlot(EItemPlace prevPlace)
{
    inherited::OnMoveToSlot(prevPlace);
    auto& inv = m_pCurrentInventory;
    if (inv && inv->OwnerIsActor())
        inv->DropBeltToRuck();
}

void CWarbelt::OnMoveToRuck(EItemPlace prevPlace)
{
    inherited::OnMoveToRuck(prevPlace);
    auto& inv = m_pCurrentInventory;
    if (inv && inv->OwnerIsActor() && prevPlace == eItemPlaceSlot)
        inv->DropBeltToRuck();
}

void CWarbelt::Hit(SHit* pHDS)
{
    inherited::Hit(pHDS);
    auto actor = smart_cast<CActor*>(H_Parent());
    if (actor && actor->GetWarbelt() == this)
        HitItemsInWarbelt(pHDS);
}

void CWarbelt::HitItemsInWarbelt(SHit* pHDS)
{
    TIItemContainer belt = m_pCurrentInventory->m_belt;
    if (belt.empty())
        return;

    switch (pHDS->type())
    {
    case ALife::eHitTypeFireWound:
    case ALife::eHitTypeWound:
    case ALife::eHitTypeWound_2: {
        u32 random_item = ::Random.randI(0, belt.size());
        auto item = belt[random_item];
        if (item)
            item->Hit(pHDS);
    }
    break;
    default: {
        pHDS->power *= (1.0f - GetHitTypeProtection(pHDS->type()));
        for (const auto& item : belt)
            item->Hit(pHDS);
    }
    break;
    }
    Msg("~ %s", __FUNCTION__);
}