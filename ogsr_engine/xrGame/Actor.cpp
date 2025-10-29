#include "stdafx.h"
#include "Actor_Flags.h"
#include "hudmanager.h"
#ifdef DEBUG
#include "ode_include.h"
#include "PHDebug.h"
#endif // DEBUG
#include "alife_space.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "xrserver_objects_alife_monsters.h"
#include "CameraLook.h"
#include "CameraFirstEye.h"
#include "player_hud.h"
#include "effectorfall.h"
#include "EffectorBobbing.h"
#include "clsid_game.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "SleepEffector.h"
#include "character_info.h"
#include "CustomOutfit.h"
#include "actorcondition.h"
#include "UIGameCustom.h"


// breakpoints
#include "../xr_3da/xr_input.h"

//
#include "Actor.h"
#include "actor_anim_defs.h"
#include "HudItem.h"
#include "ai_sounds.h"
#include "ai_space.h"
#include "inventory.h"
#include "Physics.h"
#include "level.h"
#include "GamePersistent.h"
#include "game_cl_base.h"
#include "game_cl_single.h"
#include "xrmessages.h"
#include "string_table.h"
#include "usablescriptobject.h"
#include "ExtendedGeom.h"
#include "alife_registry_wrappers.h"
#include "../Include/xrRender/Kinematics.h"
#include "..\Include/xrRender/KinematicsAnimated.h"
#include "artifact.h"
#include "CharacterPhysicsSupport.h"
#include "material_manager.h"
#include "IColisiondamageInfo.h"
#include "ui/UIMainIngameWnd.h"
#include "map_manager.h"
#include "GameTaskManager.h"
#include "actor_memory.h"
#include "Script_Game_Object.h"
#include "Game_Object_Space.h"
#include "script_callback_ex.h"
#include "InventoryBox.h"
#include "location_manager.h"
#include "PHCapture.h"
#include "CustomDetector.h"
#include "InventoryContainer.h"
#include "Helmet.h"

// Tip for action for object we're looking at
constexpr const char* m_sCarCharacterUseAction = "car_character_use";
constexpr const char* m_sCharacterUseAction = "character_use";
constexpr const char* m_sDeadCharacterUseAction = "dead_character_use";
constexpr const char* m_sDeadCharacterUseOrDragAction = "dead_character_use_or_drag";
constexpr const char* m_sInventoryItemUseAction = "inventory_item_use";
constexpr const char* m_sInventoryItemUseOrDragAction = "inventory_item_use_or_drag";
constexpr const char* m_sGameObjectDragAction = "game_object_drag";

constexpr const char* m_sInventoryContainerUseOrTakeAction = "inventory_container_use_or_take"; // обшукати/підібрати контейнер
constexpr const char* m_sInventoryBoxUseAction = "inventory_box_use"; // обшукати скриньку
constexpr const char* m_sGameObjectThrowDropAction = "game_object_throw_drop"; // Відкинути/відпустити предмет
constexpr const char* m_sGameObjectDropAction = "game_object_drop"; // Відпустити предмет

const u32 patch_frames = 50;
const float respawn_delay = 1.f;
const float respawn_auto = 7.f;

static float IReceived = 0;
static float ICoincidenced = 0;

// skeleton
static Fbox bbStandBox;
static Fbox bbCrouchBox;
static Fvector vFootCenter;
static Fvector vFootExt;

Flags32 psActorFlags{AF_KEYPRESS_ON_START | AF_CAM_COLLISION | AF_CAM_COLLISION_COP | AF_AI_VOLUMETRIC_LIGHTS | AF_3D_PDA | AF_ALWAYSRUN | AF_FIRST_PERSON_DEATH};

static bool updated{};

CActor::CActor() : CEntityAlive(), current_ik_cam_shift(0)
{
    encyclopedia_registry = xr_new<CEncyclopediaRegistryWrapper>();
    game_news_registry = xr_new<CGameNewsRegistryWrapper>();
    // Cameras
    cameras[eacFirstEye] = xr_new<CCameraFirstEye>(this);
    cameras[eacFirstEye]->Load("actor_firsteye_cam");

    if constexpr (true /*strstr(Core.Params,"-psp")*/)
        psActorFlags.set(AF_PSP, TRUE);
    else
        psActorFlags.set(AF_PSP, FALSE);

    if (psActorFlags.test(AF_PSP))
    {
        cameras[eacLookAt] = xr_new<CCameraLook2>(this);
        cameras[eacLookAt]->Load("actor_look_cam_psp");
    }
    else
    {
        cameras[eacLookAt] = xr_new<CCameraLook>(this);
        cameras[eacLookAt]->Load("actor_look_cam");
    }
    cameras[eacFreeLook] = xr_new<CCameraLook>(this);
    cameras[eacFreeLook]->Load("actor_free_cam");

    m_fFallTime = s_fFallTime;

#ifdef DEBUG
    Device.seqRender.Add(this, REG_PRIORITY_LOW);
#endif

    //разрешить использование пояса в inventory
    inventory().SetBeltUseful(true);

    m_anims = xr_new<SActorMotions>();
    m_vehicle_anims = xr_new<SActorVehicleAnims>();
    //-----------------------------------------------------------------------------------
    m_memory = xr_new<CActorMemory>(this);
    //-----------------------------------------------------------------------------------

    m_location_manager = xr_new<CLocationManager>(this);

    m_ActorItemBoostedParam.clear();
    m_ActorItemBoostedParam.resize(eRestoreBoostMax);
}

CActor::~CActor()
{
    xr_delete(m_location_manager);

    xr_delete(m_memory);

    xr_delete(encyclopedia_registry);
    xr_delete(game_news_registry);
#ifdef DEBUG
    Device.seqRender.Remove(this);
#endif
    // xr_delete(Weapons);
    for (int i = 0; i < eacMaxCam; ++i)
        xr_delete(cameras[i]);

    m_HeavyBreathSnd.destroy();
    m_BloodSnd.destroy();

    xr_delete(m_pActorEffector);

    xr_delete(m_pSleepEffector);

    xr_delete(m_pPhysics_support);

    xr_delete(m_anims);
    xr_delete(m_vehicle_anims);
}

void CActor::reinit()
{
    character_physics_support()->movement()->CreateCharacter();
    character_physics_support()->movement()->SetPhysicsRefObject(this);
    CEntityAlive::reinit();
    CInventoryOwner::reinit();

    character_physics_support()->in_Init();
    material().reinit();

    m_pUsableObject = nullptr;
    memory().reinit();

    set_input_external_handler(0);
    m_time_lock_accel = 0;
}

void CActor::reload(LPCSTR section)
{
    CEntityAlive::reload(section);
    CInventoryOwner::reload(section);
    material().reload(section);
    CStepManager::reload(section);
    memory().reload(section);
    m_location_manager->reload(section);
}

