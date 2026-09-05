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


void CImGuiHudEditorWnd::Render()
{
    if (!g_player_hud)
        return;

    if (!RenderBegin())
    {
        RenderEnd();
        return;
    }

    static float drag_pos_intensity = 0.0001f;
    static float drag_rot_intensity = 0.0001f;

    ImGui::DragFloat("Drag Pos Intensity", &drag_pos_intensity, 0.000001f, 0.000001f, 1.0f, "%.6f");
    ImGui::DragFloat("Drag Rot Intensity", &drag_rot_intensity, 0.000001f, 0.000001f, 1.0f, "%.6f");
    ImGui::Separator();

    const auto normal_idx = hud_item_measures::m_hands_offset_type_normal;
    const auto aim_idx = hud_item_measures::m_hands_offset_type_aim;
    const auto aim_alt_idx = hud_item_measures::m_hands_offset_type_aim_alt;
    const auto aim_sight_idx = hud_item_measures::m_hands_offset_type_aim_alt_sight;
    const auto aim_scope_idx = hud_item_measures::m_hands_offset_type_aim_scope;
    const auto aim_gl_idx = hud_item_measures::m_hands_offset_type_gl;
    const auto aim_gl_scope_idx = hud_item_measures::m_hands_offset_type_gl_scope;

    for (u16 i = 0; i < 2; i++)
    {
        if (auto item = g_player_hud->attached_item(i))
        {
            ImGui::Text("[%d] item: %s", i, item->m_parent_hud_item->object().cNameSect().c_str());
            ImGui::Text("[%d] hud section: %s", i, item->m_parent_hud_item->HudSection().c_str());
            ImGui::Separator();

            auto label = [=](auto name) -> std::string {
                return std::format("[{}] {}", i, name);
            };

            ImGui::DragFloat3(label("item_position").c_str(), (float*)&item->m_measures.m_item_attach[0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3(label("item_orientation").c_str(), (float*)&item->m_measures.m_item_attach[1], drag_rot_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat(label("item_scale").c_str(), &item->m_measures.m_item_scale, drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::Separator();

            ImGui::DragFloat3(label("fire_point").c_str(), (float*)&item->m_measures.m_fire_point_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3(label("fire_point2").c_str(), (float*)&item->m_measures.m_fire_point2_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3(label("shell_point").c_str(), (float*)&item->m_measures.m_shell_point_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3(label("shoot_point").c_str(), (float*)&item->m_measures.m_shoot_point_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::Separator();

            const auto idx = item->m_parent_hud_item->GetCurrentHudOffsetIdx();
            switch (idx)
            {
            case normal_idx:
                ImGui::DragFloat3(label("hands_position").c_str(), (float*)&item->m_measures.m_hands_attach[0], drag_pos_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3(label("hands_orientation").c_str(), (float*)&item->m_measures.m_hands_attach[1], drag_rot_intensity, NULL, NULL, "%.6f");
                break;
            case aim_idx:
                ImGui::DragFloat3(label("aim_hud_offset_pos").c_str(), (float*)&item->m_measures.m_hands_offset[0][aim_idx], drag_pos_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3(label("aim_hud_offset_rot").c_str(), (float*)&item->m_measures.m_hands_offset[1][aim_idx], drag_rot_intensity, NULL, NULL, "%.6f");
                break;
            case aim_alt_idx:
                ImGui::DragFloat3(label("aim_alt_hud_offset_pos").c_str(), (float*)&item->m_measures.m_hands_offset[0][aim_alt_idx], drag_pos_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3(label("aim_alt_hud_offset_rot").c_str(), (float*)&item->m_measures.m_hands_offset[1][aim_alt_idx], drag_rot_intensity, NULL, NULL, "%.6f");
                break;
            case aim_sight_idx:
                ImGui::DragFloat3(label("aim_alt_sight_hud_offset_pos").c_str(), (float*)&item->m_measures.m_hands_offset[0][aim_sight_idx], drag_pos_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3(label("aim_alt_sight_hud_offset_rot").c_str(), (float*)&item->m_measures.m_hands_offset[1][aim_sight_idx], drag_rot_intensity, NULL, NULL, "%.6f");
                break;
            case aim_scope_idx:
                ImGui::DragFloat3(label("aim_scope_hud_offset_pos").c_str(), (float*)&item->m_measures.m_hands_offset[0][aim_scope_idx], drag_pos_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3(label("aim_scope_hud_offset_rot").c_str(), (float*)&item->m_measures.m_hands_offset[1][aim_scope_idx], drag_rot_intensity, NULL, NULL, "%.6f");
                break;
            case aim_gl_idx:
                ImGui::DragFloat3(label("gl_hud_offset_pos").c_str(), (float*)&item->m_measures.m_hands_offset[0][aim_gl_idx], drag_pos_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3(label("gl_hud_offset_rot").c_str(), (float*)&item->m_measures.m_hands_offset[1][aim_gl_idx], drag_rot_intensity, NULL, NULL, "%.6f");
                break;
            case aim_gl_scope_idx:
                ImGui::DragFloat3(label("gl_scope_hud_offset_pos").c_str(), (float*)&item->m_measures.m_hands_offset[0][aim_gl_scope_idx], drag_pos_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3(label("gl_scope_hud_offset_rot").c_str(), (float*)&item->m_measures.m_hands_offset[1][aim_gl_scope_idx], drag_rot_intensity, NULL, NULL, "%.6f");
                break;
            }
            ImGui::Separator();

            ImGui::DragFloat3(label("custom_ui_pos").c_str(), (float*)&item->m_parent_hud_item->script_ui_offset[0], drag_pos_intensity, NULL, NULL, "%.6f");
            ImGui::DragFloat3(label("custom_ui_rot").c_str(), (float*)&item->m_parent_hud_item->script_ui_offset[1], drag_rot_intensity, NULL, NULL, "%.6f");
            ImGui::Separator();

            if (const auto Wpn = smart_cast<CWeaponMagazined*>(item->m_parent_hud_item))
            {
                auto& render = Level().debug_renderer();
                // Laser light offsets
                if (Wpn->IsAddonAttached(eLaser) && Wpn->IsLaserOn())
                {
                    if (Wpn->IsAiming())
                    {
                        if (Wpn->IsAimAltMode())
                            ImGui::DragFloat3(label("laserdot_aim_alt_attach_offset").c_str(), (float*)&Wpn->laserdot_aim_alt_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                        else
                            ImGui::DragFloat3(label("laserdot_aim_attach_offset").c_str(), (float*)&Wpn->laserdot_aim_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                    }
                    else
                        ImGui::DragFloat3(label("laserdot_attach_offset").c_str(), (float*)&Wpn->laserdot_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                    ImGui::Separator();

                    render.draw_aabb(Wpn->laser_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(125, 0, 0));
                }

                // Flashlight offsets
                if ((Wpn->IsAddonAttached(eFlashlight) || Wpn->laser_flashlight) && Wpn->IsFlashlightOn())
                {
                    if (Wpn->IsAiming())
                    {
                        if (Wpn->IsAimAltMode())
                        {
                            ImGui::DragFloat3(label("flashlight_aim_alt_attach_offset").c_str(), (float*)&Wpn->flashlight_aim_alt_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                            ImGui::DragFloat3(label("flashlight_aim_alt_omni_attach_offset").c_str(), (float*)&Wpn->flashlight_aim_alt_omni_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                        }
                        else
                        {
                            ImGui::DragFloat3(label("flashlight_aim_attach_offset").c_str(), (float*)&Wpn->flashlight_aim_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                            ImGui::DragFloat3(label("flashlight_aim_omni_attach_offset").c_str(), (float*)&Wpn->flashlight_aim_omni_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                        }
                    }
                    else
                    {
                        ImGui::DragFloat3(label("flashlight_attach_offset").c_str(), (float*)&Wpn->flashlight_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                        ImGui::DragFloat3(label("flashlight_omni_attach_offset").c_str(), (float*)&Wpn->flashlight_omni_hud_attach_offset, drag_pos_intensity, NULL, NULL, "%.6f");
                    }
                    ImGui::Separator();

                    render.draw_aabb(Wpn->flashlight_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(0, 56, 125));
                }

                for (int i = 0; i < eMaxAddon; ++i)
                {
                    if (Wpn->hud_attach_visual[i])
                    {
                        const auto addon_name = std::string(Wpn->hud_attach_addon_name[i]);
                        ImGui::DragFloat3(label(addon_name + "_attach_pos").c_str(), (float*)&Wpn->hud_attach_visual_offset[i][0], drag_pos_intensity, NULL, NULL, "%.6f");
                        ImGui::DragFloat3(label(addon_name + "_attach_rot").c_str(), (float*)&Wpn->hud_attach_visual_offset[i][1], drag_rot_intensity, NULL, NULL, "%.6f");
                        ImGui::DragFloat(label(addon_name + "_attach_scale").c_str(), (float*)&Wpn->hud_attach_visual_scale[i], drag_pos_intensity, NULL, NULL, "%.6f");
                        ImGui::Separator();
                    }
                }
            }
        }

    }

    if (ImGui::Button("Save to file"))
    {
        for (u16 i = 0; i < 2; i++)
            g_player_hud->SaveCfg(i);
    }

    RenderEnd();
}
