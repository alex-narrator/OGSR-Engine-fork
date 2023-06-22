#include "stdafx.h"
#include "hudmanager.h"
#include "WeaponMagazined.h"
#include "entity.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "Addons.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "level.h"
#include "object_broker.h"
#include "WeaponBinoculars.h"
#include "WeaponBinocularsVision.h"
#include "ai_object_location.h"

#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include <regex>

#include "uigamecustom.h"
#include "string_table.h"
#include "../xr_3da/LightAnimLibrary.h"

constexpr auto ROTATION_TIME = 0.25f;

CWeaponMagazined::CWeaponMagazined(LPCSTR name, ESoundTypes eSoundType) : CWeapon(name)
{
    m_eSoundShow = ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
    m_eSoundHide = ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
    m_eSoundShot = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
    m_eSoundEmptyClick = ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
    m_eSoundReload = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
    //
    m_eSoundShutter = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
}

CWeaponMagazined::~CWeaponMagazined()
{
    // sounds
    HUD_SOUND::DestroySound(sndShow);
    HUD_SOUND::DestroySound(sndHide);
    HUD_SOUND::DestroySound(sndShot);
    HUD_SOUND::DestroySound(sndSilencerShot);
    HUD_SOUND::DestroySound(sndEmptyClick);
    HUD_SOUND::DestroySound(sndReload);
    HUD_SOUND::DestroySound(sndReloadPartly);
    HUD_SOUND::DestroySound(sndFireModes);
    HUD_SOUND::DestroySound(sndZoomChange);
    HUD_SOUND::DestroySound(sndAimStart);
    HUD_SOUND::DestroySound(sndAimEnd);
    //
    HUD_SOUND::DestroySound(sndShutter);
    HUD_SOUND::DestroySound(sndShutterMisfire);
    HUD_SOUND::DestroySound(sndUnload);
    HUD_SOUND::DestroySound(sndZoomIn);
    HUD_SOUND::DestroySound(sndZoomOut);
    HUD_SOUND::DestroySound(sndNightVisionOn);
    HUD_SOUND::DestroySound(sndNightVisionOff);
    HUD_SOUND::DestroySound(sndNightVisionIdle);
    HUD_SOUND::DestroySound(sndNightVisionBroken);
    HUD_SOUND::DestroySound(sndLaserSwitch);
    HUD_SOUND::DestroySound(sndFlashlightSwitch);

    laser_light_render.destroy();
    flashlight_render.destroy();
    flashlight_omni.destroy();
    flashlight_glow.destroy();

    if (m_binoc_vision)
        xr_delete(m_binoc_vision);
}

void CWeaponMagazined::StopHUDSounds()
{
    HUD_SOUND::StopSound(sndShow);
    HUD_SOUND::StopSound(sndHide);

    HUD_SOUND::StopSound(sndEmptyClick);
    HUD_SOUND::StopSound(sndReload);
    HUD_SOUND::StopSound(sndReloadPartly);
    HUD_SOUND::StopSound(sndFireModes);
    HUD_SOUND::StopSound(sndZoomChange);
    HUD_SOUND::StopSound(sndShot);
    HUD_SOUND::StopSound(sndSilencerShot);
    HUD_SOUND::StopSound(sndAimStart);
    HUD_SOUND::StopSound(sndAimEnd);

    HUD_SOUND::StopSound(sndShutter);
    HUD_SOUND::StopSound(sndShutterMisfire);
    HUD_SOUND::StopSound(sndUnload);
    HUD_SOUND::StopSound(sndZoomIn);
    HUD_SOUND::StopSound(sndZoomOut);
    HUD_SOUND::StopSound(sndNightVisionOn);
    HUD_SOUND::StopSound(sndNightVisionOff);
    HUD_SOUND::StopSound(sndNightVisionIdle);
    HUD_SOUND::StopSound(sndNightVisionBroken);
    HUD_SOUND::StopSound(sndLaserSwitch);
    HUD_SOUND::StopSound(sndFlashlightSwitch);

    inherited::StopHUDSounds();
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

    if (IsAddonAttached(eSilencer))
        m_fAttachedSilencerCondition = wpn->m_fAttachedSilencerCondition;
    if (IsAddonAttached(eScope))
        m_fAttachedScopeCondition = wpn->m_fAttachedScopeCondition;
    if (IsAddonAttached(eLauncher))
        m_fAttachedGrenadeLauncherCondition = wpn->m_fAttachedGrenadeLauncherCondition;
    if (IsAddonAttached(eMagazine))
        m_fAttachedMagazineCondition = wpn->m_fAttachedMagazineCondition;
    //
    return bRes;
}

void CWeaponMagazined::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    CSE_ALifeItemWeaponMagazined* wpn = smart_cast<CSE_ALifeItemWeaponMagazined*>(E);
    wpn->m_AmmoIDs.clear();
    for (u8 i = 0; i < m_magazine.size(); i++)
    {
        CCartridge& l_cartridge = *(m_magazine.begin() + i);
        wpn->m_AmmoIDs.push_back(l_cartridge.m_LocalAmmoType);
    }
    wpn->m_fAttachedSilencerCondition = m_fAttachedSilencerCondition;
    wpn->m_fAttachedScopeCondition = m_fAttachedScopeCondition;
    wpn->m_fAttachedGrenadeLauncherCondition = m_fAttachedGrenadeLauncherCondition;
    wpn->m_fAttachedMagazineCondition = m_fAttachedMagazineCondition;
}

void CWeaponMagazined::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(m_iCurFireMode, output_packet);
    save_data(m_iShotNum, output_packet);
    save_data(m_bScopeSecondMode, output_packet);
    save_data(m_fRTZoomFactor, output_packet);
    save_data(m_bNightVisionSwitchedOn, output_packet);
}

void CWeaponMagazined::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_iCurFireMode, input_packet);
    SetQueueSize(GetCurrentFireMode());
    load_data(m_iShotNum, input_packet);
    load_data(m_bScopeSecondMode, input_packet);
    load_data(m_fRTZoomFactor, input_packet);
    load_data(m_bNightVisionSwitchedOn, input_packet);
}