void CActor::Load(LPCSTR section)
{
    // Msg						("Loading actor: %s",section);
    inherited::Load(section);
    material().Load(section);
    CInventoryOwner::Load(section);
    m_location_manager->Load(section);

    OnDifficultyChanged();
    //////////////////////////////////////////////////////////////////////////
    ISpatial* self = smart_cast<ISpatial*>(this);
    if (self)
    {
        self->spatial.type |= STYPE_VISIBLEFORAI;
        self->spatial.type &= ~STYPE_REACTTOSOUND;
    }
    //////////////////////////////////////////////////////////////////////////

    // m_PhysicMovementControl: General
    // m_PhysicMovementControl->SetParent		(this);
    Fbox bb;
    Fvector vBOX_center, vBOX_size;
    // m_PhysicMovementControl: BOX
    vBOX_center = pSettings->r_fvector3(section, "ph_box2_center");
    vBOX_size = pSettings->r_fvector3(section, "ph_box2_size");
    bb.set(vBOX_center, vBOX_center);
    bb.grow(vBOX_size);
    character_physics_support()->movement()->SetBox(2, bb);

    if (pSettings->line_exist(section, "ph_box4_center") && pSettings->line_exist(section, "ph_box4_size"))
    {
        vBOX_center = pSettings->r_fvector3(section, "ph_box4_center");
        vBOX_size = pSettings->r_fvector3(section, "ph_box4_size");
        bb.set(vBOX_center, vBOX_center);
        bb.grow(vBOX_size);
        character_physics_support()->movement()->SetBox(4, bb);
    }
    else
        character_physics_support()->movement()->SetBox(4, bb);

    // m_PhysicMovementControl: BOX
    vBOX_center = pSettings->r_fvector3(section, "ph_box1_center");
    vBOX_size = pSettings->r_fvector3(section, "ph_box1_size");
    bb.set(vBOX_center, vBOX_center);
    bb.grow(vBOX_size);
    character_physics_support()->movement()->SetBox(1, bb);

    if (pSettings->line_exist(section, "ph_box3_center") && pSettings->line_exist(section, "ph_box3_size"))
    {
        vBOX_center = pSettings->r_fvector3(section, "ph_box3_center");
        vBOX_size = pSettings->r_fvector3(section, "ph_box3_size");
        bb.set(vBOX_center, vBOX_center);
        bb.grow(vBOX_size);
        character_physics_support()->movement()->SetBox(3, bb);
    }
    else
        character_physics_support()->movement()->SetBox(3, bb);

    // m_PhysicMovementControl: BOX
    vBOX_center = pSettings->r_fvector3(section, "ph_box0_center");
    vBOX_size = pSettings->r_fvector3(section, "ph_box0_size");
    bb.set(vBOX_center, vBOX_center);
    bb.grow(vBOX_size);
    character_physics_support()->movement()->SetBox(0, bb);

    //// m_PhysicMovementControl: Foots
    // Fvector	vFOOT_center= pSettings->r_fvector3	(section,"ph_foot_center"	);
    // Fvector	vFOOT_size	= pSettings->r_fvector3	(section,"ph_foot_size"		);
    // bb.set	(vFOOT_center,vFOOT_center); bb.grow(vFOOT_size);
    ////m_PhysicMovementControl->SetFoots	(vFOOT_center,vFOOT_size);

    // m_PhysicMovementControl: Crash speed and mass
    float cs_min = pSettings->r_float(section, "ph_crash_speed_min");
    float cs_max = pSettings->r_float(section, "ph_crash_speed_max");
    float mass = pSettings->r_float(section, "ph_mass");
    character_physics_support()->movement()->SetCrashSpeeds(cs_min, cs_max);
    character_physics_support()->movement()->SetMass(mass);
    if (pSettings->line_exist(section, "stalker_restrictor_radius"))
        character_physics_support()->movement()->SetActorRestrictorRadius(CPHCharacter::rtStalker, pSettings->r_float(section, "stalker_restrictor_radius"));
    if (pSettings->line_exist(section, "stalker_small_restrictor_radius"))
        character_physics_support()->movement()->SetActorRestrictorRadius(CPHCharacter::rtStalkerSmall, pSettings->r_float(section, "stalker_small_restrictor_radius"));
    if (pSettings->line_exist(section, "medium_monster_restrictor_radius"))
        character_physics_support()->movement()->SetActorRestrictorRadius(CPHCharacter::rtMonsterMedium, pSettings->r_float(section, "medium_monster_restrictor_radius"));
    character_physics_support()->movement()->Load(section);

    m_fWalkAccel = pSettings->r_float(section, "walk_accel");
    m_fJumpSpeed = pSettings->r_float(section, "jump_speed");
    m_fRunFactor = pSettings->r_float(section, "run_coef");
    m_fRunBackFactor = pSettings->r_float(section, "run_back_coef");
    m_fWalkBackFactor = pSettings->r_float(section, "walk_back_coef");
    m_fCrouchFactor = pSettings->r_float(section, "crouch_coef");
    m_fClimbFactor = pSettings->r_float(section, "climb_coef");
    m_fSprintFactor = pSettings->r_float(section, "sprint_koef");

    m_fWalk_StrafeFactor = READ_IF_EXISTS(pSettings, r_float, section, "walk_strafe_coef", 1.0f);
    m_fRun_StrafeFactor = READ_IF_EXISTS(pSettings, r_float, section, "run_strafe_coef", 1.0f);

    m_hit_slowmo_jump = READ_IF_EXISTS(pSettings, r_bool, section, "hit_slowmo_jump", false);

    m_fExoFactor = 1.0f;

    m_fCamHeightFactor = pSettings->r_float(section, "camera_height_factor");
    character_physics_support()->movement()->SetJumpUpVelocity(m_fJumpSpeed);
    float AirControlParam = pSettings->r_float(section, "air_control_param");
    character_physics_support()->movement()->SetAirControlParam(AirControlParam);

    m_fPickupInfoRadius = pSettings->r_float(section, "pickup_info_radius");

    character_physics_support()->in_Load(section);

    //загрузить параметры эффектора
    //	LoadShootingEffector	("shooting_effector");
    LoadSleepEffector("sleep_effector");

    //загрузить параметры смещения firepoint
    m_vMissileOffset = pSettings->r_fvector3(section, "missile_throw_offset");

    // Weapons				= xr_new<CWeaponList> (this);

    LPCSTR hit_snd_sect = pSettings->r_string(section, "hit_sounds");
    for (int hit_type = 0; hit_type < (int)ALife::eHitTypeMax; ++hit_type)
    {
        LPCSTR hit_name = ALife::g_cafHitType2String((ALife::EHitType)hit_type);
        LPCSTR hit_snds = READ_IF_EXISTS(pSettings, r_string, hit_snd_sect, hit_name, "");
        int cnt = _GetItemCount(hit_snds);
        string128 tmp;
        VERIFY(cnt != 0);
        for (int i = 0; i < cnt; ++i)
        {
            sndHit[hit_type].emplace_back().create(_GetItem(hit_snds, i, tmp), st_Effect, sg_SourceType);
        }
        char buf[256];

        ::Sound->create(sndDie[0], strconcat(sizeof(buf), buf, *cName(), "\\die0"), st_Effect, SOUND_TYPE_MONSTER_DYING);
        ::Sound->create(sndDie[1], strconcat(sizeof(buf), buf, *cName(), "\\die1"), st_Effect, SOUND_TYPE_MONSTER_DYING);
        ::Sound->create(sndDie[2], strconcat(sizeof(buf), buf, *cName(), "\\die2"), st_Effect, SOUND_TYPE_MONSTER_DYING);
        ::Sound->create(sndDie[3], strconcat(sizeof(buf), buf, *cName(), "\\die3"), st_Effect, SOUND_TYPE_MONSTER_DYING);

        m_HeavyBreathSnd.create(pSettings->r_string(section, "heavy_breath_snd"), st_Effect, SOUND_TYPE_MONSTER_INJURING);
        m_BloodSnd.create(pSettings->r_string(section, "heavy_blood_snd"), st_Effect, SOUND_TYPE_MONSTER_INJURING);
    }

    // KRodin: это, мне кажется, лишнее.
    // if( psActorFlags.test(AF_PSP) )
    //	cam_Set(eacLookAt);
    // else
    cam_Set(eacFirstEye);

    // sheduler
    shedule.t_min = shedule.t_max = 1;

    // настройки дисперсии стрельбы
    m_fDispBase = pSettings->r_float(section, "disp_base");
    m_fDispBase = deg2rad(m_fDispBase);

    m_fDispAim = pSettings->r_float(section, "disp_aim");
    m_fDispAim = deg2rad(m_fDispAim);

    m_fDispVelFactor = pSettings->r_float(section, "disp_vel_factor");
    m_fDispAccelFactor = pSettings->r_float(section, "disp_accel_factor");
    m_fDispCrouchFactor = pSettings->r_float(section, "disp_crouch_factor");
    m_fDispCrouchNoAccelFactor = pSettings->r_float(section, "disp_crouch_no_acc_factor");

    LPCSTR default_outfit = READ_IF_EXISTS(pSettings, r_string, section, "default_outfit", 0);
    SetDefaultVisualOutfit(default_outfit);

    //-----------------------------------------
    if (pSettings->line_exist(section, "lookout_angle"))
    {
        m_fLookoutAngle = pSettings->r_float(section, "lookout_angle");
        m_fLookoutAngle = deg2rad(m_fLookoutAngle);
    }
    else
        m_fLookoutAngle = ACTOR_LOOKOUT_ANGLE;

    // Alex ADD: for smooth crouch fix
    CurrentHeight = CameraHeight();

    m_news_to_show = READ_IF_EXISTS(pSettings, r_u32, section, "news_to_show", NEWS_TO_SHOW);

    m_fThrowImpulse = READ_IF_EXISTS(pSettings, r_float, "actor_capture", "throw_impulse", 5.0f);
}

