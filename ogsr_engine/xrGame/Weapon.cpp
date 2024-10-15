// Weapon.cpp: implementation of the CWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Weapon.h"
#include "ParticlesObject.h"
#include "entity_alive.h"
#include "player_hud.h"
#include "inventory_item_impl.h"

#include "inventory.h"
#include "xrserver_objects_alife_items.h"

#include "actor.h"
#include "actoreffector.h"
#include "level.h"

#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "clsid_game.h"
#include "object_broker.h"
#include "../xr_3da/LightAnimLibrary.h"
#include "game_object_space.h"
#include "script_game_object.h"

#include "string_table.h"

#include "WeaponMagazinedWGrenade.h"
#include "GamePersistent.h"

#include "HUDManager.h"
#include "UIGameCustom.h"
#include "Torch.h"

// constexpr auto ROTATION_TIME = 0.25f;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWeapon::CWeapon(LPCSTR name)
{
    SetState(eHidden);
    SetNextState(eHidden);

    SetPending(FALSE);
    m_flags.set(FUsingCondition, TRUE);

    m_Offset.identity();
    m_StrapOffset.identity();
}

CWeapon::~CWeapon()
{
    delete_data(m_scopes);
    delete_data(m_silencers);
    delete_data(m_glaunchers);
    delete_data(m_lasers);
    delete_data(m_flashlights);
    delete_data(m_stocks);
    delete_data(m_extenders);
    delete_data(m_forends);
    delete_data(m_magazines);
}

void CWeapon::UpdateXForm()
{
    if (Device.dwFrame != dwXF_Frame)
    {
        dwXF_Frame = Device.dwFrame;

        if (0 == H_Parent())
            return;

        // Get access to entity and its visual
        CEntityAlive* E = smart_cast<CEntityAlive*>(H_Parent());

        if (!E)
            return;

        const CInventoryOwner* parent = smart_cast<const CInventoryOwner*>(E);
        if (!parent || parent && parent->use_simplified_visual())
            return;

        if (parent->attached(this))
            return;

        R_ASSERT(E);
        IKinematics* V = smart_cast<IKinematics*>(E->Visual());
        VERIFY(V);

        // Get matrices
        int boneL{BI_NONE}, boneR{BI_NONE}, boneR2{BI_NONE};

        E->g_WeaponBones(boneL, boneR, boneR2);

        if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
            boneL = boneR2;

        // KRodin: видимо такое случается иногда у некоторых визуалов нпс. Например если создать нпс с визуалом монстра наверно.
        if (boneL == BI_NONE || boneR == BI_NONE)
            return;

        // от mortan:
        // https://www.gameru.net/forum/index.php?s=&showtopic=23443&view=findpost&p=1677678
        V->CalculateBones_Invalidate();
        V->CalculateBones(true); // V->CalculateBones	();
        Fmatrix& mL = V->LL_GetTransform(u16(boneL));
        Fmatrix& mR = V->LL_GetTransform(u16(boneR));
        // Calculate
        Fmatrix mRes{};
        Fvector R{}, D{}, N{};
        D.sub(mL.c, mR.c);

        if (fis_zero(D.magnitude()))
        {
            mRes.set(E->XFORM());
            mRes.c.set(mR.c);
        }
        else
        {
            D.normalize();
            R.crossproduct(mR.j, D);

            N.crossproduct(D, R);
            N.normalize();

            mRes.set(R, N, D, mR.c);
            mRes.mulA_43(E->XFORM());
        }

        UpdatePosition(mRes);
    }
}

void CWeapon::UpdateFireDependencies_internal()
{
    if (skip_updated_frame == Device.dwFrame || Device.dwFrame != dwFP_Frame)
    {
        dwFP_Frame = Device.dwFrame;

        UpdateXForm();

        if (GetHUDmode())
        {
            HudItemData()->setup_firedeps(m_current_firedeps);
            VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
        }
        else
        {
            // 3rd person or no parent
            Fmatrix& parent = XFORM();
            Fvector& fp = vLoadedFirePoint;
            Fvector& fp2 = vLoadedFirePoint2;
            Fvector& sp = vLoadedShellPoint;

            parent.transform_tiny(m_current_firedeps.vLastFP, fp);
            parent.transform_tiny(m_current_firedeps.vLastFP2, fp2);
            parent.transform_tiny(m_current_firedeps.vLastSP, sp);
            parent.transform_tiny(m_current_firedeps.vLastShootPoint, fp);

            m_current_firedeps.vLastFD.set(0.f, 0.f, 1.f);
            parent.transform_dir(m_current_firedeps.vLastFD);

            m_current_firedeps.m_FireParticlesXForm.set(parent);
            VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
        }
    }
}

void CWeapon::ForceUpdateFireParticles()
{
    if (!GetHUDmode())
    { // update particlesXFORM real bullet direction

        if (!H_Parent())
            return;

        Fvector p, d;
        smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p, d);

        Fmatrix _pxf{};
        _pxf.k = d;
        _pxf.i.crossproduct(Fvector().set(0.0f, 1.0f, 0.0f), _pxf.k);
        _pxf.j.crossproduct(_pxf.k, _pxf.i);
        _pxf.c = XFORM().c;

        m_current_firedeps.m_FireParticlesXForm.set(_pxf);
    }
}

constexpr const char* wpn_scope_def_bone = "wpn_scope";
constexpr const char* wpn_silencer_def_bone = "wpn_silencer";
constexpr const char* wpn_launcher_def_bone_shoc = "wpn_launcher";
constexpr const char* wpn_launcher_def_bone_cop = "wpn_grenade_launcher";

