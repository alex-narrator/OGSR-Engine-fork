#include "stdafx.h"
#include "weaponshotgun.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "level.h"
#include "actor.h"
#include "Addons.h"

#include "hudmanager.h"
#include "uigamecustom.h"
#include "game_object_space.h"

CWeaponShotgun::CWeaponShotgun(void) : CWeaponCustomPistol("TOZ34")
{
    m_eSoundShotBoth = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
    m_eSoundClose = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
    m_eSoundAddCartridge = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
}

CWeaponShotgun::~CWeaponShotgun(void)
{
    // sounds
    HUD_SOUND::DestroySound(sndShotBoth);
    HUD_SOUND::DestroySound(m_sndOpen);
    HUD_SOUND::DestroySound(m_sndAddCartridge);
    HUD_SOUND::DestroySound(m_sndClose);
}

void CWeaponShotgun::net_Destroy() { inherited::net_Destroy(); }

void CWeaponShotgun::Load(LPCSTR section)
{
    inherited::Load(section);

    // Звук и анимация для выстрела дуплетом
    HUD_SOUND::LoadSound(section, "snd_shoot_duplet", sndShotBoth, m_eSoundShotBoth);

    if (pSettings->line_exist(section, "tri_state_reload"))
    {
        m_bTriStateReload = !!pSettings->r_bool(section, "tri_state_reload");
    }

    if (m_bTriStateReload)
    {
        HUD_SOUND::LoadSound(section, "snd_open_weapon", m_sndOpen, m_eSoundOpen);
        HUD_SOUND::LoadSound(section, "snd_add_cartridge", m_sndAddCartridge, m_eSoundAddCartridge);
        HUD_SOUND::LoadSound(section, "snd_close_weapon", m_sndClose, m_eSoundClose);
    }
}

void CWeaponShotgun::OnShot() { inherited::OnShot(); }

void CWeaponShotgun::Fire2Start()
{
    if (IsPending())
        return;

    inherited::Fire2Start();

    if (IsValid() && !IsMisfire())
    {
        if (!IsWorking())
        {
            if (GetState() == eReload)
                return;
            if (GetState() == eShowing)
                return;
            if (GetState() == eHiding)
                return;

            CWeapon::FireStart();

            if (!iAmmoElapsed)
                SwitchState(eMagEmpty);
            else
                SwitchState((iAmmoElapsed < iMagazineSize) ? eFire : eFire2);
        }
    }
    else if (!iAmmoElapsed)
        SwitchState(eMagEmpty);
}

void CWeaponShotgun::Fire2End()
{
    inherited::Fire2End();
    FireEnd();
}

void CWeaponShotgun::OnShotBoth()
{
    // если патронов меньше, чем 2
    if (iAmmoElapsed < iMagazineSize)
    {
        OnShot();
        return;
    }

    // звук выстрела дуплетом
    PlaySound(sndShotBoth, get_LastFP());

    // Camera
    AddShotEffector();

    // анимация дуплета
    PlayHUDMotion({"anim_shoot_both", "anm_shots_both"}, false, GetState());

    // Shell Drop
    Fvector vel;
    PHGetLinearVell(vel);
    OnShellDrop(get_LastSP(), vel);

    // огонь из 2х стволов
    StartFlameParticles();
    StartFlameParticles2();

    // дым из 2х стволов
    if (ParentIsActor())
    {
        CParticlesObject* pSmokeParticles = NULL;
        CShootingObject::StartParticles(pSmokeParticles, *m_sSmokeParticlesCurrent, get_LastFP(), zero_vel, true);
        pSmokeParticles = NULL;
        CShootingObject::StartParticles(pSmokeParticles, *m_sSmokeParticlesCurrent, get_LastFP2(), zero_vel, true);
    }
}

void CWeaponShotgun::UpdateCL()
{
    float dt = Device.fTimeDelta;

    // когда происходит апдейт состояния оружия
    // ничего другого не делать
    if (GetNextState() == GetState())
    {
        switch (GetState())
        {
        case eFire2:
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

            break;
        }
    }

    inherited::UpdateCL();
}