void CActor::PHHit(SHit& H) { m_pPhysics_support->in_Hit(H, !g_Alive()); }

struct playing_pred
{
    IC bool operator()(ref_sound& s) { return (NULL != s._feedback()); }
};

void CActor::Hit(SHit* pHDS)
{
    pHDS->aim_bullet = false;

    SHit HDS = *pHDS;
    if (HDS.hit_type < ALife::eHitTypeBurn || HDS.hit_type >= ALife::eHitTypeMax)
    {
        string256 err;
        sprintf_s(err, "Unknown/unregistered hit type [%d]", HDS.hit_type);
        R_ASSERT2(0, err);
    }
#ifdef DEBUG
    if (ph_dbg_draw_mask.test(phDbgCharacterControl))
    {
        DBG_OpenCashedDraw();
        Fvector to;
        to.add(Position(), Fvector().mul(HDS.dir, HDS.phys_impulse()));
        DBG_DrawLine(Position(), to, D3DCOLOR_XRGB(124, 124, 0));
        DBG_ClosedCashedDraw(500);
    }
#endif // DEBUG

    callback(GameObject::entity_alive_before_hit)(&HDS);
    if (HDS.ignore_flag)
        return;

    bool bPlaySound = true;
    if (!g_Alive())
        bPlaySound = false;

    if (!sndHit[HDS.hit_type].empty() && (ALife::eHitTypeTelepatic != HDS.hit_type))
    {
        ref_sound& S = sndHit[HDS.hit_type][Random.randI(sndHit[HDS.hit_type].size())];
        bool b_snd_hit_playing = sndHit[HDS.hit_type].end() != std::find_if(sndHit[HDS.hit_type].begin(), sndHit[HDS.hit_type].end(), playing_pred());

        if (ALife::eHitTypeExplosion == HDS.hit_type)
        {
            if (this == Level().CurrentControlEntity())
            {
                S.set_volume(10.0f);
                if (!m_sndShockEffector)
                {
                    m_sndShockEffector = xr_new<SndShockEffector>();
                    m_sndShockEffector->Start(this, float(S.get_length_sec() * 1000.0f), HDS.damage());
                }
            }
            else
                bPlaySound = false;
        }
        if (bPlaySound && !b_snd_hit_playing)
        {
            Fvector point = Position();
            point.y += CameraHeight();
            S.play_at_pos(this, point);
        };
    }

    // slow actor, only when he gets hit
    hit_slowmo = conditions().HitSlowmo(pHDS);

    //---------------------------------------------------------------
    if (Level().CurrentViewEntity() == this && HDS.hit_type == ALife::eHitTypeFireWound)
    {
        CObject* pLastHitter = Level().Objects.net_Find(m_iLastHitterID);
        CObject* pLastHittingWeapon = Level().Objects.net_Find(m_iLastHittingWeaponID);
        HitSector(pLastHitter, pLastHittingWeapon);
    };

    if ((mstate_real & mcSprint) && Level().CurrentControlEntity() == this && conditions().DisableSprint(pHDS))
    {
        bool const is_special_burn_hit_2_self = (pHDS->who == this) && (pHDS->boneID == BI_NONE) && (pHDS->hit_type == ALife::eHitTypeBurn);
        if (!is_special_burn_hit_2_self)
            mstate_wishful &= ~mcSprint;
    }

    HitMark(&HDS);

    if (!GodMode())
    {
        HDS.power *= (1.f - GetArtefactsProtection(pHDS->hit_type));
        inherited::Hit(&HDS);
    }
}

void CActor::HitMark(SHit* pHDS)
{
    // hit marker
    if ((pHDS->type() == ALife::eHitTypeFireWound || pHDS->type() == ALife::eHitTypeWound_2) && g_Alive() && Local() && (Level().CurrentEntity() == this))
    {
        HUD().Hit(0, pHDS->damage(), pHDS->direction());

        {
            CEffectorCam* ce = Cameras().GetCamEffector((ECamEffectorType)effFireHit);
            if (!ce)
            {
                int id = -1;
                Fvector cam_pos, cam_dir, cam_norm, hit_dir{pHDS->direction()};
                cam_Active()->Get(cam_pos, cam_dir, cam_norm);
                cam_dir.normalize_safe();
                hit_dir.normalize_safe();

                float ang_diff = angle_difference(cam_dir.getH(), hit_dir.getH());
                Fvector cp{};
                cp.crossproduct(cam_dir, hit_dir);
                bool bUp = (cp.y > 0.0f);

                Fvector cross{};
                cross.crossproduct(cam_dir, hit_dir);
                VERIFY(ang_diff >= 0.0f && ang_diff <= PI);

                float _s1 = PI_DIV_8;
                float _s2 = _s1 + PI_DIV_4;
                float _s3 = _s2 + PI_DIV_4;
                float _s4 = _s3 + PI_DIV_4;

                if (ang_diff <= _s1)
                {
                    id = 2;
                }
                else if (ang_diff > _s1 && ang_diff <= _s2)
                {
                    id = (bUp) ? 5 : 7;
                }
                else if (ang_diff > _s2 && ang_diff <= _s3)
                {
                    id = (bUp) ? 3 : 1;
                }
                else if (ang_diff > _s3 && ang_diff <= _s4)
                {
                    id = (bUp) ? 4 : 6;
                }
                else if (ang_diff > _s4)
                {
                    id = 0;
                }
                else
                {
                    VERIFY(0);
                }

                string64 sect_name;
                sprintf_s(sect_name, "effector_fire_hit_%d", id);
                AddEffector(this, effFireHit, sect_name, pHDS->damage() / 100.0f);
            }
        }
    }
}

