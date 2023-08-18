#include "stdafx.h"
#include "WarBelt.h"

#include "Inventory.h"
#include "Actor.h"

CWarbelt::CWarbelt() { SetSlot(WARBELT_SLOT); }

CWarbelt::~CWarbelt() {}

void CWarbelt::Load(LPCSTR section)
{
    inherited::Load(section);
    m_BeltArray = READ_IF_EXISTS(pSettings, r_ivector2, section, "belt_array", Ivector2{});
    m_BeltVertical = READ_IF_EXISTS(pSettings, r_bool, section, "belt_vertical", false);
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