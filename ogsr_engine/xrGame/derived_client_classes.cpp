////////////////////////////////////////////////////////////////////////////
//	Module 		: derived_client_classes.h
//	Created 	: 16.08.2014
//  Modified 	: 20.10.2014
//	Author		: Alexander Petrov
//	Description : XRay derived client classes script export
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "base_client_classes.h"
#include "derived_client_classes.h"
#include "exported_classes_def.h"
#include "script_game_object.h"

/* Декларация о стиле экспорта свойств и методов:
     * Свойства объектов экспортируются по возможности так, как они выглядят в файлах конфигурации (*.ltx), а не так как они названы в исходниках движка
     * Методы объектов экспортируются согласно стилю экспорта для game_object, т.е без использования прописных букв.
        Это позволяет сохранить единый стиль программирования в скриптах и отделить новые методы от исконно движковых версии 1.0006.
   Alexander Petrov
*/

using namespace luabind;


IC void alive_entity_set_radiation(CEntityAlive* E, float value) { E->SetfRadiation(value); }

void CEntityScript::script_register(lua_State* L)
{
    module(L)[class_<CEntity, CGameObject>("CEntity"),
              class_<CEntityAlive, CEntity>("CEntityAlive")
                  .property("radiation", &CEntityAlive::g_Radiation, &alive_entity_set_radiation) // доза в %
                  .property("condition", &CEntityAlive::conditions)];
}

void set_io_money(CInventoryOwner* IO, u32 money) { IO->set_money(money, true); }

CScriptGameObject* item_lua_object(PIItem itm)
{
    if (itm)
    {
        CGameObject* obj = smart_cast<CGameObject*>(itm);
        if (obj)
            return obj->lua_game_object();
    }
    return NULL;
}

CScriptGameObject* inventory_active_item(CInventory* I) { return item_lua_object(I->ActiveItem()); }

CScriptGameObject* get_inventory_target(CInventory* I) { return item_lua_object(I->m_pTarget); }

luabind::object get_slots(CInventoryItem* itm)
{
    auto table = luabind::newtable(ai().script_engine().lua());

    if (itm)
    {
        for (u32 i = 0; i < itm->GetSlotsCount(); i++)
        {
            table[i + 1] = itm->GetSlots()[i];
        }
    }

    return table;
}

void CInventoryScript::script_register(lua_State* L)
{
    module(L)[

        class_<CInventory>("CInventory")
            .def_readonly("max_belt", &CInventory::m_iMaxBelt)
            .def_readwrite("max_weight", &CInventory::m_fMaxWeight)
            .def_readwrite("take_dist", &CInventory::m_fTakeDist)
            .def_readonly("total_weight", &CInventory::m_fTotalWeight)
            .property("active_item", &inventory_active_item)
            .property("target", &get_inventory_target)
            .def("is_active_slot_blocked", &CInventory::IsActiveSlotBlocked)
            .def("is_slot_allowed", &CInventory::IsSlotAllowed)
            .property("prev_active_slot", &CInventory::GetPrevActiveSlot, &CInventory::SetPrevActiveSlot)
            .def_readwrite("max_belt", &CInventory::m_iMaxBelt),
        class_<IInventoryBox>("IInventoryBox")
            .def("object", &IInventoryBox::GetObjectByIndex)
            .def("object", &IInventoryBox::GetObjectByName)
            .def("object_count", &IInventoryBox::GetSize)
            .def("empty", &IInventoryBox::IsEmpty)
            .def_readwrite("in_use", &IInventoryBox::m_in_use),
        class_<CInventoryContainer, bases<IInventoryBox, CInventoryItemObject>>("CInventoryContainer"),

        class_<CInventoryOwner>("CInventoryOwner")
            .def_readonly("inventory", &CInventoryOwner::m_inventory)
            .def_readonly("talking", &CInventoryOwner::m_bTalking)
            .def_readwrite("allow_talk", &CInventoryOwner::m_bAllowTalk)
            .def_readwrite("allow_trade", &CInventoryOwner::m_bAllowTrade)
            .def_readwrite("raw_money", &CInventoryOwner::m_money)
            .property("money", &CInventoryOwner::get_money, &set_io_money)
            .def("Name", &CInventoryOwner::Name)
            .def("SetName", &CInventoryOwner::SetName)

    ];
}

