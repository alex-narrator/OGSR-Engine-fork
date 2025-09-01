#include "stdafx.h"
#include "Addons.h"
//
#include "PDA.h"
#include "SimpleDetectorSHOC.h"
#include "SimpleDetector.h"
#include "AdvancedDetector.h"
#include "EliteDetector.h"
#include "CustomDevice.h"
#include "FoodItem.h"
#include "InventoryBox.h"

using namespace luabind;

#pragma optimize("s", on)
void CScope::script_register(lua_State* L)
{
    module(L)[
        // addons
        class_<CScope, CGameObject>("CScope").def(constructor<>()), class_<CSilencer, CGameObject>("CSilencer").def(constructor<>()), class_<CGrenadeLauncher, CGameObject>("CGrenadeLauncher").def(constructor<>()), 
        // devices
        class_<CPda, CGameObject>("CPda").def(constructor<>()), class_<CCustomDetector, CGameObject>("CCustomDetector").def(constructor<>()),
        class_<CSimpleDetectorSHOC, CGameObject>("CSimpleDetectorSHOC").def(constructor<>()), class_<CScientificDetector, CGameObject>("CScientificDetector").def(constructor<>()),
        class_<CEliteDetector, CGameObject>("CEliteDetector").def(constructor<>()), class_<CAdvancedDetector, CGameObject>("CAdvancedDetector").def(constructor<>()),
        class_<CSimpleDetector, CGameObject>("CSimpleDetector").def(constructor<>()), class_<CCustomDevice, CGameObject>("CCustomDevice").def(constructor<>()),
        // food
        class_<CFoodItem, CGameObject>("CFoodItem").def(constructor<>()),
        // inventory box
        class_<CInventoryBox, CGameObject>("CInventoryBox").def(constructor<>())];
}