#include "stdafx.h"
#include "hudmanager.h"
#include "WeaponMagazined.h"
#include "weaponBM16.h"
#include "entity.h"
#include "actor.h"
#include "torch.h"
#include "ParticlesObject.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "level.h"
#include "object_broker.h"
#include "string_table.h"
#include "WeaponBinoculars.h"
#include "WeaponBinocularsVision.h"
#include "ai_object_location.h"

#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include <regex>
#include "../xr_3da/x_ray.h"
#include "../xr_3da/xr_input.h"
#include "../xr_3da/LightAnimLibrary.h"

CWeaponMagazined::CWeaponMagazined(LPCSTR name, ESoundTypes eSoundType) : CWeapon(name) {}

CWeaponMagazined::~CWeaponMagazined()
{
    if (m_binoc_vision)
        xr_delete(m_binoc_vision);
}

void CWeaponMagazined::net_Destroy()
{
    inherited::net_Destroy();
    if (m_binoc_vision)
        xr_delete(m_binoc_vision);
}

BOOL CWeaponMagazined::net_Spawn(CSE_Abstract* DC)
{
    BOOL bRes = inherited::net_Spawn(DC);
    const auto wpn = smart_cast<CSE_ALifeItemWeaponMagazined*>(DC);
    if (HasFireModes() && wpn->m_u8CurFireMode >= m_aFireModes.size())
    {
        Msg("! [%s]: %s: wrong wpn->m_u8CurFireMode[%u/%u]", __FUNCTION__, cName().c_str(), wpn->m_u8CurFireMode, m_aFireModes.size() - 1);
        wpn->m_u8CurFireMode = m_aFireModes.size() - 1;
    }
    m_iCurFireMode = wpn->m_u8CurFireMode;
    SetQueueSize(GetCurrentFireMode());
    // multitype ammo loading
    for (u32 i = 0; i < wpn->m_AmmoIDs.size(); i++)
    {
        u8 LocalAmmoType = wpn->m_AmmoIDs[i];
        if (i >= m_magazine.size())
            continue;
        CCartridge& l_cartridge = *(m_magazine.begin() + i);
        if (LocalAmmoType == l_cartridge.m_LocalAmmoType)
            continue;
        l_cartridge.Load(m_ammoTypes[LocalAmmoType].c_str(), LocalAmmoType);
    }
    //
    return bRes;
}

void CWeaponMagazined::Load(LPCSTR section)
{
    inherited::Load(section);

    // Sounds
    m_sounds.LoadSound(section, "snd_draw", "sndShow", SOUND_TYPE_ITEM_TAKING);
    m_sounds.LoadSound(section, "snd_holster", "sndHide", SOUND_TYPE_ITEM_HIDING);
    m_sounds.LoadSound(section, "snd_shoot", "sndShot", SOUND_TYPE_WEAPON_SHOOTING);
    m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", SOUND_TYPE_WEAPON_EMPTY_CLICKING);

     if (pSettings->line_exist(section, "snd_shoot_actor"))
        m_sounds.LoadSound(section, "snd_shoot_actor", "sndShotActor", SOUND_TYPE_WEAPON_SHOOTING);

     if (pSettings->line_exist(section, "snd_shoot_silencer_actor"))
         m_sounds.LoadSound(section, "snd_shoot_silencer_actor", "sndShotSilencerActor", SOUND_TYPE_WEAPON_SHOOTING);

    if (pSettings->line_exist(section, "snd_reload_empty"))
        m_sounds.LoadSound(section, "snd_reload_empty", "sndReload", SOUND_TYPE_WEAPON_RECHARGING);
    else
        m_sounds.LoadSound(section, "snd_reload", "sndReload", SOUND_TYPE_WEAPON_RECHARGING);

    if (pSettings->line_exist(section, "snd_reload_jammed"))
        m_sounds.LoadSound(section, "snd_reload_jammed", "sndReloadJammed", SOUND_TYPE_WEAPON_RECHARGING);

    if (pSettings->line_exist(section, "snd_reload_jammed_last"))
        m_sounds.LoadSound(section, "snd_reload_jammed_last", "sndReloadJammedLast", SOUND_TYPE_WEAPON_RECHARGING);

    if (pSettings->line_exist(section, "snd_reload_empty")) // OpenXRay-style неполная перезарядка
        m_sounds.LoadSound(section, "snd_reload", "sndReloadPartly", SOUND_TYPE_WEAPON_RECHARGING);
    else if (pSettings->line_exist(section, "snd_reload_partly")) // OGSR-style неполная перезарядка
        m_sounds.LoadSound(section, "snd_reload_partly", "sndReloadPartly", SOUND_TYPE_WEAPON_RECHARGING);

    if (pSettings->line_exist(section, "snd_fire_modes"))
        m_sounds.LoadSound(section, "snd_fire_modes", "sndFireModes", SOUND_TYPE_WEAPON_EMPTY_CLICKING);
    if (pSettings->line_exist(section, "snd_zoom_change"))
        m_sounds.LoadSound(section, "snd_zoom_change", "sndZoomChange", SOUND_TYPE_WEAPON_EMPTY_CLICKING);

    if (pSettings->line_exist(section, "snd_aim_start"))
        m_sounds.LoadSound(section, "snd_aim_start", "sndAimStart", SOUND_TYPE_ITEM_TAKING);
    if (pSettings->line_exist(section, "snd_aim_end"))
        m_sounds.LoadSound(section, "snd_aim_end", "sndAimEnd", SOUND_TYPE_ITEM_HIDING);

    //
    m_sounds.LoadSound(section, pSettings->line_exist(section, "snd_shutter") ? "snd_shutter" : "snd_draw", "sndShutter", SOUND_TYPE_WEAPON_RECHARGING);
    m_sounds.LoadSound(section, pSettings->line_exist(section, "snd_shutter_misfire") ? "snd_shutter_misfire" : "snd_draw", "sndShutterMisfire", SOUND_TYPE_WEAPON_RECHARGING);
    //
    if (pSettings->line_exist(section, "snd_unload"))
        m_sounds.LoadSound(section, "snd_unload", "sndUnload", SOUND_TYPE_WEAPON_RECHARGING);

    /*m_pSndShotCurrent = &sndShot;*/
    m_sSndShotCurrent = IsAddonAttached(eSilencer) ? "sndSilencerShot" : "sndShot";

    //звуки и партиклы глушителя, еслит такой есть
    if (m_eSilencerStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "silencer_flame_particles"))
            m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
        if (pSettings->line_exist(section, "silencer_smoke_particles"))
            m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");
        m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", SOUND_TYPE_WEAPON_SHOOTING);
    }
    //  [7/20/2005]
    if (pSettings->line_exist(section, "dispersion_start"))
        m_iShootEffectorStart = pSettings->r_u8(section, "dispersion_start");
    else
        m_iShootEffectorStart = 0;
    //  [7/20/2005]
    //  [7/21/2005]
    if (pSettings->line_exist(section, "fire_modes"))
    {
        m_bHasDifferentFireModes = true;
        shared_str FireModesList = pSettings->r_string(section, "fire_modes");
        int ModesCount = _GetItemCount(FireModesList.c_str());
        m_aFireModes.clear();
        for (int i = 0; i < ModesCount; i++)
        {
            string16 sItem;
            _GetItem(FireModesList.c_str(), i, sItem);
            int FireMode = atoi(sItem);
            m_aFireModes.push_back(FireMode);
        }
        m_iCurFireMode = ModesCount - 1;
        m_iPrefferedFireMode = READ_IF_EXISTS(pSettings, r_s16, section, "preffered_fire_mode", -1);
    }
    else
        m_bHasDifferentFireModes = false;

    m_bVision = !!READ_IF_EXISTS(pSettings, r_bool, section, "vision_present", false);
    m_fire_zoomout_time = READ_IF_EXISTS(pSettings, r_u32, section, "fire_zoomout_time", u32(-1));

    m_bHasChamber = READ_IF_EXISTS(pSettings, r_bool, section, "has_chamber", true);
    m_fShellDropDelay = READ_IF_EXISTS(pSettings, r_float, section, "shell_drop_delay", 0.f);

    if (pSettings->line_exist(section, "bullet_bones"))
    {
        bHasBulletsToHide = true;
        LPCSTR str = pSettings->r_string(section, "bullet_bones");
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 bullet_bone_name;
            _GetItem(str, i, bullet_bone_name);
            bullets_bones.push_back(bullet_bone_name);
            bullet_cnt++;
        }
    }

    m_bFlameParticlesHideInZoom = READ_IF_EXISTS(pSettings, r_bool, section, "flame_particles_hide_in_zoom", false);
}

void CWeaponMagazined::FireStart()
{
    if (IsValid() && (!IsMisfire() || IsGrenadeMode()))
    {
        if (!IsWorking() || AllowFireWhileWorking())
        {
            if (GetState() == eReload)
                return;
            if (GetState() == eShowing)
                return;
            if (GetState() == eHiding)
                return;
            if (GetState() == eMisfire)
                return;

            inherited::FireStart();

            if (iAmmoElapsed == 0)
                OnMagazineEmpty();
            else
                SwitchState(eFire);
        }
    }
    else if (IsMisfire() && !IsGrenadeMode())
    {
        if (smart_cast<CActor*>(H_Parent()))
        {
            Misfire();
        }
    }
    else if (eReload != GetState() && eMisfire != GetState())
        OnMagazineEmpty();
}

int CWeaponMagazined::CheckAmmoBeforeReload(u32& v_ammoType)
{
    if (m_set_next_ammoType_on_reload != u32(-1))
        v_ammoType = m_set_next_ammoType_on_reload;

    // Msg("Ammo type in next reload : %d", m_set_next_ammoType_on_reload);

    if (m_ammoTypes.size() <= v_ammoType)
    {
        // Msg("Ammo type is wrong : %d", v_ammoType);
        return 0;
    }

    LPCSTR tmp_sect_name = m_ammoTypes[v_ammoType].c_str();

    if (!tmp_sect_name)
    {
        // Msg("Sect name is wrong");
        return 0;
    }

    CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(tmp_sect_name, AddonAttachable(eMagazine), &m_magazines)); //(m_pCurrentInventory->GetAny(tmp_sect_name));

    if (!ammo && !m_bLockType)
    {
        for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
        {
            //проверить патроны всех подходящих типов
            ammo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[i].c_str(), AddonAttachable(eMagazine), &m_magazines)); //(m_pCurrentInventory->GetAny(m_ammoTypes[i].c_str()));
            if (ammo)
            {
                v_ammoType = i;
                break;
            }
        }
    }

    // Msg("Ammo type %d", v_ammoType);

    return GetAmmoCount(v_ammoType);
}