void CWeapon::Load(LPCSTR section)
{
    inherited::Load(section);
    CShootingObject::Load(section);

    if (pSettings->line_exist(section, "flame_particles_2"))
        m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");

    if (!m_bForcedParticlesHudMode)
        m_bParticlesHudMode = !!pSettings->line_exist(hud_sect, "item_visual");

#ifdef DEBUG
    {
        Fvector pos, ypr;
        pos = pSettings->r_fvector3(section, "position");
        ypr = pSettings->r_fvector3(section, "orientation");
        ypr.mul(PI / 180.f);

        m_Offset.setHPB(ypr.x, ypr.y, ypr.z);
        m_Offset.translate_over(pos);
    }

    m_StrapOffset = m_Offset;
    if (pSettings->line_exist(section, "strap_position") && pSettings->line_exist(section, "strap_orientation"))
    {
        Fvector pos, ypr;
        pos = pSettings->r_fvector3(section, "strap_position");
        ypr = pSettings->r_fvector3(section, "strap_orientation");
        ypr.mul(PI / 180.f);

        m_StrapOffset.setHPB(ypr.x, ypr.y, ypr.z);
        m_StrapOffset.translate_over(pos);
    }
#endif

    // load ammo classes
    m_ammoTypes.clear();
    LPCSTR S = pSettings->r_string(section, "ammo_class");
    if (S && S[0])
    {
        string128 _ammoItem;
        int count = _GetItemCount(S);
        for (int it = 0; it < count; ++it)
        {
            _GetItem(S, it, _ammoItem);
            m_ammoTypes.push_back(_ammoItem);
        }
    }

    iAmmoElapsed = pSettings->r_s32(section, "ammo_elapsed");
    iMagazineSize = pSettings->r_s32(section, "ammo_mag_size");

    ////////////////////////////////////////////////////
    // дисперсия стрельбы

    // подбрасывание камеры во время отдачи
    camMaxAngle = pSettings->r_float(section, "cam_max_angle");
    camMaxAngle = deg2rad(camMaxAngle);
    camRelaxSpeed = pSettings->r_float(section, "cam_relax_speed");
    camRelaxSpeed = deg2rad(camRelaxSpeed);
    if (pSettings->line_exist(section, "cam_relax_speed_ai"))
    {
        camRelaxSpeed_AI = pSettings->r_float(section, "cam_relax_speed_ai");
        camRelaxSpeed_AI = deg2rad(camRelaxSpeed_AI);
    }
    else
    {
        camRelaxSpeed_AI = camRelaxSpeed;
    }

    //	camDispersion		= pSettings->r_float		(section,"cam_dispersion"	);
    //	camDispersion		= deg2rad					(camDispersion);

    camMaxAngleHorz = pSettings->r_float(section, "cam_max_angle_horz");
    camMaxAngleHorz = deg2rad(camMaxAngleHorz);
    camStepAngleHorz = pSettings->r_float(section, "cam_step_angle_horz");
    camStepAngleHorz = deg2rad(camStepAngleHorz);
    camDispertionFrac = READ_IF_EXISTS(pSettings, r_float, section, "cam_dispertion_frac", 0.7f);
    //  [8/2/2005]
    // m_fParentDispersionModifier = READ_IF_EXISTS(pSettings, r_float, section, "parent_dispersion_modifier",1.0f);
    m_fPDM_disp_base = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_base", 1.0f);
    m_fPDM_disp_vel_factor = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_vel_factor", 1.0f);
    m_fPDM_disp_accel_factor = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_accel_factor", 1.0f);
    m_fPDM_disp_crouch = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch", 1.0f);
    m_fPDM_disp_crouch_no_acc = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch_no_acc", 1.0f);
    //  [8/2/2005]
    m_bCamRecoilCompensation = !!READ_IF_EXISTS(pSettings, r_bool, section, "cam_recoil_compensation", true);

    fireDispersionConditionFactor = pSettings->r_float(section, "fire_dispersion_condition_factor");
    misfireProbability = pSettings->r_float(section, "misfire_probability");
    misfireConditionK = READ_IF_EXISTS(pSettings, r_float, section, "misfire_condition_k", 1.0f);
    conditionDecreasePerShot = pSettings->r_float(section, "condition_shot_dec");
    conditionDecreasePerShotOnHit = READ_IF_EXISTS(pSettings, r_float, section, "condition_shot_dec_on_hit", 0.f);
    conditionDecreasePerShotSilencer = READ_IF_EXISTS(pSettings, r_float, section, "condition_shot_dec_silencer", conditionDecreasePerShot);

    vLoadedFirePoint = pSettings->r_fvector3(section, "fire_point");

    if (pSettings->line_exist(section, "fire_point2"))
        vLoadedFirePoint2 = pSettings->r_fvector3(section, "fire_point2");
    else
        vLoadedFirePoint2 = vLoadedFirePoint;

    m_fMinRadius = pSettings->r_float(section, "min_radius");
    m_fMaxRadius = pSettings->r_float(section, "max_radius");

    // информация о возможных апгрейдах и их визуализации в инвентаре
    m_eScopeStatus              = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_u8, section, "scope_status", 0);
    m_eSilencerStatus           = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_u8, section, "silencer_status", 0);
    m_eGrenadeLauncherStatus    = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_u8, section, "grenade_launcher_status", 0);
    m_eLaserStatus              = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_u8, section, "laser_status", 0);
    m_eFlashlightStatus         = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_u8, section, "flashlight_status", 0);
    m_eStockStatus              = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_u8, section, "stock_status", 0);
    m_eExtenderStatus           = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_u8, section, "extender_status", 0);
    m_eForendStatus             = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_u8, section, "forend_status", 0);

    m_bZoomEnabled          = READ_IF_EXISTS(pSettings, r_bool, section, "zoom_enabled", true);
    m_bScopeShowIndicators  = READ_IF_EXISTS(pSettings, r_bool, section, "scope_show_indicators", true);

    m_fZoomFactor = CurrentZoomFactor();

    LPCSTR str{};
    if (m_eScopeStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "scope_name"))
        {
            str = pSettings->r_string(section, "scope_name");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 scope_section;
                _GetItem(str, i, scope_section);
                m_scopes.push_back(scope_section);
            }
        }
    }
    if (m_eSilencerStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "silencer_name"))
        {
            str = pSettings->r_string(section, "silencer_name");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 silencer_section;
                _GetItem(str, i, silencer_section);
                m_silencers.push_back(silencer_section);
            }
        }
    }
    if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "grenade_launcher_name"))
        {
            str = pSettings->r_string(section, "grenade_launcher_name");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 glauncher_section;
                _GetItem(str, i, glauncher_section);
                m_glaunchers.push_back(glauncher_section);
            }
        }
    }
    if (m_eLaserStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "laser_name"))
        {
            str = pSettings->r_string(section, "laser_name");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 laser_section;
                _GetItem(str, i, laser_section);
                m_lasers.push_back(laser_section);
            }
        }
    }
    if (m_eFlashlightStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "flashlight_name"))
        {
            str = pSettings->r_string(section, "flashlight_name");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 flashlight_section;
                _GetItem(str, i, flashlight_section);
                m_flashlights.push_back(flashlight_section);
            }
        }
    }
    if (m_eStockStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "stock_name"))
        {
            str = pSettings->r_string(section, "stock_name");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 stock_section;
                _GetItem(str, i, stock_section);
                m_stocks.push_back(stock_section);
            }
        }
    }
    if (m_eExtenderStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "extender_name"))
        {
            str = pSettings->r_string(section, "extender_name");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 extender_section;
                _GetItem(str, i, extender_section);
                m_extenders.push_back(extender_section);
            }
        }
    }
    if (m_eForendStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "forend_name"))
        {
            str = pSettings->r_string(section, "forend_name");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 forend_section;
                _GetItem(str, i, forend_section);
                m_forends.push_back(forend_section);
            }
        }
    }
    if (pSettings->line_exist(section, "magazine_class"))
    {
        SetMagazineAttached(true);
        str = pSettings->r_string(section, "magazine_class");
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mag_section;
            _GetItem(str, i, mag_section);
            m_magazines.push_back(mag_section);
        }
    }

    // Кости мировой модели оружия
    if (pSettings->line_exist(section, "scope_bone"))
    {
        const char* S = pSettings->r_string(section, "scope_bone");
        if (S && strlen(S))
        {
            const int count = _GetItemCount(S);
            string128 _scope_bone{};
            for (int it = 0; it < count; ++it)
            {
                _GetItem(S, it, _scope_bone);
                m_sWpn_scope_bones.push_back(_scope_bone);
            }
        }
        else
            m_sWpn_scope_bones.push_back(wpn_scope_def_bone);
    }
    else
        m_sWpn_scope_bones.push_back(wpn_scope_def_bone);

    m_sWpn_silencer_bone = READ_IF_EXISTS(pSettings, r_string, section, "silencer_bone", wpn_silencer_def_bone);
    m_sWpn_launcher_bone = READ_IF_EXISTS(pSettings, r_string, section, "launcher_bone", wpn_launcher_def_bone_shoc);
    m_sWpn_laser_bone = READ_IF_EXISTS(pSettings, r_string, section, "laser_ray_bones", "");
    m_sWpn_flashlight_bone = READ_IF_EXISTS(pSettings, r_string, section, "torch_cone_bones", "");
    m_sWpn_magazine_bone = READ_IF_EXISTS(pSettings, r_string, section, "magazine_bone", "");
    m_sWpn_stock_bone = READ_IF_EXISTS(pSettings, r_string, section, "stock_bone", "");

    if (pSettings->line_exist(section, "hidden_bones"))
    {
        const char* S = pSettings->r_string(section, "hidden_bones");
        if (S && strlen(S))
        {
            const int count = _GetItemCount(S);
            string128 _hidden_bone{};
            for (int it = 0; it < count; ++it)
            {
                _GetItem(S, it, _hidden_bone);
                hidden_bones.push_back(_hidden_bone);
            }
        }
    }

    // Кости худовой модели оружия - если не прописаны, используются имена из конфига мировой модели.
    if (pSettings->line_exist(hud_sect, "scope_bone"))
    {
        const char* S = pSettings->r_string(hud_sect, "scope_bone");
        if (S && strlen(S))
        {
            const int count = _GetItemCount(S);
            string128 _scope_bone{};
            for (int it = 0; it < count; ++it)
            {
                _GetItem(S, it, _scope_bone);
                m_sHud_wpn_scope_bones.push_back(_scope_bone);
            }
        }
        else
            m_sHud_wpn_scope_bones = m_sWpn_scope_bones;
    }
    else
        m_sHud_wpn_scope_bones = m_sWpn_scope_bones;

    m_sHud_wpn_silencer_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "silencer_bone", m_sWpn_silencer_bone);
    m_sHud_wpn_launcher_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "launcher_bone", m_sWpn_launcher_bone);
    m_sHud_wpn_laser_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "laser_ray_bones", m_sWpn_laser_bone);
    m_sHud_wpn_flashlight_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "torch_cone_bones", m_sWpn_flashlight_bone);
    m_sHud_wpn_magazine_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "magazine_bone", m_sWpn_magazine_bone);
    m_sHud_wpn_stock_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "stock_bone", m_sWpn_stock_bone);

    if (pSettings->line_exist(hud_sect, "hidden_bones"))
    {
        const char* S = pSettings->r_string(hud_sect, "hidden_bones");
        if (S && strlen(S))
        {
            const int count = _GetItemCount(S);
            string128 _hidden_bone{};
            for (int it = 0; it < count; ++it)
            {
                _GetItem(S, it, _hidden_bone);
                hud_hidden_bones.push_back(_hidden_bone);
            }
        }
    }
    else
        hud_hidden_bones = hidden_bones;

    //addon meshes
    LoadAddonMeshes(section);
    LoadAddonMeshesHud();

    InitAddons();

    m_bHideCrosshairInZoom = READ_IF_EXISTS(pSettings, r_bool, hud_sect, "zoom_hide_crosshair", true);

    m_bZoomInertionAllow = READ_IF_EXISTS(pSettings, r_bool, hud_sect, "allow_zoom_inertion", READ_IF_EXISTS(pSettings, r_bool, "features", "default_allow_zoom_inertion", true));
    m_bScopeZoomInertionAllow = READ_IF_EXISTS(pSettings, r_bool, hud_sect, "allow_scope_zoom_inertion", READ_IF_EXISTS(pSettings, r_bool, "features", "default_allow_scope_zoom_inertion", true));

    //////////////////////////////////////////////////////////

    m_bHasTracers = READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
    m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", u8(-1));

    string256 temp;
    for (int i = egdNovice; i < egdCount; ++i)
    {
        strconcat(sizeof(temp), temp, "hit_probability_", get_token_name(difficulty_type_token, i));
        m_hit_probability[i] = READ_IF_EXISTS(pSettings, r_float, section, temp, 1.f);
    }
}

void CWeapon::LoadFireParams(LPCSTR section, LPCSTR prefix)
{
    camDispersion = pSettings->r_float(section, "cam_dispersion");
    camDispersion = deg2rad(camDispersion);

    if (pSettings->line_exist(section, "cam_dispersion_inc"))
    {
        camDispersionInc = pSettings->r_float(section, "cam_dispersion_inc");
        camDispersionInc = deg2rad(camDispersionInc);
    }
    else
        camDispersionInc = 0;

    CShootingObject::LoadFireParams(section, prefix);
}

