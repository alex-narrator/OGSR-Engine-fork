#include "stdafx.h"
#include "Addons.h"
//
#include "PDA.h"
#include "SimpleDetectorSHOC.h"
#include "SimpleDetector.h"
#include "AdvancedDetector.h"
#include "EliteDetector.h"
#include "FoodItem.h"

using namespace luabind;

#pragma optimize("s", on)
void CScope::script_register(lua_State* L) 
{ 
	module(L)[
		//addons
		class_<CScope, CGameObject>("CScope").def(constructor<>()),
		class_<CSilencer, CGameObject>("CSilencer").def(constructor<>()),
		class_<CGrenadeLauncher, CGameObject>("CGrenadeLauncher").def(constructor<>()),
		class_<CLaser, CGameObject>("CLaser").def(constructor<>()),
		class_<CFlashlight, CGameObject>("CFlashlight").def(constructor<>()),
		class_<CStock, CGameObject>("CStock").def(constructor<>()),
		class_<CExtender, CGameObject>("CExtender").def(constructor<>()),
		class_<CForend, CGameObject>("CForend").def(constructor<>()),
		//devices
		class_<CPda, CGameObject>("CPda").def(constructor<>()), 
		class_<CSimpleDetectorSHOC, CGameObject>("CSimpleDetectorSHOC").def(constructor<>()),
		class_<CScientificDetector, CGameObject>("CScientificDetector").def(constructor<>()),
		class_<CEliteDetector, CGameObject>("CEliteDetector").def(constructor<>()), 
		class_<CAdvancedDetector, CGameObject>("CAdvancedDetector").def(constructor<>()),
		class_<CSimpleDetector, CGameObject>("CSimpleDetector").def(constructor<>()),
		//food
		class_<CFoodItem, CGameObject>("CFoodItem").def(constructor<>())
	];
}

void CGrenadeLauncher::Load(LPCSTR section){
	m_fGrenadeVel = pSettings->r_float(section, "grenade_vel");
	inherited::Load(section);
}