void CWeaponMagazined::Reload()
{
    inherited::Reload();
    TryReload();
}

bool CWeaponMagazined::TryToGetAmmo(u32 id)
{
    bool mag_weapon = AddonAttachable(eMagazine);
    if (!m_bDirectReload)
        m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[id].c_str(), mag_weapon, &m_magazines));
    return m_pAmmo && m_pAmmo->m_boxCurr && (!mag_weapon || m_pAmmo->IsBoxReloadable() || !iAmmoElapsed || IsGrenadeMode());
}

bool CWeaponMagazined::TryReload()
{
    if (m_bDirectReload)
    {
        SetPending(TRUE);
        SwitchState(eReload);
        return true;
    }
    if (m_pCurrentInventory)
    {
        if (TryToGetAmmo(m_ammoType) || unlimited_ammo())
        {
            SetPending(TRUE);
            SwitchState(eReload);
            return true;
        }
        for (u32 i = 0; i < m_ammoTypes.size(); ++i)
        {
            if (TryToGetAmmo(i))
            {
                m_ammoType = i;
                SetPending(TRUE);
                SwitchState(eReload);
                return true;
            }
        }
    }
    SwitchState(eIdle);
    return false;
}

void CWeaponMagazined::OnMagazineEmpty()
{
    //попытка стрелять когда нет патронов
    if (GetState() == eIdle)
    {
        OnEmptyClick();
        return;
    }

    if (GetNextState() != eMagEmpty && GetNextState() != eReload)
    {
        SwitchState(eMagEmpty);
    }

    inherited::OnMagazineEmpty();
}

void CWeaponMagazined::UnloadMagazine(bool spawn_ammo)
{
    last_hide_bullet = -1;

    int chamber_ammo = HasChamber() && AddonAttachable(eMagazine) && !IsGrenadeMode();
    UnloadAmmo(iAmmoElapsed - chamber_ammo, spawn_ammo, IsAddonAttached(eMagazine) && AddonAttachable(eMagazine) && !IsGrenadeMode());
}

void CWeaponMagazined::ReloadMagazine()
{
    m_dwAmmoCurrentCalcFrame = 0;
    // устранить осечку при полной перезарядке
    if (IsMisfire() && (!HasChamber() || m_magazine.empty()))
    {
        SetMisfire(false);
    }
    // переменная блокирует использование
    // только разных типов патронов
    //	static bool l_lockType = false;
    if (!m_bLockType && !m_bDirectReload)
    {
        m_pAmmo = NULL;
    }

    if (!m_pCurrentInventory)
        return;

    if (m_set_next_ammoType_on_reload != u32(-1))
    {
        m_ammoType = m_set_next_ammoType_on_reload;
        m_set_next_ammoType_on_reload = u32(-1);
    }

    if (m_bDirectReload)
    {
        m_bDirectReload = false;
    }
    else if (!unlimited_ammo())
    {
        bool mag_weapon = AddonAttachable(eMagazine);
        // попытаться найти в инвентаре патроны текущего типа
        m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[m_ammoType].c_str(), mag_weapon, &m_magazines));
        if (!m_pAmmo && !m_bLockType)
        {
            for (u32 i = 0; i < m_ammoTypes.size(); ++i)
            {
                // проверить патроны всех подходящих типов
                m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[i].c_str(), mag_weapon, &m_magazines));
                if (m_pAmmo)
                {
                    m_ammoType = i;
                    break;
                }
            }
        }
    }

    // нет патронов для перезарядки
    if (!m_pAmmo && !unlimited_ammo())
        return;

    // разрядить магазин, если загружаем патронами другого типа
    if (!m_bLockType && !m_magazine.empty() && (!m_pAmmo || m_pAmmo->m_ammoSect != m_magazine.back().m_ammoSect || m_pAmmo->IsBoxReloadable()) ||
        m_magazine.empty() && AddonAttachable(eMagazine) && !unlimited_ammo()) // разрядить пустой магазин
        UnloadMagazine();

    if (AddonAttachable(eMagazine) && !unlimited_ammo() && !IsGrenadeMode())
    {
        bool b_attaching_magazine = m_pAmmo->IsBoxReloadable();
        int mag_size = b_attaching_magazine ? m_pAmmo->m_boxSize : 0;

        iMagazineSize = mag_size + HasChamber();
        m_cur_magazine = b_attaching_magazine ? (u8)std::distance(m_magazines.begin(), std::find(m_magazines.begin(), m_magazines.end(), m_pAmmo->cNameSect())) : 0;
        SetMagazineAttached(b_attaching_magazine);
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
        m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), u8(m_ammoType));
    CCartridge l_cartridge = m_DefaultCartridge;
    while (iAmmoElapsed < iMagazineSize)
    {
        if (!unlimited_ammo())
        {
            if (!m_pAmmo->GetCartridge())
                break;
        }
        ++iAmmoElapsed;
        l_cartridge.Load(m_ammoTypes[m_ammoType].c_str(), u8(m_ammoType));
        //l_cartridge.m_LocalAmmoType = u8(m_ammoType);
        m_magazine.push_back(l_cartridge);
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    // выкинуть коробку патронов, если она пустая
    if (m_pAmmo && !m_pAmmo->m_boxCurr)
        m_pAmmo->DestroyObject(); // SetDropManual(TRUE);

    // дозарядить оружие до полного магазина
    if (iMagazineSize > iAmmoElapsed && !AddonAttachable(eMagazine) && !unlimited_ammo())
    {
        m_bLockType = true;
        ReloadMagazine();
        m_bLockType = false;
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeaponMagazined::Misfire()
{
    inherited::Misfire();
    SetPending(TRUE);
    SwitchState(eMisfire);
}

void CWeaponMagazined::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);
    switch (S)
    {
    case eIdle: switch2_Idle(); break;
    case eFire: switch2_Fire(); break;
    case eFire2: switch2_Fire2(); break;
    case eMisfire: {
        PlayAnimCheckMisfire();
    }
    break;
    case eMagEmpty: {
        const bool need_play_empty_click = (oldState != eFire && oldState != eFire2) || !dont_interrupt_shot_anm;
        switch2_Empty(need_play_empty_click);

        if (GetNextState() != eReload && need_play_empty_click)
        {
            SwitchState(eIdle);
        }
        break;
    }
    case eReload: switch2_Reload(); break;
    case eShowing: switch2_Showing(); break;
    case eHiding: switch2_Hiding(); break;
    case eHidden: switch2_Hidden(); break;
    case eShutter: switch2_Shutter(); break;
    }
}

void CWeaponMagazined::UpdateCL()
{
    inherited::UpdateCL();
    float dt = Device.fTimeDelta;

    //когда происходит апдейт состояния оружия
    //ничего другого не делать
    if (GetNextState() == GetState())
    {
        switch (GetState())
        {
        case eShowing:
        case eHiding:
        case eReload:
        case eMisfire:
        case eSprintStart:
        case eSprintEnd:
        case eIdle:
            fTime -= dt;
            if (fTime < 0)
                fTime = 0;
            break;
        case eFire:
            if (!IsMisfire() && iAmmoElapsed > 0)
                state_Fire(dt);

            if (fTime <= 0)
            {
                if (!IsMisfire() && GetAmmoElapsed() == 0)
                    OnMagazineEmpty();
                StopShooting();
            }
            else
            {
                fTime -= dt;
            }

            if (m_fire_zoomout_time != u32(-1) && IsZoomed() && m_dwStateTime > m_fire_zoomout_time)
                OnZoomOut();

            break;
        case eMagEmpty:
        case eHidden: break;
        }
    }

    if (H_Parent() && IsZoomed() && !IsRotatingToZoom() && m_binoc_vision)
        m_binoc_vision->Update();

    if (m_iShellDropTime > 0 && Device.dwTimeGlobal >= m_iShellDropTime)
    {
        m_iShellDropTime = 0;
        Fvector vel;
        PHGetLinearVell(vel);
        OnShellDrop(get_LastSP(), vel);
    }

    UpdateSounds();
    UpdateLaser();
    UpdateFlashlight();
    UpdateMagazineVisibility();
}

void CWeaponMagazined::UpdateSounds()
{
    if (Device.dwFrame == dwUpdateSounds_Frame)
        return;

    dwUpdateSounds_Frame = Device.dwFrame;
    m_sounds.UpdateAllSoundsPositions(get_LastFP());
}

void CWeaponMagazined::state_Fire(float dt)
{
    VERIFY(fTimeToFire > 0.f);

    Fvector p1, d;
    p1.set(get_LastFP());
    d.set(get_LastFD());

    auto Parent = H_Parent();
    if (!Parent)
        return;

    auto ParentEnt = smart_cast<CEntity*>(Parent);
    if (!ParentEnt)
        return; //Такое иногда бывает. Не понятно почему, но бывает. Например был случай когда пыталось стрелять оружие лежащее в ящике.

#ifdef DEBUG
    CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
    if (!io->inventory().ActiveItem())
    {
        Log("current_state", GetState());
        Log("next_state", GetNextState());
        Log("state_time", m_dwStateTime);
        Log("item_sect", cNameSect().c_str());
        Log("H_Parent", H_Parent()->cNameSect().c_str());
    }
#endif

    ParentEnt->g_fireParams(this, p1, d);

    if (m_iShotNum == 0)
    {
        m_vStartPos = p1;
        m_vStartDir = d;
    }

    VERIFY(!m_magazine.empty());
    //	Msg("%d && %d && (%d || %d) && (%d || %d)", !m_magazine.empty(), fTime<=0, IsWorking(), m_bFireSingleShot, m_iQueueSize < 0, m_iShotNum < m_iQueueSize);
    while (!m_magazine.empty() && fTime <= 0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
    {
        m_bFireSingleShot = false;

        VERIFY(fTimeToFire > 0.f);
        // если у оружия есть разные размеры очереди
        // привилегированный режим очереди не полный автомат
        // текущий режим очереди является привилегированным
        // или кол-во выстрелов попадает в предел привилегированного режима
        if (m_bHasDifferentFireModes && m_iPrefferedFireMode != -1 && (GetCurrentFireMode() == m_iPrefferedFireMode || m_iShotNum < m_iPrefferedFireMode))
        {
            VERIFY(fTimeToFirePreffered > 0.f);
            fTime += fTimeToFirePreffered; // установим скорострельность привилегированного режима
            // Msg("fTimeToFirePreffered = %.6f", fTimeToFirePreffered);
        }
        else
        {
            VERIFY(fTimeToFire > 0.f);
            fTime += fTimeToFire;
            // Msg("fTimeToFire = %.6f", fTimeToFire);
        }

        ++m_iShotNum;

        CheckForMisfire();
        OnShot();

        if (smart_cast<CWeaponBM16*>(this) && IsMisfire())
            return;

        if (m_iShotNum > m_iShootEffectorStart)
            FireTrace(p1, d);
        else
            FireTrace(m_vStartPos, m_vStartDir);
    }

    if (m_iShotNum == m_iQueueSize)
        m_bStopedAfterQueueFired = true;

    UpdateSounds();
}

void CWeaponMagazined::PlayShotSound()
{
    if (ParentIsActor())
    {
        if (IsAddonAttached(eSilencer))
        {
            if (SoundExist("sndShotSilencerActor"))
            {
                PlaySound("sndShotSilencerActor", get_LastFP(), true);
                return;
            }
        }
        else
        {
            if (SoundExist("sndShotActor"))
            {
                PlaySound("sndShotActor", get_LastFP(), true);
                return;
            }
        }
    }
    PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), true);
}

