#include "stdafx.h"
#include "torch.h"
#include "actor.h"
#include "../xr_3da/LightAnimLibrary.h"
#include "PhysicsShell.h"
#include "xrserver_objects_alife_items.h"

#include "HUDManager.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xr_3da/camerabase.h"
#include "inventory.h"

#include "UIGameCustom.h"
#include "actorEffector.h"
#include "HUDTarget.h"

static const float TIME_2_HIDE = 5.f;
static const float TORCH_INERTION_CLAMP = PI_DIV_6;
static const float TORCH_INERTION_SPEED_MAX = 7.5f;
static const float TORCH_INERTION_SPEED_MIN = 0.5f;
static Fvector TORCH_OFFSET = {-0.2f, +0.1f, -0.3f};
static const Fvector OMNI_OFFSET = {-0.2f, +0.1f, -0.1f};
static const float OPTIMIZATION_DISTANCE = 100.f;

static bool stalker_use_dynamic_lights = false;

extern ENGINE_API int g_current_renderer;

CTorch::CTorch(void)
{
    light_render = ::Render->light_create();
    light_render->set_type(IRender_Light::SPOT);
    light_render->set_shadow(true);
    light_render->set_moveable(true);
    light_omni = ::Render->light_create();
    light_omni->set_type(IRender_Light::POINT);
    light_omni->set_shadow(false);
    light_omni->set_moveable(true);

    // Disabling shift by x and z axes for 1st render,
    // because we don't have dynamic lighting in it.
    if (g_current_renderer == 1)
    {
        TORCH_OFFSET.x = 0;
        TORCH_OFFSET.z = 0;
    }

    m_camera_torch_offset = TORCH_OFFSET;
    m_camera_omni_offset = OMNI_OFFSET;
}

CTorch::~CTorch(void)
{
    light_render.destroy();
    light_omni.destroy();
}

void CTorch::Load(LPCSTR section)
{
    inherited::Load(section);
    m_light_descr_sect = READ_IF_EXISTS(pSettings, r_string, section, "light_definition", nullptr);
    LoadLightDefinitions(m_light_descr_sect);
}

void CTorch::Switch() { Switch(!m_switched_on); }

void CTorch::Switch(bool turn_on)
{
    if (!m_light_descr_sect)
        return;

    inherited::Switch(turn_on);

    m_switched_on = turn_on;
    light_render->set_active(turn_on);
    light_omni->set_active(turn_on);

    if (!!light_trace_bone)
    {
        IKinematics* pVisual = smart_cast<IKinematics*>(Visual());
        VERIFY(pVisual);
        u16 bi = pVisual->LL_BoneID(light_trace_bone);

        pVisual->LL_SetBoneVisible(bi, turn_on, TRUE);
        pVisual->CalculateBones(TRUE);
    }
}

bool CTorch::IsPowerOn() const { return (m_switched_on); }

BOOL CTorch::net_Spawn(CSE_Abstract* DC)
{
    auto torch = smart_cast<CSE_ALifeItemTorch*>(DC);
    R_ASSERT(torch);
    cNameVisual_set(torch->get_visual());

    R_ASSERT(!CFORM());
    R_ASSERT(smart_cast<IKinematics*>(Visual()));
    collidable.model = xr_new<CCF_Skeleton>(this);

    if (!inherited::net_Spawn(DC))
        return (FALSE);

    //включить/выключить фонарик
    Switch(torch->m_active);
    VERIFY(!torch->m_active || (torch->ID_Parent != 0xffff));

    return (TRUE);
}

void CTorch::net_Destroy()
{
    Switch(false);
    inherited::net_Destroy();
}

void CTorch::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
    m_focus.set(Position());
}

void CTorch::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);
    time2hide = TIME_2_HIDE;

    Switch(false);
}