BOOL CWeapon::net_Spawn(CSE_Abstract* DC)
{
    BOOL bResult = inherited::net_Spawn(DC);
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeItemWeapon* E = smart_cast<CSE_ALifeItemWeapon*>(e);

    iAmmoElapsed = E->a_elapsed;
    m_flagsAddOnState = E->m_addon_flags.get();
    m_flagsWeaponState = E->m_weapon_flags.get();
    m_ammoType = E->ammo_type;
    SetState(E->wpn_state);
    SetNextState(E->wpn_state);

    // addons check
    if (AddonAttachable(eScope) && IsAddonAttached(eScope) && E->m_cur_scope >= m_scopes.size())
    {
        Msg("! [%s]: %s: wrong scope current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_scope, m_scopes.size() - 1);
        E->m_cur_scope = 0;
    }
    m_cur_scope = E->m_cur_scope;

    if (AddonAttachable(eSilencer) && IsAddonAttached(eSilencer) && E->m_cur_silencer >= m_silencers.size())
    {
        Msg("! [%s]: %s: wrong silencer current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_silencer, m_silencers.size() - 1);
        E->m_cur_silencer = 0;
    }
    m_cur_silencer = E->m_cur_silencer;

    if (AddonAttachable(eLauncher) && IsAddonAttached(eLauncher) && E->m_cur_glauncher >= m_glaunchers.size())
    {
        Msg("! [%s]: %s: wrong grenade launcher current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_glauncher, m_glaunchers.size() - 1);
        E->m_cur_glauncher = 0;
    }
    m_cur_glauncher = E->m_cur_glauncher;

    if (AddonAttachable(eLaser) && IsAddonAttached(eLaser) && E->m_cur_laser >= m_lasers.size())
    {
        Msg("! [%s]: %s: wrong laser current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_laser, m_lasers.size() - 1);
        E->m_cur_laser = 0;
    }
    m_cur_laser = E->m_cur_laser;

    if (AddonAttachable(eFlashlight) && IsAddonAttached(eFlashlight) && E->m_cur_flashlight >= m_flashlights.size())
    {
        Msg("! [%s]: %s: wrong flashlight current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_flashlight, m_flashlights.size() - 1);
        E->m_cur_flashlight = 0;
    }
    m_cur_flashlight = E->m_cur_flashlight;

    if (AddonAttachable(eStock) && IsAddonAttached(eStock) && E->m_cur_stock >= m_stocks.size())
    {
        Msg("! [%s]: %s: wrong stock current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_stock, m_stocks.size() - 1);
        E->m_cur_stock = 0;
    }
    m_cur_stock = E->m_cur_stock;

    if (AddonAttachable(eExtender) && IsAddonAttached(eExtender) && E->m_cur_extender >= m_extenders.size())
    {
        Msg("! [%s]: %s: wrong extender current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_extender, m_extenders.size() - 1);
        E->m_cur_extender = 0;
    }
    m_cur_extender = E->m_cur_extender;

    if (AddonAttachable(eForend) && IsAddonAttached(eForend) && E->m_cur_forend >= m_forends.size())
    {
        Msg("! [%s]: %s: wrong forend current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_forend, m_forends.size() - 1);
        E->m_cur_forend = 0;
    }
    m_cur_forend = E->m_cur_forend;

    if (AddonAttachable(eMagazine))
    {
        //Msg("weapon [%s] spawned with magazine status [%s]", cName().c_str(), E->m_bIsMagazineAttached ? "atached" : "detached");
        int mag_size{};
        if (IsAddonAttached(eMagazine))
        {
            if (E->m_cur_magazine >= m_magazines.size())
            {
                Msg("! [%s]: %s: wrong magazine current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_magazine, m_magazines.size() - 1);
                E->m_cur_magazine = 0;
            }
            m_cur_magazine = E->m_cur_magazine;
            mag_size = (int)pSettings->r_s32(GetAddonName(eMagazine), "box_size");
        }
        iMagazineSize = mag_size + HasChamber(); // учтём дополнительный патрон в патроннике
    }

    if (m_ammoType >= m_ammoTypes.size())
    {
        Msg("! [%s]: %s: wrong m_ammoType[%u/%u]", __FUNCTION__, cName().c_str(), m_ammoType, m_ammoTypes.size() - 1);
        m_ammoType = 0;
        auto se_obj = alife_object();
        if (se_obj)
        {
            auto W = smart_cast<CSE_ALifeItemWeapon*>(se_obj);
            W->ammo_type = m_ammoType;
        }
    }

    m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), u8(m_ammoType));
    if (iAmmoElapsed)
    {
        // нож автоматически заряжается двумя патронами, хотя
        // размер магазина у него 0. Что бы зря не ругаться, проверим
        // что в конфиге размер магазина не нулевой.
        if (iMagazineSize && iAmmoElapsed > iMagazineSize)
        {
            Msg("! [%s]: %s: wrong iAmmoElapsed[%u/%u]", __FUNCTION__, cName().c_str(), iAmmoElapsed, iMagazineSize);
            iAmmoElapsed = iMagazineSize;
            auto se_obj = alife_object();
            if (se_obj)
            {
                auto W = smart_cast<CSE_ALifeItemWeapon*>(se_obj);
                W->a_elapsed = iAmmoElapsed;
            }
        }
        m_fCurrentCartirdgeDisp = m_DefaultCartridge.m_kDisp;
        for (int i = 0; i < iAmmoElapsed; ++i)
            m_magazine.push_back(m_DefaultCartridge);
    }

    m_bZoomMode = E->m_bZoom;
    if (m_bZoomMode)
        OnZoomIn();
    else
        OnZoomOut();

    UpdateAddonsVisibility();
    InitAddons();

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    return bResult;
}

void CWeapon::net_Destroy()
{
    inherited::net_Destroy();

    // удалить объекты партиклов
    StopFlameParticles();
    StopFlameParticles2();
    StopLight();
    Light_Destroy();

    m_magazine.clear();
    m_magazine.shrink_to_fit();
}

BOOL CWeapon::IsUpdating()
{
    bool bIsActiveItem = m_pCurrentInventory && m_pCurrentInventory->ActiveItem() == this;
    return bIsActiveItem || bWorking || IsPending() || getVisible();
}

void CWeapon::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);

    CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(E);
    wpn->wpn_flags = IsUpdating() ? 1 : 0;
    wpn->a_elapsed = u16(iAmmoElapsed);
    wpn->m_addon_flags.flags = m_flagsAddOnState;
    wpn->ammo_type = (u8)m_ammoType;
    wpn->wpn_state = (u8)GetState();
    wpn->m_bZoom = (u8)m_bZoomMode;
    //
    wpn->m_weapon_flags.flags = m_flagsWeaponState;
    wpn->m_cur_scope = m_cur_scope;
    wpn->m_cur_silencer = m_cur_silencer;
    wpn->m_cur_glauncher = m_cur_glauncher;
    wpn->m_cur_laser = m_cur_laser;
    wpn->m_cur_flashlight = m_cur_flashlight;
    wpn->m_cur_stock = m_cur_stock;
    wpn->m_cur_extender = m_cur_extender;
    wpn->m_cur_forend = m_cur_forend;
    wpn->m_cur_magazine = m_cur_magazine;
}

void CWeapon::save(NET_Packet& output_packet) { inherited::save(output_packet); }

void CWeapon::load(IReader& input_packet) { inherited::load(input_packet); }

void CWeapon::OnEvent(NET_Packet& P, u16 type)
{
    switch (type)
    {
    case GE_WPN_STATE_CHANGE: {
        u8 state;
        P.r_u8(state);
        P.r_u8(m_sub_state);
        //не знаю чи дуже потрібен код нижче, але через нього постіно скидається тип наступних набоїв для перезарядки
        //u8 NextAmmo = P.r_u8();
        //if (NextAmmo == u8(-1))
        //    m_set_next_ammoType_on_reload = u32(-1);
        //else
        //    m_set_next_ammoType_on_reload = u8(NextAmmo);

        OnStateSwitch(u32(state), GetState());
    }
    break;
    default: {
        inherited::OnEvent(P, type);
    }
    break;
    }
};

void CWeapon::shedule_Update(u32 dT)
{
    // Inherited
    inherited::shedule_Update(dT);
}

void CWeapon::OnH_B_Independent(bool just_before_destroy)
{
    RemoveShotEffector();

    inherited::OnH_B_Independent(just_before_destroy);

    // завершить принудительно все процессы что шли
    FireEnd();
    SetPending(FALSE);
    SwitchState(eIdle);

    m_strapped_mode = false;
    OnZoomOut();
    m_fZoomRotationFactor = 0.f;
    UpdateXForm();
}

void CWeapon::OnH_A_Independent()
{
    inherited::OnH_A_Independent();
    Light_Destroy();
};

void CWeapon::OnH_A_Chield()
{
    inherited::OnH_A_Chield();

    UpdateAddonsVisibility();
};

void CWeapon::OnActiveItem()
{
    inherited::OnActiveItem();
    // если мы занружаемся и оружие было в руках
    SetState(eIdle);
    SetNextState(eIdle);
}

void CWeapon::OnHiddenItem()
{
    inherited::OnHiddenItem();
    SetState(eHidden);
    SetNextState(eHidden);
    m_set_next_ammoType_on_reload = u32(-1);
}

void CWeapon::OnH_B_Chield()
{
    inherited::OnH_B_Chield();

    OnZoomOut();
    m_set_next_ammoType_on_reload = u32(-1);
}

u8 CWeapon::idle_state()
{
    auto* actor = smart_cast<CActor*>(H_Parent());

    if (actor)
    {
        u32 st = actor->get_state();
        if (st & mcSprint)
            return eSubstateIdleSprint;
        else if (st & mcAnyAction && !(st & mcJump) && !(st & mcFall))
            return eSubstateIdleMoving;
    }

    return eIdle;
}

void CWeapon::UpdateCL()
{
    inherited::UpdateCL();

    UpdateHUDAddonsVisibility();

    // подсветка от выстрела
    UpdateLight();

    // нарисовать партиклы
    UpdateFlameParticles();
    UpdateFlameParticles2();

    VERIFY(smart_cast<IKinematics*>(Visual()));

    if (GetState() == eIdle)
    {
        auto state = idle_state();
        if (m_idle_state != state)
        {
            m_idle_state = state;
            if (GetNextState() != eMagEmpty && GetNextState() != eReload)
            {
                SwitchState(eIdle);
            }
        }
    }
    else
        m_idle_state = eIdle;
}


void CWeapon::renderable_Render()
{
    UpdateXForm();
    // нарисовать подсветку
    RenderLight();

    for (int i=0; i<eMaxAddon; ++i)
    {
        if (!world_attach_visual[i])
            continue;
        Fmatrix m_res{};
        auto visual = Visual()->dcast_PKinematics();
        u16 bone_id = visual->LL_BoneID(world_attach_bone_name[i]);
        Fmatrix bone_trans = visual->LL_GetTransform(bone_id);

        Fmatrix offset{};
        Fvector pos = world_attach_visual_offset[i][0];
        Fvector rot = world_attach_visual_offset[i][1];
        offset.setHPB(rot.x, rot.y, rot.z);
        offset.translate_over(pos);
        m_res.mul(bone_trans, offset);
        m_res.mulA_43(XFORM());

        ::Render->set_Transform(&m_res);
        ::Render->add_Visual(world_attach_visual[i]);
    }

    inherited::renderable_Render();
}

bool CWeapon::need_renderable() { return !Device.m_SecondViewport.IsSVPFrame() /*&& !(IsZoomed() && ZoomTexture() && !IsRotatingToZoom())*/; }

void CWeapon::render_hud_mode()
{
    for (int i = 0; i < eMaxAddon; ++i)
    {
        if (!hud_attach_visual[i])
            continue;        
        Fmatrix m_res{};
        auto visual = HudItemData()->m_model;
        u16 bone_id = visual->LL_BoneID(hud_attach_bone_name[i]);
        Fmatrix bone_trans = visual->LL_GetTransform(bone_id);

        Fmatrix offset{};
        Fvector pos = hud_attach_visual_offset[i][0];
        Fvector rot = hud_attach_visual_offset[i][1];
        offset.setHPB(rot.x, rot.y, rot.z);
        offset.translate_over(pos);

        m_res.mul(bone_trans, offset);
        m_res.mulA_43(HudItemData()->m_item_transform);

        ::Render->set_Transform(&m_res);
        ::Render->add_Visual(hud_attach_visual[i]);
    }

    inherited::render_hud_mode();
}

void CWeapon::signal_HideComplete()
{
    if (H_Parent())
        setVisible(FALSE);
    SetPending(FALSE);
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
    Position().set(trans.c);
    XFORM().mul(trans, m_strapped_mode ? m_StrapOffset : m_Offset);
    VERIFY(!fis_zero(DET(renderable.xform)));
}

bool CWeapon::Action(s32 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;

    switch (cmd)
    {
    case kWPN_FIRE: {
        // если оружие чем-то занято, то ничего не делать
        {
            if (flags & CMD_START)
            {
                if (IsPending())
                    return false;
                FireStart();
            }
            else
                FireEnd();
        };
    }
        return true;
    case kWPN_NEXT: {
        if (IsPending())
        {
            return false;
        }

        if (flags & CMD_START)
        {
            u32 next_ammo_type = GetNextAmmoType();
            if (next_ammo_type != u32(-1) && TryToGetAmmo(next_ammo_type))
            {
                m_set_next_ammoType_on_reload = next_ammo_type;
                LPCSTR ammo_sect = pSettings->r_string(m_ammoTypes[m_set_next_ammoType_on_reload].c_str(), "inv_name_short");
                string1024 str;
                sprintf(str, "%s: %s", CStringTable().translate("st_next_ammo_type").c_str(), CStringTable().translate(ammo_sect).c_str());
                HUD().GetUI()->AddInfoMessage("item_usage", str, false);
            }
        }
    }
        return true;
    case kWPN_ZOOM: {
        if (IsZoomEnabled())
        {
            if (flags & CMD_START && !IsPending())
            {
                if (!psActorFlags.is(AF_HOLD_TO_AIM) && IsZoomed())
                    OnZoomOut();
                else
                    OnZoomIn();
            }
            else if (IsZoomed() && psActorFlags.is(AF_HOLD_TO_AIM))
            {
                OnZoomOut();
            }
            return true;
        }
        else
            return false;
    }
    case kWPN_ZOOM_INC:
    case kWPN_ZOOM_DEC: {
        if (IsZoomEnabled() && IsZoomed() && m_bScopeDynamicZoom && IsAddonAttached(eScope) && (flags & CMD_START))
        {
            // если в режиме ПГ - не будем давать использовать динамический зум
            if (IsGrenadeMode() || IsSecondScopeMode())
                return false;
            ZoomChange(cmd == kWPN_ZOOM_INC);
            return true;
        }
        else
            return false;
    }
    case kWPN_FUNC: {
        if (Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION)))
        {
            if (HasScopeSecond() && (flags & CMD_START))
            {
                m_bScopeSecondMode = !m_bScopeSecondMode;
                if (IsZoomed())
                    OnZoomOut(true);
                return true;
            }
        }
    }
    }
    return false;
}

void CWeapon::ZoomChange(bool inc)
{
    bool wasChanged{};
    auto GetZoomStepDelta = [](float scope_factor, float max_scope_factor, u32 step_count) {
        float total_zoom_delta = max_scope_factor - scope_factor;
        return total_zoom_delta / step_count;
    };
    if (SecondVPEnabled())
    {
        const float currentZoomFactor = m_fRTZoomFactor;
        m_fZoomFactor += GetZoomStepDelta(m_fSecondVPZoomFactor, m_fMaxScopeZoomFactor, m_uZoomStepCount) * (inc ? 1 : -1); // delta;
        clamp(m_fSecondVPZoomFactor, m_fScopeZoomFactor, m_fMaxScopeZoomFactor);
        wasChanged = !fsimilar(currentZoomFactor, m_fRTZoomFactor);
    }
    else
    {
        const float currentZoomFactor = m_fZoomFactor;
        m_fZoomFactor += GetZoomStepDelta(m_fScopeZoomFactor, m_fMaxScopeZoomFactor, m_uZoomStepCount) * (inc ? 1 : -1); // delta;
        clamp(m_fZoomFactor, m_fScopeZoomFactor, m_fMaxScopeZoomFactor);
        wasChanged = !fsimilar(currentZoomFactor, m_fZoomFactor);
        if (H_Parent() && !IsRotatingToZoom() && !SecondVPEnabled())
            m_fRTZoomFactor = m_fZoomFactor; // store current
    }
    if (wasChanged)
    {
        OnZoomChanged();
    }
}

void CWeapon::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, shared_str ammo_sect_in_mag, u32 ParentID)
{
    if (!m_ammoTypes.size())
        return;

    if (!ammoSect)
        ammoSect = m_ammoTypes.front().c_str();

    CSE_Abstract* D = F_entity_Create(ammoSect);
    if (auto l_pA = smart_cast<CSE_ALifeItemAmmo*>(D))
    {
        R_ASSERT(l_pA);
        l_pA->m_boxSize = (u16)pSettings->r_s32(ammoSect, "box_size");
        D->s_name = ammoSect;
        D->set_name_replace("");
        D->s_gameid = u8(GameID());
        D->s_RP = 0xff;
        D->ID = 0xffff;
        if (ParentID == 0xffffffff)
            D->ID_Parent = (u16)H_Parent()->ID();
        else
            D->ID_Parent = (u16)ParentID;

        D->ID_Phantom = 0xffff;
        D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
        D->RespawnTime = 0;
        l_pA->m_tNodeID = ai_location().level_vertex_id();

        if (boxCurr == 0xffffffff)
            boxCurr = l_pA->m_boxSize;
        
        if (pSettings->line_exist(ammoSect, "ammo_types") && !!ammo_sect_in_mag)
            l_pA->m_cur_ammo_type = (u8)std::distance(l_pA->m_ammoTypes.begin(), std::find(l_pA->m_ammoTypes.begin(), l_pA->m_ammoTypes.end(), ammo_sect_in_mag));

        NET_Packet P;
        if (boxCurr > 0)
        {
            while (boxCurr)
            {
                l_pA->a_elapsed = (u16)(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);
                D->Spawn_Write(P, TRUE);
                Level().Send(P, net_flags(TRUE));

                if (boxCurr > l_pA->m_boxSize)
                    boxCurr -= l_pA->m_boxSize;
                else
                    boxCurr = 0;
            }
        }
        else
        {
            D->Spawn_Write(P, TRUE);
            Level().Send(P, net_flags(TRUE));
        }
    };
    F_entity_Destroy(D);
}

int CWeapon::GetAmmoCurrent(bool use_item_to_spawn) const
{
    int l_count = iAmmoElapsed;
    if (!m_pCurrentInventory)
        return l_count;

    // чтоб не делать лишних пересчетов
    if (m_pCurrentInventory->ModifyFrame() <= m_dwAmmoCurrentCalcFrame)
        if (!ParentIsActor()) // очень некрасивый костыль!
            return l_count + iAmmoCurrent;

    //	Msg("[%s] get ammo current [%d]:[%s] weapon [%s] - before types cycle", m_pCurrentInventory->GetOwner()->Name(), l_count + iAmmoCurrent, *m_ammoTypes[m_ammoType],
    //this->Name_script());

    m_dwAmmoCurrentCalcFrame = Device.dwFrame;
    iAmmoCurrent = 0;

    for (int i = 0; i < (int)m_ammoTypes.size(); ++i)
    {
        iAmmoCurrent += GetAmmoCount_forType(m_ammoTypes[i]);

        if (!use_item_to_spawn)
            continue;

        if (!inventory_owner().item_to_spawn())
            continue;

        iAmmoCurrent += inventory_owner().ammo_in_box_to_spawn();

        // Msg("[%s] get ammo current [%d]:[%s] weapon [%s] ", m_pCurrentInventory->GetOwner()->Name(), l_count + iAmmoCurrent, m_ammoTypes[i].c_str(), Name_script());
    }
    return l_count + iAmmoCurrent;
}

int CWeapon::GetAmmoCount(u8 ammo_type, u32 max) const
{
    VERIFY(m_pInventory);
    R_ASSERT(ammo_type < m_ammoTypes.size());

    return GetAmmoCount_forType(m_ammoTypes[ammo_type], max);
}

int CWeapon::GetAmmoCount_forType(shared_str const& ammo_type, u32 max) const
{
    u32 res = 0;
    auto callback = [&](const auto pIItem) -> bool {
        auto* ammo = smart_cast<CWeaponAmmo*>(pIItem);
        if (ammo->cNameSect() == ammo_type)
            res += ammo->m_boxCurr;
        return (max > 0 && res >= max);
    };

    m_pCurrentInventory->IterateAmmo(callback);

    return res;
}

float CWeapon::GetConditionMisfireProbability() const
{
    float mis{};

    // вероятность осечки от патрона
    if (!m_magazine.empty())
    {
        const auto& l_cartridge = m_magazine.back();
        mis = l_cartridge.m_misfireProbability;
    }
    // вероятность осечки от состояния оружия
    if (GetCondition() < 0.95f)
        mis += (misfireProbability + powf(1.f - GetCondition(), 3.f) * misfireConditionK);

    clamp(mis, 0.0f, 0.99f);
    return mis;
}

bool CWeapon::CheckForMisfire()
{
    if (Core.Features.test(xrCore::Feature::npc_simplified_shooting))
    {
        CActor* actor = smart_cast<CActor*>(H_Parent());
        if (!actor)
            return false;
    }

    float rnd = ::Random.randF(0.f, 1.f);
    float mp = GetConditionMisfireProbability();
    if (rnd < mp)
    {
        FireEnd();
        SetMisfire(true);
        //SwitchState(eMisfire);

        return true;
    }
    else
    {
        return false;
    }
}

void CWeapon::UpdateHUDAddonsVisibility()
{
    if (!GetHUDmode())
        return;

    if (AddonAttachable(eScope))
    {
        LPCSTR scope_hud_sect = READ_IF_EXISTS(pSettings, r_string, cNameSect(), "hud", nullptr);
        if (IsAddonAttached(eScope))
        {
            string1024 _res{};
            sprintf(_res, "%s_hud_sect", GetAddonName(eScope).c_str());
            if (pSettings->line_exist(cNameSect(), _res))
                scope_hud_sect = pSettings->r_string(cNameSect(), _res);
        }

        if (scope_hud_sect && xr_strcmp(hud_sect.c_str(), scope_hud_sect) != 0)
        {
            SetHudSection(scope_hud_sect);
            LoadAddonMeshesHud();
        }

        HudItemData()->set_bone_visible(m_sHud_wpn_scope_bones, IsAddonAttached(eScope), TRUE);
    }

    if (m_eScopeStatus == ALife::eAddonDisabled)
        HudItemData()->set_bone_visible(m_sHud_wpn_scope_bones, FALSE, TRUE);
    else if (m_eScopeStatus == ALife::eAddonPermanent)
        HudItemData()->set_bone_visible(m_sHud_wpn_scope_bones, TRUE, TRUE);

    if (HudItemData()->m_model->LL_BoneID(m_sHud_wpn_silencer_bone) != BI_NONE)
    {
        if (AddonAttachable(eSilencer))
        {
            bool b_show_on_model = READ_IF_EXISTS(pSettings, r_bool, GetAddonName(eSilencer), "show_on_model", true);
            HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, IsAddonAttached(eSilencer) && b_show_on_model);
        }

        if (m_eSilencerStatus == ALife::eAddonDisabled)
            HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, FALSE, TRUE);
        else if (m_eSilencerStatus == ALife::eAddonPermanent)
            HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, TRUE, TRUE);
    }

    if (!HudItemData()->has_bone(m_sHud_wpn_launcher_bone) && HudItemData()->has_bone(wpn_launcher_def_bone_cop))
        m_sHud_wpn_launcher_bone = wpn_launcher_def_bone_cop;

    if (AddonAttachable(eLauncher))
        HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, IsAddonAttached(eLauncher));

    if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled)
        HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, FALSE, TRUE);
    else if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
        HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, TRUE, TRUE);

    if (m_sHud_wpn_laser_bone.size() && IsAddonAttached(eLaser))
        HudItemData()->set_bone_visible(m_sHud_wpn_laser_bone, IsLaserOn(), TRUE);

    if (m_sHud_wpn_flashlight_bone.size() && IsAddonAttached(eFlashlight))
        HudItemData()->set_bone_visible(m_sHud_wpn_flashlight_bone, IsFlashlightOn(), TRUE);

    if (m_sHud_wpn_stock_bone.size() && AddonAttachable(eStock))
        HudItemData()->set_bone_visible(m_sHud_wpn_stock_bone, IsAddonAttached(eStock));

    for (const shared_str& bone_name : hud_hidden_bones)
        HudItemData()->set_bone_visible(bone_name, FALSE, TRUE);

    // addon meshes
    const auto weapon_visual = HudItemData()->m_model;
    for (const auto& mesh_idx : m_scope_meshes_hud)
        weapon_visual->SetRFlag(mesh_idx, IsAddonAttached(eScope));
    for (const auto& mesh_idx : m_silencer_meshes_hud)
        weapon_visual->SetRFlag(mesh_idx, IsAddonAttached(eSilencer));
    for (const auto& mesh_idx : m_launcher_meshes_hud)
        weapon_visual->SetRFlag(mesh_idx, IsAddonAttached(eLauncher));
    for (const auto& mesh_idx : m_laser_meshes_hud)
        weapon_visual->SetRFlag(mesh_idx, IsAddonAttached(eLaser));
    for (const auto& mesh_idx : m_flashlight_meshes_hud)
        weapon_visual->SetRFlag(mesh_idx, IsAddonAttached(eFlashlight));
    for (const auto& mesh_idx : m_stock_meshes_hud)
        weapon_visual->SetRFlag(mesh_idx, IsAddonAttached(eStock));
    //
    InitAddonsVisualHud();

    callback(GameObject::eOnUpdateHUDAddonsVisibiility)();
}

