#include "stdafx.h"
#include "script_light.h"

using namespace luabind;

#pragma optimize("s", on)
void CScriptLight::script_register(lua_State* L)
{
    module(L)[class_<CScriptLight>("script_light")
                  .def(constructor<>())
                  .property("enabled", &CScriptLight::IsEnabled, &CScriptLight::Enable)
                  .def("set_position", (void(CScriptLight::*)(Fvector)) & CScriptLight::SetPosition)
                  .def("set_position", (void(CScriptLight::*)(float, float, float)) & CScriptLight::SetPosition)
                  .def("set_direction", (void(CScriptLight::*)(Fvector))(&CScriptLight::SetDirection))
                  .def("set_direction", (void(CScriptLight::*)(float, float, float)) & CScriptLight::SetDirection)
                  .def("set_direction", (void(CScriptLight::*)(Fvector, Fvector))(&CScriptLight::SetDirection))
                  .def("set_angle", &CScriptLight::SetAngle)
                  .def("update", &CScriptLight::Update)
                  .def("set_rgb", &CScriptLight::SetRGB)
                  .def("set_animation", &CScriptLight::SetAnimation)
                  .def("set_color", &CScriptLight::SetColor)
                  .def("set_texture", &CScriptLight::SetTexture)
                  .def("set_type", &CScriptLight::SetType)
                  .def("set_range", &CScriptLight::SetRange)
                  .def("set_shadow", &CScriptLight::SetShadow)
                  .def("set_brightness", &CScriptLight::SetBrightness)
                  .def("set_virtual_size", &CScriptLight::SetVirtualSize)
                  .def("set_volumetric", &CScriptLight::SetVolumetric)
                  .def("set_volumetric_quality", &CScriptLight::SetVolumetricQuality)
                  .def("set_volumetric_distance", &CScriptLight::SetVolumetricDistance)
                  .def("set_volumetric_intensity",&CScriptLight::SetVolumetricIntensity)
                  .def("set_hud_mode", &CScriptLight::SetHudMode)];
}