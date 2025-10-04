#include "stdafx.h"
#include "Flashlight.h"
#include "script_game_object.h"
#include "inventory_item_object.h"
#include "../xr_3da/LightAnimLibrary.h"

IRender_Light* CFlashlight::GetLight(int target) const
{
    if (target == 0)
        return light_render._get();
    else if (light_omni)
        return light_omni._get();

    return nullptr;
}

void CFlashlight::SetAnimation(LPCSTR name) { lanim = LALib.FindItem(name); }

void CFlashlight::SetBrightness(float brightness)
{
    fBrightness = brightness;
    auto c = m_color;
    c.mul_rgb(fBrightness);
    light_render->set_color(c);
}

void CFlashlight::SetColor(const Fcolor& color, int target)
{
    switch (target)
    {
    case 0:
        m_color = color;
        light_render->set_color(m_color);
        break;
    case 1:
        if (light_omni)
            light_omni->set_color(color);
        break;
    }
}

void CFlashlight::SetRGB(float r, float g, float b, int target)
{
    Fcolor c;
    c.a = 1;
    c.r = r;
    c.g = g;
    c.b = b;
    SetColor(c, target);
}

void CFlashlight::SetAngle(float angle, int target)
{
    switch (target)
    {
    case 0: light_render->set_cone(angle); break;
    case 1:
        if (light_omni)
            light_omni->set_cone(angle);
        break;
    }
}

void CFlashlight::SetRange(float range, int target)
{
    switch (target)
    {
    case 0: {
        light_render->set_range(range);
        break;
    }
    case 1:
        if (light_omni)
            light_omni->set_range(range);
        break;
    }
}

void CFlashlight::SetTexture(LPCSTR texture, int target)
{
    switch (target)
    {
    case 0: light_render->set_texture(texture); break;
    case 1:
        if (light_omni)
            light_omni->set_texture(texture);
        break;
    }
}
void CFlashlight::SetVirtualSize(float size, int target)
{
    switch (target)
    {
    case 0: light_render->set_virtual_size(size);
    case 1:
        if (light_omni)
            light_omni->set_virtual_size(size);
    }
}

using namespace luabind;
#pragma optimize("s", on)
void CFlashlight::script_register(lua_State* L)
{
    module(L)[class_<CFlashlight, CGameObject>("CFlashlight")
                  .def(constructor<>())
                  // alpet: управление параметрами света
                  .def_readonly("on", &CFlashlight::m_bWorking)
                  .def("enable", (void(CFlashlight::*)(bool))(&CFlashlight::Switch))
                  .def("switch", (void(CFlashlight::*)())(&CFlashlight::Switch))
                  .def("get_light", &CFlashlight::GetLight)
                  .def("set_animation", &CFlashlight::SetAnimation)
                  .def("set_angle", &CFlashlight::SetAngle)
                  .def("set_brightness", &CFlashlight::SetBrightness)
                  .def("set_color", &CFlashlight::SetColor)
                  .def("set_rgb", &CFlashlight::SetRGB)
                  .def("set_range", &CFlashlight::SetRange)
                  .def("set_texture", &CFlashlight::SetTexture)
                  .def("set_virtual_size", &CFlashlight::SetVirtualSize)];
}