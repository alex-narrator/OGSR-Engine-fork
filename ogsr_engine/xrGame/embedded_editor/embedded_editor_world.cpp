////////////////////////////////////////////////////////////////////////////
//	Module 		: embedded_editor_world.cpp
//	Created 	: 05.11.2025
//  Modified 	: 05.11.2025
//	Author		: alex.narrator
//	Description : ImGui World Item Editor
////////////////////////////////////////////////////////////////////////////

#include "stdAfx.h"
#include "embedded_editor_world.h"
#include "embedded_editor_helper.h"
#include "../WeaponMagazined.h"
#include "../Inventory.h"
#include "debug_renderer.h"

void ShowWorldEditor(bool& show)
{
    ImguiWnd wnd("World Editor", &show);
    if (wnd.Collapsed)
        return;

    bool showSeparator = true;
    auto Wpn = smart_cast<CWeaponMagazined*>(Actor()->inventory().ActiveItem());

    static float drag_intensity = 0.0001f;

    ImGui::DragFloat("Drag Intensity", &drag_intensity, 0.000001f, 0.000001f, 1.0f, "%.6f");
    if (showSeparator)
        ImGui::Separator();

    if (!Wpn)
        return;

    ImGui::Text("section: %s", Wpn->cNameSect().c_str());
    auto& render = Level().debug_renderer();
    // Laser light offsets
    if (Wpn->IsAddonAttached(eLaser) && Wpn->IsLaserOn())
    {
        if (showSeparator)
            ImGui::Separator();
        ImGui::DragFloat3("laserdot_attach_offset", (float*)&Wpn->laserdot_world_attach_offset, drag_intensity, NULL, NULL, "%.6f");

        render.draw_aabb(Wpn->laser_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(125, 0, 0));
    }

    // Flashlight offsets
    if ((Wpn->IsAddonAttached(eFlashlight) || Wpn->laser_flashlight) && Wpn->IsFlashlightOn())
    {
        if (showSeparator)
            ImGui::Separator();
        ImGui::DragFloat3("flashlight_attach_offset", (float*)&Wpn->flashlight_world_attach_offset, drag_intensity, NULL, NULL, "%.6f");
        ImGui::DragFloat3("flashlight_omni_attach_offset", (float*)&Wpn->flashlight_omni_world_attach_offset, drag_intensity, NULL, NULL, "%.6f");

        render.draw_aabb(Wpn->flashlight_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(0, 56, 125));
    }

    for (int i = 0; i < eMaxAddon; ++i)
    {
        if (Wpn->world_attach_visual[i])
        {
            if (showSeparator)
                ImGui::Separator();
            ImGui::DragFloat3((std::string(Wpn->world_attach_addon_name[i]) + "attach_pos").c_str(), (float*)&Wpn->world_attach_visual_offset[i][0], drag_intensity, NULL, NULL,
                              "%.6f");
            ImGui::DragFloat3((std::string(Wpn->world_attach_addon_name[i]) + "_attach_rot").c_str(), (float*)&Wpn->world_attach_visual_offset[i][1], drag_intensity, NULL, NULL,
                              "%.6f");
            ImGui::DragFloat3((std::string(Wpn->world_attach_addon_name[i]) + "_attach_scale").c_str(), (float*)&Wpn->world_attach_visual_scale[i], drag_intensity, NULL, NULL,
                              "%.6f");
        }
    }

    /*if (ImGui::Button("Save"))*/
    if (ImGui::Button("Print to log"))
    {
        Log("####################################");
        Msg("[%s]", Wpn->cNameSect().c_str());
        auto pos = Wpn->laserdot_world_attach_offset;
        Msg("laserdot_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
        pos = Wpn->flashlight_world_attach_offset;
        Msg("flashlight_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
        pos = Wpn->flashlight_omni_world_attach_offset;
        Msg("flashlight_omni_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
        for (int i = 0; i < eMaxAddon; ++i)
        {
            if (Wpn->world_attach_visual[i])
            {
                pos = Wpn->world_attach_visual_offset[i][0];
                Msg("%s_attach_pos = %g,%g,%g", Wpn->world_attach_addon_name[i], pos.x, pos.y, pos.z);
                pos = Wpn->world_attach_visual_offset[i][1];
                Msg("%s_attach_rot = %g,%g,%g", Wpn->world_attach_addon_name[i], pos.x, pos.y, pos.z);
                Msg("%s_attach_scale = %g", Wpn->world_attach_addon_name[i], Wpn->world_attach_visual_scale[i]);
            }
        }
        Log("####################################");
    }
}