void CWeapon::UpdateAddonsVisibility()
{
    auto pWeaponVisual = smart_cast<IKinematics*>(Visual());
    VERIFY(pWeaponVisual);

    UpdateHUDAddonsVisibility();

    if (AddonAttachable(eScope))
    {
        LPCSTR scope_visual{};
        if (IsAddonAttached(eScope))
        {
            string1024 res_sect{};
            sprintf(res_sect, "%s_visual", GetAddonName(eScope).c_str());
            scope_visual = READ_IF_EXISTS(pSettings, r_string, cNameSect(), res_sect, nullptr);
        }
        else
            scope_visual = READ_IF_EXISTS(pSettings, r_string, cNameSect(), "visual", nullptr);

        if (scope_visual && xr_strcmp(cNameVisual().c_str(), scope_visual) != 0)
        {
            cNameVisual_set(scope_visual);
            //візуал змінився, його треба отримати заново для подальших маніпуляцій з видимістю кісток
            pWeaponVisual = smart_cast<IKinematics*>(Visual());
        }
    }

    ///////////////////////////////////////////////////////////////////
    u16 bone_id{};

    for (const auto& sbone : m_sWpn_scope_bones)
    {
        bone_id = pWeaponVisual->LL_BoneID(sbone);

        if (AddonAttachable(eScope) && bone_id != BI_NONE)
        {
            pWeaponVisual->LL_SetBoneVisible(bone_id, IsAddonAttached(eScope), TRUE);
        }

        if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
            pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
        else if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE && !pWeaponVisual->LL_GetBoneVisible(bone_id))
            pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
    }

    ///////////////////////////////////////////////////////////////////

    bone_id = pWeaponVisual->LL_BoneID(m_sWpn_silencer_bone);

    if (AddonAttachable(eSilencer) && (bone_id != BI_NONE))
    {
        bool b_show_on_model = READ_IF_EXISTS(pSettings, r_bool, GetAddonName(eSilencer), "show_on_model", true);
        pWeaponVisual->LL_SetBoneVisible(bone_id, IsAddonAttached(eSilencer) && b_show_on_model, TRUE);
    }

    if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
        pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
    else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE && !pWeaponVisual->LL_GetBoneVisible(bone_id))
        pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);

    ///////////////////////////////////////////////////////////////////

    bone_id = pWeaponVisual->LL_BoneID(m_sWpn_launcher_bone);

    if (AddonAttachable(eLauncher))
    {
        pWeaponVisual->LL_SetBoneVisible(bone_id, IsAddonAttached(eLauncher), TRUE);
    }

    if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
        pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
    else if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE && !pWeaponVisual->LL_GetBoneVisible(bone_id))
        pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);

    ///////////////////////////////////////////////////////////////////

    if (m_sWpn_laser_bone.size() && IsAddonAttached(eLaser))
    {
        bone_id = pWeaponVisual->LL_BoneID(m_sWpn_laser_bone);
        if (bone_id != BI_NONE)
        {
            pWeaponVisual->LL_SetBoneVisible(bone_id, IsLaserOn(), TRUE);
        }
    }

    ///////////////////////////////////////////////////////////////////

    if (m_sWpn_flashlight_bone.size() && IsAddonAttached(eFlashlight))
    {
        bone_id = pWeaponVisual->LL_BoneID(m_sWpn_flashlight_bone);
        if (bone_id != BI_NONE)
        {
            pWeaponVisual->LL_SetBoneVisible(bone_id, IsFlashlightOn(), TRUE);
        }
    }

    if (m_sWpn_stock_bone.size() && AddonAttachable(eStock))
    {
        bone_id = pWeaponVisual->LL_BoneID(m_sWpn_stock_bone);
        pWeaponVisual->LL_SetBoneVisible(bone_id, IsAddonAttached(eStock), false);
    }

    ///////////////////////////////////////////////////////////////////

    for (const auto& bone_name : hidden_bones)
    {
        bone_id = pWeaponVisual->LL_BoneID(bone_name);
        if (bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
            pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
    }

    ///////////////////////////////////////////////////////////////////

    //addon meshes
    for (const auto& mesh_idx : m_scope_meshes)
        pWeaponVisual->SetRFlag(mesh_idx, IsAddonAttached(eScope));
    for (const auto& mesh_idx : m_silencer_meshes)
        pWeaponVisual->SetRFlag(mesh_idx, IsAddonAttached(eSilencer));
    for (const auto& mesh_idx : m_launcher_meshes)
        pWeaponVisual->SetRFlag(mesh_idx, IsAddonAttached(eLauncher));
    for (const auto& mesh_idx : m_laser_meshes)
        pWeaponVisual->SetRFlag(mesh_idx, IsAddonAttached(eLaser));
    for (const auto& mesh_idx : m_flashlight_meshes)
        pWeaponVisual->SetRFlag(mesh_idx, IsAddonAttached(eFlashlight));
    for (const auto& mesh_idx : m_stock_meshes)
        pWeaponVisual->SetRFlag(mesh_idx, IsAddonAttached(eStock));
    //
    InitAddonsVisual();

    callback(GameObject::eOnUpdateAddonsVisibiility)();

    pWeaponVisual->CalculateBones_Invalidate();
    pWeaponVisual->CalculateBones();
}

