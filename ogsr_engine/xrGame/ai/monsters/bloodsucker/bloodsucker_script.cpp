#include "stdafx.h"
#include "bloodsucker.h"

#include "luabind/luabind.hpp"
using namespace luabind;


void CAI_Bloodsucker::script_register(lua_State* L)
{
    module(L)[class_<CAI_Bloodsucker, CGameObject>("CAI_Bloodsucker").def(constructor<>()).def("force_visibility_state", &CAI_Bloodsucker::force_visibility_state)];
}
