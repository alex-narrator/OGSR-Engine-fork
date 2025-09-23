#include "stdafx.h"
#include "weaponshotgun.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "level.h"
#include "actor.h"
#include "../xr_3da/x_ray.h"

CWeaponShotgun::CWeaponShotgun(void) : CWeaponCustomPistol("TOZ34")
{
    //m_eSoundShotBoth = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
    //m_eSoundClose = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
    //m_eSoundAddCartridge = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
    //m_bLockType = true; // Запрещает заряжать в дробовики патроны разного типа
}

//CWeaponShotgun::~CWeaponShotgun(void)
//{
//    // sounds
//    HUD_SOUND::DestroySound(sndShotBoth);
//    HUD_SOUND::DestroySound(m_sndOpen);
//    HUD_SOUND::DestroySound(m_sndAddCartridge);
//    HUD_SOUND::DestroySound(m_sndAddCartridgeSecond);
//    HUD_SOUND::DestroySound(m_sndAddCartridgeStart);
//    HUD_SOUND::DestroySound(m_sndAddCartridgeEmpty);
//    HUD_SOUND::DestroySound(m_sndClose);
//    HUD_SOUND::DestroySound(m_sndCloseEmpty);
//    HUD_SOUND::DestroySound(m_sndBreech);
//    HUD_SOUND::DestroySound(m_sndBreechJammed);
//}

//void CWeaponShotgun::net_Destroy() { inherited::net_Destroy(); }

void CWeaponShotgun::Load(LPCSTR section)
{
    inherited::Load(section);

    // Звук и анимация для выстрела дуплетом
    /*HUD_SOUND::LoadSound(section, "snd_shoot_duplet", sndShotBoth, m_eSoundShotBoth);*/
    m_sounds.LoadSound(section, "snd_shoot_duplet", "sndShotBoth", SOUND_TYPE_WEAPON_SHOOTING);

    //if (pSettings->line_exist(section, "tri_state_reload"))
    //{
    //    m_bTriStateReload = !!pSettings->r_bool(section, "tri_state_reload");
    //}
    m_bTriStateReload = READ_IF_EXISTS(pSettings, r_bool, section, "tri_state_reload", false);
    m_bDrumMagazineReload = READ_IF_EXISTS(pSettings, r_bool, section, "drum_magazine_reload", false);

    if (m_bTriStateReload)
    {
        /*HUD_SOUND::LoadSound(section, "snd_open_weapon", m_sndOpen, m_eSoundOpen);*/
        /*HUD_SOUND::LoadSound(section, "snd_add_cartridge", m_sndAddCartridge, m_eSoundAddCartridge);*/
        //if (pSettings->line_exist(section, "snd_add_cartridge_second"))
        //    HUD_SOUND::LoadSound(section, "snd_add_cartridge_second", m_sndAddCartridgeSecond, m_eSoundAddCartridge);
        //if (pSettings->line_exist(section, "snd_add_cartridge_empty"))
        //    HUD_SOUND::LoadSound(section, "snd_add_cartridge_empty", m_sndAddCartridgeEmpty, m_eSoundAddCartridge);
        //if (pSettings->line_exist(section, "snd_add_cartridge_start"))
        //    HUD_SOUND::LoadSound(section, "snd_add_cartridge_start", m_sndAddCartridgeStart, m_eSoundAddCartridge);
        /*HUD_SOUND::LoadSound(section, "snd_close_weapon", m_sndClose, m_eSoundClose);*/
        //if (pSettings->line_exist(section, "snd_close_weapon_empty"))
        //    HUD_SOUND::LoadSound(section, "snd_close_weapon_empty", m_sndCloseEmpty, m_eSoundClose);
        //if (pSettings->line_exist(section, "snd_breechblock"))
        //    HUD_SOUND::LoadSound(section, "snd_breechblock", m_sndBreech, m_eSoundClose);
        //if (pSettings->line_exist(section, "snd_jam"))
        //    HUD_SOUND::LoadSound(section, "snd_jam", m_sndBreechJammed, m_eSoundClose);

        m_sounds.LoadSound(section, "snd_open_weapon", "sndOpen", SOUND_TYPE_WEAPON_RECHARGING);
        m_sounds.LoadSound(section, "snd_add_cartridge", "sndAddCartridge", SOUND_TYPE_WEAPON_RECHARGING);
        m_sounds.LoadSound(section, "snd_close_weapon", "sndClose", SOUND_TYPE_WEAPON_RECHARGING);
        if (pSettings->line_exist(section, "snd_add_cartridge_second"))
            m_sounds.LoadSound(section, "snd_add_cartridge_second", "sndAddCartridgeSecond", SOUND_TYPE_WEAPON_RECHARGING);
        if (pSettings->line_exist(section, "snd_add_cartridge_empty"))
            m_sounds.LoadSound(section, "snd_add_cartridge_empty", "sndAddCartridgeEmpty", SOUND_TYPE_WEAPON_RECHARGING);
        if (pSettings->line_exist(section, "snd_add_cartridge_start"))
            m_sounds.LoadSound(section, "snd_add_cartridge_start", "sndAddCartridgeStart", SOUND_TYPE_WEAPON_RECHARGING);
        if (pSettings->line_exist(section, "snd_close_weapon_empty"))
            m_sounds.LoadSound(section, "snd_close_weapon_empty", "sndCloseEmpty", SOUND_TYPE_WEAPON_RECHARGING);
        if (pSettings->line_exist(section, "snd_breechblock"))
            m_sounds.LoadSound(section, "snd_breechblock", "sndBreech", SOUND_TYPE_WEAPON_RECHARGING);
        if (pSettings->line_exist(section, "snd_jam"))
            m_sounds.LoadSound(section, "snd_jam", "sndBreechJammed", SOUND_TYPE_WEAPON_RECHARGING);
    }
}

