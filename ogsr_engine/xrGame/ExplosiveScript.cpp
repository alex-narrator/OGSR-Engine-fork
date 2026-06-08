#include "stdafx.h"
#include "Explosive.h"

using namespace luabind;


void CExplosive::script_register(lua_State* L) { 
	module(L)[
		class_<CExplosive>("explosive")
			.def("explode", (&CExplosive::Explode))
			.def("ready_to_explode", (&CExplosive::IsReadyToExplode))
            .def("exploded", (&CExplosive::IsExploded))
            .def("exploding", (&CExplosive::IsExploding))
            .def("initiator", (&CExplosive::Initiator))
            .def("initiator", (&CExplosive::SetInitiator))
	];
}