void CActor::HitSignal(float perc, Fvector& vLocalDir, CObject* who, s16 element, int type)
{
    if (g_Alive())
    {
        // stop-motion
        if (character_physics_support()->movement()->Environment() == CPHMovementControl::peOnGround ||
            character_physics_support()->movement()->Environment() == CPHMovementControl::peAtWall)
        {
            Fvector zeroV;
            zeroV.set(0, 0, 0);
            character_physics_support()->movement()->SetVelocity(zeroV);
        }

        // check damage bone
        Fvector D;
        XFORM().transform_dir(D, vLocalDir);

        float yaw, pitch;
        D.getHP(yaw, pitch);
        IRenderVisual* pV = Visual();
        IKinematicsAnimated* tpKinematics = smart_cast<IKinematicsAnimated*>(pV);
        IKinematics* pK = smart_cast<IKinematics*>(pV);
        VERIFY(tpKinematics);
#pragma todo("Dima to Dima : forward-back bone impulse direction has been determined incorrectly!")
        MotionID motion_ID =
            m_anims->m_normal.m_damage[iFloor(pK->LL_GetBoneInstance(element).get_param(1) + (angle_difference(r_model_yaw + r_model_yaw_delta, yaw) <= PI_DIV_2 ? 0 : 1))];
        float power_factor = perc / 100.f;
        clamp(power_factor, 0.f, 1.f);
        VERIFY(motion_ID.valid());
        tpKinematics->PlayFX(motion_ID, power_factor);
        callback(GameObject::eHit)(lua_game_object(), perc, vLocalDir, smart_cast<const CGameObject*>(who)->lua_game_object(), element, type);
    }
}
void start_tutorial(LPCSTR name);
void CActor::Die(CObject* who)
{
    inherited::Die(who);

    {
        xr_vector<CInventorySlot>::iterator I = inventory().m_slots.begin();
        xr_vector<CInventorySlot>::iterator E = inventory().m_slots.end();

        for (u32 slot_idx = 0; I != E; ++I, ++slot_idx)
        {
            if (slot_idx == inventory().GetActiveSlot())
            {
                if ((*I).m_pIItem)
                {
                    (*I).m_pIItem->SetDropManual(TRUE);
                }
                continue;
            }
            else
            {
                CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>((*I).m_pIItem);
                if (pOutfit)
                    continue;
            };
            if ((*I).m_pIItem)
                inventory().Ruck((*I).m_pIItem);
        };

        ///!!! чистка пояса
        TIItemContainer& l_blist = inventory().m_belt;
        while (!l_blist.empty())
            inventory().Ruck(l_blist.front());
    }

    if (!psActorFlags.test(AF_FIRST_PERSON_DEATH))
    {
        cam_Set(eacFreeLook);
    }
    mstate_wishful &= ~mcAnyMove;
    mstate_real &= ~mcAnyMove;

    ::Sound->play_at_pos(sndDie[Random.randI(SND_DIE_COUNT)], this, Position());

    m_HeavyBreathSnd.stop();
    m_BloodSnd.stop();

    start_tutorial("game_over");
    xr_delete(m_sndShockEffector);
}

void CActor::SwitchOutBorder(bool new_border_state)
{
    if (new_border_state)
    {
        callback(GameObject::eExitLevelBorder)(lua_game_object());
    }
    else
    {
        //.		Msg("enter level border");
        callback(GameObject::eEnterLevelBorder)(lua_game_object());
    }
    m_bOutBorder = new_border_state;
}

void CActor::g_Physics(Fvector& _accel, float jump, float dt)
{
    // Correct accel
    Fvector accel;
    accel.set(_accel);
    hit_slowmo -= dt;
    if (hit_slowmo < 0)
        hit_slowmo = 0.f;

    accel.mul(1.f - hit_slowmo);

    if (g_Alive())
    {
        if (mstate_real & mcClimb && !cameras[eacFirstEye]->bClampYaw)
            accel.set(0.f, 0.f, 0.f);
        character_physics_support()->movement()->Calculate(accel, cameras[cam_active]->vDirection, 0, jump, dt, false);
        bool new_border_state = character_physics_support()->movement()->isOutBorder();
        if (m_bOutBorder != new_border_state && Level().CurrentControlEntity() == this)
        {
            SwitchOutBorder(new_border_state);
        }
        character_physics_support()->movement()->GetPosition(Position());
        character_physics_support()->movement()->bSleep = false;
    }

    if (Local() && g_Alive())
    {
        if (character_physics_support()->movement()->gcontact_Was)
            Cameras().AddCamEffector(xr_new<CEffectorFall>(character_physics_support()->movement()->gcontact_Power));
        if (!fis_zero(character_physics_support()->movement()->gcontact_HealthLost))
        {
            ICollisionDamageInfo* di = character_physics_support()->movement()->CollisionDamageInfo();
            bool b_hit_initiated = di->GetAndResetInitiated();
            Fvector hdir;
            di->HitDir(hdir);
            SetHitInfo(this, NULL, 0, Fvector().set(0, 0, 0), hdir);
            //				Hit
            //(m_PhysicMovementControl->gcontact_HealthLost,hdir,di->DamageInitiator(),m_PhysicMovementControl->ContactBone(),di->HitPos(),0.f,ALife::eHitTypeStrike);//s16(6 +
            //2*::Random.randI(0,2))
            if (Level().CurrentControlEntity() == this)
            {
                SHit HDS = SHit(character_physics_support()->movement()->gcontact_HealthLost, hdir, di->DamageInitiator(), character_physics_support()->movement()->ContactBone(),
                                di->HitPos(), 0.f, di->HitType(), 0.0f, b_hit_initiated);
                //				Hit(&HDS);

                NET_Packet l_P;
                HDS.GenHeader(GE_HIT, ID());
                HDS.whoID = di->DamageInitiator()->ID();
                HDS.weaponID = di->DamageInitiator()->ID();
                HDS.Write_Packet(l_P);

                u_EventSend(l_P);
            }
        }
    }
}
float g_fov = 67.5f; // 75.0f - SWM

float CActor::currentFOV()
{
    float current_fov = g_fov;
    const auto pWeapon = smart_cast<CWeapon*>(inventory().ActiveItem());

    if (eacFirstEye == cam_active && pWeapon && pWeapon->IsZoomed() && !pWeapon->IsRotatingToZoom())
        current_fov /= pWeapon->GetZoomFactor();

    return current_fov;
}

bool Is3dssZoomed{};