void CWeaponShotgun::OnShot()
{
    inherited::OnShot();

    const auto alias = IsMisfire() ? "sndBreechJammed" : "sndBreech";
    if (m_sounds.FindSoundItem(alias, false))
        PlaySound(alias, get_LastFP());
}

void CWeaponShotgun::Fire2Start()
{
    if (IsPending())
        return;

    inherited::Fire2Start();

    if (IsValid())
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

            SwitchState((iAmmoElapsed < iMagazineSize) ? eFire : eFire2);
        }
    }
    else
        SwitchState(eMagEmpty);
}

void CWeaponShotgun::Fire2End()
{
    inherited::Fire2End();
    FireEnd();
}

void CWeaponShotgun::OnShotBoth()
{
    //если патронов меньше, чем 2
    if (iAmmoElapsed < iMagazineSize)
    {
        OnShot();
        return;
    }

    //звук выстрела дуплетом
    PlaySound("sndShotBoth", get_LastFP());

    // Camera
    AddShotEffector();

    // анимация дуплета
    PlayHUDMotion({"anim_shoot_both", "anm_shots_both"}, IS_OGSR_GA, GetState());

    // Shell Drop
    Fvector vel;
    PHGetLinearVell(vel);
    OnShellDrop(get_LastSP(), vel);

    if (ShouldPlayFlameParticles())
    {
        StartFlameParticles();
        ForceUpdateFireParticles();
    }

    //дым из 2х стволов
    if (ParentIsActor())
    {
        CParticlesObject* pSmokeParticles = NULL;
        CShootingObject::StartParticles(pSmokeParticles, m_sSmokeParticlesCurrent.c_str(), get_LastFP(), {}, true);
        pSmokeParticles = NULL;
        CShootingObject::StartParticles(pSmokeParticles, m_sSmokeParticlesCurrent.c_str(), get_LastFP2(), {}, true);
    }
}