void CWeaponMagazined::OnShot()
{
    PlayShotSound();

    // Camera
    AddShotEffector();

    // Animation
    PlayAnimShoot();

    // Shell Drop
    Fvector vel;
    if (fis_zero(m_fShellDropDelay))
    {
        PHGetLinearVell(vel);
        OnShellDrop(get_LastSP(), vel);
    }
    else
        m_iShellDropTime = Device.dwTimeGlobal + m_fShellDropDelay * 1000;

    // Огонь из ствола
    if (ShouldPlayFlameParticles())
    {
        const auto& ammo = m_magazine.back();
        const auto& shot_particles = ammo.m_sShotParticles;
        // якщо в набої вказано партікли пострілу - використовувати їх
        if (!shot_particles.empty())
            m_sFlameParticlesCurrent = shot_particles[Random.randI(0, shot_particles.size())];
        else
            m_sFlameParticlesCurrent = IsAddonAttached(eSilencer) ? m_sSilencerFlameParticles : m_sFlameParticles;

        if (ammo.m_bShotLight)
            LoadLights(ammo.m_ammoSect.c_str(), "");
        else
            LoadLights(cNameSect().c_str(), IsAddonAttached(eSilencer) ? "silencer_" : "");

        StartFlameParticles();
        ForceUpdateFireParticles();
    }

    //дым из ствола
    StartSmokeParticles(get_LastFP(), vel);

    update_visual_bullet_textures();
}

void CWeaponMagazined::OnEmptyClick()
{
    PlayAnimFakeShoot();
    PlaySound("sndEmptyClick", get_LastFP());
}

void CWeaponMagazined::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eReload:
        ReloadMagazine();
        HandleCartridgeInChamber();
        m_sounds.StopSound("sndReload");
        m_sounds.StopSound("sndReloadPartly");
        m_sounds.StopSound("sndReloadJammed");
        m_sounds.StopSound("sndReloadJammedLast");
        bullet_update = true;
        SwitchState(eIdle);
        break; // End of reload animation
    case eHiding: SwitchState(eHidden); break; // End of Hide
    case eIdle: switch2_Idle(); break; // Keep showing idle
    case eShowing: {
        update_visual_bullet_textures(true);
        SwitchState(eIdle);
        break;
    }
    case eShutter:
        ShutterAction();
        SwitchState(eIdle);
        break; // End of Shutter animation
    case eMisfire:
    case eFire:
    case eFire2: SwitchState(eIdle); break;
    default: inherited::OnAnimationEnd(state);
    }
}

void CWeaponMagazined::switch2_Idle()
{
    SetPending(FALSE);
    PlayAnimIdle();
}

#ifdef DEBUG
#include "ai\stalker\ai_stalker.h"
#include "object_handler_planner.h"
#endif
void CWeaponMagazined::switch2_Fire()
{
    CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
#ifdef DEBUG
    CInventoryItem* ii = smart_cast<CInventoryItem*>(this);
    VERIFY2(io, make_string("no inventory owner, item %s", *cName()));

    if (ii != io->inventory().ActiveItem())
        Msg("! not an active item, item %s, owner %s, active item %s", *cName(), *H_Parent()->cName(),
            io->inventory().ActiveItem() ? *io->inventory().ActiveItem()->object().cName() : "no_active_item");

    if (!(io && (ii == io->inventory().ActiveItem())))
    {
        CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(H_Parent());
        if (stalker)
        {
            stalker->planner().show();
            stalker->planner().show_current_world_state();
            stalker->planner().show_target_world_state();
        }
    }
#else
    if (!io)
        return;
#endif // DEBUG

    m_bStopedAfterQueueFired = false;
    m_bFireSingleShot = true;
    m_iShotNum = 0;
}
void CWeaponMagazined::switch2_Empty(const bool empty_click_anim_play)
{
    if (smart_cast<CActor*>(H_Parent()))
    {
        if (empty_click_anim_play)
            OnEmptyClick();
        return;
    }

    /*OnZoomOut();*/

    if (!TryReload())
    {
        if (empty_click_anim_play)
            OnEmptyClick();
    }
    else
    {
        inherited::FireEnd();
    }
}

void CWeaponMagazined::PlayReloadSound()
{
    if ((IsMisfire() && iAmmoElapsed == 1) && m_sounds.FindSoundItem("sndReloadJammedLast", false))
        PlaySound("sndReloadJammedLast", get_LastFP());
    else if (IsMisfire() && m_sounds.FindSoundItem("sndReloadJammed", false))
        PlaySound("sndReloadJammed", get_LastFP());
    else if (IsPartlyReloading() && m_sounds.FindSoundItem("sndReloadPartly",false))
        PlaySound("sndReloadPartly", get_LastFP());
    else
        PlaySound("sndReload", get_LastFP());
}

void CWeaponMagazined::switch2_Reload()
{
    CWeapon::FireEnd();

    PlayReloadSound();
    PlayAnimReload();
    SetPending(TRUE);
    bullet_update = false;
}

void CWeaponMagazined::switch2_Hiding()
{
    CWeapon::FireEnd();

    StopHUDSounds();
    PlaySound("sndHide", get_LastFP());

    PlayAnimHide();
    SetPending(TRUE);
}

void CWeaponMagazined::switch2_Hidden()
{
    CWeapon::FireEnd();

    m_sounds.StopSound("sndReload");
    m_sounds.StopSound("sndReloadPartly");
    m_sounds.StopSound("sndReloadJammed");
    m_sounds.StopSound("sndReloadJammedLast");
    StopCurrentAnimWithoutCallback();

    signal_HideComplete();
    RemoveShotEffector();
}
void CWeaponMagazined::switch2_Showing()
{
    PlaySound("sndShow", get_LastFP());

    SetPending(TRUE);
    PlayAnimShow();
}

bool CWeaponMagazined::Action(s32 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;

    //если оружие чем-то занято, то ничего не делать
    if (IsPending() && cmd != kWPN_FIREMODE_PREV && cmd != kWPN_FIREMODE_NEXT)
        return false;

    switch (cmd)
    {
    case kWPN_RELOAD: {
        if (flags & CMD_START)
        {
            if (pInput->iGetAsyncKeyState(get_action_dik(kADDITIONAL_ACTION)))
            {
                OnShutter();
                return true;
            }
            else if (CanBeReloaded())
            {
                Reload();
                return true;
            }
        }
    }
    break;
    case kWPN_FIREMODE_PREV: {
        if (flags & CMD_START)
        {
            OnPrevFireMode(flags & CMD_OPT);
            return true;
        }
    }
    break;
    case kWPN_FIREMODE_NEXT: {
        if (flags & CMD_START)
        {
            OnNextFireMode(flags & CMD_OPT);
            return true;
        }
    }
    break;
    }
    return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
    auto pScope = std::find(m_scopes.begin(), m_scopes.end(), pIItem->object().cNameSect()) != m_scopes.end();
    auto pSilencer = std::find(m_silencers.begin(), m_silencers.end(), pIItem->object().cNameSect()) != m_silencers.end();
    auto pLaser = std::find(m_lasers.begin(), m_lasers.end(), pIItem->object().cNameSect()) != m_lasers.end();
    auto pFlashlight = std::find(m_flashlights.begin(), m_flashlights.end(), pIItem->object().cNameSect()) != m_flashlights.end();
    auto pStock = std::find(m_stocks.begin(), m_stocks.end(), pIItem->object().cNameSect()) != m_stocks.end();
    auto pForend = std::find(m_forends.begin(), m_forends.end(), pIItem->object().cNameSect()) != m_forends.end();

    if (pScope && m_eScopeStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
        return true;
    else if (pSilencer && m_eSilencerStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
        return true;
    else if (pLaser && m_eLaserStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) == 0)
        return true;
    else if (pFlashlight && m_eFlashlightStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) == 0)
        return true;
    else if (pStock && m_eStockStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonStock) == 0)
        return true;
    else if (pForend && m_eForendStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonForend) == 0)
        return true;
    else
        return inherited::CanAttach(pIItem);
}

