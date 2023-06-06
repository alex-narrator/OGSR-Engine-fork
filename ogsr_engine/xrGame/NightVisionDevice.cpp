#include "stdafx.h"
#include "NightVisionDevice.h"
#include "xrserver_objects_alife_items.h"
#include "actor.h"
#include "Inventory.h"
#include "level.h"
#include "HUDManager.h"
#include "actorEffector.h"
#include "ai_sounds.h"

CNightVisionDevice::CNightVisionDevice(void) { SetSlot(TORCH_SLOT); }

CNightVisionDevice::~CNightVisionDevice(void)
{
    HUD_SOUND::DestroySound(sndNightVisionOn);
    HUD_SOUND::DestroySound(sndNightVisionOff);
    HUD_SOUND::DestroySound(sndNightVisionIdle);
    HUD_SOUND::DestroySound(sndNightVisionBroken);
}

void CNightVisionDevice::Load(LPCSTR section)
{
    inherited::Load(section);

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

void CNightVisionDevice::Switch() { Switch(!m_bNightVisionOn); }

void CNightVisionDevice::Switch(bool turn_on)
{
    auto pA = smart_cast<CActor*>(H_Parent());
    if (!pA)
        return;

    auto pActorTorch = pA->GetNightVisionDevice();
    if (pActorTorch && pActorTorch != this)
        return;

    if (turn_on && fis_zero(GetPowerLevel()))
        return;
    inherited::Switch(turn_on);

    bool bPlaySoundFirstPerson = (pA == Level().CurrentViewEntity());

    if (!!m_NightVisionSect)
    {
        const char* disabled_names = READ_IF_EXISTS(pSettings, r_string, cNameSect(), "disabled_maps", nullptr);
        const char* curr_map = Level().name().c_str();
        bool b_allow = true;
        if (disabled_names)
        {
            u32 cnt = _GetItemCount(disabled_names);
            string512 tmp;
            for (u32 i = 0; i < cnt; ++i)
            {
                _GetItem(disabled_names, i, tmp);
                if (!stricmp(tmp, curr_map))
                {
                    b_allow = false;
                    break;
                }
            }
        }

        if (!b_allow)
        {
            HUD_SOUND::PlaySound(sndNightVisionBroken, pA->Position(), pA, bPlaySoundFirstPerson);
            return;
        }
        else
        {
            m_bNightVisionOn = turn_on;

            if (m_bNightVisionOn)
            {
                CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
                if (!pp)
                {
                    AddEffector(pA, effNightvision, m_NightVisionSect);
                    HUD_SOUND::PlaySound(sndNightVisionOn, pA->Position(), pA, bPlaySoundFirstPerson);
                    HUD_SOUND::PlaySound(sndNightVisionIdle, pA->Position(), pA, bPlaySoundFirstPerson, true);
                }
            }
        }
    }
    else
    {
        m_bNightVisionOn = false;
    }

    if (!m_bNightVisionOn)
    {
        CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
        if (pp)
        {
            pp->Stop(1.0f);
            HUD_SOUND::PlaySound(sndNightVisionOff, pA->Position(), pA, bPlaySoundFirstPerson);
            HUD_SOUND::StopSound(sndNightVisionIdle);
        }
    }
}

void CNightVisionDevice::UpdateSwitch()
{
    auto* pA = smart_cast<CActor*>(H_Parent());
    if (pA && m_bNightVisionOn && !pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision))
        Switch(true);
}

BOOL CNightVisionDevice::net_Spawn(CSE_Abstract* DC)
{
    bool res = inherited::net_Spawn(DC);

    auto night_vis = smart_cast<CSE_ALifeItemNightVisionDevice*>(DC);

    m_bNightVisionOn = night_vis->m_nightvision_active;

    return res;
}

void CNightVisionDevice::net_Destroy()
{
    Switch(false);
    inherited::net_Destroy();
}

void CNightVisionDevice::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    Switch(false);

    HUD_SOUND::StopSound(sndNightVisionOn);
    HUD_SOUND::StopSound(sndNightVisionOff);
    HUD_SOUND::StopSound(sndNightVisionIdle);

    // m_NightVisionChargeTime		= m_NightVisionRechargeTime;
}

void CNightVisionDevice::UpdateCL()
{
    inherited::UpdateCL();
    UpdateSwitch();
}

void CNightVisionDevice::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    auto night_vis = smart_cast<CSE_ALifeItemNightVisionDevice*>(E);
    night_vis->m_nightvision_active = m_bNightVisionOn;
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    night_vis->m_attached = (pA && pA->attached(this));
}

bool CNightVisionDevice::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    return pA ? (pA->GetNightVisionDevice() == this) : true;
}

void CNightVisionDevice::afterDetach()
{
    inherited::afterDetach();
    if (smart_cast<CActor*>(H_Parent()))
        Switch(false);
}