void CWeaponShotgun::UpdateCL()
{
    float dt = Device.fTimeDelta;

    //когда происходит апдейт состояния оружия
    //ничего другого не делать
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
        Fvector p1, d;
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

        //выстрел из обоих стволов
        FireTrace(p1, d);
        FireTrace(p1, d);
        fTime += fTimeToFire * 2.f;

        // Patch for "previous frame position" :)))
        dwFP_Frame = 0xffffffff;
        dwXF_Frame = 0xffffffff;
    }
}

//void CWeaponShotgun::UpdateSounds()
//{
//    inherited::UpdateSounds();
//    const auto& pos = get_LastFP();
//    UpdateSoundPosition("sndShotBoth", pos);
//    UpdateSoundPosition("sndOpen", pos);
//    UpdateSoundPosition("sndAddCartridge", pos);
//    UpdateSoundPosition("sndAddCartridgeSecond", pos);
//    UpdateSoundPosition("sndAddCartridgeEmpty", pos);
//    UpdateSoundPosition("sndClose", pos);
//    UpdateSoundPosition("sndCloseEmpty", pos);
//    UpdateSoundPosition("sndBreech", pos);
//    UpdateSoundPosition("sndBreechJammed", pos);
//}

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

    //if (inherited::Action(cmd, flags))
    //    return true;

    const bool stop_reload = m_bTriStateReload && GetState() == eReload && !IsMisfire() && (flags & CMD_START) && (m_sub_state == eSubstateReloadInProcess || m_sub_state == eSubstateReloadBegin);

    if (inherited::Action(cmd, flags) && !(cmd == kWPN_ZOOM && stop_reload))
        return true;

    if (stop_reload)
    {
        switch (cmd)
        {
        case kWPN_FIRE:
        case kWPN_NEXT:
        case kWPN_RELOAD:
        case kWPN_ZOOM:
            // остановить перезарядку
            m_stop_triStateReload = true;
            return true;
        }
    }

    return false;
}

void CWeaponShotgun::OnAnimationEnd(u32 state)
{
    if (!m_bTriStateReload || state != eReload)
        return inherited::OnAnimationEnd(state);

    auto ProcessReloadEnd = [this] {
        if (IsMisfire())
        {
            SetMisfire(false);
        }
        m_sub_state = eSubstateReloadBegin;
        SwitchState(eIdle);
    };

    switch (m_sub_state)
    {
    case eSubstateReloadBegin: {
        if (IsMisfire() && has_anm_reload_jammed)
            ProcessReloadEnd();
        else
        {
            m_sub_state = IsMisfire() ? eSubstateReloadEnd : eSubstateReloadInProcess;
            SwitchState(eReload);
        }
        break;
    }
    case eSubstateReloadInProcess: {
        AddCartridge(1);
        if (m_stop_triStateReload || !HaveCartridgeInInventory(1) || m_magazine.size() >= iMagazineSize)
            m_sub_state = eSubstateReloadEnd;

        SwitchState(eReload);
        break;
    }
    case eSubstateReloadEnd: {
        ProcessReloadEnd();
        break;
    }
    };
}

void CWeaponShotgun::Reload()
{
    OnZoomOut();
    if (m_bTriStateReload)
    {
        m_stop_triStateReload = false;
        TriStateReload();
    }
    else
        TryReload();
}