void CWeaponMagazined::Load(LPCSTR section)
{
    inherited::Load(section);

    // Sounds
    HUD_SOUND::LoadSound(section, "snd_draw", sndShow, m_eSoundShow);
    HUD_SOUND::LoadSound(section, "snd_holster", sndHide, m_eSoundHide);
    HUD_SOUND::LoadSound(section, "snd_shoot", sndShot, m_eSoundShot);
    HUD_SOUND::LoadSound(section, "snd_empty", sndEmptyClick, m_eSoundEmptyClick);

    if (pSettings->line_exist(section, "snd_reload_empty"))
        HUD_SOUND::LoadSound(section, "snd_reload_empty", sndReload, m_eSoundReload);
    else
        HUD_SOUND::LoadSound(section, "snd_reload", sndReload, m_eSoundReload);

    if (pSettings->line_exist(section, "snd_reload_empty"))
    { // OpenXRay-style неполная перезарядка
        HUD_SOUND::LoadSound(section, "snd_reload", sndReloadPartly, m_eSoundReload);
        sndReloadPartlyExist = true;
    }
    else if (pSettings->line_exist(section, "snd_reload_partly"))
    { // OGSR-style неполная перезарядка
        HUD_SOUND::LoadSound(section, "snd_reload_partly", sndReloadPartly, m_eSoundReload);
        sndReloadPartlyExist = true;
    }

    HUD_SOUND::LoadSound(section, pSettings->line_exist(section, "snd_fire_modes") ? "snd_fire_modes" : "snd_empty", sndFireModes, m_eSoundEmptyClick);
    if (pSettings->line_exist(section, "snd_zoom_change"))
        HUD_SOUND::LoadSound(section, "snd_zoom_change", sndZoomChange, m_eSoundEmptyClick);
    //
    HUD_SOUND::LoadSound(section, pSettings->line_exist(section, "snd_shutter") ? "snd_shutter" : "snd_draw", sndShutter, m_eSoundShutter);
    HUD_SOUND::LoadSound(section, pSettings->line_exist(section, "snd_shutter_misfire") ? "snd_shutter_misfire" : "snd_draw", sndShutterMisfire, m_eSoundShutter);
    //
    if (pSettings->line_exist(section, "snd_aim_start"))
        HUD_SOUND::LoadSound(section, "snd_aim_start", sndAimStart, m_eSoundShow);
    if (pSettings->line_exist(section, "snd_aim_end"))
        HUD_SOUND::LoadSound(section, "snd_aim_end", sndAimEnd, m_eSoundHide);

    if (pSettings->line_exist(section, "snd_unload"))
        HUD_SOUND::LoadSound(section, "snd_unload", sndUnload, m_eSoundReload);

    m_pSndShotCurrent = &sndShot;

    // звуки и партиклы глушителя, еслит такой есть
    if (m_eSilencerStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "silencer_flame_particles"))
            m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
        if (pSettings->line_exist(section, "silencer_smoke_particles"))
            m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");
        if (pSettings->line_exist(section, "snd_silncer_shot"))
            HUD_SOUND::LoadSound(section, "snd_silncer_shot", sndSilencerShot, m_eSoundShot);
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
    SetQueueSize(GetCurrentFireMode());

    m_fire_zoomout_time = READ_IF_EXISTS(pSettings, r_u32, section, "fire_zoomout_time", u32(-1));

    m_str_count_tmpl = READ_IF_EXISTS(pSettings, r_string, "features", "wpn_magazined_str_count_tmpl", "{AE}/{AC}");

    m_bHasChamber = READ_IF_EXISTS(pSettings, r_bool, section, "has_chamber", true);

    m_bShowAmmoCounter = READ_IF_EXISTS(pSettings, r_bool, section, "ammo_counter", true);
}

void CWeaponMagazined::FireStart()
{
    if (IsValid() && !IsMisfire())
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
    else if (IsMisfire() && ParentIsActor())
    {
        Misfire();            
    }
    else if (eReload != GetState() && eMisfire != GetState())
        OnMagazineEmpty();
}

void CWeaponMagazined::Reload()
{
    inherited::Reload();
    TryReload();
}