bool CWeapon::Activate(bool now)
{
    UpdateAddonsVisibility();
    return inherited::Activate(now);
}

float CWeapon::CurrentZoomFactor()
{
    float res{1.f};
    if (SecondVPEnabled())
        return res;
    if (IsAddonAttached(eScope))
    {
        if (m_bScopeDynamicZoom && !IsSecondScopeMode())
        {
            clamp(m_fRTZoomFactor, m_fScopeZoomFactor, m_fMaxScopeZoomFactor);
            res = m_fRTZoomFactor;
            //			Msg("m_fRTZoomFactor res = [%.2f]", res);
        }
        else
        {
            res = IsSecondScopeMode() ? m_fScopeZoomFactorSecond : m_fScopeZoomFactor;
            //			Msg("m_fScopeZoomFactor res = [%.2f]", res);
        }
    }
    else
    {
        res = m_fIronSightZoomFactor;
        //		Msg("m_fIronSightZoomFactor res = [%.2f]", res);
    }
    return res;
}

bool CWeapon::HasScopeSecond() const { return IsAddonAttached(eScope) && !IsGrenadeMode() && m_bHasScopeSecond; }

bool CWeapon::IsSecondScopeMode() const { return IsAddonAttached(eScope) && !IsGrenadeMode() && m_bScopeSecondMode; }