void CWeaponShotgun::TriStateReload()
{
    if (HaveCartridgeInInventory(1)/* || IsMisfire()*/)
    {
        StartCartridge = true;
        m_sub_state = is_gunslinger_weapon && !IsMisfire() ? eSubstateReloadInProcess : eSubstateReloadBegin;
        SwitchState(eReload);
    }
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
    case eSubstateReloadBegin: {
        if (HaveCartridgeInInventory(1) || IsMisfire())
        {
            if (IsMisfire())
            {
                const char* Anm = (iAmmoElapsed == 1 && AnimationExist("anm_reload_jammed_last")) ? "anm_reload_jammed_last" : "anm_reload_jammed";
                has_anm_reload_jammed = AnimationExist(Anm);
                if (has_anm_reload_jammed)
                {
                    if (iAmmoElapsed == 1 && SoundExist("sndReloadJammedLast"))
                        PlaySound("sndReloadJammedLast", get_LastFP());
                    else if (SoundExist("sndReloadJammed"))
                        PlaySound("sndReloadJammed", get_LastFP());

                    PlayHUDMotion(Anm, true, GetState());
                    SetPending(TRUE);
                    break;
                }
            }
            PlaySound("sndOpen", get_LastFP());
            if (GetAmmoElapsed() < 1 && AnimationExist("anm_open_empty"))
            {
                PlayHUDMotion({"anm_open_empty"}, true, GetState());
            }
            else
            {
                PlayHUDMotion({"anim_open_weapon", "anm_open"}, true, GetState());
            }
            SetPending(TRUE);
        }
        break;
    }
    case eSubstateReloadInProcess: {
        if (HaveCartridgeInInventory(1))
        {
            if (GetAmmoElapsed() < 1)
            {
                PlaySound(SoundExist("AddCartridgeEmpty") ? "AddCartridgeEmpty" : "sndAddCartridge", get_LastFP());
                PlayHUDMotion({"anm_add_cartridge_empty", "anim_add_cartridge", "anm_add_cartridge"}, false, GetState());
                StartCartridge = false;
            }
            else if (SecondCartridge)
            {
                PlaySound(SoundExist("AddCartridgeSecond") ? "AddCartridgeSecond" : "sndAddCartridge", get_LastFP());
                PlayHUDMotion({"anm_add_cartridge_second", "anim_add_cartridge", "anm_add_cartridge"}, false, GetState());
            }
            else
            {
                PlaySound(SoundExist("sndAddCartridgeStart") ? "sndAddCartridgeStart" : "sndAddCartridge", get_LastFP());
                PlayHUDMotion({StartCartridge ? "anm_add_cartridge_start" : "anm_add_cartridge", "anim_add_cartridge", "anm_add_cartridge"}, false, GetState());
                StartCartridge = false;
            }
            SetPending(TRUE);
            SecondCartridge = GetAmmoElapsed() < 1;
        }
        break;
    }
    case eSubstateReloadEnd: {
        PlayHUDMotion({IsMisfire() ? "anm_close_jammed" : (SecondCartridge ? "anm_close_empty" : "nullptr"), "anim_close_weapon", "anm_close"}, true, GetState());
        PlaySound(((IsMisfire() || SecondCartridge) && SoundExist("sndCloseEmpty")) ? "sndCloseEmpty" : "sndClose", get_LastFP());
        SetPending(TRUE);
        break;
    }
    };
}

void CWeaponShotgun::PlayAnimShutter()
{
    AnimationExist("anm_shutter") ? PlayHUDMotion("anm_shutter", true, GetState()) : PlayHUDMotion({"anm_shots"}, true, GetState());
    PlaySound("sndShutter", get_LastFP());
}
void CWeaponShotgun::PlayAnimShutterMisfire()
{
    if (AnimationExist("anm_shutter_misfire"))
    {
        PlayHUDMotion("anm_shutter_misfire", true, GetState());
        PlaySound("sndShutterMisfire", get_LastFP());
        return;
    }
    PlayAnimShutter();
}

