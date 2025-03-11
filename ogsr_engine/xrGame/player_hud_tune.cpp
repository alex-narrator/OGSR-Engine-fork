#include "stdafx.h"
#include "player_hud.h"
#include "level.h"
#include "debug_renderer.h"
#include "../xr_3da/xr_input.h"
#include "HudManager.h"
#include "HudItem.h"
#include "WeaponMagazined.h"
#include <array>

enum HUD_ADJUST_MODE : int
{
    OFF,
    HUD_POS,
    HUD_ROT,
    ITM_POS,
    ITM_ROT,
    FIRE_POINT,
    FIRE_POINT2,
    SHELL_POINT,
    ADJUST_DELTA_POS,
    ADJUST_DELTA_ROT,
    LASETDOT_POS,
    FLASHLIGHT_POS,
    SCRIPT_UI_POS,
    SCRIPT_UI_ROT,
    HUD_ADDON_ATTACH_POS,
    HUD_ADDON_ATTACH_ROT,
    WORLD_ADDON_ATTACH_POS,
    WORLD_ADDON_ATTACH_ROT,
    HUD_ADDON_ATTACH_SCALE,
    WORLD_ADDON_ATTACH_SCALE,
    _HUD_ADJUST_MODES_COUNT_
};

static constexpr std::array<std::tuple<int, const char*>, _HUD_ADJUST_MODES_COUNT_> ADJUST_MODES_DB{{
    {DIK_NUMPAD0, ""},
    {DIK_NUMPAD1, "adjusting HUD POSITION"},
    {DIK_NUMPAD2, "adjusting HUD ROTATION"},
    {DIK_NUMPAD3, "adjusting ITEM POSITION"},
    {DIK_NUMPAD4, "adjusting ITEM ROTATION"},
    {DIK_NUMPAD5, "adjusting FIRE POINT"},
    {DIK_NUMPAD6, "adjusting FIRE POINT 2"},
    {DIK_NUMPAD7, "adjusting SHELL POINT"},
    {DIK_NUMPAD8, "adjusting pos STEP"},
    {DIK_NUMPAD9, "adjusting rot STEP"},
    {DIK_1, "adjusting LASER POINT"},
    {DIK_2, "adjusting FLASHLIGHT POINT"},
    {DIK_3, "adjusting SCRIPT UI POSITION"},
    {DIK_4, "adjusting SCRIPT UI ROTATION"},
    {DIK_5, "adjusting HUD ADDON ATTACH POSITION"},
    {DIK_6, "adjusting HUD ADDON ATTACH ROTATION"},
    {DIK_7, "adjusting WORLD ADDON ATTACH POSITION"},
    {DIK_8, "adjusting WORLD ADDON ATTACH ROTATION"},
    {DIK_9, "adjusting HUD ADDON ATTACH SCALE"},
    {DIK_0, "adjusting WORLD ADDON ATTACH SCALE"},
}};

int g_bHudAdjustMode = OFF;
int g_bHudAdjustItemIdx = 0;
float g_bHudAdjustDeltaPos = 0.0005f;
float g_bHudAdjustDeltaRot = 0.05f;

static bool is_attachable_item_tuning_mode()
{
    return pInput->iGetAsyncKeyState(DIK_LSHIFT) || pInput->iGetAsyncKeyState(DIK_Z) || pInput->iGetAsyncKeyState(DIK_X) || pInput->iGetAsyncKeyState(DIK_C);
}

static void tune_remap(const Ivector& in_values, Ivector& out_values)
{
    if (pInput->iGetAsyncKeyState(DIK_LSHIFT))
    {
        out_values = in_values;
    }
    else if (pInput->iGetAsyncKeyState(DIK_Z))
    { // strict by X
        out_values.x = in_values.y;
        out_values.y = 0;
        out_values.z = 0;
    }
    else if (pInput->iGetAsyncKeyState(DIK_X))
    { // strict by Y
        out_values.x = 0;
        out_values.y = in_values.y;
        out_values.z = 0;
    }
    else if (pInput->iGetAsyncKeyState(DIK_C))
    { // strict by Z
        out_values.x = 0;
        out_values.y = 0;
        out_values.z = in_values.y;
    }
    else
    {
        out_values.set(0, 0, 0);
    }
}