void CWeapon::OnZoomIn()
{
    m_bZoomMode = true;
    m_fZoomFactor = CurrentZoomFactor();
    if (IsAddonAttached(eScope) && !IsGrenadeMode())
    {
        if (!m_bScopeZoomInertionAllow)
            AllowHudInertion(FALSE);
    }
    else if (!m_bZoomInertionAllow)
        AllowHudInertion(FALSE);
    if (auto pActor = smart_cast<CActor*>(H_Parent()))
        pActor->callback(GameObject::eOnActorWeaponZoomIn)(lua_game_object());
}

void CWeapon::OnZoomOut(bool rezoom)
{
    m_fZoomFactor = 1.f;
    if (m_bZoomMode)
    {
        m_bZoomMode = false;
        if (auto pActor = smart_cast<CActor*>(H_Parent()))
        {
            pActor->callback(GameObject::eOnActorWeaponZoomOut)(lua_game_object());
        }
    }
    AllowHudInertion(TRUE);
    ResetSubStateTime();
}

void CWeapon::SwitchState(u32 S)
{
    SetNextState(S); // Very-very important line of code!!! :)
    if (CHudItem::object().Local() && !CHudItem::object().getDestroy() /* && (S!=NEXT_STATE)*/
        && m_pCurrentInventory)
    {
        // !!! Just single entry for given state !!!
        NET_Packet P;
        CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
        P.w_u8(u8(S));
        P.w_u8(u8(m_sub_state));
        P.w_u8(u8(m_ammoType & 0xff));
        P.w_u8(u8(iAmmoElapsed & 0xff));
        P.w_u8(u8(m_set_next_ammoType_on_reload & 0xff));
        CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
    }
}

void CWeapon::OnMagazineEmpty() { VERIFY((u32)iAmmoElapsed == m_magazine.size()); }

void CWeapon::reinit()
{
    CShootingObject::reinit();
    CHudItemObject::reinit();
}

void CWeapon::reload(LPCSTR section)
{
    CShootingObject::reload(section);
    CHudItemObject::reload(section);

    m_can_be_strapped = true;
    m_strapped_mode = false;

    if (pSettings->line_exist(section, "strap_bone0"))
        m_strap_bone0 = pSettings->r_string(section, "strap_bone0");
    else
        m_can_be_strapped = false;

    if (pSettings->line_exist(section, "strap_bone1"))
        m_strap_bone1 = pSettings->r_string(section, "strap_bone1");
    else
        m_can_be_strapped = false;

    if (m_eScopeStatus == ALife::eAddonAttachable)
    {
        m_addon_holder_range_modifier = READ_IF_EXISTS(pSettings, r_float, GetAddonName(eScope), "holder_range_modifier", m_holder_range_modifier);
        m_addon_holder_fov_modifier = READ_IF_EXISTS(pSettings, r_float, GetAddonName(eScope), "holder_fov_modifier", m_holder_fov_modifier);
    }
    else
    {
        m_addon_holder_range_modifier = m_holder_range_modifier;
        m_addon_holder_fov_modifier = m_holder_fov_modifier;
    }

    {
        Fvector pos, ypr;
        pos = pSettings->r_fvector3(section, "position");
        ypr = pSettings->r_fvector3(section, "orientation");
        ypr.mul(PI / 180.f);

        m_Offset.setHPB(ypr.x, ypr.y, ypr.z);
        m_Offset.translate_over(pos);
    }

    m_StrapOffset = m_Offset;
    if (pSettings->line_exist(section, "strap_position") && pSettings->line_exist(section, "strap_orientation"))
    {
        Fvector pos, ypr;
        pos = pSettings->r_fvector3(section, "strap_position");
        ypr = pSettings->r_fvector3(section, "strap_orientation");
        ypr.mul(PI / 180.f);

        m_StrapOffset.setHPB(ypr.x, ypr.y, ypr.z);
        m_StrapOffset.translate_over(pos);
    }
    else
        m_can_be_strapped = false;

    m_ef_main_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_main_weapon_type", u32(-1));
    m_ef_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_weapon_type", u32(-1));
}

void CWeapon::create_physic_shell() { CPhysicsShellHolder::create_physic_shell(); }

void CWeapon::activate_physic_shell() { CPhysicsShellHolder::activate_physic_shell(); }

void CWeapon::setup_physic_shell() { CPhysicsShellHolder::setup_physic_shell(); }

bool CWeapon::can_kill() const
{
    if (GetAmmoCurrent(true) || m_ammoTypes.empty() || unlimited_ammo())
        return (true);

    return (false);
}

CInventoryItem* CWeapon::can_kill(CInventory* inventory) const
{
    if (GetAmmoElapsed() || m_ammoTypes.empty())
        return (const_cast<CWeapon*>(this));

    for (const auto& item : inventory->m_all)
    {
        CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(item);
        if (!inventory_item)
            continue;

        xr_vector<shared_str>::const_iterator i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
        if (i != m_ammoTypes.end())
            return (inventory_item);
    }

    return (0);
}

const CInventoryItem* CWeapon::can_kill(const xr_vector<const CGameObject*>& items) const
{
    if (m_ammoTypes.empty())
        return (this);

    xr_vector<const CGameObject*>::const_iterator I = items.begin();
    xr_vector<const CGameObject*>::const_iterator E = items.end();
    for (; I != E; ++I)
    {
        const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(*I);
        if (!inventory_item)
            continue;

        xr_vector<shared_str>::const_iterator i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
        if (i != m_ammoTypes.end())
            return (inventory_item);
    }

    return (0);
}

bool CWeapon::ready_to_kill() const { return (!IsMisfire() && ((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2)) && GetAmmoElapsed()); }

bool CWeapon::IsAiming() const { return ((IsZoomed() && m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_fZoomRotationFactor > 0.f)); }

// Получить индекс текущих координат худа
u8 CWeapon::GetCurrentHudOffsetIdx() const
{
    if (IsAiming())
    {
        //const bool has_gl = AddonAttachable(eLauncher) && IsAddonAttached(eLauncher);
        const bool has_scope = AddonAttachable(eScope) && IsAddonAttached(eScope);

        if (IsSecondScopeMode())
        {
            if (has_scope)
                return hud_item_measures::m_hands_offset_type_aim_alt_scope;
            else
                return hud_item_measures::m_hands_offset_type_aim_alt;
        }

        if (IsGrenadeMode())
        {
            if (has_scope)
                return hud_item_measures::m_hands_offset_type_gl_scope;
            else
                return hud_item_measures::m_hands_offset_type_gl;
        }
        else
        {
            if (has_scope)
                return hud_item_measures::m_hands_offset_type_aim_scope;
            else
                return hud_item_measures::m_hands_offset_type_aim;
        }
    }

    return hud_item_measures::m_hands_offset_type_normal;
}

void CWeapon::SetAmmoElapsed(int ammo_count)
{
    iAmmoElapsed = ammo_count;

    u32 uAmmo = u32(iAmmoElapsed);

    if (uAmmo != m_magazine.size())
    {
        if (uAmmo > m_magazine.size())
        {
            CCartridge l_cartridge;
            l_cartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));
            while (uAmmo > m_magazine.size())
                m_magazine.push_back(l_cartridge);
        }
        else
        {
            while (uAmmo < m_magazine.size())
                m_magazine.pop_back();
        };
    };
}

u32 CWeapon::ef_main_weapon_type() const
{
    VERIFY(m_ef_main_weapon_type != u32(-1));
    return (m_ef_main_weapon_type);
}

u32 CWeapon::ef_weapon_type() const
{
    VERIFY(m_ef_weapon_type != u32(-1));
    return (m_ef_weapon_type);
}

bool CWeapon::IsNecessaryItem(const shared_str& item_sect) { return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end()); }

void CWeapon::modify_holder_params(float& range, float& fov) const
{
    if (!IsAddonAttached(eScope))
    {
        inherited::modify_holder_params(range, fov);
        return;
    }
    range *= m_addon_holder_range_modifier;
    fov *= m_addon_holder_fov_modifier;
}

bool CWeapon::IsHudModeNow() { return (HudItemData()); }

bool CWeapon::unlimited_ammo() const
{
    if (m_pCurrentInventory)
        return inventory_owner().unlimited_ammo() && m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited);
    else
        return false;
};

