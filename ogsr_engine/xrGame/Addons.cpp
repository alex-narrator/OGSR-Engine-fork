#include "stdafx.h"
#include "Addons.h"

using namespace luabind;

#pragma optimize("s", on)
void CScope::script_register(lua_State* L) { module(L)[class_<CScope, CGameObject>("CScope").def(constructor<>())]; }

void CGrenadeLauncher::Load(LPCSTR section){
	m_fGrenadeVel = pSettings->r_float(section, "grenade_vel");
	inherited::Load(section);
}