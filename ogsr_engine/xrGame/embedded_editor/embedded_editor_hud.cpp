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
#include "../player_hud.h"
#include "../WeaponMagazined.h"
#include "../Inventory.h"
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
    auto Wpn = smart_cast<CWeaponMagazined*>(Actor()->inventory().ActiveItem());

    static float drag_intensity = 0.0001f;

    ImGui::DragFloat("Drag Intensity", &drag_intensity, 0.000001f, 0.000001f, 1.0f, "%.6f");

    if (item)
    {
        if (showSeparator)
            ImGui::Separator();
        ImGui::Text("Item 0: %s", item->m_parent_hud_item->object().cNameSect().c_str());
        ImGui::Text("hud section: %s", item->m_parent_hud_item->HudSection().c_str());

        if (showSeparator)
            ImGui::Separator();
        ImGui::DragFloat3("hands_position",				(float*)&item->m_measures.m_hands_attach[0],		drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("hands_orientation",			(float*)&item->m_measures.m_hands_attach[1],		drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("item_position",				(float*)&item->m_measures.m_item_attach[0],			drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("item_orientation",           (float*)&item->m_measures.m_item_attach[1],			drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("aim_hud_offset_pos",			(float*)&item->m_measures.m_hands_offset[0][1],		drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("aim_hud_offset_rot",			(float*)&item->m_measures.m_hands_offset[1][1],		drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("gl_hud_offset_pos",			(float*)&item->m_measures.m_hands_offset[0][2],		drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("gl_hud_offset_rot",			(float*)&item->m_measures.m_hands_offset[1][2],		drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("fire_point",					(float*)&item->m_measures.m_fire_point_offset[0],	drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("fire_point2",                (float*)&item->m_measures.m_fire_point2_offset[0],	drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("shell_point",                (float*)&item->m_measures.m_shell_point_offset[0],	drag_intensity, NULL, NULL, "%.6f");

        if (showSeparator)
            ImGui::Separator();
        ImGui::DragFloat3("custom_ui_pos",              (float*)&item->m_parent_hud_item->script_ui_offset[0], drag_intensity, NULL, NULL, "%.6f");
        ImGui::DragFloat3("custom_ui_rot",              (float*)&item->m_parent_hud_item->script_ui_offset[1], drag_intensity, NULL, NULL, "%.6f");

        if (Wpn)
        {
            auto& render = Level().debug_renderer();
            // Laser light offsets
            if (Wpn->IsAddonAttached(eLaser) && Wpn->IsLaserOn())
            {
                if (showSeparator)
                    ImGui::Separator();                
                ImGui::DragFloat3("laserdot_attach_offset",             (float*)&Wpn->laserdot_hud_attach_offset, drag_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3("laserdot_aim_attach_offset",         (float*)&Wpn->laserdot_aim_hud_attach_offset, drag_intensity, NULL, NULL, "%.6f");
                
                render.draw_aabb(Wpn->laser_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(125, 0, 0));
            }

            // Flashlight offsets
            if ((Wpn->IsAddonAttached(eFlashlight) || Wpn->laser_flashlight) && Wpn->IsFlashlightOn())
            {
                if (showSeparator)
                    ImGui::Separator();                
                ImGui::DragFloat3("flashlight_attach_offset",           (float*)&Wpn->flashlight_hud_attach_offset, drag_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3("flashlight_aim_attach_offset",       (float*)&Wpn->flashlight_aim_hud_attach_offset, drag_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3("flashlight_omni_attach_offset",      (float*)&Wpn->flashlight_omni_hud_attach_offset, drag_intensity, NULL, NULL, "%.6f");
                ImGui::DragFloat3("flashlight_aim_omni_attach_offset",  (float*)&Wpn->flashlight_aim_omni_hud_attach_offset, drag_intensity, NULL, NULL, "%.6f");

                render.draw_aabb(Wpn->flashlight_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(0, 56, 125));
            }

            for (int i = 0; i < eMaxAddon; ++i)
            {
                if (Wpn->hud_attach_visual[i])
                {
                    if (showSeparator)
                        ImGui::Separator();
                    ImGui::DragFloat3((std::string(Wpn->hud_attach_addon_name[i]) + "_attach_pos").c_str(), (float*)&Wpn->hud_attach_visual_offset[i][0], drag_intensity, NULL, NULL,"%.6f");
                    ImGui::DragFloat3((std::string(Wpn->hud_attach_addon_name[i]) + "_attach_rot").c_str(), (float*)&Wpn->hud_attach_visual_offset[i][1], drag_intensity, NULL, NULL, "%.6f");
                    ImGui::DragFloat3((std::string(Wpn->hud_attach_addon_name[i]) + "_attach_scale").c_str(), (float*)&Wpn->hud_attach_visual_scale[i], drag_intensity, NULL, NULL, "%.6f");
                }
            }
        }
    }

    auto item_1 = g_player_hud->attached_item(1);

    if (item_1)
    {
        if (showSeparator)
            ImGui::Separator();
        ImGui::Text("Item 1: %s", item_1->m_parent_hud_item->object().cNameSect().c_str());
        ImGui::Text("hud section: %s", item_1->m_parent_hud_item->HudSection().c_str());

        if (showSeparator)
            ImGui::Separator();
        ImGui::DragFloat3("hands_position",		(float*)&item_1->m_measures.m_hands_attach[0][0],		drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("hands_orientation",	(float*)&item_1->m_measures.m_hands_attach[1][0],		drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("item_position",		(float*)&item_1->m_measures.m_item_attach[0],			drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("item_orientation",   (float*)&item_1->m_measures.m_item_attach[1],			drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("fire_point",			(float*)&item_1->m_measures.m_fire_point_offset[0],	    drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("fire_point2",        (float*)&item_1->m_measures.m_fire_point2_offset[0],	drag_intensity, NULL, NULL, "%.6f");
		ImGui::DragFloat3("shell_point",        (float*)&item_1->m_measures.m_shell_point_offset[0],	drag_intensity, NULL, NULL, "%.6f");

        if (showSeparator)
            ImGui::Separator();
        ImGui::DragFloat3("custom_ui_pos",      (float*)&item_1->m_parent_hud_item->script_ui_offset[0], drag_intensity, NULL, NULL, "%.6f");
        ImGui::DragFloat3("custom_ui_rot",      (float*)&item_1->m_parent_hud_item->script_ui_offset[1], drag_intensity, NULL, NULL, "%.6f");
    }

    //if (ImGui::Button("Save"))
    //{
    //    // TODO ImGui fix

    //    // g_player_hud->SaveCfg(0);
    //    // g_player_hud->SaveCfg(1);
    //}
    if (ImGui::Button("Print to log"))
    {
        const bool is_16x9 = UI()->is_widescreen();
        if (item)
        {
            Log("####################################");
            Msg("[%s]", item->m_parent_hud_item->HudSection().c_str());
            auto pos = item->m_measures.m_hands_attach[0];
            Msg("hands_position%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);
            pos = item->m_measures.m_hands_attach[1];
            Msg("hands_orientation%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);

            pos = item->m_measures.m_item_attach[0];
            Msg("item_position%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);
            pos = item->m_measures.m_item_attach[1];
            Msg("item_orientation%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);

            pos = item->m_measures.m_hands_offset[0][1];
            Msg("aim_hud_offset_pos%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);
            pos = item->m_measures.m_hands_offset[1][1];
            Msg("aim_hud_offset_rot%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);

            pos = item->m_measures.m_hands_offset[0][2];
            Msg("gl_hud_offset_pos%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);
            pos = item->m_measures.m_hands_offset[1][2];
            Msg("gl_hud_offset_rot%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);

            pos = item->m_measures.m_fire_point_offset;
            Msg("fire_point = %g,%g,%g", pos.x, pos.y, pos.z);
            pos = item->m_measures.m_fire_point2_offset;
            Msg("fire_point2 = %g,%g,%g", pos.x, pos.y, pos.z);
            pos = item->m_measures.m_shell_point_offset;
            Msg("shell_point = %g,%g,%g", pos.x, pos.y, pos.z);

            pos = item->m_parent_hud_item->script_ui_offset[0];
            Msg("custom_ui_pos = %g,%g,%g", pos.x, pos.y, pos.z);
            pos = item->m_parent_hud_item->script_ui_offset[1];
            Msg("custom_ui_rot = %g,%g,%g", pos.x, pos.y, pos.z);

            if (Wpn)
            {
                if (Wpn->IsAddonAttached(eLaser))
                {
                    pos = Wpn->laserdot_hud_attach_offset;
                    Msg("laserdot_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
                    pos = Wpn->laserdot_aim_hud_attach_offset;
                    Msg("laserdot_aim_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
                }

                if (Wpn->IsAddonAttached(eFlashlight) || Wpn->laser_flashlight)
                {
                    pos = Wpn->flashlight_hud_attach_offset;
                    Msg("flashlight_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
                    pos = Wpn->flashlight_aim_hud_attach_offset;
                    Msg("flashlight_aim_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
                    pos = Wpn->flashlight_omni_hud_attach_offset;
                    Msg("flashlight_omni_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
                    pos = Wpn->flashlight_aim_omni_hud_attach_offset;
                    Msg("flashlight_aim_omni_attach_offset = %g,%g,%g", pos.x, pos.y, pos.z);
                }

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
            }
            Log("####################################");
        }

        if (item_1)
        {
            Log("####################################");
            Msg("[%s]", item_1->m_parent_hud_item->HudSection().c_str());
            auto pos = item_1->m_measures.m_hands_attach[0];
            Msg("hands_position%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);
            pos = item_1->m_measures.m_hands_attach[1];
            Msg("hands_orientation%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);

            pos = item_1->m_measures.m_item_attach[0];
            Msg("item_position%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);
            pos = item_1->m_measures.m_item_attach[1];
            Msg("item_orientation%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos.x, pos.y, pos.z);

            pos = item_1->m_measures.m_fire_point_offset;
            Msg("fire_point = %g,%g,%g", pos.x, pos.y, pos.z);
            pos = item_1->m_measures.m_fire_point2_offset;
            Msg("fire_point2 = %g,%g,%g", pos.x, pos.y, pos.z);
            pos = item_1->m_measures.m_shell_point_offset;
            Msg("shell_point = %g,%g,%g", pos.x, pos.y, pos.z);

            pos = item_1->m_parent_hud_item->script_ui_offset[0];
            Msg("custom_ui_pos = %g,%g,%g", pos.x, pos.y, pos.z);
            pos = item_1->m_parent_hud_item->script_ui_offset[1];
            Msg("custom_ui_rot = %g,%g,%g", pos.x, pos.y, pos.z);

            Log("####################################");
        }
    }
}