static void calc_cam_diff_pos(const Fmatrix& item_transform, const Fvector& diff, Fvector& res)
{
    Fmatrix cam_m{};
    cam_m.i.set(Device.vCameraRight);
    cam_m.j.set(Device.vCameraTop);
    cam_m.k.set(Device.vCameraDirection);
    cam_m.c.set(Device.vCameraPosition);

    Fvector res1;
    cam_m.transform_dir(res1, diff);

    Fmatrix item_transform_i{};
    item_transform_i.invert(item_transform);
    item_transform_i.transform_dir(res, res1);
}

void attachable_hud_item::tune(const Ivector& values)
{
    if (!is_attachable_item_tuning_mode())
        return;

    Fvector diff{};

    if (g_bHudAdjustMode == ITM_POS || g_bHudAdjustMode == ITM_ROT)
    {
        if (g_bHudAdjustMode == ITM_POS)
        {
            if (values.x)
                diff.x = (values.x > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
            if (values.y)
                diff.y = (values.y > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
            if (values.z)
                diff.z = (values.z > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;

            Fvector d;
            Fmatrix ancor_m;
            m_parent->calc_transform(m_attach_place_idx, Fidentity, ancor_m);
            calc_cam_diff_pos(ancor_m, diff, d);
            m_measures.m_item_attach[0].add(d);
        }
        else if (g_bHudAdjustMode == ITM_ROT)
        {
            if (values.x)
                diff.x = (values.x > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;
            if (values.y)
                diff.y = (values.y > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;
            if (values.z)
                diff.z = (values.z > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;

            Fvector d;
            Fmatrix ancor_m;
            m_parent->calc_transform(m_attach_place_idx, Fidentity, ancor_m);

            calc_cam_diff_pos(m_item_transform, diff, d);
            m_measures.m_item_attach[1].add(d);
        }
    }

    if (g_bHudAdjustMode == SCRIPT_UI_POS || g_bHudAdjustMode == SCRIPT_UI_ROT)
    {
        if (g_bHudAdjustMode == SCRIPT_UI_POS)
        {
            if (values.x)
                diff.x = (values.x > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
            if (values.y)
                diff.y = (values.y > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
            if (values.z)
                diff.z = (values.z > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;

            m_parent_hud_item->script_ui_offset[0].add(diff);
        }
        else if (g_bHudAdjustMode == SCRIPT_UI_ROT)
        {
            if (values.x)
                diff.x = (values.x > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;
            if (values.y)
                diff.y = (values.y > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;
            if (values.z)
                diff.z = (values.z > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;

            m_parent_hud_item->script_ui_offset[1].add(diff);
        }
    }

    if (g_bHudAdjustMode == HUD_ADDON_ATTACH_POS || g_bHudAdjustMode == HUD_ADDON_ATTACH_ROT || 
        g_bHudAdjustMode == WORLD_ADDON_ATTACH_POS || g_bHudAdjustMode == WORLD_ADDON_ATTACH_ROT || 
        g_bHudAdjustMode == HUD_ADDON_ATTACH_SCALE || g_bHudAdjustMode == WORLD_ADDON_ATTACH_SCALE)
    {
        if (g_bHudAdjustMode == HUD_ADDON_ATTACH_POS || g_bHudAdjustMode == WORLD_ADDON_ATTACH_POS || g_bHudAdjustMode == HUD_ADDON_ATTACH_SCALE || g_bHudAdjustMode == WORLD_ADDON_ATTACH_SCALE)
        {
            if (values.x)
                diff.x = (values.x > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
            if (values.y)
                diff.y = (values.y > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
            if (values.z)
                diff.z = (values.z > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;

            if (auto Wpn = smart_cast<CWeapon*>(m_parent_hud_item))
            {
                for (int i = 0; i < eMaxAddon; ++i)
                {
                    if (Wpn->hud_attach_visual[i])
                    {
                        if (g_bHudAdjustMode == HUD_ADDON_ATTACH_POS)
                            &Wpn->hud_attach_visual_offset[i][0].add(diff);
                        if (g_bHudAdjustMode == HUD_ADDON_ATTACH_SCALE)
                            Wpn->hud_attach_visual_scale[i] += diff.x;
                    }
                    if (Wpn->world_attach_visual[i])
                    {
                        if (g_bHudAdjustMode == WORLD_ADDON_ATTACH_POS)
                            Wpn->world_attach_visual_offset[i][0].add(diff);
                        if (g_bHudAdjustMode == WORLD_ADDON_ATTACH_SCALE)
                            Wpn->world_attach_visual_scale[i] += diff.x;
                    }
                }
            }
        }
        else if (g_bHudAdjustMode == HUD_ADDON_ATTACH_ROT || g_bHudAdjustMode == WORLD_ADDON_ATTACH_ROT)
        {
            if (values.x)
                diff.x = (values.x > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;
            if (values.y)
                diff.y = (values.y > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;
            if (values.z)
                diff.z = (values.z > 0) ? g_bHudAdjustDeltaRot : -g_bHudAdjustDeltaRot;

            if (auto Wpn = smart_cast<CWeapon*>(m_parent_hud_item))
            {
                for (int i = 0; i < eMaxAddon; ++i)
                {
                    if (g_bHudAdjustMode == HUD_ADDON_ATTACH_ROT && Wpn->hud_attach_visual[i])
                    {
                        &Wpn->hud_attach_visual_offset[i][1].add(diff);
                    }
                    if (g_bHudAdjustMode == WORLD_ADDON_ATTACH_ROT && Wpn->world_attach_visual[i])
                    {
                        Wpn->world_attach_visual_offset[i][1].add(diff);
                    }
                }
            }
        }
    }

    if (g_bHudAdjustMode == FIRE_POINT || g_bHudAdjustMode == FIRE_POINT2 || g_bHudAdjustMode == SHELL_POINT || g_bHudAdjustMode == LASETDOT_POS ||
        g_bHudAdjustMode == FLASHLIGHT_POS)
    {
        if (values.x)
            diff.x = (values.x > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
        if (values.y)
            diff.y = (values.y > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
        if (values.z)
            diff.z = (values.z > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;

        if (g_bHudAdjustMode == FIRE_POINT)
            m_measures.m_fire_point_offset.add(diff);
        else if (g_bHudAdjustMode == FIRE_POINT2)
            m_measures.m_fire_point2_offset.add(diff);
        else if (g_bHudAdjustMode == SHELL_POINT)
            m_measures.m_shell_point_offset.add(diff);
        else if (g_bHudAdjustMode == LASETDOT_POS)
        {
            if (auto Wpn = smart_cast<CWeaponMagazined*>(m_parent_hud_item))
                Wpn->laserdot_attach_offset.add(diff);
        }
        else if (g_bHudAdjustMode == FLASHLIGHT_POS)
        {
            if (auto Wpn = smart_cast<CWeaponMagazined*>(m_parent_hud_item))
                Wpn->flashlight_attach_offset.add(diff);
        }
    }
}

void attachable_hud_item::debug_draw_firedeps()
{
    const bool bForce = g_bHudAdjustMode == ITM_POS || g_bHudAdjustMode == ITM_ROT;

    if (g_bHudAdjustMode == FIRE_POINT || g_bHudAdjustMode == FIRE_POINT2 || g_bHudAdjustMode == SHELL_POINT || g_bHudAdjustMode == LASETDOT_POS ||
        g_bHudAdjustMode == FLASHLIGHT_POS || bForce)
    {
        auto& render = Level().debug_renderer();

        firedeps fd;
        setup_firedeps(fd);

        if (g_bHudAdjustMode == FIRE_POINT || bForce)
        {
            render.draw_aabb(fd.vLastFP, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(255, 0, 0));
            render.draw_aabb(fd.vLastShootPoint, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(5, 107, 0));
        }
        else if (g_bHudAdjustMode == FIRE_POINT2)
        {
            render.draw_aabb(fd.vLastFP2, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(0, 0, 255));
        }
        else if (g_bHudAdjustMode == SHELL_POINT)
        {
            render.draw_aabb(fd.vLastSP, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(0, 255, 0));
        }
        else if (g_bHudAdjustMode == LASETDOT_POS)
        {
            if (auto Wpn = smart_cast<CWeaponMagazined*>(m_parent_hud_item))
                render.draw_aabb(Wpn->laser_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(125, 0, 0));
        }
        else if (g_bHudAdjustMode == FLASHLIGHT_POS)
        {
            if (auto Wpn = smart_cast<CWeaponMagazined*>(m_parent_hud_item))
                render.draw_aabb(Wpn->flashlight_pos, 0.01f, 0.01f, 0.01f, D3DCOLOR_XRGB(0, 56, 125));
        }
    }
}

void player_hud::tune(const Ivector& _values)
{
    Ivector values;
    tune_remap(_values, values);

    if (g_bHudAdjustMode == HUD_POS || g_bHudAdjustMode == HUD_ROT)
    {
        Fvector diff{};

        float _curr_dr = g_bHudAdjustDeltaRot;

        if (!m_attached_items[g_bHudAdjustItemIdx])
            return;

        const u8 idx = m_attached_items[g_bHudAdjustItemIdx]->m_parent_hud_item->GetCurrentHudOffsetIdx();
        if (idx)
            _curr_dr /= 20.0f;

        Fvector& pos_ = (idx != 0) ? m_attached_items[g_bHudAdjustItemIdx]->hands_offset_pos() : m_attached_items[g_bHudAdjustItemIdx]->hands_attach_pos();
        Fvector& rot_ = (idx != 0) ? m_attached_items[g_bHudAdjustItemIdx]->hands_offset_rot() : m_attached_items[g_bHudAdjustItemIdx]->hands_attach_rot();

        if (g_bHudAdjustMode == HUD_POS)
        {
            if (values.x)
                diff.x = (values.x > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
            if (values.y)
                diff.y = (values.y > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;
            if (values.z)
                diff.z = (values.z > 0) ? g_bHudAdjustDeltaPos : -g_bHudAdjustDeltaPos;

            pos_.add(diff);
        }
        else if (g_bHudAdjustMode == HUD_ROT)
        {
            if (values.x)
                diff.x = (values.x > 0) ? _curr_dr : -_curr_dr;
            if (values.y)
                diff.y = (values.y > 0) ? _curr_dr : -_curr_dr;
            if (values.z)
                diff.z = (values.z > 0) ? _curr_dr : -_curr_dr;

            rot_.add(diff);
        }
    }
    else if (g_bHudAdjustMode == ADJUST_DELTA_POS || g_bHudAdjustMode == ADJUST_DELTA_ROT)
    {
        if (g_bHudAdjustMode == ADJUST_DELTA_POS && (values.z))
            g_bHudAdjustDeltaPos += (values.z > 0) ? 0.001f : -0.001f;

        if (g_bHudAdjustMode == ADJUST_DELTA_ROT && (values.z))
            g_bHudAdjustDeltaRot += (values.z > 0) ? 0.1f : -0.1f;
    }
    else if (auto hi = m_attached_items[g_bHudAdjustItemIdx])
        hi->tune(values);
}

void player_hud::DumpParamsToLog() 
{
    const bool is_16x9 = UI()->is_widescreen();
    if (!m_attached_items[g_bHudAdjustItemIdx])
        return;
    auto hud_sect = m_attached_items[g_bHudAdjustItemIdx]->m_sect_name.c_str();

    if (g_bHudAdjustMode == HUD_POS || g_bHudAdjustMode == HUD_ROT)
    {
        const u8 idx = m_attached_items[g_bHudAdjustItemIdx]->m_parent_hud_item->GetCurrentHudOffsetIdx();
        Fvector& pos_ = (idx != 0) ? m_attached_items[g_bHudAdjustItemIdx]->hands_offset_pos() : m_attached_items[g_bHudAdjustItemIdx]->hands_attach_pos();
        Fvector& rot_ = (idx != 0) ? m_attached_items[g_bHudAdjustItemIdx]->hands_offset_rot() : m_attached_items[g_bHudAdjustItemIdx]->hands_attach_rot();

        if (idx == hud_item_measures::m_hands_offset_type_normal)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("hands_position%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
            Msg("hands_orientation%s = %g,%g,%g", is_16x9 ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
            Log("####################################");
        }
        else if (idx == hud_item_measures::m_hands_offset_type_aim)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("aim_hud_offset_pos%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
            Msg("aim_hud_offset_rot%s = %g,%g,%g", is_16x9 ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
            Log("####################################");
        }
        else if (idx == hud_item_measures::m_hands_offset_type_aim_alt)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("aim_alt_hud_offset_pos%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
            Msg("aim_alt_hud_offset_rot%s = %g,%g,%g", is_16x9 ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
            Log("####################################");
        }
        else if (idx == hud_item_measures::m_hands_offset_type_aim_alt_scope)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("aim_alt_scope_hud_offset_pos%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
            Msg("aim_alt_scope_hud_offset_rot%s = %g,%g,%g", is_16x9 ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
            Log("####################################");
        }
        else if (idx == hud_item_measures::m_hands_offset_type_aim_scope)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("aim_scope_hud_offset_pos%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
            Msg("aim_scope_hud_offset_rot%s = %g,%g,%g", is_16x9 ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
            Log("####################################");
        }
        else if (idx == hud_item_measures::m_hands_offset_type_gl)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("gl_hud_offset_pos%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
            Msg("gl_hud_offset_rot%s	 = %g,%g,%g", is_16x9 ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
            Log("####################################");
        }
        else if (idx == hud_item_measures::m_hands_offset_type_gl_scope)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("gl_scope_hud_offset_pos%s = %g,%g,%g", is_16x9 ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
            Msg("gl_scope_hud_offset_rot%s = %g,%g,%g", is_16x9 ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
            Log("####################################");
        }
    }
    else
    {
        auto& measures = m_attached_items[g_bHudAdjustItemIdx]->m_measures;
        if (g_bHudAdjustMode == ITM_POS || g_bHudAdjustMode == ITM_ROT)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("item_position = %g,%g,%g", measures.m_item_attach[0].x, measures.m_item_attach[0].y, measures.m_item_attach[0].z);
            Msg("item_orientation = %g,%g,%g", measures.m_item_attach[1].x, measures.m_item_attach[1].y, measures.m_item_attach[1].z);
            Log("####################################");
        }
        else if (g_bHudAdjustMode == SCRIPT_UI_POS || g_bHudAdjustMode == SCRIPT_UI_ROT)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            auto hi = m_attached_items[g_bHudAdjustItemIdx]->m_parent_hud_item;
            auto pos = hi->script_ui_offset[0];
            auto rot = hi->script_ui_offset[1];
            Msg("custom_ui_pos = %g,%g,%g", pos.x, pos.y, pos.z);
            Msg("custom_ui_rot = %g,%g,%g", rot.x, rot.y, rot.z);
            Log("####################################");
        }
        else if (g_bHudAdjustMode == HUD_ADDON_ATTACH_POS || g_bHudAdjustMode == HUD_ADDON_ATTACH_ROT || 
            g_bHudAdjustMode == WORLD_ADDON_ATTACH_POS || g_bHudAdjustMode == WORLD_ADDON_ATTACH_ROT || 
            g_bHudAdjustMode == HUD_ADDON_ATTACH_SCALE || g_bHudAdjustMode == WORLD_ADDON_ATTACH_SCALE)
        {
            if (auto Wpn = smart_cast<CWeapon*>(m_attached_items[g_bHudAdjustItemIdx]->m_parent_hud_item))
            {
                for (int i = 0; i < eMaxAddon; ++i)
                {
                    if ((g_bHudAdjustMode == HUD_ADDON_ATTACH_POS || g_bHudAdjustMode == HUD_ADDON_ATTACH_ROT || g_bHudAdjustMode == HUD_ADDON_ATTACH_SCALE) && Wpn->hud_attach_visual[i])
                    {
                        Log("####################################");
                        Msg("[%s]", hud_sect);
                        auto pos = Wpn->hud_attach_visual_offset[i][0];
                        auto rot = Wpn->hud_attach_visual_offset[i][1];
                        auto scale = Wpn->hud_attach_visual_scale[i];
                        Msg("%s_attach_pos = %g,%g,%g", Wpn->hud_attach_addon_name[i], pos.x, pos.y, pos.z);
                        Msg("%s_attach_rot = %g,%g,%g", Wpn->hud_attach_addon_name[i], rot.x, rot.y, rot.z);
                        Msg("%s_attach_scale = %g", Wpn->hud_attach_addon_name[i], scale);
                        Log("####################################");
                    }
                    if ((g_bHudAdjustMode == WORLD_ADDON_ATTACH_POS || g_bHudAdjustMode == WORLD_ADDON_ATTACH_ROT || g_bHudAdjustMode == WORLD_ADDON_ATTACH_SCALE) && Wpn->world_attach_visual[i])
                    {
                        Log("####################################");
                        Msg("[%s]", Wpn->cNameSect().c_str());
                        auto pos = Wpn->world_attach_visual_offset[i][0];
                        auto rot = Wpn->world_attach_visual_offset[i][1];
                        auto scale = Wpn->world_attach_visual_scale[i];
                        Msg("%s_attach_pos = %g,%g,%g", Wpn->world_attach_addon_name[i], pos.x, pos.y, pos.z);
                        Msg("%s_attach_rot = %g,%g,%g", Wpn->world_attach_addon_name[i], rot.x, rot.y, rot.z);
                        Msg("%s_attach_scale = %g", Wpn->hud_attach_addon_name[i], scale);
                        Log("####################################");
                    }
                }
            }
        }
        else if (g_bHudAdjustMode == FIRE_POINT || g_bHudAdjustMode == FIRE_POINT2 || g_bHudAdjustMode == SHELL_POINT || g_bHudAdjustMode == LASETDOT_POS ||
                 g_bHudAdjustMode == FLASHLIGHT_POS)
        {
            Log("####################################");
            Msg("[%s]", hud_sect);
            Msg("fire_point = %g,%g,%g", measures.m_fire_point_offset.x, measures.m_fire_point_offset.y, measures.m_fire_point_offset.z);
            Msg("fire_point2 = %g,%g,%g", measures.m_fire_point2_offset.x, measures.m_fire_point2_offset.y, measures.m_fire_point2_offset.z);
            Msg("shell_point = %g,%g,%g", measures.m_shell_point_offset.x, measures.m_shell_point_offset.y, measures.m_shell_point_offset.z);
            if (auto Wpn = smart_cast<CWeaponMagazined*>(m_attached_items[g_bHudAdjustItemIdx]->m_parent_hud_item))
            {
                Msg("laserdot_attach_offset = %g,%g,%g", Wpn->laserdot_attach_offset.x, Wpn->laserdot_attach_offset.y, Wpn->laserdot_attach_offset.z);
                Msg("torch_attach_offset = %g,%g,%g", Wpn->flashlight_attach_offset.x, Wpn->flashlight_attach_offset.y, Wpn->flashlight_attach_offset.z);
            }
            Log("####################################");
        }
    }
}

void hud_draw_adjust_mode()
{
    if (!g_bHudAdjustMode)
        return;

    const char* _text{};
    if (pInput->iGetAsyncKeyState(DIK_LSHIFT))
        _text =
            "press SHIFT+NUM 0-return|1-hud_pos|2-hud_rot|3-itm_pos|4-itm_rot|5-fire_point|6-fire_point2|7-shell_point|8-pos_step|9-rot_step    ||||||    press "
            "SHIFT+1-laser_point|2-flashlight_point|3-custom_ui_pos|4-custom_ui_rot|5-hud_addon_attach_pos|6-hud_addon_attach_rot|7-world_addon_attach_pos|8-world_addon_attach_rot|9-hud_addon_attach_scale|0-world_addon_attach_scale";
    else if (pInput->iGetAsyncKeyState(DIK_LCONTROL))
        _text = "press CTRL+NUM 0-item idx 1|1-item idx 2";
    else
        _text = std::get<1>(ADJUST_MODES_DB.at(g_bHudAdjustMode));

    if (_text)
    {
        CGameFont* F = UI()->Font()->pFontDI;
        F->SetAligment(CGameFont::alCenter);
        F->OutSetI(0.f, -0.8f);
        F->SetColor(D3DCOLOR_XRGB(125, 0, 0));
        F->OutNext(_text);
        F->OutNext("for item: [%d] [%s]", g_bHudAdjustItemIdx,
                   g_player_hud->attached_item(u16(g_bHudAdjustItemIdx)) ? g_player_hud->attached_item(u16(g_bHudAdjustItemIdx))->m_sect_name.c_str() : "NOT FOUND");
        F->OutNext("delta values: dP=[%f], dR=[%f]", g_bHudAdjustDeltaPos, g_bHudAdjustDeltaRot);
        F->OutNext("[Z]-x axis, [X]-y axis, [C]-z axis    ||||||    [<---LEFT/RIGHT--->]-x axis, [UP/DOWN]-y axis, [PageUP/PageDown]-z axis");

        F->OutSkip();
        F->OutNext("L.SHIFT + ENTER - dump to log");
    }
}

void hud_adjust_mode_keyb(int dik)
{
    if (!g_bHudAdjustMode) //Включать этот режим только через консоль
        return;

    if (pInput->iGetAsyncKeyState(DIK_LSHIFT) && pInput->iGetAsyncKeyState(DIK_RETURN))
    {
        g_player_hud->DumpParamsToLog();
        return;
    }

    if (pInput->iGetAsyncKeyState(DIK_LSHIFT))
    {
        int mode{};
        for (const auto& [key, str] : ADJUST_MODES_DB)
        {
            if (key == dik)
            {
                g_bHudAdjustMode = mode;
                return;
            }
            mode++;
        }
    }
    else if (pInput->iGetAsyncKeyState(DIK_LCONTROL))
    {
        if (dik == DIK_NUMPAD0)
            g_bHudAdjustItemIdx = 0;
        else if (dik == DIK_NUMPAD1)
            g_bHudAdjustItemIdx = 1;
    }
}