bool CWeaponMagazined::CanDetach(const char* item_section_name)
{
    if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) &&
        std::find(m_scopes.begin(), m_scopes.end(), item_section_name) != m_scopes.end())
        return true;
    else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
             std::find(m_silencers.begin(), m_silencers.end(), item_section_name) != m_silencers.end())
        return true;
    else if (m_eLaserStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) &&
             std::find(m_lasers.begin(), m_lasers.end(), item_section_name) != m_lasers.end())
        return true;
    else if (m_eFlashlightStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) &&
             std::find(m_flashlights.begin(), m_flashlights.end(), item_section_name) != m_flashlights.end())
        return true;
    else if (m_eStockStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonStock) &&
             std::find(m_stocks.begin(), m_stocks.end(), item_section_name) != m_stocks.end())
        return true;
    else if (m_eForendStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonForend) &&
             std::find(m_forends.begin(), m_forends.end(), item_section_name) != m_forends.end())
        return true;
    else
        return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazined::Attach(PIItem pIItem, bool b_send_event)
{
    bool result = false;

    auto pScope = std::find(m_scopes.begin(), m_scopes.end(), pIItem->object().cNameSect()) != m_scopes.end();
    auto pSilencer = std::find(m_silencers.begin(), m_silencers.end(), pIItem->object().cNameSect()) != m_silencers.end();
    auto pLaser = std::find(m_lasers.begin(), m_lasers.end(), pIItem->object().cNameSect()) != m_lasers.end();
    auto pFlashlight = std::find(m_flashlights.begin(), m_flashlights.end(), pIItem->object().cNameSect()) != m_flashlights.end();
    auto pStock = std::find(m_stocks.begin(), m_stocks.end(), pIItem->object().cNameSect()) != m_stocks.end();
    auto pForend = std::find(m_forends.begin(), m_forends.end(), pIItem->object().cNameSect()) != m_forends.end();

    if (pScope && m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
    {
        m_cur_scope = (u8)std::distance(m_scopes.begin(), std::find(m_scopes.begin(), m_scopes.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
        result = true;
    }
    else if (pSilencer && m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
    {
        m_cur_silencer = (u8)std::distance(m_silencers.begin(), std::find(m_silencers.begin(), m_silencers.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
        result = true;
    }
    else if (pLaser && m_eLaserStatus == CSE_ALifeItemWeapon::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) == 0)
    {
        m_cur_laser = (u8)std::distance(m_lasers.begin(), std::find(m_lasers.begin(), m_lasers.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonLaser;
        result = true;
    }
    else if (pFlashlight && m_eFlashlightStatus == CSE_ALifeItemWeapon::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) == 0)
    {
        m_cur_flashlight = (u8)std::distance(m_flashlights.begin(), std::find(m_flashlights.begin(), m_flashlights.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonFlashlight;
        result = true;
    }
    else if (pStock && m_eStockStatus == CSE_ALifeItemWeapon::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonStock) == 0)
    {
        m_cur_stock = (u8)std::distance(m_stocks.begin(), std::find(m_stocks.begin(), m_stocks.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonStock;
        result = true;
    }
    else if (pForend && m_eForendStatus == CSE_ALifeItemWeapon::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonForend) == 0)
    {
        m_cur_forend = (u8)std::distance(m_forends.begin(), std::find(m_forends.begin(), m_forends.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonForend;
        result = true;
    }

    if (result)
    {
        if (b_send_event)
            pIItem->object().DestroyObject();

        if (!ScopeRespawn())
        {
            UpdateAddonsVisibility();
            InitAddons();
        }

        return true;
    }
    else
        return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazined::Detach(const char* item_section_name, bool b_spawn_item, float item_condition)
{
    if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) &&
        std::find(m_scopes.begin(), m_scopes.end(), item_section_name) != m_scopes.end())
    {
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonScope;
        //
        m_cur_scope = 0;
        if (!ScopeRespawn())
        {
            UpdateAddonsVisibility();
            InitAddons();
        }

        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
             std::find(m_silencers.begin(), m_silencers.end(), item_section_name) != m_silencers.end())
    {
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;
        //
        m_cur_silencer = 0;
        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else if (m_eLaserStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) &&
             std::find(m_lasers.begin(), m_lasers.end(), item_section_name) != m_lasers.end())
    {
        SwitchLaser(false);
        if (laser_flashlight)
            SwitchFlashlight(false);
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonLaser;
        //
        m_cur_laser = 0;
        //
        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else if (m_eFlashlightStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) &&
             std::find(m_flashlights.begin(), m_flashlights.end(), item_section_name) != m_flashlights.end())
    {
        SwitchFlashlight(false);
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonFlashlight;
        //
        m_cur_flashlight = 0;
        //
        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else if (m_eStockStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonStock) &&
             std::find(m_stocks.begin(), m_stocks.end(), item_section_name) != m_stocks.end())
    {
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonStock;
        //
        m_cur_stock = 0;
        //
        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else if (m_eForendStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonForend) &&
             std::find(m_forends.begin(), m_forends.end(), item_section_name) != m_forends.end())
    {
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonForend;
        //
        m_cur_forend = 0;
        //
        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else
        return inherited::Detach(item_section_name, b_spawn_item, item_condition);
}

void CWeaponMagazined::InitAddons()
{
    m_fZoomRotateTime = READ_IF_EXISTS(pSettings, r_float, hud_sect, "zoom_rotate_time", ROTATION_TIME);
    // Приціл
    LPCSTR sect = IsAddonAttached(eScope) && AddonAttachable(eScope) ? GetAddonName(eScope).c_str() : cNameSect().c_str();
    LoadScopeParams(sect);
    // глушник
    ApplySilencerParams();
    // ЛЦВ
    if (IsAddonAttached(eLaser))
    {
        sect = AddonAttachable(eLaser) ? GetAddonName(eLaser).c_str() : cNameSect().c_str();
        LoadLaserParams(sect);
        laser_flashlight = pSettings->line_exist(sect, "light_definition");
        if (laser_flashlight)
            LoadFlashlightParams(sect);
    }
    // ліхтарик
    if (IsAddonAttached(eFlashlight))
    {
        sect = AddonAttachable(eFlashlight) ? GetAddonName(eFlashlight).c_str() : cNameSect().c_str();
        LoadFlashlightParams(sect);
    }
    // до цих параметрів можуть додаватися коефіцієнти у функціях нижче
    m_fAimControlInertionK = 0.f;
    m_fAimInertionK = 0.f;
    // приклад
    ApplyStockParams();
    // цівка
    ApplyForendParams();

    inherited::InitAddons();
}

void CWeaponMagazined::LoadScopeParams(LPCSTR section)
{
    if (IsZoomed())
        OnZoomOut();

    m_fIronSightZoomFactor = READ_IF_EXISTS(pSettings, r_float, section, "ironsight_zoom_factor", 1.0f);
    m_fScopeInertionFactor = READ_IF_EXISTS(pSettings, r_float, section, "scope_inertion_factor", 1.f);
    m_fZoomHudFov = READ_IF_EXISTS(pSettings, r_float, section, "scope_zoom_hud_fov", 0.0f);
    m_f3dssHudFov = READ_IF_EXISTS(pSettings, r_float, section, "scope_lense_hud_fov", 0.0f);

    // second scope mode
    m_bHasAimAlt = READ_IF_EXISTS(pSettings, r_bool, section, "aim_alt", false) && !READ_IF_EXISTS(pSettings, r_bool, cNameSect(), "ignore_aim_alt", false) ||
        READ_IF_EXISTS(pSettings, r_bool, cNameSect(), "aim_alt_forced", false);
    if (m_bHasAimAlt)
        m_fScopeZoomFactorAlt = READ_IF_EXISTS(pSettings, r_float, section, "scope_zoom_factor_alt", 1.0f);

    if (!m_bHasAimAlt)
        m_bAimAltMode = false;

    m_fZoomRotateTime_K = AddonAttachable(eScope) && IsAddonAttached(eScope) ? READ_IF_EXISTS(pSettings, r_float, GetAddonName(eScope), "zoom_rotate_time_k", 0.f) : 0.f;

    if (!IsAddonAttached(eScope))
    {
        m_bScopeDynamicZoom = m_bVision = false;
        return;
    }

    m_sounds.StopSound("sndZoomIn");
    m_sounds.StopSound("sndZoomOut");

    if (pSettings->line_exist(section, "snd_zoomin"))
        m_sounds.LoadSound(section, "snd_zoomin", "sndZoomIn", SOUND_TYPE_ITEM_USING);
    if (pSettings->line_exist(section, "snd_zoomout"))
        m_sounds.LoadSound(section, "snd_zoomout", "sndZoomOut", SOUND_TYPE_ITEM_USING);

    m_bVision = !!READ_IF_EXISTS(pSettings, r_bool, section, "vision_present", false);
    if (m_bVision)
        binoc_vision_sect = section;

    auto scope_zoom_line = READ_IF_EXISTS(pSettings, r_string, section, "scope_zoom_factor", nullptr);
    m_bScopeDynamicZoom = false;
    if (scope_zoom_line)
    {
        int _count = _GetItemCount(scope_zoom_line);
        ASSERT_FMT(_count >= 1, "!![%s] : No scope_zoom_factor params found in section [%s]", __FUNCTION__, section);
        string128 tmp;
        m_fScopeZoomFactor = atof(_GetItem(scope_zoom_line, 0, tmp));
        m_fRTZoomFactor = m_fScopeZoomFactor;
        if (_count > 1)
        {
            m_fMaxScopeZoomFactor = atof(_GetItem(scope_zoom_line, 1, tmp));
            m_bScopeDynamicZoom = true;

            m_sounds.StopSound("sndZoomChange");

            if (pSettings->line_exist(section, "snd_zoom_change"))
                m_sounds.LoadSound(section, "snd_zoom_change", "sndZoomChange", SOUND_TYPE_ITEM_USING);
        }
        m_uZoomStepCount = _count > 2 ? atoi(_GetItem(scope_zoom_line, 2, tmp)) : 2;
    }
}

void CWeaponMagazined::ApplySilencerParams()
{
    if (IsAddonAttached(eSilencer) && AddonAttachable(eSilencer))
    {
        conditionDecreasePerShotSilencer = READ_IF_EXISTS(pSettings, r_float, GetAddonName(eSilencer), "condition_shot_dec_silencer", 0.f);
    }

    if (IsAddonAttached(eSilencer))
    {
        // flame
        if (AddonAttachable(eSilencer) && pSettings->line_exist(GetAddonName(eSilencer), "silencer_flame_particles"))
            m_sSilencerFlameParticles = pSettings->r_string(GetAddonName(eSilencer), "silencer_flame_particles");
        else if (pSettings->line_exist(cNameSect(), "silencer_flame_particles"))
            m_sSilencerFlameParticles = pSettings->r_string(cNameSect(), "silencer_flame_particles");
        else
            m_sSilencerFlameParticles = m_sFlameParticles.c_str();
        // smoke
        if (AddonAttachable(eSilencer) && pSettings->line_exist(GetAddonName(eSilencer), "silencer_smoke_particles"))
            m_sSilencerSmokeParticles = pSettings->r_string(GetAddonName(eSilencer), "silencer_smoke_particles");
        else if (pSettings->line_exist(cNameSect(), "silencer_smoke_particles"))
            m_sSilencerSmokeParticles = pSettings->r_string(cNameSect(), "silencer_smoke_particles");
        else
            m_sSilencerSmokeParticles = m_sSmokeParticles.c_str();

        m_sounds.StopSound("sndSilencerShot");

        if (AddonAttachable(eSilencer) && pSettings->line_exist(GetAddonName(eSilencer), "snd_silncer_shot"))
            m_sounds.LoadSound(GetAddonName(eSilencer).c_str(), "snd_silncer_shot", "sndSilencerShot", SOUND_TYPE_WEAPON_SHOOTING);
        else if (pSettings->line_exist(cNameSect(), "snd_silncer_shot"))
            m_sounds.LoadSound(cNameSect().c_str(), "snd_silncer_shot", "sndSilencerShot", SOUND_TYPE_WEAPON_SHOOTING);
        else
            m_sSndShotCurrent = "sndShot";

        m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
        m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
        m_sSndShotCurrent = "sndSilencerShot";

        // сила выстрела
        LoadFireParams(cNameSect().c_str(), "");

        // подсветка от выстрела
        LoadLights(cNameSect().c_str(), "silencer_");
        if (AddonAttachable(eSilencer))
            ApplySilencerKoeffs();
    }
    else
    {
        m_sFlameParticlesCurrent = m_sFlameParticles;
        m_sSmokeParticlesCurrent = m_sSmokeParticles;
        m_sSndShotCurrent = "sndShot";

        // сила выстрела
        LoadFireParams(cNameSect().c_str(), "");
        // подсветка от выстрела
        LoadLights(cNameSect().c_str(), "");
    }
}

void CWeaponMagazined::ApplySilencerKoeffs()
{
    auto silencer_sect = GetAddonName(eSilencer);

    float BHP_k = READ_IF_EXISTS(pSettings, r_float, silencer_sect, "bullet_hit_power_k", 0.f);
    float BS_k = READ_IF_EXISTS(pSettings, r_float, silencer_sect, "bullet_speed_k", 0.f);
    float FDB_k = READ_IF_EXISTS(pSettings, r_float, silencer_sect, "fire_dispersion_base_k", 0.f);
    float CD_k = READ_IF_EXISTS(pSettings, r_float, silencer_sect, "cam_dispersion_k", 0.f);

    if (!fis_zero(BHP_k))
    {
        for (int i = 0; i < egdCount; ++i)
            fvHitPower[i] += fvHitPower[i] * BHP_k;
    }
    if (!fis_zero(BS_k))
    {
        fHitImpulse += fHitImpulse * BS_k;
        m_fStartBulletSpeed += m_fStartBulletSpeed * BS_k;
    }
    if (!fis_zero(FDB_k))
        fireDispersionBase += fireDispersionBase * FDB_k;
    if (!fis_zero(CD_k))
    {
        camDispersion += camDispersion * CD_k;
        camDispersionInc += camDispersionInc * CD_k;
    }
}

void CWeaponMagazined::ApplyStockParams()
{
    if (!IsAddonAttached(eStock) || !AddonAttachable(eStock))
        return;

    auto stock_sect = GetAddonName(eStock);

    float CD_k = READ_IF_EXISTS(pSettings, r_float, stock_sect, "cam_dispersion_k", 0.f);
    float ZR_k = READ_IF_EXISTS(pSettings, r_float, stock_sect, "zoom_rotate_time_k", 0.f);
    float AI_k = READ_IF_EXISTS(pSettings, r_float, stock_sect, "aim_inertion_k", 0.f);
    float CI_k = READ_IF_EXISTS(pSettings, r_float, stock_sect, "aim_control_inertion_k", 0.f);
    if (!fis_zero(CD_k))
    {
        camDispersion += camDispersion * CD_k;
        camDispersionInc += camDispersionInc * CD_k;
    }
    if (!fis_zero(ZR_k))
    {
        m_fZoomRotateTime += m_fZoomRotateTime * ZR_k;
    }
    m_fAimInertionK += AI_k;
    m_fAimControlInertionK += CI_k;
}

void CWeaponMagazined::ApplyForendParams()
{
    if (!IsAddonAttached(eForend) || !AddonAttachable(eForend))
        return;

    auto forend_sect = GetAddonName(eForend);

    float CD_k = READ_IF_EXISTS(pSettings, r_float, forend_sect, "cam_dispersion_k", 0.f);
    float AI_k = READ_IF_EXISTS(pSettings, r_float, forend_sect, "aim_inertion_k", 0.f);
    float CI_k = READ_IF_EXISTS(pSettings, r_float, forend_sect, "aim_control_inertion_k", 0.f);
    if (!fis_zero(CD_k))
    {
        camDispersion += camDispersion * CD_k;
        camDispersionInc += camDispersionInc * CD_k;
    }
    m_fAimInertionK += AI_k;
    m_fAimControlInertionK += CI_k;
}

void CWeaponMagazined::LoadLaserParams(LPCSTR section)
{
    if (!IsAddonAttached(eLaser))
        return;

    shared_str wpn_sect = cNameSect();
    laserdot_hud_attach_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "laserdot_attach_bone", "wpn_body");
    laserdot_hud_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, hud_sect, "laserdot_attach_offset", Fvector{});
    laserdot_aim_hud_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, hud_sect, "laserdot_aim_attach_offset", laserdot_hud_attach_offset);

    laserdot_world_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "laserdot_attach_offset", Fvector{});

    const bool b_r2 = true;

    const char* m_light_section = pSettings->r_string(section, "laser_light_definition");

    laser_lanim = LALib.FindItem(READ_IF_EXISTS(pSettings, r_string, m_light_section, "color_animator", ""));

    laser_light_render = ::Render->light_create();
    laser_light_render->set_type(IRender_Light::SPOT);
    laser_light_render->set_shadow(true);

    const Fcolor clr = READ_IF_EXISTS(pSettings, r_fcolor, m_light_section, b_r2 ? "color_r2" : "color", (Fcolor{1.0f, 0.0f, 0.0f, 1.0f}));
    laser_fBrightness = clr.intensity();
    laser_light_render->set_color(clr);
    const float range = READ_IF_EXISTS(pSettings, r_float, m_light_section, b_r2 ? "range_r2" : "range", 100.f);
    laser_light_render->set_range(range);
    laser_cone_angle = deg2rad(READ_IF_EXISTS(pSettings, r_float, m_light_section, "spot_angle", 1.f));
    laser_light_render->set_cone(laser_cone_angle);
    laser_light_render->set_texture(READ_IF_EXISTS(pSettings, r_string, m_light_section, "spot_texture", nullptr));
}

void CWeaponMagazined::LoadFlashlightParams(LPCSTR section)
{
    if (!IsAddonAttached(eFlashlight) && !laser_flashlight)
        return;

    shared_str wpn_sect = cNameSect();
    flashlight_hud_attach_bone = READ_IF_EXISTS(pSettings, r_string, hud_sect, "flashlight_light_bone", "wpn_body");
    flashlight_hud_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, hud_sect, "flashlight_attach_offset", Fvector{});
    flashlight_aim_hud_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, hud_sect, "flashlight_aim_attach_offset", flashlight_hud_attach_offset);
    flashlight_omni_hud_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, hud_sect, "flashlight_omni_attach_offset", Fvector{});
    flashlight_aim_omni_hud_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, hud_sect, "flashlight_aim_omni_attach_offset", flashlight_omni_hud_attach_offset);

    flashlight_world_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "flashlight_attach_offset", Fvector{});
    flashlight_omni_world_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "flashlight_omni_world_attach_offset", Fvector{});

    const bool b_r2 = true;

    const char* m_light_section = pSettings->r_string(section, "light_definition");

    flashlight_lanim = LALib.FindItem(READ_IF_EXISTS(pSettings, r_string, m_light_section, "color_animator", ""));

    flashlight_render = ::Render->light_create();
    flashlight_render->set_type(IRender_Light::SPOT);
    flashlight_render->set_shadow(true);

    const Fcolor clr = READ_IF_EXISTS(pSettings, r_fcolor, m_light_section, b_r2 ? "color_r2" : "color", (Fcolor{0.6f, 0.55f, 0.55f, 1.0f}));
    flashlight_fBrightness = clr.intensity();
    flashlight_render->set_color(clr);
    const float range = READ_IF_EXISTS(pSettings, r_float, m_light_section, b_r2 ? "range_r2" : "range", 50.f);
    flashlight_render->set_range(range);
    flashlight_render->set_cone(deg2rad(READ_IF_EXISTS(pSettings, r_float, m_light_section, "spot_angle", 60.f)));
    flashlight_render->set_texture(READ_IF_EXISTS(pSettings, r_string, m_light_section, "spot_texture", nullptr));

    flashlight_omni = ::Render->light_create();
    flashlight_omni->set_type((IRender_Light::LT)(READ_IF_EXISTS(pSettings, r_u8, m_light_section, "omni_type",IRender_Light::SPOT))); // KRodin: вообще omni это обычно поинт, но поинт светит во все стороны от себя, поэтому тут спот используется по умолчанию.
    flashlight_omni->set_shadow(false);

    const Fcolor oclr = READ_IF_EXISTS(pSettings, r_fcolor, m_light_section, b_r2 ? "omni_color_r2" : "omni_color", (Fcolor{1.0f, 1.0f, 1.0f, 0.0f}));
    flashlight_omni->set_color(oclr);
    const float orange = READ_IF_EXISTS(pSettings, r_float, m_light_section, b_r2 ? "omni_range_r2" : "omni_range", 0.25f);
    flashlight_omni->set_range(orange);
}

