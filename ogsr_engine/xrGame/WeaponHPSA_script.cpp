#include "stdafx.h"
#include "WeaponHPSA.h"

using namespace luabind;


void CWeaponHPSA::script_register(lua_State* L) { module(L)[class_<CWeaponHPSA, CGameObject>("CWeaponHPSA").def(constructor<>())]; }