void CTorch::UpdateCL()
{
    inherited::UpdateCL();

    if (!m_switched_on)
        return;

    CActor* actor = smart_cast<CActor*>(H_Parent());

#pragma todo("Simp: Добавить отдельную команду/настройку?")
    //light_render->set_shadow(actor || !Core.Features.test(xrCore::Feature::npc_simplified_shooting));

    if (useVolumetric)
    {
        if (actor)
            light_render->set_volumetric(useVolumetricForActor);
        else
            light_render->set_volumetric(psActorFlags.test(AF_AI_VOLUMETRIC_LIGHTS));
    }

    CBoneInstance& BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(guid_bone);
    Fmatrix M{};

    if (H_Parent())
    {
        // if (actor)
        // {
// todo("переделать под новый рендер!")
            // light_render->set_actor_torch(true);
        // }

        if (H_Parent()->XFORM().c.distance_to_sqr(Device.vCameraPosition) < _sqr(OPTIMIZATION_DISTANCE))
        {
            // near camera

            smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones(TRUE);

            M.mul_43(XFORM(), BI.mTransform);
        }
        else
        {
            // approximately the same
            M = H_Parent()->XFORM();
            H_Parent()->Center(M.c);
            M.c.y += H_Parent()->Radius() * 2.f / 3.f;
        }

        if (actor && actor->g_Alive())
        {
            if (actor->active_cam() == eacLookAt)
            {
                m_prev_hp.x =
                    angle_inertion_var(m_prev_hp.x, -actor->cam_Active()->yaw, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
                m_prev_hp.y =
                    angle_inertion_var(m_prev_hp.y, -actor->cam_Active()->pitch, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
            }
            else
            {
                m_prev_hp.x =
                    angle_inertion_var(m_prev_hp.x, -actor->cam_FirstEye()->yaw, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
                m_prev_hp.y =
                    angle_inertion_var(m_prev_hp.y, -actor->cam_FirstEye()->pitch, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
            }

            Fvector dir, pos, right, up;
            if (m_bind_to_camera && actor->active_cam() == eacFirstEye)
            {
                float target_dist = HUD().GetTarget()->GetRealDist();
                if (m_min_target_dist > 0.f && target_dist >= 0.f && target_dist < m_min_target_dist)
                    target_dist = m_min_target_dist - target_dist;
                else
                    target_dist = 0.f;

                dir = actor->Cameras().Direction();
                Fvector::generate_orthonormal_basis_normalized(dir, up, right);
                pos = actor->Cameras().Position();
                Fvector offset = pos;
                offset.mad(right, m_camera_torch_offset.x);
                offset.mad(up, m_camera_torch_offset.y);
                offset.mad(dir, m_camera_torch_offset.z - target_dist);
                light_render->set_position(offset);
                offset = pos;
                offset.mad(right, m_camera_omni_offset.x);
                offset.mad(up, m_camera_omni_offset.y);
                offset.mad(dir, m_camera_omni_offset.z - target_dist);
                light_omni->set_position(offset);
            }
            else
            {
                dir.setHP(m_prev_hp.x + m_delta_h, m_prev_hp.y);
                Fvector::generate_orthonormal_basis_normalized(dir, up, right);
                pos = M.c;
                Fvector offset = pos;
                offset.mad(M.i, TORCH_OFFSET.x);
                offset.mad(M.j, TORCH_OFFSET.y);
                offset.mad(M.k, TORCH_OFFSET.z);
                light_render->set_position(offset);
                offset = pos;
                offset.mad(M.i, OMNI_OFFSET.x);
                offset.mad(M.j, OMNI_OFFSET.y);
                offset.mad(M.k, OMNI_OFFSET.z);
                light_omni->set_position(offset);
            }

			light_render->set_rotation( dir, right );
			light_omni->set_rotation( dir, right );
		}// if(actor)
		else 
		{
			light_render->set_position	(M.c);
			light_render->set_rotation	(M.k,M.i);

            Fvector offset = M.c;
            offset.mad(M.i, OMNI_OFFSET.x);
            offset.mad(M.j, OMNI_OFFSET.y);
            offset.mad(M.k, OMNI_OFFSET.z);
            light_omni->set_position(M.c);
            light_omni->set_rotation(M.k, M.i);
        }
    }
    else
    {
// todo("переделать под новый рендер!")
        // light_render->set_actor_torch(false);
        if (getVisible() && m_pPhysicsShell)
        {
            M.mul(XFORM(), BI.mTransform);

            //. what should we do in case when
            // light_render is not active at this moment,
            // but m_switched_on is true?
            //			light_render->set_rotation	(M.k,M.i);
            //			light_render->set_position	(M.c);
            //			glow_render->set_position	(M.c);
            //			glow_render->set_direction	(M.k);
            //
            //			time2hide					-= Device.fTimeDelta;
            //			if (time2hide<0)
            {
                m_switched_on = false;
                light_render->set_active(false);
                light_omni->set_active(false);
            }
        } // if (getVisible() && m_pPhysicsShell)
    }

    if (!m_switched_on)
        return;

    // calc color animator
    if (!lanim)
        return;

    int frame;
    // возвращает в формате BGR
    u32 clr = lanim->CalculateBGR(Device.fTimeGlobal, frame);

    Fcolor fclr{};
    fclr.set((float)color_get_B(clr) / 255.f, (float)color_get_G(clr) / 255.f, (float)color_get_R(clr) / 255.f, 1.f);
    fclr.mul_rgb(fBrightness);

    light_render->set_color(fclr);
    light_omni->set_color(fclr);
}

void CTorch::create_physic_shell() { CPhysicsShellHolder::create_physic_shell(); }

void CTorch::activate_physic_shell() { CPhysicsShellHolder::activate_physic_shell(); }

void CTorch::setup_physic_shell() { CPhysicsShellHolder::setup_physic_shell(); }

void CTorch::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    CSE_ALifeItemTorch* torch = smart_cast<CSE_ALifeItemTorch*>(E);
    torch->m_active = m_switched_on;
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    torch->m_attached = (pA && pA->attached(this));
}

bool CTorch::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    return pA ? (pA->inventory().m_slots[GetSlot()].m_pIItem == this) : true;
}
void CTorch::afterDetach()
{
    inherited::afterDetach();
    Switch(false);
}
void CTorch::renderable_Render(u32 context_id, IRenderable* root) { inherited::renderable_Render(context_id, root); }

void CTorch::calc_m_delta_h(float range) { m_delta_h = PI_DIV_2 - atan((range * 0.5f) / _abs(TORCH_OFFSET.x)); }

float CTorch::get_range() const { return light_render->get_range(); }

void CTorch::LoadLightDefinitions(shared_str light_sect)
{
    if (!light_sect)
        return;

    light_trace_bone = READ_IF_EXISTS(pSettings, r_string, light_sect, "light_trace_bone", nullptr);

    constexpr bool b_r2 = true;

    lanim = LALib.FindItem(pSettings->r_string(light_sect, "color_animator"));
    guid_bone = smart_cast<IKinematics*>(Visual())->LL_BoneID(pSettings->r_string(light_sect, "guide_bone"));
    VERIFY(guid_bone != BI_NONE);

    m_color = pSettings->r_fcolor(light_sect, b_r2 ? "color_r2" : "color");
    fBrightness = m_color.intensity();
    m_fRange = pSettings->r_float(light_sect, (b_r2) ? "range_r2" : "range");
    light_render->set_color(m_color);
    light_render->set_range(m_fRange);

    if (b_r2)
    {
        useVolumetric = READ_IF_EXISTS(pSettings, r_bool, light_sect, "volumetric_enabled", true);
        useVolumetricForActor = READ_IF_EXISTS(pSettings, r_bool, light_sect, "volumetric_for_actor", false);
        light_render->set_volumetric(useVolumetric);
        if (useVolumetric)
        {
            float volIntensity = READ_IF_EXISTS(pSettings, r_float, light_sect, "volumetric_intensity", 0.2f);
            volIntensity = std::clamp(volIntensity, 0.f, 1.f);
            light_render->set_volumetric_intensity(volIntensity);

            float volDistance = READ_IF_EXISTS(pSettings, r_float, light_sect, "volumetric_distance", 1.f);
            volDistance = std::clamp(volDistance, 0.f, 1.f);
            light_render->set_volumetric_distance(volDistance);
        }
    }

    Fcolor clr_o = pSettings->r_fcolor(light_sect, (b_r2) ? "omni_color_r2" : "omni_color");
    float range_o = pSettings->r_float(light_sect, (b_r2) ? "omni_range_r2" : "omni_range");
    light_omni->set_color(clr_o);
    light_omni->set_range(range_o);

    light_render->set_cone(deg2rad(pSettings->r_float(light_sect, "spot_angle")));
    light_render->set_texture(pSettings->r_string(light_sect, "spot_texture"));

    calc_m_delta_h(m_fRange);

    m_bind_to_camera = READ_IF_EXISTS(pSettings, r_bool, light_sect, "bind_to_camera", false);
    m_camera_torch_offset = READ_IF_EXISTS(pSettings, r_fvector3, light_sect, "camera_torch_offset", TORCH_OFFSET);
    m_camera_omni_offset = READ_IF_EXISTS(pSettings, r_fvector3, light_sect, "camera_omni_offset", OMNI_OFFSET);
    m_min_target_dist = READ_IF_EXISTS(pSettings, r_float, light_sect, "camera_min_target_dist", m_min_target_dist);

	light_render->set_type((IRender_Light::LT)(READ_IF_EXISTS(pSettings, r_u8, light_sect, "type", IRender_Light::SPOT)));
    light_omni->set_type((IRender_Light::LT)(READ_IF_EXISTS(pSettings, r_u8, light_sect, "omni_type", IRender_Light::POINT)));
}