void CWeaponMagazined::UpdateLaser()
{
    if (IsAddonAttached(eLaser))
    {
        auto io = smart_cast<CInventoryOwner*>(H_Parent());
        if (!laser_light_render->get_active() && IsLaserOn() && (!H_Parent() || (io && this == io->inventory().ActiveItem())))
        {
            laser_light_render->set_active(true);
            UpdateAddonsVisibility();
        }
        else if (laser_light_render->get_active() && (!IsLaserOn() || !(!H_Parent() || (io && this == io->inventory().ActiveItem()))))
        {
            laser_light_render->set_active(false);
            UpdateAddonsVisibility();
        }

        if (laser_light_render->get_active())
        {
            laser_pos = get_LastFP();
            Fvector laser_dir{get_LastFD()};

            auto desire_cone = (GetHUDmode() && Actor()->active_cam() == eacFirstEye) ? laser_cone_angle / GetZoomFactor() : laser_cone_angle;

            laser_light_render->set_cone(desire_cone);

            if (GetHUDmode())
            {
                auto& attach_offset = IsAiming() ? laserdot_aim_hud_attach_offset : laserdot_hud_attach_offset;
                GetBoneOffsetPosDir(laserdot_hud_attach_bone, laser_pos, laser_dir, attach_offset);
                CorrectDirFromWorldToHud(laser_dir);
            }
            else
            {
                XFORM().transform_tiny(laser_pos, laserdot_world_attach_offset);
            }

            Fmatrix laserXForm{};
            laserXForm.identity();
            laserXForm.k.set(laser_dir);
            Fvector::generate_orthonormal_basis_normalized(laserXForm.k, laserXForm.j, laserXForm.i);

            laser_light_render->set_position(laser_pos);
            laser_light_render->set_rotation(laserXForm.k, laserXForm.i);

            // calc color animator
            if (laser_lanim)
            {
                int frame;
                const u32 clr = laser_lanim->CalculateBGR(Device.fTimeGlobal, frame);

                Fcolor fclr{(float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f};
                fclr.mul_rgb(laser_fBrightness / 255.f);
                laser_light_render->set_color(fclr);
            }
        }
    }
}

void CWeaponMagazined::UpdateFlashlight()
{
    if (IsAddonAttached(eFlashlight) || laser_flashlight)
    {
        auto io = smart_cast<CInventoryOwner*>(H_Parent());
        if (!flashlight_render->get_active() && IsFlashlightOn() && (!H_Parent() || (io && this == io->inventory().ActiveItem())))
        {
            flashlight_render->set_active(true);
            flashlight_omni->set_active(true);
            UpdateAddonsVisibility();
        }
        else if (flashlight_render->get_active() && (!IsFlashlightOn() || !(!H_Parent() || (io && this == io->inventory().ActiveItem()))))
        {
            flashlight_render->set_active(false);
            flashlight_omni->set_active(false);
            UpdateAddonsVisibility();
        }

        if (flashlight_render->get_active())
        {
            flashlight_pos = get_LastFP();
            Fvector flashlight_dir{get_LastFD()}, 
                flashlight_pos_omni{get_LastFP()}, 
                flashlight_dir_omni{get_LastFD()};

            if (GetHUDmode())
            {
                const auto b_aiming = IsAiming();

                auto& attach_offset = b_aiming ? flashlight_aim_hud_attach_offset : flashlight_hud_attach_offset;
                GetBoneOffsetPosDir(flashlight_hud_attach_bone, flashlight_pos, flashlight_dir, attach_offset);
                CorrectDirFromWorldToHud(flashlight_dir);

                auto& omni_attach_offset = b_aiming ? flashlight_aim_omni_hud_attach_offset : flashlight_omni_hud_attach_offset;
                GetBoneOffsetPosDir(flashlight_hud_attach_bone, flashlight_pos_omni, flashlight_dir_omni, omni_attach_offset);
                CorrectDirFromWorldToHud(flashlight_dir_omni);
            }
            else
            {
                XFORM().transform_tiny(flashlight_pos, flashlight_world_attach_offset);

                XFORM().transform_tiny(flashlight_pos_omni, flashlight_omni_world_attach_offset);
            }

            Fmatrix flashlightXForm{};
            flashlightXForm.identity();
            flashlightXForm.k.set(flashlight_dir);
            Fvector::generate_orthonormal_basis_normalized(flashlightXForm.k, flashlightXForm.j, flashlightXForm.i);
            flashlight_render->set_position(flashlight_pos);
            flashlight_render->set_rotation(flashlightXForm.k, flashlightXForm.i);

            Fmatrix flashlightomniXForm{};
            flashlightomniXForm.identity();
            flashlightomniXForm.k.set(flashlight_dir_omni);
            Fvector::generate_orthonormal_basis_normalized(flashlightomniXForm.k, flashlightomniXForm.j, flashlightomniXForm.i);
            flashlight_omni->set_position(flashlight_pos_omni);
            flashlight_omni->set_rotation(flashlightomniXForm.k, flashlightomniXForm.i);

            // calc color animator
            if (flashlight_lanim)
            {
                int frame;
                const u32 clr = flashlight_lanim->CalculateBGR(Device.fTimeGlobal, frame);

                Fcolor fclr{(float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f};
                fclr.mul_rgb(flashlight_fBrightness / 255.f);
                flashlight_render->set_color(fclr);
                flashlight_omni->set_color(fclr);
            }
        }
    }
}

//виртуальные функции для проигрывания анимации HUD
void CWeaponMagazined::PlayAnimShow()
{
    PlayHUDMotion({IsMisfire() ? "anm_show_jammed" : (iAmmoElapsed == 0 ? "anm_show_empty" : "nullptr"), "anim_draw", "anm_show"}, false, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
    PlayHUDMotion({IsMisfire() ? "anm_hide_jammed" : (iAmmoElapsed == 0 ? "anm_hide_empty" : "nullptr"), "anim_holster", "anm_hide"}, true, GetState());
}

void CWeaponMagazined::PlayAnimReload()
{
    //if (IsMisfire())
    //    PlayHUDMotion({iAmmoElapsed == 1 ? "anm_reload_jammed_last" : "anm_reload_jammed", "anm_reload_jammed", "anm_reload_empty", "anim_reload", "anm_reload"}, true, GetState());
    //else if (IsPartlyReloading())
    //    PlayHUDMotion({"anim_reload_partly", "anm_reload_partly", "anim_reload", "anm_reload"}, true, GetState());
    //else
    //    PlayHUDMotion({"anm_reload_empty", "anim_reload", "anm_reload"}, true, GetState());

    if (IsPartlyReloading())
        PlayHUDMotion({"anim_reload_partly", "anm_reload_partly", "anim_reload", "anm_reload"}, true, GetState());
    else if (IsSingleReloading())
    {
        if (AnimationExist({"anm_reload_single"}))
            PlayHUDMotion("anm_reload_single", true, GetState());
        else
            PlayHUDMotion({"anim_draw", "anm_show"}, false, GetState());
    }
    else if (IsMisfire())
        PlayHUDMotion({iAmmoElapsed == 1 ? "anm_reload_jammed_last" : "anm_reload_jammed", "anm_reload_jammed", "anm_reload_empty", "anim_reload", "anm_reload"}, true, GetState());
    else
        PlayHUDMotion({"anm_reload_empty", "anim_reload", "anm_reload"}, true, GetState());
}

const char* CWeaponMagazined::GetAnimAimName()
{
    if (auto pActor = smart_cast<const CActor*>(H_Parent()))
    {
        if (AnmIdleMovingAllowed())
        {
            if (const u32 state = pActor->get_state(); state & mcAnyMove)
            {
                if (IsAddonAttached(eScope))
                    return xr_strconcat(guns_aim_anm, "anm_idle_aim_scope_moving", IsMisfire() ? "_jammed" : (iAmmoElapsed == 0 ? "_empty" : ""));
                else
                    return xr_strconcat(guns_aim_anm, "anm_idle_aim_moving", (state & mcFwd) ? "_forward" : ((state & mcBack) ? "_back" : ""),
                                        (state & mcLStrafe) ? "_left" : ((state & mcRStrafe) ? "_right" : ""), IsMisfire() ? "_jammed" : (iAmmoElapsed == 0 ? "_empty" : ""));
            }
        }
    }
    return nullptr;
}

void CWeaponMagazined::PlayAnimAim()
{
    if (IsRotatingToZoom() && !IsRotatingFromZoom())
    {
        string128 guns_aim_start_anm;
        xr_strconcat(guns_aim_start_anm, "anm_idle_aim_start", IsMisfire() ? "_jammed" : (iAmmoElapsed == 0 ? "_empty" : ""));
        if (AnimationExist(guns_aim_start_anm))
        {
            PlayHUDMotion(guns_aim_start_anm, true, GetState());
            PlaySound("sndAimStart", get_LastFP());
            return;
        }
    }

    if (const char* guns_aim_anm = GetAnimAimName())
    {
        if (AnimationExist(guns_aim_anm))
        {
            PlayHUDMotion(guns_aim_anm, true, GetState());
            return;
        }
    }

    PlayHUDMotion({IsMisfire() ? "anm_idle_aim_jammed" : (iAmmoElapsed == 0 ? "anm_idle_aim_empty" : "nullptr"), "anim_idle_aim", "anm_idle_aim"}, true, GetState());
}

void CWeaponMagazined::PlayAnimIdle()
{
    if (GetState() != eIdle)
        return;

    if (IsZoomed())
        PlayAnimAim();
    else
    {
        if (IsRotatingFromZoom() && !IsRotatingToZoom())
        {
            string128 guns_aim_end_anm;
            xr_strconcat(guns_aim_end_anm, "anm_idle_aim_end", IsMisfire() ? "_jammed" : (iAmmoElapsed == 0 ? "_empty" : ""));
            if (AnimationExist(guns_aim_end_anm))
            {
                PlayHUDMotion(guns_aim_end_anm, true, GetState());
                PlaySound("sndAimEnd", get_LastFP());
                return;
            }
        }

        inherited::PlayAnimIdle();
    }
}

void CWeaponMagazined::PlayAnimShoot()
{
    string128 guns_shoot_anm;
    xr_strconcat(guns_shoot_anm, "anm_shoot", (IsZoomed() && !IsRotatingToZoom()) ? (IsAddonAttached(eScope) ? "_aim_scope" : "_aim") : "",
                 IsMisfire() ? "_jammed" : (GetAmmoElapsed() == 1 ? "_last" : ""), IsAddonAttached(eSilencer) ? "_sil" : "");

    PlayHUDMotion({guns_shoot_anm, "anim_shoot", "anm_shots"}, false, GetState());
}

void CWeaponMagazined::PlayAnimFakeShoot()
{
    auto wpn = smart_cast<CWeapon*>(this);
    string128 guns_fakeshoot_anm;
    xr_strconcat(guns_fakeshoot_anm, "anm_fakeshoot",
                 (IsZoomed() && !IsRotatingToZoom()) ? (IsMisfire() ? "_aim_jammed" : "_aim") : ((IsGrenadeMode() && IsMisfire()) ? "_jammed" : ""),
                 ((iAmmoElapsed == 0 && !IsGrenadeMode()) || (wpn && wpn->GetAmmoElapsed2() == 0 && IsGrenadeMode())) ? "_empty" : "",
                 IsAddonAttached(eLauncher) ? (!IsGrenadeMode() ? "_w_gl" : "_g") : "");
    if (AnimationExist(guns_fakeshoot_anm))
        PlayHUDMotion(guns_fakeshoot_anm, true, GetState());
}

void CWeaponMagazined::PlayAnimCheckMisfire()
{
    string128 guns_fakeshoot_anm;
    xr_strconcat(guns_fakeshoot_anm, "anm_fakeshoot", IsMisfire() ? "_jammed" : "", IsAddonAttached(eLauncher) ? (!IsGrenadeMode() ? "_w_gl" : "_g") : "");
    if (AnimationExist(guns_fakeshoot_anm))
    {
        //if (IsZoomed())
        //    OnZoomOut();

        PlayHUDMotion(guns_fakeshoot_anm, true, GetState());
        
        SetPending(TRUE);
    }
    else
    {
        SwitchState(eIdle);
    }
}

void CWeaponMagazined::OnMotionMark(u32 state, const motion_marks& M)
{
    inherited::OnMotionMark(state, M);

    if (state == eReload)
    {
        if (bHasBulletsToHide && xr_strcmp(M.name.c_str(), "lmg_reload") == 0)
        {
            auto ammo_type = m_ammoType;
            int ae = CheckAmmoBeforeReload(ammo_type);

            if (ammo_type == m_ammoType)
            {
                ae += iAmmoElapsed;
            }

            last_hide_bullet = (ae >= bullet_cnt || unlimited_ammo()) ? bullet_cnt : bullet_cnt - ae - 1;
            HUD_VisualBulletUpdate();
        }
        else
        {
            ReloadMagazine();
        }
    }
}

void CWeaponMagazined::OnZoomIn()
{
    inherited::OnZoomIn();

    if (GetState() == eIdle)
        PlayAnimIdle();

    if (auto pActor = smart_cast<CActor*>(H_Parent()))
    {
        CEffectorCam* ec = pActor->Cameras().GetCamEffector(eCEActorMoving);
        if (ec)
            pActor->Cameras().RemoveCamEffector(eCEActorMoving);

        auto ezi = smart_cast<CEffectorZoomInertion*>(pActor->Cameras().GetCamEffector(eCEZoom));
        if (!ezi)
        {
            ezi = (CEffectorZoomInertion*)pActor->Cameras().AddCamEffector(xr_new<CEffectorZoomInertion>());
            ezi->Init(this);
        }
        R_ASSERT(ezi);

        if (IsAddonAttached(eScope) && !IsGrenadeMode())
        {
            PlaySound("sndZoomIn", H_Parent()->Position());
            if (IsAimAltMode())
                return;
            if (m_bVision && !m_binoc_vision)
                m_binoc_vision = xr_new<CBinocularsVision>(this);
        }
    }
}
void CWeaponMagazined::OnZoomOut(bool rezoom)
{
    if (!m_bZoomMode)
        return;
    inherited::OnZoomOut(rezoom);
    if (GetState() == eIdle)
        PlayAnimIdle();
    if (IsAddonAttached(eScope) && !IsGrenadeMode() && H_Parent())
    {
        PlaySound("sndZoomOut", H_Parent()->Position());
    }

    if (auto pActor = smart_cast<CActor*>(H_Parent()))
    {
        pActor->Cameras().RemoveCamEffector(eCEZoom);
        if (m_bVision)
        {
            VERIFY(m_binoc_vision);
            xr_delete(m_binoc_vision);
        }
    }
    if (rezoom)
        OnZoomIn();
}

void CWeaponMagazined::OnZoomChanged()
{
    PlaySound("sndZoomChange", get_LastFP());

    if (auto pActor = smart_cast<CActor*>(H_Parent()))
        pActor->callback(GameObject::eOnActorWeaponZoomChange)(lua_game_object());
}

//переключение режимов стрельбы одиночными и очередями
bool CWeaponMagazined::SwitchMode()
{
    if (eIdle != GetState() || IsPending())
        return false;

    if (SingleShotMode())
        m_iQueueSize = WEAPON_ININITE_QUEUE;
    else
        m_iQueueSize = 1;

    PlaySound("sndFireModes", get_LastFP());

    return true;
}

void CWeaponMagazined::OnNextFireMode(bool opt)
{
    if (!m_bHasDifferentFireModes)
        return;
    if (opt && m_iCurFireMode + 1 == m_aFireModes.size())
        return;
    m_iCurFireMode = (m_iCurFireMode + 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
    PlaySound("sndFireModes", get_LastFP());
}

void CWeaponMagazined::OnPrevFireMode(bool opt)
{
    if (!m_bHasDifferentFireModes)
        return;
    if (opt && m_iCurFireMode == 0)
        return;
    m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
    PlaySound("sndFireModes", get_LastFP());
}

void CWeaponMagazined::OnH_A_Chield()
{
    if (m_bHasDifferentFireModes)
    {
        CActor* actor = smart_cast<CActor*>(H_Parent());
        if (!actor)
            SetQueueSize(-1);
        else
            SetQueueSize(GetCurrentFireMode());
    };
    inherited::OnH_A_Chield();
};

void CWeaponMagazined::SetQueueSize(int size)
{
    m_iQueueSize = size;
    if (m_iQueueSize == -1)
        strcpy_s(m_sCurFireMode, " (A)");
    else
        sprintf_s(m_sCurFireMode, " (%d)", m_iQueueSize);
};

float CWeaponMagazined::GetWeaponDeterioration()
{
    if (!m_bHasDifferentFireModes || m_iPrefferedFireMode == -1 || u32(GetCurrentFireMode()) <= u32(m_iPrefferedFireMode))
        return inherited::GetWeaponDeterioration();
    return (inherited::GetWeaponDeterioration() * m_iShotNum);
}

void CWeaponMagazined::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    CSE_ALifeItemWeaponMagazined* wpn = smart_cast<CSE_ALifeItemWeaponMagazined*>(E);
    wpn->m_u8CurFireMode = u8(m_iCurFireMode & 0x00ff);
    wpn->m_AmmoIDs.clear();
    for (u8 i = 0; i < m_magazine.size(); i++)
    {
        CCartridge& l_cartridge = *(m_magazine.begin() + i);
        wpn->m_AmmoIDs.push_back(l_cartridge.m_LocalAmmoType);
    }
}

void CWeaponMagazined::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(m_iShotNum, output_packet);
    save_data(m_bAimAltMode, output_packet);
    save_data(m_fRTZoomFactor, output_packet);
}

void CWeaponMagazined::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_iShotNum, input_packet);
    load_data(m_bAimAltMode, input_packet);
    load_data(m_fRTZoomFactor, input_packet);
}