LPCSTR CWeapon::GetCurrentAmmo_ShortName()
{
    if (m_magazine.empty())
        return ("");
    CCartridge& l_cartridge = m_magazine.back();
    return l_cartridge.m_InvShortName.c_str();
}

LPCSTR CWeapon::GetCurrentAmmoNameSect()
{
    if (m_magazine.empty())
        return ("");
    CCartridge& l_cartridge = m_magazine.back();
    return l_cartridge.m_ammoSect.c_str();
}

float CWeapon::GetAmmoInMagazineWeight(const decltype(CWeapon::m_magazine)& mag) const
{
    float res{}, last_ammo_weight{};
    const char* last_type{};
    for (auto& c : mag)
    {
        // Usually ammos in mag have same type, use this fact to improve performance
        if (last_type != c.m_ammoSect.c_str())
        {
            last_type = c.m_ammoSect.c_str();
            last_ammo_weight = c.Weight();
        }
        res += last_ammo_weight;
    }
    return res;
}

float CWeapon::Weight() const
{
    float res = inherited::Weight();
    for (u32 i = 0; i < eMagazine; i++)
    {
        if (AddonAttachable(i) && IsAddonAttached(i))
            res += pSettings->r_float(GetAddonName(i), "inv_weight");
    }
    res += GetAmmoInMagazineWeight(m_magazine);
    return res;
}

u32 CWeapon::Cost() const
{
    u32 res = inherited::Cost();
    if (!Core.Features.test(xrCore::Feature::wpn_cost_include_addons))
        return res;
    for (u32 i = 0; i < eMagazine; i++)
    {
        if (AddonAttachable(i) && IsAddonAttached(i))
            res += pSettings->r_float(GetAddonName(i), "cost");
    }
    return res;
}

void CWeapon::Hide(bool now)
{
    if (now)
    {
        OnStateSwitch(eHidden, GetState());
        SetState(eHidden);
        StopHUDSounds();
    }
    else
        SwitchState(eHiding);
    OnZoomOut();
}

void CWeapon::Show(bool now)
{
    if (now)
    {
        StopCurrentAnimWithoutCallback();
        OnStateSwitch(eIdle, GetState());
        SetState(eIdle);
        StopHUDSounds();
    }
    else
        SwitchState(eShowing);
}

bool CWeapon::show_crosshair() { return psActorFlags.test(AF_CROSSHAIR_DBG) || !(IsZoomed() && ZoomHideCrosshair()); }

bool CWeapon::show_indicators() { return !(IsZoomed() && !IsRotatingToZoom() && !m_bScopeShowIndicators); }

bool CWeapon::ParentMayHaveAimBullet() { return ParentIsActor(); }

bool CWeapon::ParentIsActor() const
{
    if (!H_Parent())
        return false;
    return smart_cast<const CActor*>(H_Parent());
}

const float& CWeapon::hit_probability() const
{
    VERIFY((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster));
    return (m_hit_probability[egdNovice]);
}

// Обновление необходимости включения второго вьюпорта +SecondVP+
// Вызывается только для активного оружия игрока
void CWeapon::UpdateSecondVP()
{
    // + CActor::UpdateCL();
    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (!pActor)
        return;
    CInventoryOwner* inv_owner = pActor->cast_inventory_owner();
    bool b_is_active_item = inv_owner && (inv_owner->m_inventory->ActiveItem() == this);
    R_ASSERT(b_is_active_item); // Эта функция должна вызываться только для оружия в руках нашего игрока
    bool bCond_1 = m_fZoomRotationFactor > 0.05f; // Мы должны целиться
    bool bCond_3 = pActor->cam_Active() == pActor->cam_FirstEye(); // Мы должны быть от 1-го лица
    Device.m_SecondViewport.SetSVPActive(bCond_1 && bCond_3 && SecondVPEnabled());
}

bool CWeapon::SecondVPEnabled() const
{
    bool bCond_2 = m_fSecondVPZoomFactor > 0.0f; // В конфиге должен быть прописан фактор зума (scope_lense_fov_factor) больше чем 0
    bool bCond_4 = !IsGrenadeMode(); // Мы не должны быть в режиме подствольника
    bool bcond_6 = psActorFlags.test(AF_3D_SCOPES);
    return bCond_2 && bCond_4 && bcond_6;
}

// Чувствительность мышкии с оружием в руках во время прицеливания
float CWeapon::GetControlInertionFactor() const
{
    float fInertionFactor = inherited::GetControlInertionFactor();
    if (IsZoomed() && !IsRotatingToZoom())
    {
        if (SecondVPEnabled() && m_bScopeDynamicZoom)
        {
            const float delta_factor_total = 1 - m_fSecondVPZoomFactor;
            float min_zoom_factor = 1 + delta_factor_total * m_fMinZoomK;
            float k = (m_fRTZoomFactor - min_zoom_factor) / (m_fSecondVPZoomFactor - min_zoom_factor);
            return (m_fScopeInertionFactor - fInertionFactor) * k + fInertionFactor;
        }
        else
            return (fInertionFactor + fInertionFactor * m_fAimControlInertionK);
    }
    return fInertionFactor;
}

float CWeapon::GetSecondVPFov() const
{
    float fov_factor = m_fSecondVPZoomFactor;
    if (m_bScopeDynamicZoom)
    {
        fov_factor = m_fRTZoomFactor;
    }
    return atanf(tanf(g_fov * (0.5f * PI / 180)) / fov_factor) / (0.5f * PI / 180);
}

// Получить HUD FOV от текущего оружия игрока
float CWeapon::GetHudFov()
{
    const float last_nw_hf = inherited::GetHudFov();
    if (m_fZoomRotationFactor > 0.0f)
    {
        if (SecondVPEnabled() && m_fSecondVPHudFov > 0.0f)
        {
            // В линзе зума
            const float fDiff = last_nw_hf - m_fSecondVPHudFov;
            return m_fSecondVPHudFov + (fDiff * (1 - m_fZoomRotationFactor));
        }
        if ((m_eScopeStatus == CSE_ALifeItemWeapon::eAddonDisabled || IsAddonAttached(eScope)) && !IsGrenadeMode() && m_fZoomHudFov > 0.0f)
        {
            // В процессе зума
            const float fDiff = last_nw_hf - m_fZoomHudFov;
            return m_fZoomHudFov + (fDiff * (1 - m_fZoomRotationFactor));
        }
    }
    return last_nw_hf;
}

void CWeapon::OnBulletHit()
{
    if (!fis_zero(conditionDecreasePerShotOnHit))
        ChangeCondition(-conditionDecreasePerShotOnHit);
}

void CWeapon::SaveAttachableParams()
{
    const char* sect_name = cNameSect().c_str();
    string_path buff;
    FS.update_path(buff, "$logs$", make_string("_world\\%s.ltx", sect_name).c_str());

    CInifile pHudCfg(buff, FALSE, FALSE, TRUE);

    sprintf_s(buff, "%f,%f,%f", m_Offset.c.x, m_Offset.c.y, m_Offset.c.z);
    pHudCfg.w_string(sect_name, "position", buff);

    Fvector ypr{};
    m_Offset.getHPB(ypr.x, ypr.y, ypr.z);
    ypr.mul(180.f / PI);
    sprintf_s(buff, "%f,%f,%f", ypr.x, ypr.y, ypr.z);
    pHudCfg.w_string(sect_name, "orientation", buff);

    if (pSettings->line_exist(sect_name, "strap_position") && pSettings->line_exist(sect_name, "strap_orientation"))
    {
        sprintf_s(buff, "%f,%f,%f", m_StrapOffset.c.x, m_StrapOffset.c.y, m_StrapOffset.c.z);
        pHudCfg.w_string(sect_name, "strap_position", buff);
        m_StrapOffset.getHPB(ypr.x, ypr.y, ypr.z);
        ypr.mul(180.f / PI);
        sprintf_s(buff, "%f,%f,%f", ypr.x, ypr.y, ypr.z);
        pHudCfg.w_string(sect_name, "strap_orientation", buff);
    }

    Msg("--[%s] data saved to [%s]", __FUNCTION__, pHudCfg.fname());
}

void CWeapon::ParseCurrentItem(CGameFont* F) { F->OutNext("WEAPON IN STRAPPED MODE: [%d]", m_strapped_mode); }

u32 CWeapon::GetNextAmmoType()
{
    u32 l_newType = m_set_next_ammoType_on_reload;
    bool mag_weapon = AddonAttachable(eMagazine);
    for (;;)
    {
        if (++l_newType >= m_ammoTypes.size())
        {
            for (l_newType = 0; l_newType < m_ammoTypes.size(); ++l_newType)
                if (unlimited_ammo() || m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[l_newType].c_str(), mag_weapon, &m_magazines))
                    break;
            break;
        }
        if (unlimited_ammo() || m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[l_newType].c_str(), mag_weapon, &m_magazines))
            break;
    }
    if (l_newType != m_set_next_ammoType_on_reload && l_newType < m_ammoTypes.size())
        return l_newType;
    else
        return u32(-1);
}