// Real Wolf: Одна реализация на все участки кода.20.01.15
bool CWeaponMagazined::TryToGetAmmo(u32 id)
{
    if (!m_bDirectReload)
    {
        bool mag_weapon = AddonAttachable(eMagazine);
        m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[id].c_str(), ParentIsActor(), mag_weapon, mag_weapon));
    }
    return m_pAmmo && m_pAmmo->m_boxCurr && (!AddonAttachable(eMagazine) || m_pAmmo->IsBoxReloadable() || !iAmmoElapsed || IsGrenadeMode());
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
    // попытка стрелять когда нет патронов
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
        m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[m_ammoType].c_str(), ParentIsActor(), mag_weapon, mag_weapon));
        if (!m_pAmmo && !m_bLockType)
        {
            for (u32 i = 0; i < m_ammoTypes.size(); ++i)
            {
                // проверить патроны всех подходящих типов
                m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[i].c_str(), ParentIsActor(), mag_weapon, mag_weapon));
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
        m_fAttachedMagazineCondition = b_attaching_magazine ? m_pAmmo->GetCondition() : 1.f;
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
        m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), u8(m_ammoType));
    CCartridge l_cartridge = m_DefaultCartridge;
    while (iAmmoElapsed < iMagazineSize)
    {
        if (!unlimited_ammo())
        {
            if (!m_pAmmo->Get(l_cartridge))
                break;
        }
        ++iAmmoElapsed;
        l_cartridge.m_LocalAmmoType = u8(m_ammoType);
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
    case eMisfire:
        PlayAnimCheckMisfire();
        PlaySound(sndEmptyClick, get_LastFP());
        SetPending(TRUE);
        break;
    case eMagEmpty:
        switch2_Empty();
        if (GetNextState() != eReload)
        {
            SwitchState(eIdle);
        }
        break;
    case eReload:
        switch2_Reload();
        break;
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

    // когда происходит апдейт состояния оружия
    // ничего другого не делать
    if (GetNextState() == GetState())
    {
        switch (GetState())
        {
        case eShowing:
        case eHiding:
        case eReload:
        case eMisfire:
        case eIdle:
            fTime -= dt;
            if (fTime < 0)
                fTime = 0;
            break;
        case eFire:
            if (iAmmoElapsed > 0)
                state_Fire(dt);

            if (fTime <= 0)
            {
                if (iAmmoElapsed == 0)
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
        case eMagEmpty: state_MagEmpty(dt); break;
        case eHidden: break;
        }
    }

    if (H_Parent() && IsZoomed() && !IsRotatingToZoom())
    {
        if (m_binoc_vision)
            m_binoc_vision->Update();
        UpdateSwitchNightVision();
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

    // ref_sound positions
    if (sndShow.playing())
        sndShow.set_position(get_LastFP());
    if (sndHide.playing())
        sndHide.set_position(get_LastFP());
    if (sndShot.playing())
        sndShot.set_position(get_LastFP());
    if (sndSilencerShot.playing())
        sndSilencerShot.set_position(get_LastFP());
    if (sndReload.playing())
        sndReload.set_position(get_LastFP());
    if (sndReloadPartly.playing())
        sndReloadPartly.set_position(get_LastFP());
    if (sndEmptyClick.playing())
        sndEmptyClick.set_position(get_LastFP());
    if (sndFireModes.playing())
        sndFireModes.set_position(get_LastFP());
    if (sndZoomChange.playing())
        sndZoomChange.set_position(get_LastFP());
    if (sndAimStart.playing())
        sndAimStart.set_position(get_LastFP());
    if (sndAimEnd.playing())
        sndAimEnd.set_position(get_LastFP());

    if (sndShutter.playing())
        sndShutter.set_position(get_LastFP());
    if (sndShutterMisfire.playing())
        sndShutterMisfire.set_position(get_LastFP());
    if (sndUnload.playing())
        sndUnload.set_position(get_LastFP());
    if (sndZoomIn.playing())
        sndZoomChange.set_position(get_LastFP());
    if (sndZoomOut.playing())
        sndZoomChange.set_position(get_LastFP());
}

void CWeaponMagazined::state_Fire(float dt)
{
    VERIFY(fTimeToFire > 0.f);

    Fvector p1{}, d{};
    p1.set(get_LastFP());
    d.set(get_LastFD());

    if (!H_Parent())
        return;

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

    smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p1, d);
    if (m_iShotNum == 0)
    {
        m_vStartPos = p1;
        m_vStartDir = d;
    };

    VERIFY(!m_magazine.empty());
    //	Msg("%d && %d && (%d || %d) && (%d || %d)", !m_magazine.empty(), fTime<=0, IsWorking(), m_bFireSingleShot, m_iQueueSize < 0, m_iShotNum < m_iQueueSize);
    while (!m_magazine.empty() && fTime <= 0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
    {
        if (CheckForMisfire())
        {
            OnEmptyClick();
            StopShooting();
            // чтобы npc мог выбросить хреновый патрон
            if (!ParentIsActor())
            {
                ShutterAction();
            }
            return;
        }

        m_bFireSingleShot = false;

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
        //
        // повысить изношенность глушителя
        DeteriorateSilencerAttachable(-GetSilencerDeterioration());
        //

        ++m_iShotNum;

        OnShot();

        static int i = 0;
        if (i || m_iShotNum > m_iShootEffectorStart)
            FireTrace(p1, d);
        else
            FireTrace(m_vStartPos, m_vStartDir);
    }

    if (m_iShotNum == m_iQueueSize)
        m_bStopedAfterQueueFired = true;

    UpdateSounds();
}

void CWeaponMagazined::state_MagEmpty(float dt) {}

void CWeaponMagazined::OnShot()
{
    // Sound
    PlaySound(*m_pSndShotCurrent, get_LastFP(), true);

    // Camera
    AddShotEffector();

    // Animation
    PlayAnimShoot();

    // Shell Drop
    Fvector vel;
    PHGetLinearVell(vel);
    OnShellDrop(get_LastSP(), vel);

    // Огонь из ствола
    StartFlameParticles();

    // дым из ствола
    ForceUpdateFireParticles();
    StartSmokeParticles(get_LastFP(), vel);
}

void CWeaponMagazined::OnEmptyClick() { PlaySound(sndEmptyClick, get_LastFP()); }

void CWeaponMagazined::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eReload:
        ReloadMagazine();
        HandleCartridgeInChamber();
        HUD_SOUND::StopSound(sndReload);
        HUD_SOUND::StopSound(sndReloadPartly);
        SwitchState(eIdle);
        break; // End of reload animation
    case eHiding: SwitchState(eHidden); break; // End of Hide
    case eShowing: SwitchState(eIdle); break; // End of Show
    case eIdle:
        switch2_Idle();
        break; // Keep showing idle
    //
    case eShutter:
        ShutterAction();
        SwitchState(eIdle);
        break; // End of Shutter animation
    case eMisfire: SwitchState(eIdle); break; // End of misfire animation
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
void CWeaponMagazined::switch2_Empty()
{
    if (ParentIsActor())
    {
        OnEmptyClick();
        return;
    }
    OnZoomOut();
    if (!TryReload())
    {
        OnEmptyClick();
    }
    else
    {
        inherited::FireEnd();
    }
}
void CWeaponMagazined::PlayReloadSound()
{
    if (IsPartlyReloading() && sndReloadPartlyExist)
        PlaySound(sndReloadPartly, get_LastFP());
    else
        PlaySound(sndReload, get_LastFP());
}

void CWeaponMagazined::switch2_Reload()
{
    CWeapon::FireEnd();
    PlayReloadSound();
    PlayAnimReload();
    SetPending(TRUE);
}

void CWeaponMagazined::switch2_Hiding()
{
    CWeapon::FireEnd();
    StopHUDSounds();
    PlaySound(sndHide, get_LastFP());
    PlayAnimHide();
    SetPending(TRUE);
}

void CWeaponMagazined::switch2_Hidden()
{
    CWeapon::FireEnd();
    HUD_SOUND::StopSound(sndReload);
    HUD_SOUND::StopSound(sndReloadPartly);
    StopCurrentAnimWithoutCallback();
    signal_HideComplete();
    RemoveShotEffector();
}
void CWeaponMagazined::switch2_Showing()
{
    PlaySound(sndShow, get_LastFP());
    SetPending(TRUE);
    PlayAnimShow();
}

bool CWeaponMagazined::Action(s32 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;
    // если оружие чем-то занято, то ничего не делать
    if (IsPending() && cmd != kWPN_FIREMODE_PREV && cmd != kWPN_FIREMODE_NEXT)
        return false;
    switch (cmd)
    {
    case kWPN_RELOAD: {
        if (flags & CMD_START)
        {
            if (Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION)))
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
        };
    }
    break;
    case kWPN_FIREMODE_NEXT: {
        if (flags & CMD_START)
        {
            OnNextFireMode(flags & CMD_OPT);
            return true;
        };
    }
    break;
    }
    return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
    auto pScope = smart_cast<CScope*>(pIItem);
    auto pSilencer = smart_cast<CSilencer*>(pIItem);
    auto pLaser = smart_cast<CLaser*>(pIItem);
    auto pFlashlight = smart_cast<CFlashlight*>(pIItem);
    auto pStock = smart_cast<CStock*>(pIItem);
    auto pForend = smart_cast<CForend*>(pIItem);

    if (pScope && m_eScopeStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0 &&
        std::find(m_scopes.begin(), m_scopes.end(), pIItem->object().cNameSect()) != m_scopes.end())
        return true;
    else if (pSilencer && m_eSilencerStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
             std::find(m_silencers.begin(), m_silencers.end(), pIItem->object().cNameSect()) != m_silencers.end())
        return true;
    else if (pLaser && m_eLaserStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) == 0 &&
             std::find(m_lasers.begin(), m_lasers.end(), pIItem->object().cNameSect()) != m_lasers.end())
        return true;
    else if (pFlashlight && m_eFlashlightStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) == 0 &&
             std::find(m_flashlights.begin(), m_flashlights.end(), pIItem->object().cNameSect()) != m_flashlights.end())
        return true;
    else if (pStock && m_eStockStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonStock) == 0 &&
             std::find(m_stocks.begin(), m_stocks.end(), pIItem->object().cNameSect()) != m_stocks.end())
        return true;
    else if (pForend && m_eForendStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonForend) == 0 &&
             std::find(m_forends.begin(), m_forends.end(), pIItem->object().cNameSect()) != m_forends.end())
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

    auto pScope = smart_cast<CScope*>(pIItem);
    auto pSilencer = smart_cast<CSilencer*>(pIItem);
    auto pLaser = smart_cast<CLaser*>(pIItem);
    auto pFlashlight = smart_cast<CFlashlight*>(pIItem);
    auto pStock = smart_cast<CStock*>(pIItem);
    auto pForend = smart_cast<CForend*>(pIItem);

    if (pScope && m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
    {
        m_cur_scope = (u8)std::distance(m_scopes.begin(), std::find(m_scopes.begin(), m_scopes.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
        result = true;
        m_fAttachedScopeCondition = pIItem->GetCondition();
    }
    else if (pSilencer && m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
    {
        m_cur_silencer = (u8)std::distance(m_silencers.begin(), std::find(m_silencers.begin(), m_silencers.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
        result = true;
        m_fAttachedSilencerCondition = pIItem->GetCondition();
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
        {
            // уничтожить подсоединенную вещь из инвентаря
            //.			pIItem->Drop					();
            pIItem->object().DestroyObject();
        };

        //		if ( !ScopeRespawn( pIItem ) ) {
        UpdateAddonsVisibility();
        InitAddons();
        //}

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
        if (b_spawn_item)
            item_condition = m_fAttachedScopeCondition;
        m_fAttachedScopeCondition = 1.f;
        //
        UpdateAddonsVisibility();
        InitAddons();

        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
             std::find(m_silencers.begin(), m_silencers.end(), item_section_name) != m_silencers.end())
    {
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;
        //
        m_cur_silencer = 0;
        if (b_spawn_item)
            item_condition = m_fAttachedSilencerCondition;
        m_fAttachedSilencerCondition = 1.f;
        //
        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else if (m_eLaserStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) &&
             std::find(m_lasers.begin(), m_lasers.end(), item_section_name) != m_lasers.end())
    {
        SwitchLaser(false);
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
    //////////////////////////////////////////////////////////////////////////
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
    }
    // ліхтарик
    if (IsAddonAttached(eFlashlight))
    {
        sect = AddonAttachable(eFlashlight) ? GetAddonName(eFlashlight).c_str() : cNameSect().c_str();
        LoadFlashlightParams(sect);
    }
    // до цих параметрів можуть додаватися коефіцієнти у функціях нижче
    m_fZoomRotateTime = READ_IF_EXISTS(pSettings, r_float, hud_sect, "zoom_rotate_time", ROTATION_TIME);
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
    m_fScopeInertionFactor = READ_IF_EXISTS(pSettings, r_float, section, "scope_inertion_factor", GetControlInertionFactor());

    if (m_UIScope)
        xr_delete(m_UIScope);

    if (m_UIScopeSecond)
        xr_delete(m_UIScopeSecond);

    // second scope mode
    m_bHasScopeSecond = READ_IF_EXISTS(pSettings, r_bool, section, "scope_second", false);
    if (m_bHasScopeSecond)
    {
        m_fScopeZoomFactorSecond = READ_IF_EXISTS(pSettings, r_float, section, "scope_zoom_factor_second", 1.0f);
        if (LPCSTR scope_second_tex_name = READ_IF_EXISTS(pSettings, r_string, section, "scope_texture_second", nullptr))
        {
            m_UIScopeSecond = xr_new<CUIStaticItem>();
            m_UIScopeSecond->Init(scope_second_tex_name, Core.Features.test(xrCore::Feature::scope_textures_autoresize) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
        }
    }

    if (!IsAddonAttached(eScope) || IsScopeBroken())
    {
        m_bScopeDynamicZoom = m_bVision = m_bNightVisionEnabled = m_bRangeMeter = false;

        if (IsScopeBroken())
        {
            m_fScopeZoomFactor = m_fIronSightZoomFactor;
            if (LPCSTR scope_tex_name_broken = READ_IF_EXISTS(pSettings, r_string, section, "scope_texture_broken", nullptr))
            {
                m_UIScope = xr_new<CUIStaticItem>();
                m_UIScope->Init(scope_tex_name_broken, Core.Features.test(xrCore::Feature::scope_textures_autoresize) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
            }
            if (LPCSTR scope_second_tex_name_broken = READ_IF_EXISTS(pSettings, r_string, section, "scope_texture_second_broken", nullptr))
            {
                m_UIScopeSecond = xr_new<CUIStaticItem>();
                m_UIScopeSecond->Init(scope_second_tex_name_broken, Core.Features.test(xrCore::Feature::scope_textures_autoresize) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
            }
        }

        return;
    }

    HUD_SOUND::StopSound(sndZoomIn);
    HUD_SOUND::StopSound(sndZoomOut);

    if (pSettings->line_exist(section, "snd_zoomin"))
        HUD_SOUND::LoadSound(section, "snd_zoomin", sndZoomIn, SOUND_TYPE_ITEM_USING);
    if (pSettings->line_exist(section, "snd_zoomout"))
        HUD_SOUND::LoadSound(section, "snd_zoomout", sndZoomOut, SOUND_TYPE_ITEM_USING);

    m_bVision = !!READ_IF_EXISTS(pSettings, r_bool, section, "vision_present", false);
    if (m_bVision)
        binoc_vision_sect = section;

    m_bNightVisionEnabled = !!READ_IF_EXISTS(pSettings, r_bool, section, "night_vision", false);
    if (m_bNightVisionEnabled)
    {
        HUD_SOUND::StopSound(sndNightVisionOn);
        HUD_SOUND::StopSound(sndNightVisionOff);
        HUD_SOUND::StopSound(sndNightVisionIdle);
        HUD_SOUND::StopSound(sndNightVisionBroken);

        if (pSettings->line_exist(section, "snd_night_vision_on"))
            HUD_SOUND::LoadSound(section, "snd_night_vision_on", sndNightVisionOn, SOUND_TYPE_ITEM_USING);
        if (pSettings->line_exist(section, "snd_night_vision_off"))
            HUD_SOUND::LoadSound(section, "snd_night_vision_off", sndNightVisionOff, SOUND_TYPE_ITEM_USING);
        if (pSettings->line_exist(section, "snd_night_vision_idle"))
            HUD_SOUND::LoadSound(section, "snd_night_vision_idle", sndNightVisionIdle, SOUND_TYPE_ITEM_USING);
        if (pSettings->line_exist(section, "snd_night_vision_broken"))
            HUD_SOUND::LoadSound(section, "snd_night_vision_broken", sndNightVisionBroken, SOUND_TYPE_ITEM_USING);

        m_NightVisionSect = READ_IF_EXISTS(pSettings, r_string, section, "night_vision_effector", nullptr);
    }

    auto scope_zoom_line = READ_IF_EXISTS(pSettings, r_string, section, "scope_zoom_factor", nullptr);
    m_bScopeDynamicZoom = false;
    if (scope_zoom_line)
    {
        int _count = _GetItemCount(scope_zoom_line);
        ASSERT_FMT(_count >= 1, "!![%s] : No scope_zoom_factor params found in section [%s]", __FUNCTION__, section);
        string128 tmp;
        m_fScopeZoomFactor = atof(_GetItem(scope_zoom_line, 0, tmp));
        if (_count > 1)
        {
            m_fMaxScopeZoomFactor = atof(_GetItem(scope_zoom_line, 1, tmp));
            m_bScopeDynamicZoom = true;

            HUD_SOUND::StopSound(sndZoomChange);
            HUD_SOUND::DestroySound(sndZoomChange);

            if (pSettings->line_exist(section, "snd_zoom_change"))
                HUD_SOUND::LoadSound(section, "snd_zoom_change", sndZoomChange, SOUND_TYPE_ITEM_USING);
        }
        m_uZoomStepCount = _count > 2 ? atoi(_GetItem(scope_zoom_line, 2, tmp)) : 2;
    }

    m_bRangeMeter = READ_IF_EXISTS(pSettings, r_bool, section, "range_meter", false);
    if (m_bRangeMeter)
    {
        Fvector2 fvec_def{0.f, 0.05f};
        m_vRangeMeterOffset = READ_IF_EXISTS(pSettings, r_fvector2, section, "range_meter_offset", fvec_def);
        m_uRangeMeterColor = READ_IF_EXISTS(pSettings, r_color, section, "range_meter_color", color_argb(255, 255, 255, 255));
    }

    LPCSTR scope_tex_name = READ_IF_EXISTS(pSettings, r_string, section, "scope_texture", nullptr);
    if (!scope_tex_name)
        return;
    m_UIScope = xr_new<CUIStaticItem>();
    m_UIScope->Init(scope_tex_name, Core.Features.test(xrCore::Feature::scope_textures_autoresize) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
}

void CWeaponMagazined::ApplySilencerParams()
{
    if (IsAddonAttached(eSilencer) && AddonAttachable(eSilencer))
    {
        conditionDecreasePerShotSilencer = READ_IF_EXISTS(pSettings, r_float, GetAddonName(eSilencer), "condition_shot_dec_silencer", 0.f);
        conditionDecreasePerShotSilencerSelf = READ_IF_EXISTS(pSettings, r_float, GetAddonName(eSilencer), "condition_shot_dec", 0.f);
    }

    if (IsAddonAttached(eSilencer) && !IsSilencerBroken())
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

        HUD_SOUND::StopSound(sndSilencerShot);

        if (AddonAttachable(eSilencer) && pSettings->line_exist(GetAddonName(eSilencer), "snd_silncer_shot"))
            HUD_SOUND::LoadSound(GetAddonName(eSilencer).c_str(), "snd_silncer_shot", sndSilencerShot, m_eSoundShot);
        else if (pSettings->line_exist(cNameSect(), "snd_silncer_shot"))
            HUD_SOUND::LoadSound(cNameSect().c_str(), "snd_silncer_shot", sndSilencerShot, m_eSoundShot);
        else
            sndSilencerShot = sndShot;

        m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
        m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
        m_pSndShotCurrent = &sndSilencerShot;

        // сила выстрела
        LoadFireParams(cNameSect().c_str(), "");

        // подсветка от выстрела
        LPCSTR light_prfx = READ_IF_EXISTS(pSettings, r_string, cNameSect(), "silencer_", "");
        LoadLights(cNameSect().c_str(), light_prfx);
        if (AddonAttachable(eSilencer))
            ApplySilencerKoeffs();
    }
    else
    {
        m_sFlameParticlesCurrent = m_sFlameParticles;
        m_sSmokeParticlesCurrent = m_sSmokeParticles;
        m_pSndShotCurrent = &sndShot;

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
    laserdot_attach_bone = READ_IF_EXISTS(pSettings, r_string, wpn_sect, "laserdot_attach_bone", nullptr);
    laserdot_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "laserdot_attach_offset", Fvector{});
    laserdot_world_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "laserdot_world_attach_offset", Fvector{});
    laserdot_attach_aim_dist = READ_IF_EXISTS(pSettings, r_float, wpn_sect, "laserdot_attach_aim_dist", 0.5f);

    const bool b_r2 = psDeviceFlags.test(rsR2) || psDeviceFlags.test(rsR3) || psDeviceFlags.test(rsR4);

    const char* m_light_section = pSettings->r_string(section, "laser_light_section");

    laser_lanim = LALib.FindItem(READ_IF_EXISTS(pSettings, r_string, m_light_section, "color_animator", ""));

    laser_light_render = ::Render->light_create();
    laser_light_render->set_type(IRender_Light::SPOT);
    laser_light_render->set_shadow(true);

    const Fcolor clr = READ_IF_EXISTS(pSettings, r_fcolor, m_light_section, b_r2 ? "color_r2" : "color", (Fcolor{1.0f, 0.0f, 0.0f, 1.0f}));
    laser_fBrightness = clr.intensity();
    laser_light_render->set_color(clr);
    const float range = READ_IF_EXISTS(pSettings, r_float, m_light_section, b_r2 ? "range_r2" : "range", 100.f);
    laser_light_render->set_range(range);
    laser_light_render->set_cone(deg2rad(READ_IF_EXISTS(pSettings, r_float, m_light_section, "spot_angle", 1.f)));
    laser_light_render->set_texture(READ_IF_EXISTS(pSettings, r_string, m_light_section, "spot_texture", nullptr));

    HUD_SOUND::StopSound(sndLaserSwitch);
    if (pSettings->line_exist(section, "snd_laser_switch"))
        HUD_SOUND::LoadSound(section, "snd_laser_switch", sndLaserSwitch, SOUND_TYPE_ITEM_USING);
}

void CWeaponMagazined::LoadFlashlightParams(LPCSTR section)
{
    if (!IsAddonAttached(eFlashlight))
        return;

    shared_str wpn_sect = cNameSect();
    flashlight_attach_bone = READ_IF_EXISTS(pSettings, r_string, wpn_sect, "torch_light_bone", "wpn_body");
    flashlight_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "torch_attach_offset", Fvector{});
    flashlight_omni_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "torch_omni_attach_offset", Fvector{});
    flashlight_world_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "torch_world_attach_offset", Fvector{});
    flashlight_omni_world_attach_offset = READ_IF_EXISTS(pSettings, r_fvector3, wpn_sect, "torch_omni_world_attach_offset", Fvector{});
    flashlight_attach_aim_dist = READ_IF_EXISTS(pSettings, r_float, wpn_sect, "torch_attach_aim_dist", 0.5f);

    const bool b_r2 = psDeviceFlags.test(rsR2) || psDeviceFlags.test(rsR3) || psDeviceFlags.test(rsR4);

    const char* m_light_section = pSettings->r_string(section, "flashlight_section");

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
    flashlight_omni->set_type(
        (IRender_Light::LT)(READ_IF_EXISTS(pSettings, r_u8, m_light_section, "omni_type",
                                           2))); // KRodin: вообще omni это обычно поинт, но поинт светит во все стороны от себя, поэтому тут спот используется по умолчанию.
    flashlight_omni->set_shadow(false);

    const Fcolor oclr = READ_IF_EXISTS(pSettings, r_fcolor, m_light_section, b_r2 ? "omni_color_r2" : "omni_color", (Fcolor{1.0f, 1.0f, 1.0f, 0.0f}));
    flashlight_omni->set_color(oclr);
    const float orange = READ_IF_EXISTS(pSettings, r_float, m_light_section, b_r2 ? "omni_range_r2" : "omni_range", 0.25f);
    flashlight_omni->set_range(orange);

    flashlight_glow = ::Render->glow_create();
    flashlight_glow->set_texture(READ_IF_EXISTS(pSettings, r_string, m_light_section, "glow_texture", "glow\\glow_torch_r2"));
    flashlight_glow->set_color(clr);
    flashlight_glow->set_radius(READ_IF_EXISTS(pSettings, r_float, m_light_section, "glow_radius", 0.3f));

    HUD_SOUND::StopSound(sndFlashlightSwitch);
    if (pSettings->line_exist(section, "snd_flashlight_switch"))
        HUD_SOUND::LoadSound(section, "snd_flashlight_switch", sndFlashlightSwitch, SOUND_TYPE_ITEM_USING);
}

void SetToScreenCenter(Fvector& dir, Fvector& pos, float distance, CWeapon* wpn)
{
    auto pActor = smart_cast<CActor*>(wpn->H_Parent());
    if (!pActor)
        return;
    dir = pActor->Cameras().Direction();
    pos = pActor->Cameras().Position();
    pos.mad(pos, dir, distance);
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
            Fvector laser_dir = get_LastFD();

            if (GetHUDmode())
            {
                if (IsZoomed() && !IsRotatingToZoom())
                {
                    SetToScreenCenter(laser_dir, laser_pos, laserdot_attach_aim_dist, this);
                }
                else if (laserdot_attach_bone.size())
                {
                    GetBoneOffsetPosDir(laserdot_attach_bone, laser_pos, laser_dir, laserdot_attach_offset);
                    CorrectDirFromWorldToHud(laser_dir);
                }
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
    if (IsAddonAttached(eFlashlight))
    {
        auto io = smart_cast<CInventoryOwner*>(H_Parent());
        if (!flashlight_render->get_active() && IsFlashlightOn() && (!H_Parent() || (io && this == io->inventory().ActiveItem())))
        {
            flashlight_render->set_active(true);
            flashlight_omni->set_active(true);
            flashlight_glow->set_active(true);
            UpdateAddonsVisibility();
        }
        else if (flashlight_render->get_active() && (!IsFlashlightOn() || !(!H_Parent() || (io && this == io->inventory().ActiveItem()))))
        {
            flashlight_render->set_active(false);
            flashlight_omni->set_active(false);
            flashlight_glow->set_active(false);
            UpdateAddonsVisibility();
        }

        if (flashlight_render->get_active())
        {
            Fvector flashlight_pos_omni{}, flashlight_dir{}, flashlight_dir_omni{};

            if (GetHUDmode())
            {
                if (IsZoomed() && !IsRotatingToZoom())
                {
                    SetToScreenCenter(flashlight_dir, flashlight_pos, flashlight_attach_aim_dist, this);
                    SetToScreenCenter(flashlight_dir_omni, flashlight_pos_omni, flashlight_attach_aim_dist, this);
                }
                else
                {
                    GetBoneOffsetPosDir(flashlight_attach_bone, flashlight_pos, flashlight_dir, flashlight_attach_offset);
                    CorrectDirFromWorldToHud(flashlight_dir);

                    GetBoneOffsetPosDir(flashlight_attach_bone, flashlight_pos_omni, flashlight_dir_omni, flashlight_omni_attach_offset);
                    CorrectDirFromWorldToHud(flashlight_dir_omni);
                }
            }
            else
            {
                flashlight_dir = get_LastFD();
                XFORM().transform_tiny(flashlight_pos, flashlight_world_attach_offset);

                flashlight_dir_omni = get_LastFD();
                XFORM().transform_tiny(flashlight_pos_omni, flashlight_omni_world_attach_offset);
            }

            Fmatrix flashlightXForm{};
            flashlightXForm.identity();
            flashlightXForm.k.set(flashlight_dir);
            Fvector::generate_orthonormal_basis_normalized(flashlightXForm.k, flashlightXForm.j, flashlightXForm.i);
            flashlight_render->set_position(flashlight_pos);
            flashlight_render->set_rotation(flashlightXForm.k, flashlightXForm.i);

            flashlight_glow->set_position(flashlight_pos);
            flashlight_glow->set_direction(flashlightXForm.k);

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
                flashlight_glow->set_color(fclr);
            }
        }
    }
}

// виртуальные функции для проигрывания анимации HUD
void CWeaponMagazined::PlayAnimShow()
{
    VERIFY(GetState() == eShowing);
    PlayHUDMotion({"anim_draw", "anm_show"}, false, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
    VERIFY(GetState() == eHiding);
    PlayHUDMotion({"anim_holster", "anm_hide"}, true, GetState());
}

void CWeaponMagazined::PlayAnimReload()
{
    VERIFY(GetState() == eReload);
    if (IsPartlyReloading())
        PlayHUDMotion({"anim_reload_partly", "anm_reload_partly", "anim_reload", "anm_reload"}, true, GetState());
    else if (IsSingleReloading())
    {
        if (AnimationExist("anim_reload_single"))
            PlayHUDMotion("anim_reload_single", true, GetState());
        else
            PlayHUDMotion({"anim_draw", "anm_show"}, false, GetState());
    }
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
                {
                    strcpy_s(guns_aim_anm, "anm_idle_aim_scope_moving");
                    return guns_aim_anm;
                }
                else
                    return xr_strconcat(guns_aim_anm, "anm_idle_aim_moving", (state & mcFwd) ? "_forward" : ((state & mcBack) ? "_back" : ""),
                                        (state & mcLStrafe) ? "_left" : ((state & mcRStrafe) ? "_right" : ""));
            }
        }
    }
    return nullptr;
}

void CWeaponMagazined::PlayAnimAim()
{
    if (IsRotatingToZoom())
    {
        if (AnimationExist("anm_idle_aim_start"))
        {
            PlayHUDMotion("anm_idle_aim_start", true, GetState());
            PlaySound(sndAimStart, get_LastFP());
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
    PlayHUDMotion({"anim_idle_aim", "anm_idle_aim"}, true, GetState());
}

void CWeaponMagazined::PlayAnimIdle()
{
    if (GetState() != eIdle)
        return;
    if (IsZoomed())
        PlayAnimAim();
    else
    {
        if (IsRotatingFromZoom())
        {
            if (AnimationExist("anm_idle_aim_end"))
            {
                PlayHUDMotion("anm_idle_aim_end", true, GetState());
                PlaySound(sndAimEnd, get_LastFP());
                return;
            }
        }
        inherited::PlayAnimIdle();
    }
}

void CWeaponMagazined::PlayAnimShoot()
{
    VERIFY(GetState() == eFire || GetState() == eFire2);

    string_path guns_shoot_anm{};
    xr_strconcat(guns_shoot_anm, "anm_shoot", (IsZoomed() && !IsRotatingToZoom()) ? (IsAddonAttached(eScope) ? "_aim_scope" : "_aim") : "",
                 IsAddonAttached(eSilencer) ? "_sil" : "");

    PlayHUDMotion({guns_shoot_anm, "anim_shoot", "anm_shots"}, false, GetState());
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

        CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>(pActor->Cameras().GetCamEffector(eCEZoom));
        if (!S)
        {
            S = (CEffectorZoomInertion*)pActor->Cameras().AddCamEffector(xr_new<CEffectorZoomInertion>());
            S->Init(this);
        };
        S->SetRndSeed(pActor->GetZoomRndSeed());
        R_ASSERT(S);

        if (IsAddonAttached(eScope) && !IsGrenadeMode())
        {
            PlaySound(sndZoomIn, H_Parent()->Position());
            if (IsSecondScopeMode())
                return;
            if (m_bVision && !m_binoc_vision)
                m_binoc_vision = xr_new<CBinocularsVision>(this);
            if (m_bNightVisionSwitchedOn)
                SwitchNightVision(true, false);
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
        PlaySound(sndZoomOut, H_Parent()->Position());
    }

    if (auto pActor = smart_cast<CActor*>(H_Parent()))
    {
        pActor->SetHardHold(false);
        pActor->Cameras().RemoveCamEffector(eCEZoom);
        if (m_bVision)
        {
            VERIFY(m_binoc_vision);
            xr_delete(m_binoc_vision);
        }
        SwitchNightVision(false, false);
    }
    if (rezoom)
        OnZoomIn();
}

void CWeaponMagazined::OnZoomChanged()
{
    PlaySound(sndZoomChange, get_LastFP());
    m_fRTZoomFactor = m_fZoomFactor; // store current
}

// переключение режимов стрельбы одиночными и очередями
bool CWeaponMagazined::SwitchMode()
{
    if (eIdle != GetState() || IsPending())
        return false;
    if (SingleShotMode())
        m_iQueueSize = WEAPON_ININITE_QUEUE;
    else
        m_iQueueSize = 1;
    PlaySound(sndEmptyClick, get_LastFP());
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
    PlayAnimFiremodes();
};

void CWeaponMagazined::OnPrevFireMode(bool opt)
{
    if (!m_bHasDifferentFireModes)
        return;
    if (opt && m_iCurFireMode == 0)
        return;
    m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
    PlayAnimFiremodes();
};

void CWeaponMagazined::OnH_A_Chield()
{
    if (m_bHasDifferentFireModes)
    {
        SetQueueSize(ParentIsActor() ? GetCurrentFireMode() : -1);
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

float CWeaponMagazined::GetWeaponDeterioration() const
{
    if (!m_bHasDifferentFireModes || m_iPrefferedFireMode == -1 || u32(GetCurrentFireMode()) <= u32(m_iPrefferedFireMode))
        return inherited::GetWeaponDeterioration();
    return (inherited::GetWeaponDeterioration() * m_iShotNum);
}

#include "ui/UIMainIngameWnd.h"
void CWeaponMagazined::GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count)
{
    auto CurrentHUD = HUD().GetUI()->UIMainIngameWnd;
    bool b_wpn_info = CurrentHUD->IsHUDElementAllowed(eActiveItem);
    bool b_gear_info = CurrentHUD->IsHUDElementAllowed(eGear);

    bool b_use_mags = AddonAttachable(eMagazine) && !IsGrenadeMode();

    const int AE = GetAmmoElapsed(), AC = b_use_mags ? GetMagazineCount() : GetAmmoCurrent();

    if (AE == 0 || m_magazine.empty())
        icon_sect_name = "";
    else
        icon_sect_name = b_use_mags && IsAddonAttached(eMagazine) && (b_gear_info && !b_wpn_info) ? GetAddonName(eMagazine).c_str() : m_magazine.back().m_ammoSect.c_str();

    string256 sItemName;
    strcpy_s(sItemName, CStringTable().translate(AE > 0 ? pSettings->r_string(icon_sect_name.c_str(), "inv_name_short") : "st_not_loaded").c_str());

    if ((HasFireModes() || IsGrenadeMode()) && b_wpn_info)
        strcat_s(sItemName, GetCurrentFireModeStr());

    str_name = sItemName;

    // static const std::regex ae_re{ R"(\{AE\})" }, ac_re{ R"(\{AC\})" };
    // str_count = m_str_count_tmpl;
    // str_count = std::regex_replace(str_count, ae_re, std::to_string(AE));
    // str_count = std::regex_replace(str_count, ac_re, unlimited_ammo() ? "--" : std::to_string(b_use_mags ? AC : AC - AE));

    string256 sAmmoInMag{};
    if (ShowAmmoCounter())
        sprintf(sAmmoInMag, "%d", AE);
    else
        sprintf(sAmmoInMag, "%s", CStringTable().translate(GetAmmoElapsedStr()).c_str());

    if (b_wpn_info && b_gear_info)
        sprintf_s(sItemName, "[%s]%d", sAmmoInMag, b_use_mags ? AC : AC - AE);
    else if (b_wpn_info)
        sprintf_s(sItemName, "[%s]", sAmmoInMag);
    else if (b_gear_info)
        sprintf_s(sItemName, "%d", b_use_mags ? AC : AC - AE);
    else if (unlimited_ammo())
        sprintf_s(sItemName, "[%s]--", sAmmoInMag);

    str_count = sItemName;
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
        m_fAttachedMagazineCondition = 1.f;
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
            if (auto l_pA = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(_item.first, ParentIsActor(), true)))
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

float CWeaponMagazined::GetSilencerDeterioration() { return conditionDecreasePerShotSilencerSelf; };

// работа затвора
void CWeaponMagazined::OnShutter() { SwitchState(eShutter); }
//
void CWeaponMagazined::switch2_Shutter()
{
    if (IsZoomed())
        OnZoomOut();
    IsMisfire() ? PlayAnimShutterMisfire() : PlayAnimShutter();
    SetPending(TRUE);
}
//
void CWeaponMagazined::PlayAnimShutter()
{
    VERIFY(GetState() == eShutter);
    AnimationExist("anm_shutter") ? PlayHUDMotion("anm_shutter", true, GetState()) : PlayHUDMotion({"anim_draw", "anm_show"}, true, GetState(), false);
    PlaySound(sndShutter, get_LastFP());
}
void CWeaponMagazined::PlayAnimShutterMisfire()
{
    VERIFY(GetState() == eShutter);
    AnimationExist("anm_shutter_misfire") ? PlayHUDMotion("anm_shutter_misfire", true, GetState()) : PlayHUDMotion({"anim_draw", "anm_show"}, true, GetState(), false);
    PlaySound(sndShutterMisfire, get_LastFP());
}
void CWeaponMagazined::PlayAnimFiremodes()
{
    if (AnimationExist("anm_fire_modes"))
    {
        PlayHUDMotion("anm_fire_modes", true, GetState());
    }
    PlaySound(sndFireModes, get_LastFP());
}

void CWeaponMagazined::PlayAnimCheckMisfire()
{
    string128 check_misfire;
    xr_strconcat(check_misfire, "anm_check_misfire", IsAddonAttached(eLauncher) ? (!IsGrenadeMode() ? "_w_gl" : "_g") : "");
    if (AnimationExist(check_misfire))
    {
        if (IsZoomed())
            OnZoomOut();
        PlayHUDMotion(check_misfire, true, GetState());
    }
    else
        SwitchState(eIdle);
}
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

void CWeaponMagazined::SwitchNightVision(bool vision_on, bool switch_sound)
{
    if (!m_bNightVisionEnabled || IsGrenadeMode())
        return;

    CActor* pA = smart_cast<CActor*>(H_Parent());
    if (!pA)
        return;

    if (vision_on)
    {
        m_bNightVisionOn = true;
    }
    else
    {
        m_bNightVisionOn = false;
    }

    if (m_bNightVisionOn)
    {
        CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvisionScope);
        if (!pp)
        {
            if (!!m_NightVisionSect)
            {
                AddEffector(pA, effNightvisionScope, m_NightVisionSect);
                if (switch_sound)
                    PlaySound(sndNightVisionOn, pA->Position());
                PlaySound(sndNightVisionIdle, pA->Position());
            }
        }
    }
    else
    {
        CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvisionScope);
        if (pp)
        {
            pp->Stop(1.0f);
            if (switch_sound)
                PlaySound(sndNightVisionOff, pA->Position());
            HUD_SOUND::StopSound(sndNightVisionIdle);
        }
    }
}

void CWeaponMagazined::UpdateSwitchNightVision()
{
    if (!m_bNightVisionEnabled || !m_bNightVisionSwitchedOn)
        return;

    auto* pA = smart_cast<CActor*>(H_Parent());
    if (pA && m_bNightVisionOn && !pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision))
        SwitchNightVision(true, false);
}

void CWeaponMagazined::SwitchNightVision()
{
    if (!m_bNightVisionEnabled || IsGrenadeMode() || IsSecondScopeMode())
        return;
    m_bNightVisionSwitchedOn = !m_bNightVisionSwitchedOn;
    SwitchNightVision(!m_bNightVisionOn, true);
}

void CWeaponMagazined::DeteriorateSilencerAttachable(float fDeltaCondition)
{
    if (IsAddonAttached(eSilencer) && AddonAttachable(eSilencer) && !fis_zero(conditionDecreasePerShotSilencerSelf))
    {
        if (fis_zero(m_fAttachedSilencerCondition))
            Detach(GetAddonName(eSilencer).c_str(), false);
        else
        {
            CCartridge& l_cartridge = m_magazine.back(); // с учетом влияния конкретного патрона
            m_fAttachedSilencerCondition += fDeltaCondition + (fDeltaCondition * l_cartridge.m_impair);
            clamp(m_fAttachedSilencerCondition, 0.f, 1.f);
        }
    }
};
//
float CWeaponMagazined::GetConditionMisfireProbability() const
{
    float mis = inherited::GetConditionMisfireProbability();
    // вероятность осечки от магазина
    if (AddonAttachable(eMagazine) && IsAddonAttached(eMagazine))
    {
        mis += READ_IF_EXISTS(pSettings, r_float, GetAddonName(eMagazine), "misfire_probability_box", 0.0f);
        mis += (1.f - m_fAttachedMagazineCondition);
    }
    clamp(mis, 0.0f, 0.99f);
    return mis;
}
//
u32 CWeaponMagazined::GetMagazineCount() const
{
    u32 iMagazinesCount{};
    for (const auto& magazine_sect : m_magazines)
        iMagazinesCount += Actor()->inventory().GetSameItemCount(magazine_sect.c_str(), false);
    return iMagazinesCount;
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

void CWeaponMagazined::ChangeAttachedSilencerCondition(float fDeltaCondition)
{
    if (fis_zero(m_fAttachedSilencerCondition))
        return;
    m_fAttachedSilencerCondition += fDeltaCondition;
    clamp(m_fAttachedSilencerCondition, 0.f, 1.f);
}

void CWeaponMagazined::ChangeAttachedScopeCondition(float fDeltaCondition)
{
    if (fis_zero(m_fAttachedScopeCondition))
        return;
    m_fAttachedScopeCondition += fDeltaCondition;
    clamp(m_fAttachedScopeCondition, 0.f, 1.f);
}

void CWeaponMagazined::ChangeAttachedGrenadeLauncherCondition(float fDeltaCondition)
{
    if (fis_zero(m_fAttachedGrenadeLauncherCondition))
        return;
    m_fAttachedGrenadeLauncherCondition += fDeltaCondition;
    clamp(m_fAttachedGrenadeLauncherCondition, 0.f, 1.f);
}

void CWeaponMagazined::Hit(SHit* pHDS)
{
    if (IsHitToAddon(pHDS))
        return;
    inherited::Hit(pHDS);
}

#include "../Include/xrRender/Kinematics.h"
bool CWeaponMagazined::IsHitToAddon(SHit* pHDS)
{
    auto pWeaponVisual = smart_cast<IKinematics*>(Visual());
    VERIFY(pWeaponVisual);

    bool result = false;
    float hit_power = pHDS->damage();
    hit_power *= m_HitTypeK[pHDS->hit_type];

    u16 bone_id{};
    for (const auto& sbone : m_sWpn_scope_bones)
    {
        bone_id = pWeaponVisual->LL_BoneID(sbone);

        if (IsAddonAttached(eScope) && pWeaponVisual->LL_GetBoneVisible(bone_id))
        {
            if (pHDS->bone() == bone_id)
            {
                ChangeAttachedScopeCondition(-hit_power);
                result = true;
            }
        }
    }

    if (IsAddonAttached(eSilencer))
    {
        bone_id = pWeaponVisual->LL_BoneID(m_sWpn_silencer_bone);
        if (pHDS->bone() == bone_id)
        {
            ChangeAttachedSilencerCondition(-hit_power);
            result = true;
        }
    }

    if (IsAddonAttached(eLauncher))
    {
        bone_id = pWeaponVisual->LL_BoneID(m_sWpn_launcher_bone);
        if (pHDS->bone() == bone_id)
        {
            ChangeAttachedGrenadeLauncherCondition(-hit_power);
            result = true;
        }
    }

    if (result && hud_mode && IsZoomed())
        OnZoomOut();

    if (IsSilencerBroken() || IsScopeBroken() || IsGrenadeLauncherBroken())
        InitAddons();

    return result;
}

bool CWeaponMagazined::IsSilencerBroken() const
{
    return fis_zero(m_fAttachedSilencerCondition) || IsAddonAttached(eSilencer) && !AddonAttachable(eSilencer) && fis_zero(GetCondition());
}

bool CWeaponMagazined::IsScopeBroken() const { return fis_zero(m_fAttachedScopeCondition) || IsAddonAttached(eScope) && !AddonAttachable(eScope) && fis_zero(GetCondition()); }

bool CWeaponMagazined::IsGrenadeLauncherBroken() const
{
    return fis_zero(m_fAttachedGrenadeLauncherCondition) || IsAddonAttached(eLauncher) && !AddonAttachable(eLauncher) && fis_zero(GetCondition());
}

LPCSTR CWeaponMagazined::GetCurrentMagazine_ShortName(bool to_show)
{
    if (!IsAddonAttached(eMagazine))
        return ("");
    LPCSTR mag_short_name = pSettings->r_string(GetAddonName(eMagazine), "inv_name_short");
    return CStringTable().translate(mag_short_name).c_str();
}

void CWeaponMagazined::SwitchLaser(bool on)
{
    if (!IsAddonAttached(eLaser))
        return;

    SetLaserOn(on);

    if (AnimationExist("anm_laser"))
        PlayHUDMotion("anm_laser", true, GetState());
    PlaySound(sndLaserSwitch, get_LastFP());

    if (!IsLaserOn())
    {
        laser_light_render->set_active(false);
    }
}

void CWeaponMagazined::SwitchFlashlight(bool on)
{
    if (!IsAddonAttached(eFlashlight))
        return;

    SetFlashlightOn(on);

    if (AnimationExist("anm_flashlight"))
        PlayHUDMotion("anm_flashlight", true, GetState());
    PlaySound(sndFlashlightSwitch, get_LastFP());

    if (!IsFlashlightOn())
    {
        flashlight_render->set_active(false);
        flashlight_omni->set_active(false);
        flashlight_glow->set_active(false);
    }
}

void CWeaponMagazined::UnloadWeaponFull()
{
    PlaySound(sndUnload, get_LastFP());
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

shared_str CWeaponMagazined::GetAmmoElapsedStr() const
{
    if (iAmmoElapsed >= iMagazineSize - HasChamber())
        return "st_mag_full";
    else if (iAmmoElapsed == 0)
        return "st_mag_empty";
    else if (iAmmoElapsed >= iMagazineSize * 0.75f)
        return "st_mag_quarter_empty";
    else if (iAmmoElapsed <= iMagazineSize * 0.25f)
        return "st_mag_quarter_full";
    else
        return "st_mag_half_full";
}

bool CWeaponMagazined::ShowAmmoCounter() const
{
    if (AddonAttachable(eMagazine) && IsAddonAttached(eMagazine) && !IsGrenadeMode())
    {
        bool res = READ_IF_EXISTS(pSettings, r_bool, GetAddonName(eMagazine), "ammo_counter", false);
        return res;
    }
    return m_bShowAmmoCounter;
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
    }
    if (GetHUDmode())
    {
        if (m_sHud_wpn_magazine_bone.size())
        {
            HudItemData()->set_bone_visible(m_sHud_wpn_magazine_bone, show);
        }
    }
}