void CWeaponMagazined::OnDrawUI()
{
    if (H_Parent() && IsZoomed() && !IsRotatingToZoom() && m_binoc_vision)
        m_binoc_vision->Draw();
    inherited::OnDrawUI();
}
void CWeaponMagazined::net_Relcase(CObject* object)
{
    if (!m_binoc_vision)
        return;

    m_binoc_vision->remove_links(object);
}

void CWeaponMagazined::UnloadAmmo(int unload_count, bool spawn_ammo, bool detach_magazine)
{
    shared_str ammo_in_mag_sect{};
    if (detach_magazine && !unlimited_ammo())
    {
        if (iAmmoElapsed <= (int)HasChamber() && spawn_ammo) // spawn mag empty
            SpawnAmmo(0, GetAddonName(eMagazine).c_str());
        else
            ammo_in_mag_sect = m_magazine.front().m_ammoSect;

        iMagazineSize = HasChamber();
        SetMagazineAttached(false);
    }

    xr_map<LPCSTR, u16> l_ammo;
    for (int i = 0; i < unload_count; ++i)
    {
        CCartridge& l_cartridge = m_magazine.back();
        LPCSTR ammo_sect = detach_magazine ? GetAddonName(eMagazine).c_str() : l_cartridge.m_ammoSect.c_str();

        if (!l_ammo[ammo_sect])
            l_ammo[ammo_sect] = 1;
        else
            l_ammo[ammo_sect]++;

        if (detach_magazine)
            m_magazine.erase(m_magazine.begin());
        else
            m_magazine.pop_back();

        --iAmmoElapsed;
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    if (!spawn_ammo)
        return;

    for (auto& _item : l_ammo)
    {
        if (m_pCurrentInventory && !detach_magazine)
        { // упаковать разряжаемые патроны в неполную пачку
            if (auto l_pA = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(_item.first)))
            {
                u16 l_free = l_pA->m_boxSize - l_pA->m_boxCurr;
                l_pA->m_boxCurr = l_pA->m_boxCurr + (l_free < _item.second ? l_free : _item.second);
                _item.second = _item.second - (l_free < _item.second ? l_free : _item.second);
            }
        }
        if (_item.second && !unlimited_ammo())
            SpawnAmmo(_item.second, _item.first, ammo_in_mag_sect);
    }
}