void CActor::UpdateCL()
{
    if (m_feel_touch_characters > 0)
    {
        for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
        {
            CPhysicsShellHolder* sh = smart_cast<CPhysicsShellHolder*>(*it);
            if (sh && sh->character_physics_support())
            {
                sh->character_physics_support()->movement()->UpdateObjectBox(character_physics_support()->movement()->PHCharacter());
            }
        }
    }

    m_snd_noise -= 0.3f * Device.fTimeDelta;

    VERIFY2(_valid(renderable.xform), *cName());
    inherited::UpdateCL();
    VERIFY2(_valid(renderable.xform), *cName());
    m_pPhysics_support->in_UpdateCL();
    VERIFY2(_valid(renderable.xform), *cName());

    if (g_Alive())
        PickupModeUpdate();

    PickupModeUpdate_COD();

    if (Level().CurrentEntity() && this->ID() == Level().CurrentEntity()->ID())
    {
        psHUD_Flags.set(HUD_DRAW_RT, true);
    }

    Is3dssZoomed = false;
    m_bZoomAimingMode = false;

    if (CWeapon* pWeapon = smart_cast<CWeapon*>(inventory().ActiveItem()))
    {
        if (pWeapon->IsZoomed())
        {
            if (auto ezi = smart_cast<CEffectorZoomInertion*>(Cameras().GetCamEffector(eCEZoom)))
                ezi->SetParams(pWeapon->GetControlInertionFactor() / GetExoFactor());

            m_bZoomAimingMode = true;
        }

        Is3dssZoomed = pWeapon->IsAiming() && pWeapon->Is3dssEnabled();

        if (Level().CurrentEntity() && this->ID() == Level().CurrentEntity()->ID())
        {
            psHUD_Flags.set(HUD_DRAW_RT, pWeapon->show_indicators());

            // Обновляем информацию об оружии в шейдерах
#pragma todo("Simp: подумать над тем что передавать в y и w")
            shader_exports.set_hud_params(Fvector4{pWeapon->GetZRotatingFactor(), pWeapon->CurrentZoomFactor(), pWeapon->GetLastHudFov(), pWeapon->GetScopeZoomFactor()});
        }
    }
    else
    {
        if (Level().CurrentEntity() && this->ID() == Level().CurrentEntity()->ID())
        {
            // Очищаем информацию об оружии в шейдерах
            shader_exports.set_hud_params({});
        }
    }

    UpdateDefferedMessages();

    if (g_Alive())
        CStepManager::update();

    spatial.type |= STYPE_REACTTOSOUND;

    if (m_sndShockEffector)
    {
        if (this == Level().CurrentViewEntity())
        {
            m_sndShockEffector->Update();

            if (!m_sndShockEffector->InWork() || !g_Alive())
                xr_delete(m_sndShockEffector);
        }
        else
            xr_delete(m_sndShockEffector);
    }

    Fmatrix trans;
    if (cam_Active() == cam_FirstEye())
    {
        Cameras().hud_camera_Matrix(trans);
    }
    else
    {
        Cameras().camera_Matrix(trans);
    }

    if (IsFocused())
    {
        trans.c.sub(Device.vCameraPosition);
        g_player_hud->update(trans);
    }

    {
        float outfit_cond{-1.f}, /*wpn_cond{-1.f}*/ helmet_cond{-1.f};
        if (auto outfit = GetOutfit())
            outfit_cond = outfit->GetCondition();
        //if (auto wpn = inventory().ActiveItem())
        //    wpn_cond = wpn->GetCondition();
        if (auto helmet = GetHelmet())
            helmet_cond = helmet->GetCondition();
        shader_exports.set_actor_params(Fvector{this->conditions().GetHealth(), outfit_cond, /*wpn_cond*/ helmet_cond});
    }

    // Обновление позиции актора для коллизии с травой/кустами
    if (ps_ssfx_grass_interactive.x > 0.f)
    {
        // Не знаю что лучше использовать - позицию камеры или актора. Вроде для травы с позицией актора получше выглядит. Для кустов - не понятно, что лучше.
        // grass_shader_data.pos[0].set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, -1);
        const auto& pos = Position();
        grass_shader_data.pos[0].set(pos.x, pos.y, pos.z, -1.f);
    }
    else
        grass_shader_data.pos[0].set(0.f, 0.f, 0.f, -1.f);
    grass_shader_data.dir[0].set(0.0f, -99.0f, 0.0f, 1.0f);
}

constexpr u32 TASKS_UPDATE_TIME = 1u;

float NET_Jump = 0;

