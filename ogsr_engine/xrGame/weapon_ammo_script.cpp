#include "stdafx.h"
#include "WeaponAmmo.h"
#include "inventory_item_object.h"
#include "script_game_object.h"

using namespace luabind;
#pragma optimize("s", on)
void CWeaponAmmo::script_register(lua_State* L)
{
    module(L)[
        class_<CWeaponAmmo, CGameObject>("CWeaponAmmo")
                  .def(constructor<>())
                  .def_readwrite("unloaded_from_weapon", &CWeaponAmmo::m_bUnloadedFromWeapon)
                  .def_readonly("box_size", &CWeaponAmmo::m_boxSize)
                  .def_readonly("box_curr", &CWeaponAmmo::m_boxCurr)
                  .def_readonly("box_ammo_type", &CWeaponAmmo::m_cur_ammo_type)
                  .def("is_magazine", &CWeaponAmmo::IsBoxReloadable)
    ];
}