CParticlesObject* monster_play_particles(CBaseMonster* monster, LPCSTR name, const Fvector& position, const Fvector& dir, BOOL auto_remove, BOOL xformed)
{
    return monster->PlayParticles(name, position, dir, auto_remove, xformed);
}

void CMonsterScript::script_register(lua_State* L)
{
    module(L)[class_<CBaseMonster, bases<CInventoryOwner, CEntityAlive>>("CBaseMonster")
                  .def_readwrite("agressive", &CBaseMonster::m_bAggressive)
                  .def_readwrite("angry", &CBaseMonster::m_bAngry)
                  .def_readwrite("damaged", &CBaseMonster::m_bDamaged)
                  .def_readwrite("grownlig", &CBaseMonster::m_bGrowling)
                  .def_readwrite("run_turn_left", &CBaseMonster::m_bRunTurnLeft)
                  .def_readwrite("run_turn_right", &CBaseMonster::m_bRunTurnRight)
                  .def_readwrite("sleep", &CBaseMonster::m_bSleep)
                  .def_readwrite("state_invisible", &CBaseMonster::state_invisible)];
}

int curr_fire_mode(CWeaponMagazined* wpn) { return wpn->GetCurrentFireMode(); }

#ifdef NLC_EXTENSIONS
extern void attach_upgrades(lua_State* L);
#endif

SRotation& CWeaponScript::FireDeviation(CWeapon* wpn) { return wpn->constDeviation; }

luabind::object CWeaponScript::get_fire_modes(CWeaponMagazined* wpn)
{
    lua_State* L = ai().script_engine().lua();
    luabind::object t = newtable(L);
    auto& vector = wpn->m_aFireModes;
    int index = 1;
    for (auto it = vector.begin(); it != vector.end(); ++it, ++index)
        t[index] = *it;

    return t;
}

void CWeaponScript::set_fire_modes(CWeaponMagazined* wpn, luabind::object const& t)
{
    if (LUA_TTABLE != t.type())
        return;
    auto& vector = wpn->m_aFireModes;
    vector.clear();
    for (auto it = t.begin(); it != t.end(); ++it)
    {
        int m = object_cast<int>(*it);
        vector.push_back(m);
    }
}

luabind::object CWeaponScript::get_hit_power(CWeapon* wpn)
{
    lua_State* L = ai().script_engine().lua();
    luabind::object t = newtable(L);
    auto& vector = wpn->fvHitPower;

    t[1] = vector.x;
    t[2] = vector.y;
    t[3] = vector.z;
    t[4] = vector.w;

    return t;
}

void CWeaponScript::set_hit_power(CWeapon* wpn, luabind::object const& t)
{
    if (LUA_TTABLE != t.type())
        return;
    auto& vector = wpn->fvHitPower;

    vector.x = object_cast<float>(t[1]);
    vector.y = object_cast<float>(t[2]);
    vector.z = object_cast<float>(t[3]);
    vector.w = object_cast<float>(t[4]);
}

LPCSTR get_addon_name(CWeapon* I, u32 addon) { return I->GetAddonName(addon).c_str(); }
LPCSTR get_ammo_sect(CWeapon* I) { return !I->m_magazine.empty() ? I->m_magazine.back().m_ammoSect.c_str() : nullptr; }

