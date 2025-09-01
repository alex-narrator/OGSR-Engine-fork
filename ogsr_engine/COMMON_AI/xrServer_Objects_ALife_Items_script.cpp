////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Items_script.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server items for ALife simulator, script export
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_script_macroses.h"

using namespace luabind;


void CSE_ALifeInventoryItem::script_register(lua_State* L)
{
    module(L)[class_<CSE_ALifeInventoryItem>("cse_alife_inventory_item")
                  //			.def(		constructor<LPCSTR>())
                  .def_readwrite("item_condition", &CSE_ALifeInventoryItem::m_fCondition)];
}

void CSE_ALifeItem::script_register(lua_State* L)
{
    module(L)[
        //		luabind_class_item2(
        luabind_class_abstract2(CSE_ALifeItem, "cse_alife_item", CSE_ALifeDynamicObjectVisual, CSE_ALifeInventoryItem)];
}

void CSE_ALifeItemTorch::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemTorch, "cse_alife_item_torch", CSE_ALifeItem)]; }

void CSE_ALifeItemAmmo::script_register(lua_State* L) 
{ 
    module(L)[luabind_class_item1(CSE_ALifeItemAmmo, "cse_alife_item_ammo", CSE_ALifeItem)
                  .def_readwrite("ammo_elapsed", &CSE_ALifeItemAmmo::a_elapsed)
                  .def_readwrite("box_size", &CSE_ALifeItemAmmo::m_boxSize)
                  .def_readwrite("ammo_type", &CSE_ALifeItemAmmo::m_cur_ammo_type)
    ];
}

void CSE_ALifeItemWeapon::script_register(lua_State* L)
{
    module(L)[luabind_class_item1(CSE_ALifeItemWeapon, "cse_alife_item_weapon", CSE_ALifeItem)
                  .def_readwrite("ammo_current", &CSE_ALifeItemWeapon::a_current)
                  .def_readwrite("ammo_elapsed", &CSE_ALifeItemWeapon::a_elapsed)
                  .def_readwrite("weapon_state", &CSE_ALifeItemWeapon::wpn_state)
                  .def_readwrite("addon_flags", &CSE_ALifeItemWeapon::m_addon_flags)
                  .def_readwrite("ammo_type", &CSE_ALifeItemWeapon::ammo_type)

                  .def_readwrite("current_scope", &CSE_ALifeItemWeapon::m_cur_scope)
                  .def_readwrite("current_silencer", &CSE_ALifeItemWeapon::m_cur_silencer)
                  .def_readwrite("current_launcher", &CSE_ALifeItemWeapon::m_cur_glauncher)
                  .def_readwrite("current_laser", &CSE_ALifeItemWeapon::m_cur_laser)
                  .def_readwrite("current_flashlight", &CSE_ALifeItemWeapon::m_cur_flashlight)
                  .def_readwrite("current_stock", &CSE_ALifeItemWeapon::m_cur_stock)
                  .def_readwrite("current_extender", &CSE_ALifeItemWeapon::m_cur_extender)
                  .def_readwrite("current_forend", &CSE_ALifeItemWeapon::m_cur_forend)
                  .def_readwrite("current_magazine", &CSE_ALifeItemWeapon::m_cur_magazine),

              class_<enum_exporter<EWeaponAddonState>>("addon_flag")
                  .enum_("addon_flag")[
                      value("scope", int(eWeaponAddonScope)), 
                      value("launcher", int(eWeaponAddonGrenadeLauncher)), 
                      value("silencer", int(eWeaponAddonSilencer)), 
                      value("laser", int(eWeaponAddonLaser)), 
                      value("flashlight", int(eWeaponAddonFlashlight)), 
                      value("stock", int(eWeaponAddonStock)), 
                      value("extender", int(eWeaponAddonExtender)), 
                      value("forend", int(eWeaponAddonForend))
                  ]
    ];
}

void CSE_ALifeItemWeaponShotGun::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemWeaponShotGun, "cse_alife_item_weapon_shotgun", CSE_ALifeItemWeapon)]; }

void CSE_ALifeItemDetector::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemDetector, "cse_alife_item_detector", CSE_ALifeItem)]; }

void CSE_ALifeItemArtefact::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemArtefact, "cse_alife_item_artefact", CSE_ALifeItem)]; }
