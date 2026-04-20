////////////////////////////////////////////////////////////////////////////
//	Module 		: embedded_editor_hud.cpp
//	Created 	: 05.05.2021
//  Modified 	: 07.07.2025
//	Author		: Dance Maniac (M.F.S. Team)
//	Description : ImGui Hud Editor
////////////////////////////////////////////////////////////////////////////

#include "stdAfx.h"
#include "embedded_editor_hud.h"
#include "embedded_editor_helper.h"
#include "../../XR_3DA/device.h"
#include "player_hud.h"
#include "WeaponMagazined.h"
#include "Inventory.h"
#include "EliteDetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "debug_renderer.h"
#include "HUDManager.h"

void ShowHudEditor(bool& show)
{
    ImguiWnd wnd("HUD Editor", &show);
    if (wnd.Collapsed)
        return;

    if (!g_player_hud)
        return;

    bool showSeparator = true;
    auto item = g_player_hud->attached_item(0);

    static float drag_pos_intensity = 0.0001f;
    static float drag_rot_intensity = 0.0001f;

    ImGui::DragFloat("Drag Pos Intensity", &drag_pos_intensity, 0.000001f, 0.000001f, 1.0f, "%.6f");
    ImGui::DragFloat("Drag Rot Intensity", &drag_rot_intensity, 0.000001f, 0.000001f, 1.0f, "%.6f");
    if (showSeparator)
        ImGui::Separator();

    const auto normal_idx = hud_item_measures::m_hands_offset_type_normal;
    const auto aim_idx = hud_item_measures::m_hands_offset_type_aim;
    const auto aim_alt_idx = hud_item_measures::m_hands_offset_type_aim_alt;
    const auto aim_sight_idx = hud_item_measures::m_hands_offset_type_aim_alt_sight;
    const auto aim_scope_idx = hud_item_measures::m_hands_offset_type_aim_scope;
    const auto aim_gl_idx = hud_item_measures::m_hands_offset_type_gl;
    const auto aim_gl_scope_idx = hud_item_measures::m_hands_offset_type_gl_scope;

    if (item)
    {
        ImGui::Text("Item 0: %s", item->m_parent_hud_item->object().cNameSect().c_str());
        ImGui::Text("hud section: %s", item->m_parent_hud_item->HudSection().c_str());
        if (showSeparator)
            ImGui::Separator();

		ImGui::DragFloat3("item_position", (float*)&item->m_measures.m_item_attach[0], drag_pos_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("item_orientation", (float*)&item->m_measures.m_item_attach[1], drag_rot_intensity, NULL, NULL, "%.6f");
        if (showSeparator)
            ImGui::Separator();
		ImGui::DragFloat3("fire_point", (float*)&item->m_measures.m_fire_point_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("fire_point2", (float*)&item->m_measures.m_fire_point2_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("shell_point", (float*)&item->m_measures.m_shell_point_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
        if (showSeparator)
            ImGui::Separator();

        const auto idx = item->m_parent_hud_item->GetCurrentHudOffsetIdx();
        switch (idx)
        {
        case normal_idx:
            ImGui::DragFloat3("hands_position", (float*)&item->m_measures.m_hands_attach[0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("hands_orientation", (float*)&item->m_measures.m_hands_attach[1], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        case aim_idx:
            ImGui::DragFloat3("aim_hud_offset_pos", (float*)&item->m_measures.m_hands_offset[0][aim_idx], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("aim_hud_offset_rot", (float*)&item->m_measures.m_hands_offset[1][aim_idx], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        case aim_alt_idx:
            ImGui::DragFloat3("aim_alt_hud_offset_pos", (float*)&item->m_measures.m_hands_offset[0][aim_alt_idx], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("aim_alt_hud_offset_rot", (float*)&item->m_measures.m_hands_offset[1][aim_alt_idx], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        case aim_sight_idx:
            ImGui::DragFloat3("aim_alt_sight_hud_offset_pos", (float*)&item->m_measures.m_hands_offset[0][aim_sight_idx], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("aim_alt_sight_hud_offset_rot", (float*)&item->m_measures.m_hands_offset[1][aim_sight_idx], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        case aim_scope_idx:
            ImGui::DragFloat3("aim_scope_hud_offset_pos", (float*)&item->m_measures.m_hands_offset[0][aim_scope_idx], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("aim_scope_hud_offset_rot", (float*)&item->m_measures.m_hands_offset[1][aim_scope_idx], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        case aim_gl_idx:
            ImGui::DragFloat3("gl_hud_offset_pos", (float*)&item->m_measures.m_hands_offset[0][aim_gl_idx], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("gl_hud_offset_rot", (float*)&item->m_measures.m_hands_offset[1][aim_gl_idx], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        case aim_gl_scope_idx:
            ImGui::DragFloat3("gl_scope_hud_offset_pos", (float*)&item->m_measures.m_hands_offset[0][aim_gl_scope_idx], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("gl_scope_hud_offset_rot", (float*)&item->m_measures.m_hands_offset[1][aim_gl_scope_idx], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        }
        if (showSeparator)
            ImGui::Separator();

        ImGui::DragFloat3("custom_ui_pos", (float*)&item->m_parent_hud_item->script_ui_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
        ImGui::DragFloat3("custom_ui_rot", (float*)&item->m_parent_hud_item->script_ui_offset[1], drag_rot_intensity, NULL, NULL, "%.6f");
        if (showSeparator)
            ImGui::Separator();

        if (const auto Wpn = smart_cast<CWeaponMagazined*>(item->m_parent_hud_item))
        {
            auto& render = Level().debug_renderer();
            // Laser light offsets
            if (Wpn->IsAddonAttached(eLaser) && Wpn->IsLaserOn())
            {
                if (Wpn->IsAiming())
                    ImGui::DragFloat3("laserdot_aim_attach_offset", (float*)&Wpn->laserdot_aim_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                else
                    ImGui::DragFloat3("laserdot_attach_offset", (float*)&Wpn->laserdot_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                if (showSeparator)
                    ImGui::Separator();
                
                render.draw_aabb(Wpn->laser_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(125, 0, 0));
            }

            // Flashlight offsets
            if ((Wpn->IsAddonAttached(eFlashlight) || Wpn->laser_flashlight) && Wpn->IsFlashlightOn())
            {
                if (Wpn->IsAiming())
                {
                    ImGui::DragFloat3("flashlight_aim_attach_offset", (float*)&Wpn->flashlight_aim_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                    ImGui::DragFloat3("flashlight_aim_omni_attach_offset", (float*)&Wpn->flashlight_aim_omni_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                }
                else
                {
                    ImGui::DragFloat3("flashlight_attach_offset", (float*)&Wpn->flashlight_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                    ImGui::DragFloat3("flashlight_omni_attach_offset", (float*)&Wpn->flashlight_omni_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                }
                if (showSeparator)
                    ImGui::Separator();

                render.draw_aabb(Wpn->flashlight_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(0, 56, 125));
            }

            for (int i = 0; i < eMaxAddon; ++i)
            {
                if (Wpn->hud_attach_visual[i])
                {
                    const auto addon_name = std::string(Wpn->hud_attach_addon_name[i]);
                    ImGui::DragFloat3((addon_name + "_attach_pos").c_str(), (float*)&Wpn->hud_attach_visual_offset[i][0], drag_pos_intensity, NULL, NULL, "%.6f");
                    ImGui::DragFloat3((addon_name + "_attach_rot").c_str(), (float*)&Wpn->hud_attach_visual_offset[i][1], drag_rot_intensity, NULL, NULL, "%.6f");
                    ImGui::DragFloat((addon_name + "_attach_scale").c_str(), (float*)&Wpn->hud_attach_visual_scale[i], drag_pos_intensity, NULL, NULL, "%.6f");
                    if (showSeparator)
                        ImGui::Separator();
                }
            }
        }
    }

    auto item_1 = g_player_hud->attached_item(1);

    if (item_1)
    {
        ImGui::Text("Item 1: %s", item_1->m_parent_hud_item->object().cNameSect().c_str());
        ImGui::Text("hud section: %s", item_1->m_parent_hud_item->HudSection().c_str());
        if (showSeparator)
            ImGui::Separator();

		ImGui::DragFloat3("item_position 1", (float*)&item_1->m_measures.m_item_attach[0], drag_pos_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("item_orientation 1", (float*)&item_1->m_measures.m_item_attach[1], drag_rot_intensity, NULL, NULL, "%.6f");
        if (showSeparator)
            ImGui::Separator();

        ImGui::DragFloat3("fire_point 1", (float*)&item_1->m_measures.m_fire_point_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
        ImGui::DragFloat3("fire_point2 1", (float*)&item_1->m_measures.m_fire_point2_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
        ImGui::DragFloat3("shell_point 1", (float*)&item_1->m_measures.m_shell_point_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
        if (showSeparator)
            ImGui::Separator();

        const auto idx_1 = item_1->m_parent_hud_item->GetCurrentHudOffsetIdx();
        switch (idx_1)
        {
        case normal_idx:
            ImGui::DragFloat3("hands_position 1", (float*)&item_1->m_measures.m_hands_attach[0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("hands_orientation 1", (float*)&item_1->m_measures.m_hands_attach[1], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        case aim_idx:
            ImGui::DragFloat3("aim_hud_offset_pos 1", (float*)&item_1->m_measures.m_hands_offset[0][aim_idx], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("aim_hud_offset_rot 1", (float*)&item_1->m_measures.m_hands_offset[1][aim_idx], drag_rot_intensity, NULL, NULL, "%.6f");
            break;
        }
        if (showSeparator)
            ImGui::Separator();

        ImGui::DragFloat3("custom_ui_pos 1", (float*)&item_1->m_parent_hud_item->script_ui_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
        ImGui::DragFloat3("custom_ui_rot 1", (float*)&item_1->m_parent_hud_item->script_ui_offset[1], drag_rot_intensity, NULL, NULL, "%.6f");
        if (showSeparator)
            ImGui::Separator();

        if (const auto Det = smart_cast<CEliteDetector*>(item_1->m_parent_hud_item))
        {
            ImGui::DragFloat3("ui_pos", (float*)&Det->GetUI()->m_map_attach_offset_pos, drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3("ui_rot", (float*)&Det->GetUI()->m_map_attach_offset_rot, drag_rot_intensity, NULL, NULL, "%.6f");
            if (showSeparator)
                ImGui::Separator();
        }
    }

    if (ImGui::Button("Save to file"))
    {
        // TODO ImGui fix
        g_player_hud->SaveCfg(0);
        g_player_hud->SaveCfg(1);
    }
}
