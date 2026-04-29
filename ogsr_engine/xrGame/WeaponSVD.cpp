#include "stdafx.h"
#include "weaponsvd.h"
#include "game_object_space.h"

CWeaponSVD::CWeaponSVD(void) : CWeaponCustomPistol("SVD") {}

using namespace luabind;


void CWeaponSVD::script_register(lua_State* L) { module(L)[class_<CWeaponSVD, CGameObject>("CWeaponSVD").def(constructor<>())]; }