void CWeaponMagazined::HandleCartridgeInChamber()
{
    if (!HasChamber() || !AddonAttachable(eMagazine) || m_magazine.empty())
        return;
    // отстрел и заряжание нового патрона идёт от конца вектора m_magazine.back() - первым подаётся последний добавленный патрон
    if (m_magazine.back().m_ammoSect != m_magazine.front().m_ammoSect) // первый и последний патрон различны, значит зарядка смешанная
    { // конец заряжания магазина
        // перекладываем патрон отличного типа (первый заряженный, он же последний на отстрел) из начала вектора в конец
        // Msg("~~ weapon:[%s]|back:[%s]|front:[%s]|[1]:[%s] on reloading", Name_script(), *m_magazine.back().m_ammoSect, *m_magazine.front().m_ammoSect,
        // *m_magazine[1].m_ammoSect);
        rotate(m_magazine.begin(), m_magazine.begin() + 1, m_magazine.end());
        // Msg("~~ weapon:[%s]|back:[%s]|front:[%s]|[1]:[%s] after rotate on reloading", Name_script(), *m_magazine.back().m_ammoSect, *m_magazine.front().m_ammoSect,
        // *m_magazine[1].m_ammoSect);
    }
}

// работа затвора
void CWeaponMagazined::OnShutter() { SwitchState(eShutter); }
//
void CWeaponMagazined::switch2_Shutter()
{
    //if (IsZoomed())
    //    OnZoomOut();
    IsMisfire() ? PlayAnimShutterMisfire() : PlayAnimShutter();
    SetPending(TRUE);
}
//
void CWeaponMagazined::PlayAnimShutter()
{
    VERIFY(GetState() == eShutter);
    AnimationExist("anm_shutter") ? PlayHUDMotion("anm_shutter", true, GetState()) : PlayHUDMotion({"anim_draw", "anm_show"}, true, GetState(), false);
    PlaySound("sndShutter", get_LastFP());
}
void CWeaponMagazined::PlayAnimShutterMisfire()
{
    VERIFY(GetState() == eShutter);
    if (AnimationExist("anm_shutter_misfire"))
    {
        PlayHUDMotion("anm_shutter_misfire", true, GetState());
        PlaySound("sndShutterMisfire", get_LastFP());
        return;
    }
    PlayAnimShutter();
}

