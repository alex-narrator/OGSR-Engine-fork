#include "stdafx.h"
#include "Missile.h"

using namespace luabind;
#pragma optimize("s", on)
void CMissile::script_register(lua_State* L)
{
    module(L)[class_<CMissile, CGameObject>("CMissile")
                  .def(constructor<>())
                  .def_readwrite("destroy_time_max", &CMissile::m_dwDestroyTimeMax)
                  .def_readwrite("impact_fuze_time_max", &CMissile::m_dwImpactFuzeTimeMax)
                  .def_readwrite("impact_fuze", &CMissile::b_impact_fuze)
                  .def_readwrite("min_force", &CMissile::m_fMinForce)
                  .def_readwrite("max_force", &CMissile::m_fMaxForce)
                  .def_readwrite("const_force", &CMissile::m_fConstForce)
                  .def_readwrite("throw_force", &CMissile::m_fThrowForce)
                  .def_readonly("const_power", &CMissile::m_constpower)];
}