void CWeaponShotgun::switch2_Fire()
{
    SetPending(TRUE);
    inherited::switch2_Fire();
}

void CWeaponShotgun::switch2_Fire2()
{
    VERIFY(fTimeToFire > 0.f);

    if (fTime <= 0)
    {
        SetPending(TRUE);

        // Fire
        Fvector p1{}, d{};
        p1.set(get_LastFP());
        d.set(get_LastFD());

        CEntity* E = smart_cast<CEntity*>(H_Parent());
        if (E)
        {
#ifdef DEBUG
            CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
            if (NULL == io->inventory().ActiveItem())
            {
                Log("current_state", GetState());
                Log("next_state", GetNextState());
                Log("state_time", m_dwStateTime);
                Log("item_sect", cNameSect().c_str());
                Log("H_Parent", H_Parent()->cNameSect().c_str());
            }
#endif
            E->g_fireParams(this, p1, d);
        }

        OnShotBoth();

        // выстрел из обоих стволов
        FireTrace(p1, d);
        FireTrace(p1, d);
        fTime += fTimeToFire * 2.f;

        // Patch for "previous frame position" :)))
        dwFP_Frame = 0xffffffff;
        dwXF_Frame = 0xffffffff;
    }
}

void CWeaponShotgun::UpdateSounds()
{
    inherited::UpdateSounds();
    if (sndShotBoth.playing())
        sndShotBoth.set_position(get_LastFP());
    if (m_sndOpen.playing())
        m_sndOpen.set_position(get_LastFP());
    if (m_sndAddCartridge.playing())
        m_sndAddCartridge.set_position(get_LastFP());
    if (m_sndClose.playing())
        m_sndClose.set_position(get_LastFP());
}

bool CWeaponShotgun::Action(s32 cmd, u32 flags)
{
    if (GetCurrentFireMode() == 2)
    {
        switch (cmd)
        {
        case kWPN_FIRE: {
            if (flags & CMD_START)
            {
                if (IsPending())
                    return false;
                Fire2Start();
            }
            else
                Fire2End();

            return true;
        }
        }
    }

    if (inherited::Action(cmd, flags) && !(cmd == kWPN_ZOOM && m_bTriStateReload && GetState() == eReload))
        return true;

    bool reload_stop_action = (cmd == kWPN_FIRE || cmd == kWPN_NEXT || cmd == kWPN_RELOAD || cmd == kWPN_ZOOM);
    if (m_bTriStateReload && GetState() == eReload && reload_stop_action && flags & CMD_START &&
        m_sub_state != eSubstateReloadEnd) // остановить перезагрузку
    {
        if (m_sub_state == eSubstateReloadInProcess)
            AddCartridge(1);
        m_sub_state = eSubstateReloadEnd;
        return true;
    }

    return false;
}

void CWeaponShotgun::OnAnimationEnd(u32 state)
{
    if (!m_bTriStateReload || state != eReload)
        return inherited::OnAnimationEnd(state);

    switch (m_sub_state)
    {
    case eSubstateReloadBegin: {
        m_sub_state = eSubstateReloadInProcess;
        SwitchState(eReload);
    }
    break;

    case eSubstateReloadInProcess: {
        if (0 != AddCartridge(1))
        {
            m_sub_state = eSubstateReloadEnd;
        }
        SwitchState(eReload);
    }
    break;

    case eSubstateReloadEnd: {
        m_sub_state = eSubstateReloadBegin;
        SwitchState(eIdle);
    }
    break;
    };
}

void CWeaponShotgun::Reload()
{
    if (IsZoomed())
        OnZoomOut();
    if (m_bTriStateReload)
    {
        TriStateReload();
    }
    else
        inherited::Reload();
}

void CWeaponShotgun::TriStateReload()
{
    if (!HaveCartridgeInInventory(1))
        return;
    m_sub_state = eSubstateReloadBegin;
    SwitchState(eReload);
}

