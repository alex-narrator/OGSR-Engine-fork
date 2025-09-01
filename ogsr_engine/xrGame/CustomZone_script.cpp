#include "stdafx.h"
#include "CustomZone.h"

using namespace luabind;
#pragma optimize("s", on)
u32 get_zone_state(CCustomZone* obj) { return (u32)obj->ZoneState(); }
void set_zone_state(CCustomZone* obj, u32 new_state) { obj->SetZoneState((CCustomZone::EZoneState)new_state); }

void CCustomZone::script_register(lua_State* L)
{
    module(L)[class_<CCustomZone, CSpaceRestrictor>("CustomZone")
                  //.def  ("get_state_time"						,				&CCustomZone::GetStateTime)
                  .def("power", &CCustomZone::Power)
                  .def("relative_power", &CCustomZone::RelativePower)

                  .def_readwrite("attenuation", &CCustomZone::m_fAttenuation)
                  .def_readwrite("effective_radius", &CCustomZone::m_fEffectiveRadius)
                  .def_readwrite("hit_impulse_scale", &CCustomZone::m_fHitImpulseScale)
                  .def_readwrite("max_power", &CCustomZone::m_fMaxPower)
                  .def_readwrite("state_time", &CCustomZone::m_iStateTime)
                  .def_readwrite("start_time", &CCustomZone::m_StartTime)
                  .def_readwrite("time_to_live", &CCustomZone::m_ttl)
                  .def_readwrite("zone_active", &CCustomZone::m_bZoneActive)
                  .property("zone_state", &get_zone_state, &set_zone_state)];
}