void CWeaponScript::script_register(lua_State* L)
{
#ifdef NLC_EXTENSIONS
    attach_upgrades(L);
#endif
    module(L)[class_<CWeapon, CInventoryItemObject>("CWeapon")
                  // из неэкспортируемого класса CHudItemObject:
                  .property("state", &CHudItemObject::GetState)
                  .property("next_state", &CHudItemObject::GetNextState)
                  // ============================================================================= //
                  // параметры отдачи влияющие на камеру
                  .def_readwrite("cam_max_angle", &CWeapon::camMaxAngle)
                  .def_readwrite("cam_relax_speed", &CWeapon::camRelaxSpeed)
                  .def_readwrite("cam_relax_speed_ai", &CWeapon::camRelaxSpeed_AI)
                  .def_readwrite("cam_dispersion", &CWeapon::camDispersion)
                  .def_readwrite("cam_dispersion_inc", &CWeapon::camDispersionInc)
                  .def_readwrite("cam_dispertion_frac", &CWeapon::camDispertionFrac)
                  .def_readwrite("cam_max_angle_horz", &CWeapon::camMaxAngleHorz)
                  .def_readwrite("cam_step_angle_horz", &CWeapon::camStepAngleHorz)

                  .def_readwrite("fire_dispersion_condition_factor", &CWeapon::fireDispersionConditionFactor)
                  .def_readwrite("misfire_probability", &CWeapon::misfireProbability)
                  .def_readwrite("misfire_condition_k", &CWeapon::misfireConditionK)
                  .def_readwrite("condition_shot_dec", &CWeapon::conditionDecreasePerShot)
                  .def_readwrite("condition_shot_dec_silencer", &CWeapon::conditionDecreasePerShotSilencer)

                  .def_readwrite("PDM_disp_base", &CWeapon::m_fPDM_disp_base)
                  .def_readwrite("PDM_disp_vel_factor", &CWeapon::m_fPDM_disp_vel_factor)
                  .def_readwrite("PDM_disp_accel_factor", &CWeapon::m_fPDM_disp_accel_factor)
                  .def_readwrite("PDM_crouch", &CWeapon::m_fPDM_disp_crouch)
                  .def_readwrite("PDM_crouch_no_acc", &CWeapon::m_fPDM_disp_crouch_no_acc)

                  .def_readwrite("hit_type", &CWeapon::m_eHitType)
                  .def_readwrite("hit_impulse", &CWeapon::fHitImpulse)
                  .def_readwrite("bullet_speed", &CWeapon::m_fStartBulletSpeed)
                  .def_readwrite("fire_distance", &CWeapon::fireDistance)
                  .def_readwrite("fire_dispersion_base", &CWeapon::fireDispersionBase)
                  .def_readwrite("time_to_aim", &CWeapon::m_fTimeToAim)
                  .def_readwrite("time_to_fire", &CWeapon::fTimeToFire)
                  .def_readwrite("use_aim_bullet", &CWeapon::m_bUseAimBullet)
                  .property("hit_power", &get_hit_power, &set_hit_power)

                  .def_readwrite("ammo_mag_size", &CWeapon::iMagazineSize)
                  .def_readwrite("scope_dynamic_zoom", &CWeapon::m_bScopeDynamicZoom)
                  .def_readwrite("zoom_enabled", &CWeapon::m_bZoomEnabled)
                  .def_readwrite("zoom_factor", &CWeapon::m_fZoomFactor)
                  .def_readwrite("zoom_rotate_time", &CWeapon::m_fZoomRotateTime)
                  .def_readwrite("iron_sight_zoom_factor", &CWeapon::m_fIronSightZoomFactor)
                  .def_readwrite("scope_zoom_factor", &CWeapon::m_fScopeZoomFactor)
                  .def_readonly("zoom_rotation_factor", &CWeapon::m_fZoomRotationFactor)

                  .def_readwrite("scope_zoom_hud_fov", &CWeapon::m_fZoomHudFov)
                  .def_readwrite("scope_lense_hud_fov", &CWeapon::m_f3dssHudFov)

                  .def_readonly("zoom_mode", &CWeapon::m_bZoomMode)

                  .def_readwrite("scope_inertion_factor", &CWeapon::m_fScopeInertionFactor)

                  .def("is_3dss_enabled", &CWeapon::Is3dssEnabled)

                  .property("ammo_elapsed", &CWeapon::GetAmmoElapsed, &CWeapon::SetAmmoElapsed)
                  .property("const_deviation", &CWeaponScript::FireDeviation) // отклонение при стрельбе от целика (для непристрелляного оружия).
                  .def("get_ammo_current", &CWeapon::GetAmmoCurrent)
                  //.def("load_config"						,			&CWeapon::Load)
                  .def("start_fire", &CWeapon::FireStart)
                  .def("stop_fire", &CWeapon::FireEnd)
                  .def("start_fire2", &CWeapon::Fire2Start) // огонь ножом - правой кнопкой? )
                  .def("stop_fire2", &CWeapon::Fire2End)
                  .def("stop_shoothing", &CWeapon::StopShooting)
                  .def("get_particles_xform", &CWeapon::get_ParticlesXFORM)
                  .def("get_fire_point", &CWeapon::get_CurrentFirePoint)
                  .def("get_fire_point2", &CWeapon::get_CurrentFirePoint2)
                  .def("get_fire_direction", &CWeapon::get_LastFD)
                  .def("ready_to_kill", &CWeapon::ready_to_kill)
        
                  .def("is_misfire", &CWeapon::IsMisfire)
                  .def("is_addon_attached", &CWeapon::IsAddonAttached)
                  .def("addon_attachable", &CWeapon::AddonAttachable)
                  .def("get_addon_name", &get_addon_name)
                  .def("get_ammo_sect", &get_ammo_sect)
                  .def("aim_alt_mode", &CWeapon::IsAimAltMode)
                  .def("try_to_get_ammo", &CWeapon::TryToGetAmmo)
                  .def("get_next_ammo_type", &CWeapon::GetNextAmmoType)
                  .def_readwrite("next_ammo_type", &CWeapon::m_set_next_ammoType_on_reload),

              class_<CWeaponMagazined, CWeapon>("CWeaponMagazined")
                  .def_readonly("shot_num", &CWeaponMagazined::m_iShotNum)
                  .def_readwrite("queue_size", &CWeaponMagazined::m_iQueueSize)
                  .def_readwrite("shoot_effector_start", &CWeaponMagazined::m_iShootEffectorStart)
                  .def_readwrite("cur_fire_mode", &CWeaponMagazined::m_iCurFireMode)
                  .property("fire_mode", &curr_fire_mode)
                  .property("fire_modes", &get_fire_modes, &set_fire_modes)
                  .def("attach_addon", &CWeaponMagazined::Attach)
                  .def("detach_addon", &CWeaponMagazined::Detach)
                  .def("can_attach_addon", &CWeaponMagazined::CanAttach)
                  .def("can_detach_addon", &CWeaponMagazined::CanDetach)
        
                  .def("respawn_weapon", &CWeaponMagazined::RespawnWeapon)
                  .def("firemod_string", &CWeaponMagazined::GetCurrentFireModeStr)
                  // laser
                  .def("set_laser_range", &CWeaponMagazined::SetLaserRange)
                  .def("set_laser_angle", &CWeaponMagazined::SetLaserAngle)
                  .def("set_laser_rgb", &CWeaponMagazined::SetLaserRGB)
                  .def("set_laser_on", &CWeaponMagazined::SetLaserOn)
                  .def("is_laser_on", &CWeaponMagazined::IsLaserOn)
                  .def("switch_laser", &CWeaponMagazined::SwitchLaser)
                  // flashlight
                  .def("set_flashlight_range", &CWeaponMagazined::SetFlashlightRange)
                  .def("set_flashlight_angle", &CWeaponMagazined::SetFlashlightAngle)
                  .def("set_flashlight_rgb", &CWeaponMagazined::SetFlashlightRGB)
                  .def("set_flashlight_on", &CWeaponMagazined::SetFlashlightOn)
                  .def("is_flashlight_on", &CWeaponMagazined::IsFlashlightOn)
                  .def("switch_flashlight", &CWeaponMagazined::SwitchFlashlight)
                  .def_readwrite("shell_drop_delay", &CWeaponMagazined::m_fShellDropDelay),

              class_<CWeaponMagazinedWGrenade, CWeaponMagazined>("CWeaponMagazinedWGrenade")
                  .def_readwrite("gren_mag_size", &CWeaponMagazinedWGrenade::iMagazineSize2)
                  .def("switch_gl", &CWeaponMagazinedWGrenade::SwitchMode),

              class_<enum_exporter<eWeaponAddonType>>("addon").enum_("addon")[
                  value("silencer", int(eSilencer)), value("scope", int(eScope)), value("launcher", int(eLauncher)), 
                  value("laser", int(eLaser)), value("flashlight", int(eFlashlight)), value("stock", int(eStock)), 
                  value("extender", int(eExtender)), value("forend", int(eForend)), value("magazine", int(eMagazine)), 
                  value("max", int(eMaxAddon))]];
}

void CCustomMonsterScript::script_register(lua_State* L)
{
    module(L)[class_<CCustomMonster, bases<CEntityAlive>>("CCustomMonster")
                  .def("get_dest_vertex_id", &CCustomMonsterScript::GetDestVertexId)
                  .def_readwrite("visible_for_zones", &CCustomMonster::m_visible_for_zones)
                  .def("anomaly_detector", &CCustomMonster::anomaly_detector)];
}