void CActor::shedule_Update(u32 DT)
{
    setSVU(TRUE);

    BOOL bHudView = HUDview();
    if (bHudView)
    {
        CInventoryItem* pInvItem = inventory().ActiveItem();
        if (pInvItem)
        {
            CHudItem* pHudItem = smart_cast<CHudItem*>(pInvItem);
            if (pHudItem)
            {
                if (pHudItem->IsHidden())
                {
                    g_player_hud->detach_item(pHudItem);
                }
                else
                {
                    g_player_hud->attach_item(pHudItem);
                }
            }
        }
        else
        {
            g_player_hud->detach_item_idx(0);
            // Msg("---No active item in inventory(), item 0 detached.");
        }
    }
    else
    {
        g_player_hud->detach_all_items();
        // Msg("---No hud view found, all items detached.");
    }

    //обновление инвентаря
    UpdateInventoryOwner(DT);

    static u32 tasks_update_time = 0;
    if (tasks_update_time > TASKS_UPDATE_TIME)
    {
        tasks_update_time = 0;
        GameTaskManager().UpdateTasks();
    }
    else
    {
        tasks_update_time += DT;
    }

    if (/* m_holder || */ !getEnabled() || !Ready())
    {
        m_sDefaultObjAction = nullptr;
        inherited::shedule_Update(DT);

        /*		if (OnServer())
                {
                    Check_Weapon_ShowHideState();
                };
        */
        return;
    }

    if (m_holder)
        m_holder->UpdateEx(currentFOV());

    Device.Statistic->cam_Update.Begin();
    cam_Update(float(Device.dwTimeDelta) / 1000.0f, currentFOV());
    Device.Statistic->cam_Update.End();

    Device.OnCameraUpdated(true);

    //
    clamp(DT, 0u, 100u);
    float dt = float(DT) / 1000.f;

    // Check controls, create accel, prelimitary setup "mstate_real"

    //----------- for E3 -----------------------------
    //	if (Local() && (OnClient() || Level().CurrentEntity()==this))
    if (Level().CurrentControlEntity() == this && !m_holder)
    //------------------------------------------------
    {
        CCF_Skeleton* skeleton = smart_cast<CCF_Skeleton*>(CFORM());
        skeleton->Calculate();

        g_cl_CheckControls(mstate_wishful, NET_SavedAccel, NET_Jump, dt);

        g_cl_Orientate(mstate_real, dt);
        g_Orientate(mstate_real, dt);

        g_Physics(NET_SavedAccel, NET_Jump, dt);

        g_cl_ValidateMState(dt, mstate_wishful);
        g_SetAnimation(mstate_real);

        // Check for game-contacts
        Fvector C;
        float R;
        // m_PhysicMovementControl->GetBoundingSphere	(C,R);

        Center(C);
        R = Radius();
        feel_touch_update(C, R);

        // Dropping
        if (b_DropActivated)
        {
            f_DropPower += dt * 0.1f;
            clamp(f_DropPower, 0.f, 1.f);
        }
        else
        {
            f_DropPower = 0.f;
        }

        {
            //-----------------------------------------------------
            mstate_wishful &= ~mcAccel;
            mstate_wishful &= ~mcLStrafe;
            mstate_wishful &= ~mcRStrafe;
            mstate_wishful &= ~mcLLookout;
            mstate_wishful &= ~mcRLookout;
            mstate_wishful &= ~mcFwd;
            mstate_wishful &= ~mcBack;
            if (b_ClearCrouch)
                mstate_wishful &= ~mcCrouch;
            //-----------------------------------------------------
        }
    }
    else if (!m_holder)
    {
        //Этот код не должен быть взван
        R_ASSERT(0);
    }

    NET_Jump = 0;

    inherited::shedule_Update(DT);

    //эффектор включаемый при ходьбе
    if (!m_holder)
    {
        if (!pCamBobbing)
        {
            pCamBobbing = xr_new<CEffectorBobbing>();
            Cameras().AddCamEffector(pCamBobbing);
        }
        pCamBobbing->SetState(mstate_real, conditions().IsLimping(), IsZoomAimingMode());
    }

    //звук тяжелого дыхания при уталости и хромании
    if (this == Level().CurrentControlEntity())
    {
        if (conditions().IsLimping() && g_Alive())
        {
            if (!m_HeavyBreathSnd._feedback())
            {
                m_HeavyBreathSnd.play_at_pos(this, Fvector().set(0, ACTOR_HEIGHT, 0), sm_Looped | sm_2D);
            }
            else
            {
                m_HeavyBreathSnd.set_position(Fvector().set(0, ACTOR_HEIGHT, 0));
            }
        }
        else if (m_HeavyBreathSnd._feedback())
        {
            m_HeavyBreathSnd.stop();
        }

        float bs = conditions().BleedingSpeed();
        if (bs > 0.6f)
        {
            Fvector snd_pos;
            snd_pos.set(0, ACTOR_HEIGHT, 0);
            if (!m_BloodSnd._feedback())
                m_BloodSnd.play_at_pos(this, snd_pos, sm_Looped | sm_2D);
            else
                m_BloodSnd.set_position(snd_pos);

            float v = bs + 0.25f;

            m_BloodSnd.set_volume(v);
        }
        else
        {
            if (m_BloodSnd._feedback())
                m_BloodSnd.stop();
        }

        if (!g_Alive() && m_BloodSnd._feedback())
            m_BloodSnd.stop();
    }

    //если в режиме HUD, то сама модель актера не рисуется
    if (!character_physics_support()->IsRemoved() && !m_holder)
        setVisible(!HUDview());

    //что актер видит перед собой
    collide::rq_result& RQ = HUD().GetCurrentRayQuery();

    if (!input_external_handler_installed() && !m_holder && RQ.O && RQ.O->getVisible() && RQ.range < inventory().GetTakeDist())
    {
        m_pObjectWeLookingAt = smart_cast<CGameObject*>(RQ.O);
        m_pUsableObject = smart_cast<CUsableScriptObject*>(RQ.O);
        m_pInvBoxWeLookingAt = smart_cast<IInventoryBox*>(RQ.O);
        inventory().m_pTarget = smart_cast<PIItem>(RQ.O);
        m_pPersonWeLookingAt = smart_cast<CInventoryOwner*>(RQ.O);
        m_pVehicleWeLookingAt = smart_cast<CHolderCustom*>(RQ.O);
        auto pEntityAlive = smart_cast<CEntityAlive*>(RQ.O);
        auto ph_shell_holder = smart_cast<CPhysicsShellHolder*>(RQ.O);
        auto capture = character_physics_support()->movement()->PHCapture();

        bool b_allow_drag = ph_shell_holder && ph_shell_holder->ActorCanCapture();

        if (HUD().GetUI()->MainInputReceiver() || m_holder || !IsFreeHands())
        {
            m_sDefaultObjAction = nullptr;
        }
        else if (capture && capture->taget_object())
        { // щось у руках
            m_sDefaultObjAction = m_sGameObjectDropAction; // m_sGameObjectThrowDropAction;
        }
        else if (m_pUsableObject && m_pUsableObject->tip_text())
        {
            m_sDefaultObjAction = CStringTable().translate(m_pUsableObject->tip_text()).c_str();
        }
        else if (pEntityAlive)
        {
            if (m_pPersonWeLookingAt && pEntityAlive->g_Alive())
            {
                m_sDefaultObjAction = m_sCharacterUseAction;
            }
            else if (!pEntityAlive->g_Alive())
            {
                if (b_allow_drag)
                    m_sDefaultObjAction = m_sDeadCharacterUseOrDragAction;
                else
                    m_sDefaultObjAction = m_sDeadCharacterUseAction;
            }
        }
        else if (m_pInvBoxWeLookingAt)
        {
            if (smart_cast<CInventoryContainer*>(m_pInvBoxWeLookingAt))
                m_sDefaultObjAction = m_sInventoryContainerUseOrTakeAction;
            else
                m_sDefaultObjAction = m_sInventoryBoxUseAction;
        }
        else if (m_pVehicleWeLookingAt)
        {
            m_sDefaultObjAction = m_sCarCharacterUseAction;
        }
        else if (inventory().m_pTarget && inventory().m_pTarget->CanTake())
        {
            if (b_allow_drag)
                m_sDefaultObjAction = m_sInventoryItemUseOrDragAction;
            else
                m_sDefaultObjAction = m_sInventoryItemUseAction;
        }
        else if (b_allow_drag)
        {
            m_sDefaultObjAction = m_sGameObjectDragAction;
        }
        else
        {
            m_sDefaultObjAction = nullptr;
        }
    }
    else
    {
        inventory().m_pTarget = nullptr;
        m_pPersonWeLookingAt = nullptr;
        m_sDefaultObjAction = nullptr;
        m_pUsableObject = nullptr;
        m_pObjectWeLookingAt = nullptr;
        m_pVehicleWeLookingAt = nullptr;
        m_pInvBoxWeLookingAt = nullptr;
    }

    //	UpdateSleep									();

	// для усіх предметів що впливають на параметри актора
    UpdateItemsEffect();
    if (!m_holder)
        m_pPhysics_support->in_shedule_Update(DT);

    updated = true;
};

//#include "debug_renderer.h"
void CActor::renderable_Render(u32 context_id, IRenderable* root)
{
//    if (!psAI_Flags.test(aiStalker))
    {
        inherited::renderable_Render(context_id, root);

        if (!HUDview())
        {
            CInventoryOwner::renderable_Render(context_id, root);
        }
    }
}

BOOL CActor::renderable_ShadowReceive()
{
    if (m_holder)
        return FALSE;

    return TRUE;
}

void CActor::g_PerformDrop()
{
    b_DropActivated = FALSE;

    PIItem pItem{};

    if (pInput->iGetAsyncKeyState(get_action_dik(kADDITIONAL_ACTION)))
    {
        auto huditem = g_player_hud->attached_item(1)->m_parent_hud_item;
        if (!huditem || !huditem->GetHUDmode())
            return;

        pItem = smart_cast<CInventoryItem*>(huditem);
        if (inventory().m_slots[pItem->GetSlot()].m_bPersistent)
            return;

        pItem->SetDropManual(TRUE);
        return;
    }

    pItem = inventory().ActiveItem();
    if (!pItem)
        return;

    u32 s = inventory().GetActiveSlot();
    if (inventory().m_slots[s].m_bPersistent)
        return;

    pItem->SetDropManual(TRUE);
}