//void CWeaponMagazined::PlayAnimCheckMisfire()
//{
//    string128 check_misfire;
//    xr_strconcat(check_misfire, "anm_check_misfire", IsAddonAttached(eLauncher) ? (!IsGrenadeMode() ? "_w_gl" : "_g") : "");
//    if (AnimationExist(check_misfire))
//    {
//        if (IsZoomed())
//            OnZoomOut();
//        PlayHUDMotion(check_misfire, true, GetState());
//    }
//    else
//        SwitchState(eIdle);
//}
//
void CWeaponMagazined::ShutterAction() // передёргивание затвора
{
    bool b_spawn_ammo{true};
    if (IsMisfire())
    {
        b_spawn_ammo = false;
        SetMisfire(false);
    }

    if (HasChamber() && !m_magazine.empty())
    {
        UnloadAmmo(1, b_spawn_ammo);
        // Shell Drop
        Fvector vel;
        PHGetLinearVell(vel);
        OnShellDrop(get_LastSP(), vel);
    }
}

float CWeaponMagazined::GetConditionMisfireProbability() const
{
    float mis = inherited::GetConditionMisfireProbability();
    // вероятность осечки от магазина
    if (AddonAttachable(eMagazine) && IsAddonAttached(eMagazine))
    {
        mis += READ_IF_EXISTS(pSettings, r_float, GetAddonName(eMagazine), "misfire_probability_box", 0.0f);
    }
    clamp(mis, 0.0f, 0.99f);
    return mis;
}

bool CWeaponMagazined::IsSingleReloading()
{
    if (IsPartlyReloading() || !AddonAttachable(eMagazine) || !HasChamber() || !m_pAmmo)
        return false;
    return !m_pAmmo->IsBoxReloadable();
}

float CWeaponMagazined::Weight() const
{
    float res = inherited::Weight();

    // додамо вагу порожнього магазину, бо вагу набоїв розрахували раніше
    if (AddonAttachable(eMagazine) && IsAddonAttached(eMagazine))
        res += pSettings->r_float(GetAddonName(eMagazine), "inv_weight");

    return res;
}

float CWeaponMagazined::GetZoomRotationTime() const { return m_fZoomRotateTime + m_fZoomRotateTime * m_fZoomRotateTime_K; }

void CWeaponMagazined::SwitchLaser(bool on)
{
    if (!IsAddonAttached(eLaser))
        return;

    SetLaserOn(on);

    if (AnimationExist("anm_laser"))
        PlayHUDMotion("anm_laser", true, GetState());
    PlaySound("sndLaserSwitch", get_LastFP());

    if (!IsLaserOn())
    {
        laser_light_render->set_active(false);
    }
}

void CWeaponMagazined::SwitchFlashlight(bool on)
{
    if (!IsAddonAttached(eFlashlight) && !laser_flashlight)
        return;

    SetFlashlightOn(on);

    if (AnimationExist("anm_flashlight"))
        PlayHUDMotion("anm_flashlight", true, GetState());
    PlaySound("sndFlashlightSwitch", get_LastFP());

    if (!IsFlashlightOn())
    {
        flashlight_render->set_active(false);
        flashlight_omni->set_active(false);
    }
}

void CWeaponMagazined::UnloadWeaponFull()
{
    PlaySound("sndUnload", get_LastFP());
    UnloadMagazine();
    ShutterAction();
}

void CWeaponMagazined::UnloadAndDetachAllAddons()
{
    UnloadWeaponFull();
    for (u32 i = 0; i < eMagazine; i++)
    {
        if (AddonAttachable(i) && IsAddonAttached(i))
            Detach(GetAddonName(i).c_str(), true);
    }
}

void CWeaponMagazined::DetachAll()
{
    UnloadAndDetachAllAddons();
    inherited::DetachAll();
}

#include "player_hud.h"
void CWeaponMagazined::UpdateMagazineVisibility()
{
    if (!AddonAttachable(eMagazine))
        return;
    bool show = IsAddonAttached(eMagazine) || GetState() == eReload && !IsSingleReloading();
    if (auto pWeaponVisual = smart_cast<IKinematics*>(Visual()))
    {
        if (m_sWpn_magazine_bone.size())
        {
            u16 bone_id = pWeaponVisual->LL_BoneID(m_sWpn_magazine_bone);
            pWeaponVisual->LL_SetBoneVisible(bone_id, show, TRUE);
        }
        for (const auto& mesh_idx : m_magazine_meshes)
            pWeaponVisual->SetRFlag(mesh_idx, show);
    }
    if (GetHUDmode())
    {
        if (m_sHud_wpn_magazine_bone.size())
            HudItemData()->set_bone_visible(m_sHud_wpn_magazine_bone, show);
        for (const auto& mesh_idx : m_magazine_meshes_hud)
            HudItemData()->m_model->SetRFlag(mesh_idx, show);
    }
}

bool CWeaponMagazined::ScopeRespawn()
{
    if (!AddonAttachable(eScope))
        return false;

    xr_string scope_respawn = "scope_respawn";
    if (IsAddonAttached(eScope))
    {
        scope_respawn += "_";
        scope_respawn += GetAddonName(eScope).c_str();
    }

    if (pSettings->line_exist(cNameSect(), scope_respawn.c_str()))
    {
        LPCSTR S = pSettings->r_string(cNameSect(), scope_respawn.c_str());
        if (xr_strcmp(cName().c_str(), S) != 0)
        {
            RespawnWeapon(S);
            return true;
        }
    }
    return false;
}

void CWeaponMagazined::RespawnWeapon(LPCSTR section)
{
    CSE_Abstract* _abstract = Level().spawn_item(section, Position(), ai_location().level_vertex_id(), H_Parent()->ID(), true);
    CSE_ALifeDynamicObject* sobj1 = alife_object();
    CSE_ALifeDynamicObject* sobj2 = smart_cast<CSE_ALifeDynamicObject*>(_abstract);

    NET_Packet P;
    P.w_begin(M_UPDATE);
    u32 position = P.w_tell();
    P.w_u16(0);
    sobj1->STATE_Write(P);
    u16 size = u16(P.w_tell() - position);
    P.w_seek(position, &size, sizeof(u16));
    u16 id;
    P.r_begin(id);
    P.r_u16(size);
    sobj2->STATE_Read(P, size);

    net_Export(_abstract);

    // ці параметри не фігурують у net_Export цього класу або його предків
    // todo: може таки перенести їх у експорт CInventoryItem
    auto se_item = smart_cast<CSE_ALifeInventoryItem*>(sobj2);
    se_item->m_fCondition = m_fCondition;
    /*se_item->m_fRadiationRestoreSpeed = m_ItemEffect[eRadiationRestoreSpeed];*/

    auto io = smart_cast<CInventoryOwner*>(H_Parent());
    auto ii = smart_cast<CInventoryItem*>(this);
    if (io)
    {
        if (io->inventory().InSlot(ii))
            io->SetNextItemSlot(ii->GetSlot());
        else
            io->SetNextItemSlot(0);
    }

    DestroyObject();
    sobj2->Spawn_Write(P, TRUE);
    Level().Send(P, net_flags(TRUE));
    F_entity_Destroy(_abstract);
}

void CWeaponMagazined::SetLaserRange(float range)
{
    if (!laser_light_render)
        return;
    laser_light_render->set_range(range);
}
void CWeaponMagazined::SetLaserAngle(float angle)
{
    if (!laser_light_render)
        return;
    laser_cone_angle = deg2rad(angle);
    laser_light_render->set_cone(laser_cone_angle);
}
void CWeaponMagazined::SetLaserRGB(float r, float g, float b)
{
    if (!laser_light_render)
        return;
    Fcolor c;
    c.a = 1;
    c.r = r;
    c.g = g;
    c.b = b;
    laser_light_render->set_color(c);
}
void CWeaponMagazined::SetLaserType(int type)
{
    if (!laser_light_render)
        return;
    laser_light_render->set_type((IRender_Light::LT)type);
}

void CWeaponMagazined::SetFlashlightRange(float range, int target)
{
    if (!flashlight_render)
        return;
    switch (target)
    {
    case 0: {
        flashlight_render->set_range(range);
        break;
    }
    case 1:
        if (flashlight_omni)
            flashlight_omni->set_range(range);
        break;
    }
}
void CWeaponMagazined::SetFlashlightAngle(float angle, int target)
{
    if (!flashlight_render)
        return;
    float _angle = deg2rad(angle);
    switch (target)
    {
    case 0: light_render->set_cone(_angle); break;
    case 1:
        if (flashlight_omni)
            flashlight_omni->set_cone(_angle);
        break;
    }
}
void CWeaponMagazined::SetFlashlightRGB(float r, float g, float b, int target)
{
    if (!flashlight_render)
        return;
    Fcolor c;
    c.a = 1;
    c.r = r;
    c.g = g;
    c.b = b;
    switch (target)
    {
    case 0: flashlight_render->set_color(c); break;
    case 1:
        if (flashlight_omni)
            flashlight_omni->set_color(c);
        break;
    }
}
void CWeaponMagazined::SetFlashlightType(int type, int target)
{
    if (!flashlight_render)
        return;
    switch (target)
    {
    case 0: flashlight_render->set_type((IRender_Light::LT)type);
    case 1:
        if (flashlight_omni)
            flashlight_omni->set_type((IRender_Light::LT)type);
    }
}

bool CWeaponMagazined::ShouldPlayFlameParticles()
{
    if (m_bFlameParticlesHideInZoom && IsZoomed() && !IsRotatingToZoom() && Is3dssEnabled())
        return false;

    return true;
}