void CWeaponShotgun::OnStateSwitch(u32 S, u32 oldState)
{
    if (!m_bTriStateReload || S != eReload)
    {
        inherited::OnStateSwitch(S, oldState);
        return;
    }

    CWeapon::OnStateSwitch(S, oldState);

    if (m_magazine.size() >= (u32)iMagazineSize || !HaveCartridgeInInventory(1))
    {
        m_sub_state = eSubstateReloadEnd;
        if (m_bDirectReload)
            m_bDirectReload = false;
    };

    switch (m_sub_state)
    {
    case eSubstateReloadBegin:
        if (HaveCartridgeInInventory(1))
            switch2_StartReload();
        break;
    case eSubstateReloadInProcess:
        if (HaveCartridgeInInventory(1))
            switch2_AddCartgidge();
        break;
    case eSubstateReloadEnd: switch2_EndReload(); break;
    };
}

void CWeaponShotgun::switch2_StartReload()
{
    PlaySound(m_sndOpen, get_LastFP());
    PlayAnimOpenWeapon();
    SetPending(TRUE);
}

void CWeaponShotgun::switch2_AddCartgidge()
{
    PlaySound(m_sndAddCartridge, get_LastFP());
    PlayAnimAddOneCartridgeWeapon();
    SetPending(TRUE);
}

void CWeaponShotgun::switch2_EndReload()
{
    PlaySound(m_sndClose, get_LastFP());
    PlayAnimCloseWeapon();
    SetPending(TRUE);
}

void CWeaponShotgun::PlayAnimOpenWeapon()
{
    VERIFY(GetState() == eReload);

    PlayHUDMotion({"anim_open_weapon", "anm_open"}, false, GetState());
}

void CWeaponShotgun::PlayAnimAddOneCartridgeWeapon()
{
    VERIFY(GetState() == eReload);

    PlayHUDMotion({"anim_add_cartridge", "anm_add_cartridge"}, false, GetState());
}

void CWeaponShotgun::PlayAnimCloseWeapon()
{
    VERIFY(GetState() == eReload);

    PlayHUDMotion({"anim_close_weapon", "anm_close"}, false, GetState());
}

void CWeaponShotgun::PlayAnimShutter()
{
    VERIFY(GetState() == eShutter);
    AnimationExist("anm_shutter") ? PlayHUDMotion("anm_shutter", true, GetState()) : PlayHUDMotion({"anm_shots"}, true, GetState());
    PlaySound(sndShutter, get_LastFP());
}
void CWeaponShotgun::PlayAnimShutterMisfire()
{
    VERIFY(GetState() == eShutter);
    AnimationExist("anm_shutter_misfire") ? PlayHUDMotion("anm_shutter_misfire", true, GetState()) : PlayHUDMotion({"anm_shots"}, true, GetState());
    PlaySound(sndShutterMisfire, get_LastFP());
}

bool CWeaponShotgun::HaveCartridgeInInventory(u8 cnt)
{
    if (unlimited_ammo())
        return true;
    if (!m_pCurrentInventory)
        return false;

    if (!m_bDirectReload)
    {
        m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[m_ammoType].c_str(), ParentIsActor()));

        if (!m_pAmmo)
        {
            for (u32 i = 0; i < m_ammoTypes.size(); ++i)
            {
                // проверить патроны всех подходящих типов
                m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[i].c_str(), ParentIsActor()));

                if (m_pAmmo)
                {
                    m_ammoType = i;
                    break;
                }
            }
        }
    }
    return m_pAmmo && m_pAmmo->m_boxCurr >= cnt;
}