#ifdef DEBUG
extern BOOL g_ShowAnimationInfo;
#endif // DEBUG
// HUD
void CActor::OnHUDDraw(CCustomHUD* hud, u32 context_id, IRenderable* root)
{
    g_player_hud->render_hud(context_id, root);

#if 0 // ndef NDEBUG
	if (Level().CurrentControlEntity() == this && g_ShowAnimationInfo)
	{
		string128 buf;
		HUD().Font().pFontStat->SetColor	(0xffffffff);
		HUD().Font().pFontStat->OutSet		(170,530);
		HUD().Font().pFontStat->OutNext	("Position:      [%3.2f, %3.2f, %3.2f]",VPUSH(Position()));
		HUD().Font().pFontStat->OutNext	("Velocity:      [%3.2f, %3.2f, %3.2f]",VPUSH(m_PhysicMovementControl->GetVelocity()));
		HUD().Font().pFontStat->OutNext	("Vel Magnitude: [%3.2f]",m_PhysicMovementControl->GetVelocityMagnitude());
		HUD().Font().pFontStat->OutNext	("Vel Actual:    [%3.2f]",m_PhysicMovementControl->GetVelocityActual());
		switch (m_PhysicMovementControl->Environment())
		{
		case CPHMovementControl::peOnGround:	strcpy(buf,"ground");			break;
		case CPHMovementControl::peInAir:		strcpy(buf,"air");				break;
		case CPHMovementControl::peAtWall:		strcpy(buf,"wall");				break;
		}
		HUD().Font().pFontStat->OutNext	(buf);

		if (IReceived != 0)
		{
			float Size = 0;
			Size = HUD().Font().pFontStat->GetSize();
			HUD().Font().pFontStat->SetSize(Size*2);
			HUD().Font().pFontStat->SetColor	(0xffff0000);
			HUD().Font().pFontStat->OutNext ("Input :		[%3.2f]", ICoincidenced/IReceived * 100.0f);
			HUD().Font().pFontStat->SetSize(Size);
		};
	};
#endif
}

void CActor::SetPhPosition(const Fmatrix& transform)
{
    if (!m_pPhysicsShell)
    {
        character_physics_support()->movement()->SetPosition(transform.c);
    }
    // else m_phSkeleton->S
}

void CActor::ForceTransform(const Fmatrix& m)
{
    if (!g_Alive())
        return;
    XFORM().set(m);

    if (character_physics_support()->movement()->CharacterExist())
        character_physics_support()->movement()->EnableCharacter();

    character_physics_support()->set_movement_position(m.c);
    character_physics_support()->movement()->SetVelocity(0, 0, 0);
}

void CActor::ForceTransformAndDirection(const Fmatrix& m)
{
    Fvector xyz;
    m.getHPB(xyz);

    ForceTransform(m);
    cam_Active()->Set(-xyz.x, -xyz.y, -xyz.z);
}

float CActor::Radius() const
{
    float R = inherited::Radius();
    CWeapon* W = smart_cast<CWeapon*>(inventory().ActiveItem());
    if (W)
        R += W->Radius();
    return R;
}

bool CActor::use_bolts() const { return CInventoryOwner::use_bolts(); };

void CActor::OnItemDropUpdate()
{
    CInventoryOwner::OnItemDropUpdate();

    TIItemContainer::iterator I = inventory().m_all.begin();
    TIItemContainer::iterator E = inventory().m_all.end();

    for (; I != E; ++I)
        if (!(*I)->IsInvalid() && !attached(*I))
            attach(*I);
}

void CActor::UpdateItemsEffect()
{
    auto cond = &conditions();
    if (!cond->IsTimeValid())
        return;
    float f_update_time = conditions().fdelta_time();

    // проходимо по усім рестор_параметрам для актора
    for (int i = 0; i < eRestoreBoostMax; ++i)
    {
        // nullifying values
        m_ActorItemBoostedParam[i] = 0.f;

        auto outfit = GetOutfit();
        auto helmet = GetHelmet();
        auto backpack = GetBackpack();

        if (i == eRadiationBoost)
        {
            // OBJECTS_RADIOACTIVE - new version
            for (const auto& item : inventory().m_all)
            {
                float radiation_restore_speed = item->GetItemEffect(i);
                auto art = smart_cast<CArtefact*>(item);
                bool affect = radiation_restore_speed > 0.f || art && art->CanAffect();
                float res_rrs = affect ? radiation_restore_speed : 0.f;

                if (outfit) // костюм захищає від радіації речей
                    res_rrs *= (1.f - outfit->GetHitTypeProtection(ALife::eHitTypeRadiation));
                if (backpack && inventory().InRuck(item)) // рюкзак захищає від радіації речей у рюкзаку
                    res_rrs *= (1.f - backpack->GetHitTypeProtection(ALife::eHitTypeRadiation));
                if (helmet) // шолом захищає від радіації речей
                    res_rrs *= (1.f - helmet->GetHitTypeProtection(ALife::eHitTypeRadiation));

                m_ActorItemBoostedParam[i] += res_rrs;
            }
        }
        else
        {
            // artefacts
            m_ActorItemBoostedParam[i] += GetTotalArtefactsEffect(i);
            // outfit
            if (outfit && !fis_zero(outfit->GetCondition()))
                m_ActorItemBoostedParam[i] += outfit->GetItemEffect(i);
            // helmet
            if (helmet && !fis_zero(helmet->GetCondition()))
                m_ActorItemBoostedParam[i] += helmet->GetItemEffect(i);
            // backpack
            if (backpack && !fis_zero(backpack->GetCondition()))
                m_ActorItemBoostedParam[i] += backpack->GetItemEffect(i);
        }
        // apllying boost on actor *_restore conditions
        // cond->ApplyRestoreBoost(i, m_ActorItemBoostedParam[i] * f_update_time);
        cond->ApplyInfluence(i, m_ActorItemBoostedParam[i] * f_update_time);
    }

    callback(GameObject::eUpdateItemsEffect)(f_update_time);
}

float CActor::GetArtefactsProtection(int hit_type)
{
    float res{};
    for (const auto& item : inventory().m_all)
    {
        auto artefact = smart_cast<CArtefact*>(item);
        if (artefact && artefact->CanAffect() && !fis_zero(artefact->GetHitTypeProtection(hit_type)))
        {
            res += artefact->GetHitTypeProtection(hit_type);
        }
        auto container = smart_cast<CInventoryContainer*>(item);
        if (container)
        {
            res += container->GetContainmentArtefactProtection(hit_type);
        }
    }
    return res;
}

float CActor::GetItemBoostedParams(int type) { return m_ActorItemBoostedParam[type]; }

float CActor::GetTotalArtefactsEffect(int i)
{
    float res{};
    for (const auto& item : inventory().m_all)
    {
        auto artefact = smart_cast<CArtefact*>(item);
        if (artefact && artefact->CanAffect())
        {
            res += artefact->GetItemEffect(i);
        }
        auto container = smart_cast<CInventoryContainer*>(item);
        if (container)
        {
            res += container->GetContainmentArtefactEffect(i);
        }
    }
    return res;
}

Fvector CActor::GetMissileOffset() const { return m_vMissileOffset; }

void CActor::SetMissileOffset(const Fvector& vNewOffset) { m_vMissileOffset.set(vNewOffset); }

void CActor::spawn_supplies()
{
    inherited::spawn_supplies();
    CInventoryOwner::spawn_supplies();
}

void CActor::AnimTorsoPlayCallBack(CBlend* B)
{
    CActor* actor = (CActor*)B->CallbackParam;
    actor->m_bAnimTorsoPlayed = FALSE;
}

void CActor::SetActorVisibility(u16 who, float value)
{
    xr_vector<_npc_visibility>::iterator it = std::find(m_npc_visibility.begin(), m_npc_visibility.end(), who);

    if (it == m_npc_visibility.end() && value != 0)
    {
        m_npc_visibility.resize(m_npc_visibility.size() + 1);
        _npc_visibility& v = m_npc_visibility.back();
        v.id = who;
        v.value = value;
    }
    else if (fis_zero(value))
    {
        if (it != m_npc_visibility.end())
            m_npc_visibility.erase(it);
    }
    else
    {
        (*it).value = value;
    }

    std::sort(m_npc_visibility.begin(), m_npc_visibility.end());
}

CPHDestroyable* CActor::ph_destroyable() { return smart_cast<CPHDestroyable*>(character_physics_support()); }