bool CWeapon::IsAddonAttached(u32 addon) const
{
    switch (addon)
    {
    case eSilencer:
        return (CSE_ALifeItemWeapon::eAddonAttachable == m_eSilencerStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer)) ||
            CSE_ALifeItemWeapon::eAddonPermanent == m_eSilencerStatus;
    case eScope:
        return (CSE_ALifeItemWeapon::eAddonAttachable == m_eScopeStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope)) ||
            CSE_ALifeItemWeapon::eAddonPermanent == m_eScopeStatus;
    case eLauncher:
        return (CSE_ALifeItemWeapon::eAddonAttachable == m_eGrenadeLauncherStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)) ||
            CSE_ALifeItemWeapon::eAddonPermanent == m_eGrenadeLauncherStatus;
    case eLaser:
        return (CSE_ALifeItemWeapon::eAddonAttachable == m_eLaserStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser)) ||
            CSE_ALifeItemWeapon::eAddonPermanent == m_eLaserStatus;
    case eFlashlight:
        return (CSE_ALifeItemWeapon::eAddonAttachable == m_eFlashlightStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight)) ||
            CSE_ALifeItemWeapon::eAddonPermanent == m_eFlashlightStatus;
    case eStock:
        return (CSE_ALifeItemWeapon::eAddonAttachable == m_eStockStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonStock)) ||
            CSE_ALifeItemWeapon::eAddonPermanent == m_eStockStatus;
    case eExtender:
        return (CSE_ALifeItemWeapon::eAddonAttachable == m_eExtenderStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonExtender)) ||
            CSE_ALifeItemWeapon::eAddonPermanent == m_eExtenderStatus;
    case eForend:
        return (CSE_ALifeItemWeapon::eAddonAttachable == m_eForendStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonForend)) ||
            CSE_ALifeItemWeapon::eAddonPermanent == m_eForendStatus;
    case eMagazine: return m_flagsWeaponState & CSE_ALifeItemWeapon::eWeaponMagazineAttached;
    default: return false;
    }
}

bool CWeapon::AddonAttachable(u32 addon) const
{
    switch (addon)
    {
    case eSilencer: return (CSE_ALifeItemWeapon::eAddonAttachable == m_eSilencerStatus);
    case eScope: return (CSE_ALifeItemWeapon::eAddonAttachable == m_eScopeStatus);
    case eLauncher: return (CSE_ALifeItemWeapon::eAddonAttachable == m_eGrenadeLauncherStatus);
    case eLaser: return (CSE_ALifeItemWeapon::eAddonAttachable == m_eLaserStatus);
    case eFlashlight: return (CSE_ALifeItemWeapon::eAddonAttachable == m_eFlashlightStatus);
    case eStock: return (CSE_ALifeItemWeapon::eAddonAttachable == m_eStockStatus);
    case eExtender: return (CSE_ALifeItemWeapon::eAddonAttachable == m_eExtenderStatus);
    case eForend: return (CSE_ALifeItemWeapon::eAddonAttachable == m_eForendStatus);
    case eMagazine: return !m_magazines.empty();
    default: return false;
    }
}

shared_str CWeapon::GetAddonName(u32 addon) const
{
    switch (addon)
    {
    case eSilencer: return m_silencers[m_cur_silencer];
    case eScope: return m_scopes[m_cur_scope];
    case eLauncher: return m_glaunchers[m_cur_glauncher];
    case eLaser: return m_lasers[m_cur_laser];
    case eFlashlight: return m_flashlights[m_cur_flashlight];
    case eStock: return m_stocks[m_cur_stock];
    case eExtender: return m_extenders[m_cur_extender];
    case eForend: return m_forends[m_cur_forend];
    case eMagazine: return m_magazines[m_cur_magazine];
    default: return nullptr;
    }
}

float CWeapon::GetHitPowerForActor() const { return fvHitPower[g_SingleGameDifficulty]; }

bool CWeapon::IsDirectReload(CWeaponAmmo* ammo)
{
    auto _it = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), ammo->m_ammoSect);
    if (_it == m_ammoTypes.end())
        return false;

    m_bDirectReload = true;
    m_pAmmo = ammo;

    auto i = std::distance(m_ammoTypes.begin(), _it);
    if (CanBeReloaded() && TryToGetAmmo(i))
    {
        m_ammoType = i;
        m_set_next_ammoType_on_reload = u32(-1);
        Reload();
    }
    else
        m_bDirectReload = false;

    return m_bDirectReload;
}

bool CWeapon::CanBeReloaded()
{
    return (iAmmoElapsed < iMagazineSize || IsMisfire() || AddonAttachable(eMagazine) || m_pAmmo && m_pAmmo->m_boxCurr && m_pAmmo->m_ammoSect != m_magazine.back().m_ammoSect);
}

bool CWeapon::CanBeUnloaded() { return GetAmmoElapsed() || GetAmmoElapsed2() || IsAddonAttached(eMagazine) && AddonAttachable(eMagazine); }

float CWeapon::GetCondDecPerShotToShow() const
{
    float silencer_dec_k = IsAddonAttached(eSilencer) && AddonAttachable(eSilencer) ? conditionDecreasePerShotSilencer : 0.f;
    return (conditionDecreasePerShot + conditionDecreasePerShot * silencer_dec_k);
}

const shared_str CWeapon::GetMagazineIconSect() const { return READ_IF_EXISTS(pSettings, r_string, GetAddonName(eMagazine), "mag_icon_sect", nullptr); }

bool CWeapon::ZoomHideCrosshair()
{
    auto* pA = smart_cast<CActor*>(H_Parent());
    if (pA && pA->active_cam() == eacLookAt)
        return false;
    return m_bHideCrosshairInZoom;
}

Fvector CWeapon::GetPositionForCollision() { return psHUD_Flags.test(HUD_CROSSHAIR_HARD) ? get_LastShootPoint() : inherited::GetPositionForCollision(); }
Fvector CWeapon::GetDirectionForCollision() { return psHUD_Flags.test(HUD_CROSSHAIR_HARD) ? get_LastFD() : inherited::GetDirectionForCollision(); }

void CWeapon::LoadAddonMeshes(LPCSTR section) 
{
    LPCSTR str = READ_IF_EXISTS(pSettings, r_string, section, "scope_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_scope_meshes.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, section, "silencer_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_silencer_meshes.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, section, "launcher_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_launcher_meshes.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, section, "laser_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_laser_meshes.push_back(atoi(_GetItem(str, i, mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, section, "flashlight_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_flashlight_meshes.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, section, "magazine_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_magazine_meshes.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, section, "stock_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_stock_meshes.push_back(u8(atoi(mesh_num)));
        }
}

void CWeapon::LoadAddonMeshesHud() 
{
    LPCSTR str = READ_IF_EXISTS(pSettings, r_string, hud_sect, "scope_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_scope_meshes_hud.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, hud_sect, "silencer_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_silencer_meshes_hud.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, hud_sect, "launcher_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_launcher_meshes_hud.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, hud_sect, "laser_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_laser_meshes_hud.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, hud_sect, "flashlight_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_flashlight_meshes_hud.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, hud_sect, "magazine_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_magazine_meshes_hud.push_back(u8(atoi(mesh_num)));
        }
    str = READ_IF_EXISTS(pSettings, r_string, hud_sect, "stock_meshes", nullptr);
    if (str)
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 mesh_num;
            _GetItem(str, i, mesh_num);
            m_stock_meshes_hud.push_back(u8(atoi(mesh_num)));
        }
}

/////////////////////////////////////////////////////visual addon attach/////////////////////////////////////////////////////////////

LPCSTR prefix_name[] = {
    "silencer",
    "scope",
    "grenade_launcher",
    "laser",
    "flashlight",
    "stock",
    "extender",
    "forend",
    "magazine",
};
extern int g_bHudAdjustMode;
void CWeapon::InitAddonsVisual()
{
    for (u32 i = 0; i < eMaxAddon; i++)
    {
        world_attach_addon_name[i] = nullptr;
        world_attach_visual[i] = nullptr;
        world_attach_bone_name[i] = nullptr;

        if (IsAddonAttached(i))
        {
            auto sect = cNameSect();
            LPCSTR addon_name = AddonAttachable(i) ? GetAddonName(i).c_str() : prefix_name[i];
            string1024 res_sect{};
            sprintf(res_sect, "%s_%s", addon_name, "attach_visual");
            LPCSTR visual_name = READ_IF_EXISTS(pSettings, r_string, sect, res_sect, nullptr);
            if (visual_name)
            {
             
                world_attach_addon_name[i] = addon_name;

                world_attach_visual[i] = ::Render->model_Create(visual_name);

                sprintf(res_sect, "%s_%s", addon_name, "attach_bone");
                world_attach_bone_name[i] = READ_IF_EXISTS(pSettings, r_string, sect, res_sect, "wpn_body");

                if (g_bHudAdjustMode)
                    continue;

                sprintf(res_sect, "%s_%s", addon_name, "attach_pos");
                world_attach_visual_offset[i][0] = READ_IF_EXISTS(pSettings, r_fvector3, sect, res_sect, Fvector{});

                sprintf(res_sect, "%s_%s", addon_name, "attach_rot");
                world_attach_visual_offset[i][1] = READ_IF_EXISTS(pSettings, r_fvector3, sect, res_sect, Fvector{});
            }
        }
    }
}
void CWeapon::InitAddonsVisualHud()
{ 
    for (u32 i = 0; i < eMaxAddon; i++)
    {
        hud_attach_addon_name[i] = nullptr;
        hud_attach_visual[i] = nullptr;
        hud_attach_bone_name[i] = nullptr;

        if (IsAddonAttached(i))
        {
            auto& sect = hud_sect;
            LPCSTR addon_name = AddonAttachable(i) ? GetAddonName(i).c_str() : prefix_name[i];
            string1024 res_sect{};
            sprintf(res_sect, "%s_%s", addon_name, "attach_visual");
            LPCSTR visual_name = READ_IF_EXISTS(pSettings, r_string, sect, res_sect, nullptr);
            if (visual_name)
            {
                hud_attach_addon_name[i] = addon_name;

                hud_attach_visual[i] = ::Render->model_Create(visual_name);

                sprintf(res_sect, "%s_%s", addon_name, "attach_bone");
                hud_attach_bone_name[i] = READ_IF_EXISTS(pSettings, r_string, sect, res_sect, "wpn_body");

                if (g_bHudAdjustMode)
                    continue;

                sprintf(res_sect, "%s_%s", addon_name, "attach_pos");
                hud_attach_visual_offset[i][0] = READ_IF_EXISTS(pSettings, r_fvector3, sect, res_sect, Fvector{});

                sprintf(res_sect, "%s_%s", addon_name, "attach_rot");
                hud_attach_visual_offset[i][1] = READ_IF_EXISTS(pSettings, r_fvector3, sect, res_sect, Fvector{});
            }
        }
    }
}