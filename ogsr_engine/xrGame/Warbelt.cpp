#include "stdafx.h"
#include "WarBelt.h"

#include "Inventory.h"
#include "Actor.h"


CWarbelt::CWarbelt(){
	SetSlot(WARBELT_SLOT);
}


CWarbelt::~CWarbelt(){}

void CWarbelt::Load(LPCSTR section){
	inherited::Load(section);
	m_iBeltWidth	= READ_IF_EXISTS(pSettings, r_u32,	section, "belt_width",	1);
	m_iBeltHeight	= READ_IF_EXISTS(pSettings, r_u32,	section, "belt_height",	1);
}

void CWarbelt::OnMoveToSlot(EItemPlace prevPlace){
	inherited::OnMoveToSlot(prevPlace);
	auto &inv = m_pCurrentInventory;
	if (inv){
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor) {
			inv->DropBeltToRuck();
		}
	}
}

void CWarbelt::OnMoveToRuck(EItemPlace prevPlace) {
	inherited::OnMoveToRuck(prevPlace);
	auto& inv = m_pCurrentInventory;
	if (inv && prevPlace == eItemPlaceSlot) {
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor) {
			inv->DropBeltToRuck();
		}
	}
}