u8 CWeaponShotgun::AddCartridge(u8 cnt)
{
    if (IsMisfire())
    {
        SetMisfire(false);
        if (ParentIsActor() && (Level().CurrentViewEntity() == H_Parent()))
            HUD().GetUI()->AddInfoMessage("item_state", "gun_not_jammed");
    }

    if (m_set_next_ammoType_on_reload != u32(-1))
    {
        m_ammoType = m_set_next_ammoType_on_reload;
        m_set_next_ammoType_on_reload = u32(-1);
    }

    if (m_magazine.size() >= (u32)iMagazineSize || !HaveCartridgeInInventory(1))
        return cnt;

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
        m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), u8(m_ammoType));

    CCartridge l_cartridge = m_DefaultCartridge;
    while (cnt && m_magazine.size() < (u32)iMagazineSize)
    {
        if (!unlimited_ammo())
        {
            if (!m_pAmmo->Get(l_cartridge))
                break; //-V595
        }
        --cnt;
        ++iAmmoElapsed;
        l_cartridge.m_LocalAmmoType = u8(m_ammoType);
        m_magazine.push_back(l_cartridge);
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    // выкинуть коробку патронов, если она пустая
    if (m_pAmmo && !m_pAmmo->m_boxCurr)
        m_pAmmo->SetDropManual(TRUE);

    return cnt;
}

void CWeaponShotgun::StopHUDSounds()
{
    HUD_SOUND::StopSound(m_sndOpen);
    HUD_SOUND::StopSound(m_sndAddCartridge);
    HUD_SOUND::StopSound(m_sndClose);
    inherited::StopHUDSounds();
}

bool CWeaponShotgun::CanAttach(PIItem pIItem)
{
    auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (pActor && !pActor->HasRequiredTool(pIItem))
        return false;

    auto pExtender = smart_cast<CExtender*>(pIItem);
    if (pExtender && m_eExtenderStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 == (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonExtender) &&
        std::find(m_extenders.begin(), m_extenders.end(), pIItem->object().cNameSect()) != m_extenders.end())
        return true;
    else
        return inherited::CanAttach(pIItem);
}

bool CWeaponShotgun::CanDetach(const char* item_section_name)
{
    auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (pActor && !pActor->HasRequiredTool(item_section_name))
        return false;

    if (m_eExtenderStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonExtender) &&
        std::find(m_extenders.begin(), m_extenders.end(), item_section_name) != m_extenders.end())
        return true;
    else
        return inherited::CanDetach(item_section_name);
}

bool CWeaponShotgun::Attach(PIItem pIItem, bool b_send_event)
{
    auto pExtender = smart_cast<CExtender*>(pIItem);

    if (pExtender && CSE_ALifeItemWeapon::eAddonAttachable == m_eExtenderStatus && 0 == (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonExtender))
    {
        m_cur_extender = (u8)std::distance(m_extenders.begin(), std::find(m_extenders.begin(), m_extenders.end(), pIItem->object().cNameSect()));
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonExtender;

        UnloadWeaponFull();

        if (b_send_event)
        {
            pIItem->object().DestroyObject();
        }
        UpdateAddonsVisibility();
        InitAddons();
        return true;
    }
    else
        return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponShotgun::Detach(const char* item_section_name, bool b_spawn_item, float item_condition)
{
    if (CSE_ALifeItemWeapon::eAddonAttachable == m_eExtenderStatus && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonExtender) &&
        std::find(m_extenders.begin(), m_extenders.end(), item_section_name) != m_extenders.end())
    {
        UnloadWeaponFull();

        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonExtender;

        m_cur_extender = 0;

        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
    }
    else
        return inherited::Detach(item_section_name, b_spawn_item, item_condition);
}

void CWeaponShotgun::InitAddons()
{
    iMagazineSize = pSettings->r_s32(cNameSect(), "ammo_mag_size");
    if (IsAddonAttached(eExtender))
    {
        iMagazineSize += READ_IF_EXISTS(pSettings, r_u32, GetAddonName(eExtender), "ammo_mag_size", 0);
    }
    inherited::InitAddons();
}

bool CWeaponShotgun::CanBeReloaded()
{
    if (m_bTriStateReload)
        return iAmmoElapsed < iMagazineSize;
    return inherited::CanBeReloaded();
}