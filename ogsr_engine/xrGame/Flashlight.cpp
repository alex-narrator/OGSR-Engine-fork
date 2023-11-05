#include "stdafx.h"
#include "Flashlight.h"
#include "../xr_3da/LightAnimLibrary.h"
#include "Actor.h"
#include "player_hud.h"

CFlashlight::CFlashlight()
{
    light_render = ::Render->light_create();
    light_render->set_type(IRender_Light::SPOT);
    light_render->set_shadow(true);
    light_omni = ::Render->light_create();
    light_omni->set_type(IRender_Light::POINT);
    light_omni->set_shadow(false);

    glow_render = ::Render->glow_create();
}

CFlashlight::~CFlashlight()
{
    light_render.destroy();
    light_omni.destroy();
    glow_render.destroy();
}

void CFlashlight::Load(LPCSTR section)
{
    inherited::Load(section);
    LoadLightDefinitions(READ_IF_EXISTS(pSettings, r_string, section, "light_definition", nullptr));
}

void CFlashlight::LoadLightDefinitions(shared_str light_sect)
{
    if (!light_sect)
        return;

    bool b_r2 = !!psDeviceFlags.test(rsR2);
    b_r2 |= !!psDeviceFlags.test(rsR3);
    b_r2 |= !!psDeviceFlags.test(rsR4);

    lanim = LALib.FindItem(READ_IF_EXISTS(pSettings, r_string, light_sect, "color_animator", ""));

    m_color = pSettings->r_fcolor(light_sect, b_r2 ? "color_r2" : "color");
    fBrightness = m_color.intensity();
    m_fRange = pSettings->r_float(light_sect, (b_r2) ? "range_r2" : "range");
    light_render->set_color(m_color);
    light_render->set_range(m_fRange);

    if (b_r2)
    {
        useVolumetric = READ_IF_EXISTS(pSettings, r_bool, light_sect, "volumetric_enabled", false);
        useVolumetricForActor = READ_IF_EXISTS(pSettings, r_bool, light_sect, "volumetric_for_actor", false);
        light_render->set_volumetric(useVolumetric);
        if (useVolumetric)
        {
            float volQuality = READ_IF_EXISTS(pSettings, r_float, light_sect, "volumetric_quality", 1.0f);
            volQuality = std::clamp(volQuality, 0.f, 1.f);
            light_render->set_volumetric_quality(volQuality);

            float volIntensity = READ_IF_EXISTS(pSettings, r_float, light_sect, "volumetric_intensity", 0.15f);
            volIntensity = std::clamp(volIntensity, 0.f, 10.f);
            light_render->set_volumetric_intensity(volIntensity);

            float volDistance = READ_IF_EXISTS(pSettings, r_float, light_sect, "volumetric_distance", 0.45f);
            volDistance = std::clamp(volDistance, 0.f, 1.f);
            light_render->set_volumetric_distance(volDistance);
        }
    }

    Fcolor clr_o = pSettings->r_fcolor(light_sect, (b_r2) ? "omni_color_r2" : "omni_color");
    float range_o = pSettings->r_float(light_sect, (b_r2) ? "omni_range_r2" : "omni_range");
    light_omni->set_color(clr_o);
    light_omni->set_range(range_o);

    light_render->set_cone(deg2rad(pSettings->r_float(light_sect, "spot_angle")));
    light_render->set_texture(READ_IF_EXISTS(pSettings, r_string, light_sect, "spot_texture", nullptr));

    glow_render->set_texture(pSettings->r_string(light_sect, "glow_texture"));
    glow_render->set_color(m_color);
    glow_render->set_radius(pSettings->r_float(light_sect, "glow_radius"));
}

void CFlashlight::UpdateWork()
{
    if (HudItemData())
    {
        firedeps dep;
        HudItemData()->setup_firedeps(dep);

        light_render->set_position(dep.vLastFP);
        light_omni->set_position(dep.vLastFP);
        glow_render->set_position(dep.vLastFP);

        Fvector dir = dep.m_FireParticlesXForm.k;

        light_render->set_rotation(dir, dep.m_FireParticlesXForm.i);
        light_omni->set_rotation(dir, dep.m_FireParticlesXForm.i);
        glow_render->set_direction(dir);

        if (useVolumetric)
        {
            if (smart_cast<CActor*>(H_Parent()))
                light_render->set_volumetric(useVolumetricForActor);
            else
                light_render->set_volumetric(psActorFlags.test(AF_AI_VOLUMETRIC_LIGHTS));
        }
    }

    // calc color animator
    if (!lanim)
        return;

    int frame;

    u32 clr = lanim->CalculateBGR(Device.fTimeGlobal, frame);

    Fcolor fclr;
    fclr.set((float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f);
    fclr.mul_rgb(fBrightness);
    light_render->set_color(fclr);
    light_omni->set_color(fclr);
    glow_render->set_color(fclr);
}

void CFlashlight::Switch(bool turn_on)
{
    inherited::Switch(turn_on);

    light_render->set_active(turn_on);
    light_omni->set_active(turn_on);
    glow_render->set_active(turn_on);
}