CEntityConditionSimple* CActor::create_entity_condition(CEntityConditionSimple* ec)
{
    if (!ec)
        m_entity_condition = xr_new<CActorCondition>(this);
    else
        m_entity_condition = smart_cast<CActorCondition*>(ec);

    return (inherited::create_entity_condition(m_entity_condition));
}

DLL_Pure* CActor::_construct()
{
    m_pPhysics_support = xr_new<CCharacterPhysicsSupport>(CCharacterPhysicsSupport::etActor, this);
    CEntityAlive::_construct();
    CInventoryOwner::_construct();
    CStepManager::_construct();

    return (this);
}

bool CActor::use_center_to_aim() const { return (!(mstate_real & mcCrouch)); }

bool CActor::can_attach(const CInventoryItem* inventory_item) const
{
    const CAttachableItem* item = smart_cast<const CAttachableItem*>(inventory_item);
    if (!item || /*!item->enabled() ||*/ !item->can_be_attached())
        return (false);

    //можно ли присоединять объекты такого типа
    if (m_attach_item_sections.end() == std::find(m_attach_item_sections.begin(), m_attach_item_sections.end(), inventory_item->object().cNameSect()))
        return false;

    //если уже есть присоединненый объет такого типа
    if (attached(inventory_item->object().cNameSect()))
        return false;

    return true;
}

void CActor::OnDifficultyChanged()
{
    // immunities
    VERIFY(g_SingleGameDifficulty >= egdNovice && g_SingleGameDifficulty <= egdMaster);
    LPCSTR diff_name = get_token_name(difficulty_type_token, g_SingleGameDifficulty);
    string128 tmp;
    strconcat(sizeof(tmp), tmp, "actor_immunities_", diff_name);
    conditions().LoadImmunities(tmp, pSettings);
    // hit probability
    strconcat(sizeof(tmp), tmp, "hit_probability_", diff_name);
    hit_probability = pSettings->r_float(*cNameSect(), tmp);
}

CVisualMemoryManager* CActor::visual_memory() const { return (&memory().visual()); }

float CActor::GetCarryWeight() const
{
    float res{CInventoryOwner::GetCarryWeight()};
    CPHCapture* capture = character_physics_support()->movement()->PHCapture();
    if (capture && capture->taget_object())
    {
        res += capture->taget_object()->GetMass();
        if (auto io = smart_cast<CInventoryOwner*>(capture->taget_object()))
            res += io->GetCarryWeight();
    }
    return res;
}

float CActor::GetMass() { return g_Alive() ? character_physics_support()->movement()->GetMass() : m_pPhysicsShell ? m_pPhysicsShell->getMass() : 0; }

bool CActor::is_on_ground() { return (character_physics_support()->movement()->Environment() != CPHMovementControl::peInAir); }

CCustomOutfit* CActor::GetOutfit() const { return smart_cast<CCustomOutfit*>(inventory().ItemFromSlot(OUTFIT_SLOT)); }
CInventoryContainer* CActor::GetBackpack() const { return smart_cast<CInventoryContainer*>(inventory().ItemFromSlot(BACKPACK_SLOT)); }
CHelmet* CActor::GetHelmet() const { return smart_cast<CHelmet*>(inventory().ItemFromSlot(HELMET_SLOT)); }

bool CActor::is_actor_normal() const { return mstate_real & (mcAnyAction | mcLookout | mcCrouch | mcClimb | mcSprint) ? false : true; }

bool CActor::is_actor_crouch() const { return mstate_real & (mcAnyAction | mcAccel | mcClimb | mcSprint) ? false : mstate_real & mcCrouch ? true : false; }

bool CActor::is_actor_creep() const { return mstate_real & (mcAnyAction | mcClimb | mcSprint) ? false : (mstate_real & mcCrouch && mstate_real & mcAccel) ? true : false; }

bool CActor::is_actor_climb() const { return mstate_real & (mcAnyAction | mcCrouch | mcSprint) ? false : mstate_real & mcClimb ? true : false; }

bool CActor::is_actor_walking() const
{
    bool run = false;
    if (mstate_real & mcAccel)
        run = psActorFlags.test(AF_ALWAYSRUN) ? false : true;
    else
        run = psActorFlags.test(AF_ALWAYSRUN) ? true : false;
    return ((mstate_real & (mcJump | mcFall | mcLanding | mcLanding2 | mcCrouch | mcClimb | mcSprint)) || run) ? false : mstate_real & mcAnyMove ? true : false;
}

bool CActor::is_actor_running() const
{
    bool run = false;
    if (mstate_real & mcAccel)
        run = psActorFlags.test(AF_ALWAYSRUN) ? false : true;
    else
        run = psActorFlags.test(AF_ALWAYSRUN) ? true : false;
    return mstate_real & (mcJump | mcFall | mcLanding | mcLanding2 | mcLookout | mcCrouch | mcClimb | mcSprint) ? false : (mstate_real & mcAnyMove && run) ? true : false;
}

bool CActor::is_actor_sprinting() const
{
    return mstate_real & (mcJump | mcFall | mcLanding | mcLanding2 | mcLookout | mcCrouch | mcAccel | mcClimb) ? false :
        (mstate_real & (mcFwd | mcLStrafe | mcRStrafe) && mstate_real & mcSprint)                              ? true :
                                                                                                                 false;
}

bool CActor::is_actor_crouching() const
{
    return mstate_real & (mcJump | mcFall | mcLanding | mcLanding2 | mcAccel | mcClimb) ? false : (mstate_real & mcAnyMove && mstate_real & mcCrouch) ? true : false;
}

bool CActor::is_actor_creeping() const
{
    return mstate_real & (mcJump | mcFall | mcLanding | mcLanding2 | mcClimb) ? false : (mstate_real & mcAnyMove && mstate_real & mcCrouch && mstate_real & mcAccel) ? true : false;
}

bool CActor::is_actor_climbing() const { return mstate_real & (mcJump | mcFall | mcLanding | mcLanding2) ? false : (mstate_real & mcAnyMove && mstate_real & mcClimb) ? true : false; }

bool CActor::is_actor_moving() const { return mstate_real & mcAnyAction ? true : false; }

bool CActor::unlimited_ammo() const { return !!psActorFlags.test(AF_UNLIMITEDAMMO); }

bool CActor::IsDetectorActive() const
{
    if (auto det = smart_cast<CCustomDetector*>(inventory().ItemFromSlot(DETECTOR_SLOT)))
        return det->IsPowerOn() && det->GetHUDmode();
    return false;
}

bool CActor::IsFreeHands() const
{
    const auto active_hud_item = smart_cast<CHudItem*>(inventory().ActiveItem());
    return !active_hud_item || !active_hud_item->IsPending();
}

float CActor::GetVisibility() { return m_npc_visibility.size() ? m_npc_visibility.back().value : 0.f; }
void CActor::ResetVisibility() { m_npc_visibility.clear(); };

void CActor::SetPickUpItem(CInventoryItem* pickup_item)
{
    if (pSettings->line_exist("engine_callbacks", "on_pickup_item_set"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "on_pickup_item_set");
        if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function))
            lua_function(pickup_item ? pickup_item->object().lua_game_object() : nullptr);
    }
}

void CActor::block_action(EGameActions cmd) { m_blocked_actor_actions.insert(cmd); }

void CActor::unblock_action(EGameActions cmd) { m_blocked_actor_actions.erase(cmd); }