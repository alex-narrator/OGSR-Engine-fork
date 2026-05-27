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

void CImGuiWorldEditorWnd::Render()
{
    if (!RenderBegin())
    {
        RenderEnd();
        return;
    }

    auto Wpn = smart_cast<CWeaponMagazined*>(Actor()->inventory().ActiveItem());

    static float drag_pos_intensity = 0.0001f;
    static float drag_rot_intensity = 0.0001f;

    ImGui::DragFloat("Drag Pos Intensity", &drag_pos_intensity, 0.000001f, 0.000001f, 1.0f, "%.6f");
    ImGui::DragFloat("Drag Rot Intensity", &drag_rot_intensity, 0.000001f, 0.000001f, 1.0f, "%.6f");
    ImGui::Separator();

    if (!Wpn)
    {
        RenderEnd();
        return;
    }

    ImGui::Text("section: %s", Wpn->cNameSect().c_str());
    auto& render = Level().debug_renderer();
    // Laser light offsets
    if (Wpn->IsAddonAttached(eLaser) && Wpn->IsLaserOn())
    {
        ImGui::DragFloat3("laserdot_attach_offset", (float*)&Wpn->laserdot_world_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
        render.draw_aabb(Wpn->laser_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(125, 0, 0));
        ImGui::Separator();
    }

    // Flashlight offsets
    if ((Wpn->IsAddonAttached(eFlashlight) || Wpn->laser_flashlight) && Wpn->IsFlashlightOn())
    {
        ImGui::DragFloat3("flashlight_attach_offset", (float*)&Wpn->flashlight_world_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
        ImGui::DragFloat3("flashlight_omni_attach_offset", (float*)&Wpn->flashlight_omni_world_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
        render.draw_aabb(Wpn->flashlight_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(0, 56, 125));
        ImGui::Separator();
    }

    for (int i = 0; i < eMaxAddon; ++i)
    {
        if (Wpn->world_attach_visual[i])
        {
            const auto addon_name = std::string(Wpn->world_attach_addon_name[i]);
            ImGui::DragFloat3((addon_name + "_attach_pos").c_str(), (float*)&Wpn->world_attach_visual_offset[i][0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3((addon_name + "_attach_rot").c_str(), (float*)&Wpn->world_attach_visual_offset[i][1], drag_rot_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat((addon_name + "_attach_scale").c_str(), (float*)&Wpn->world_attach_visual_scale[i], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::Separator();
        }
    }

    if (ImGui::Button("Save to file"))
    {
        Wpn->SaveCfg();
    }

    RenderEnd();
}