bool CWeaponShotgun::HaveCartridgeInInventory(u8 cnt)
{
    if (unlimited_ammo())
        return true;
    if (!m_pCurrentInventory)
        return false;

    if (!m_bDirectReload)
    {
        m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[m_ammoType].c_str()));

        if (!m_pAmmo)
        {
            for (u32 i = 0; i < m_ammoTypes.size(); ++i)
            {
                // проверить патроны всех подходящих типов
                m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[i].c_str()));

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
        SetMisfire(false);

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
            if (!m_pAmmo->GetCartridge())
                break; //-V595
        }
        --cnt;
        ++iAmmoElapsed;
        //l_cartridge.m_LocalAmmoType = u8(m_ammoType);
        l_cartridge.Load(m_ammoTypes[m_ammoType].c_str(), u8(m_ammoType));
        /*m_magazine.push_back(l_cartridge);*/
        if (!m_magazine.size() || m_bDrumMagazineReload)
            m_magazine.push_back(l_cartridge);
        else
            m_magazine.insert(m_magazine.end() - 1, l_cartridge);
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    // выкинуть коробку патронов, если она пустая
    if (m_pAmmo && !m_pAmmo->m_boxCurr)
        m_pAmmo->SetDropManual(TRUE);

    return cnt;
}

//void CWeaponShotgun::net_Export(CSE_Abstract* E)
//{
//    inherited::net_Export(E);
//    CSE_ALifeItemWeaponShotGun* sg = smart_cast<CSE_ALifeItemWeaponShotGun*>(E);
//    sg->m_AmmoIDs.clear();
//    for (u32 i = 0; i < m_magazine.size(); i++)
//    {
//        CCartridge& l_cartridge = *(m_magazine.begin() + i);
//        sg->m_AmmoIDs.push_back(l_cartridge.m_LocalAmmoType);
//    }
//}

//void CWeaponShotgun::TryReload()
//{
//    if (m_pCurrentInventory)
//    {
//        if (HaveCartridgeInInventory(1) || unlimited_ammo() || (IsMisfire() && iAmmoElapsed))
//        {
//            SetPending(TRUE);
//            SwitchState(eReload);
//            return;
//        }
//    }
//}

void CWeaponShotgun::ReloadMagazine()
{ //Используется только при отключенном tri_state_reload
    m_dwAmmoCurrentCalcFrame = 0;

    if (!m_pCurrentInventory)
        return;

    u8 cnt = AddCartridge(1);
    while (cnt == 0)
        cnt = AddCartridge(1);
}

//void CWeaponShotgun::StopHUDSounds()
//{
//    HUD_SOUND::StopSound(m_sndOpen);
//    HUD_SOUND::StopSound(m_sndAddCartridge);
//    HUD_SOUND::StopSound(m_sndAddCartridgeSecond);
//    HUD_SOUND::StopSound(m_sndAddCartridgeStart);
//    HUD_SOUND::StopSound(m_sndAddCartridgeEmpty);
//    HUD_SOUND::StopSound(m_sndClose);
//    HUD_SOUND::StopSound(m_sndCloseEmpty);
//    HUD_SOUND::StopSound(m_sndBreech);
//    HUD_SOUND::StopSound(m_sndBreechJammed);
//
//    inherited::StopHUDSounds();
//}

bool CWeaponShotgun::CanAttach(PIItem pIItem)
{
    auto pExtender = std::find(m_extenders.begin(), m_extenders.end(), pIItem->object().cNameSect()) != m_extenders.end();
    if (pExtender && m_eExtenderStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 == (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonExtender))
        return true;
    else
        return inherited::CanAttach(pIItem);
}

bool CWeaponShotgun::CanDetach(const char* item_section_name)
{
    if (m_eExtenderStatus == CSE_ALifeItemWeapon::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonExtender) &&
        std::find(m_extenders.begin(), m_extenders.end(), item_section_name) != m_extenders.end())
        return true;
    else
        return inherited::CanDetach(item_section_name);
}

bool CWeaponShotgun::Attach(PIItem pIItem, bool b_send_event)
{
    auto pExtender = std::find(m_extenders.begin(), m_extenders.end(), pIItem->object().cNameSect()) != m_extenders.end();

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
    //if (m_bTriStateReload)
    //    return iAmmoElapsed < iMagazineSize;
    //return inherited::CanBeReloaded();
    return iAmmoElapsed < iMagazineSize;
}