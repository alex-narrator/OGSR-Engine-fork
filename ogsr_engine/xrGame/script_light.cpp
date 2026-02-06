#include "stdafx.h"
#include "script_light.h"
#include "../xr_3da/LightAnimLibrary.h"

CScriptLight::CScriptLight()
{
    light_render = ::Render->light_create();
    light_render->set_active(true);
}

CScriptLight::~CScriptLight() { light_render.destroy(); }

void CScriptLight::SetAnimation(LPCSTR name) { lanim = LALib.FindItem(name); }

void CScriptLight::SetPosition(Fvector pos) { light_render->set_position(pos); }
void CScriptLight::SetPosition(float x, float y, float z) { light_render->set_position(Fvector{x, y, z}); }

void CScriptLight::SetDirection(Fvector dir) { light_render->set_rotation(dir, Fvector().set(1, 0, 0)); }
void CScriptLight::SetDirection(float x, float y, float z) { light_render->set_rotation(Fvector{x, y, z}, Fvector().set(1, 0, 0)); }
void CScriptLight::SetDirection(Fvector dir, Fvector right) { light_render->set_rotation(dir, right); }

void CScriptLight::SetAngle(float angle) { light_render->set_cone(angle); }

void CScriptLight::Enable(bool state) { light_render->set_active(state); }
bool CScriptLight::IsEnabled() const { return light_render->get_active(); }

void CScriptLight::SetHudMode(bool b) { light_render->set_hud_mode(b); }

void CScriptLight::SetBrightness(float br)
{
    fBrightness = br;
    auto c = m_color;
    c.mul_rgb(fBrightness);
    light_render->set_color(c);
}

void CScriptLight::SetColor(Fcolor color)
{
    m_color = color;
    light_render->set_color(m_color);
}

void CScriptLight::SetRGB(float r, float g, float b)
{
    Fcolor c{};
    c.a = 1;
    c.r = r;
    c.g = g;
    c.b = b;
    SetColor(c);
}

void CScriptLight::SetShadow(bool state) { light_render->set_shadow(state); }

void CScriptLight::SetRange(float range) { light_render->set_range(range); }

void CScriptLight::SetType(int type) { light_render->set_type((IRender_Light::LT)type); }

void CScriptLight::SetTexture(LPCSTR texture) { light_render->set_texture(texture); }

void CScriptLight::SetVirtualSize(float size) { light_render->set_virtual_size(size); }

void CScriptLight::SetVolumetric(bool state) { light_render->set_volumetric(state); }

void CScriptLight::SetVolumetricDistance(float dist) { light_render->set_volumetric_distance(dist); }

void CScriptLight::SetVolumetricIntensity(float intensity) { light_render->set_volumetric_intensity(intensity); }

void CScriptLight::Update()
{
    if (lanim)
    {
        int frame;
        // возвращает в формате BGR
        u32 clr = lanim->CalculateBGR(Device.fTimeGlobal, frame);

        Fcolor fclr{};
        fclr.set((float)color_get_B(clr) / 255.f, (float)color_get_G(clr) / 255.f, (float)color_get_R(clr) / 255.f, 1.f);
        fclr.mul_rgb(fBrightness);

        light_render->set_color(fclr);
    }
}