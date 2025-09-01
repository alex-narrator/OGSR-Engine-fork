#include "stdafx.h"
#include "space_restrictor.h"

Fvector get_restrictor_center(CSpaceRestrictor* SR)
{
    Fvector result;
    SR->Center(result);
    return result;
}

using namespace luabind;
#pragma optimize("s", on)
void CSpaceRestrictor::script_register(lua_State* L)
{
    module(L)[class_<CSpaceRestrictor, CGameObject>("CSpaceRestrictor")
                  .def(constructor<>())
                  .property("restrictor_center", &get_restrictor_center)
                  .property("restrictor_type", &CSpaceRestrictor::restrictor_type)
                  .property("radius", &CSpaceRestrictor::Radius)
                  .def("schedule_register", &CSpaceRestrictor::ScheduleRegister)
                  .def("schedule_unregister", &CSpaceRestrictor::ScheduleUnregister)
                  .def("is_scheduled", &CSpaceRestrictor::IsScheduled)
                  .def("active_contact", &CSpaceRestrictor::active_contact)];
}