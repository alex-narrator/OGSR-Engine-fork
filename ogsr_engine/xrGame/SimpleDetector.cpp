#include "StdAfx.h"
#include "SimpleDetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "../xr_3da/LightAnimLibrary.h"
#include "player_hud.h"

void CSimpleDetector::CreateUI()
{
    R_ASSERT(!m_ui);
    m_ui = xr_new<CUIArtefactDetectorSimple>();
    ui().construct(this);
}

CUIArtefactDetectorSimple& CSimpleDetector::ui() { return *((CUIArtefactDetectorSimple*)m_ui); }
void CSimpleDetector::UpdateAf()
{
    if (m_artefacts.m_ItemInfos.empty())
        return;

    auto it = m_artefacts.m_ItemInfos.begin();
    float min_dist = flt_max;

    for (auto& item : m_artefacts.m_ItemInfos)
    { // only nearest
        CArtefact* pAf = item.first;

        float d = Position().distance_to(pAf->Position());
        if (d < min_dist)
        {
            min_dist = d;
            it = m_artefacts.m_ItemInfos.find(pAf);
        }
        TryMakeArtefactVisible(pAf);
    }

    ITEM_INFO& af_info = it->second;
    ITEM_TYPE* item_type = af_info.curr_ref;

    float dist = min_dist;

    float fRelPow = (dist / m_fArtefactRadius);
    clamp(fRelPow, 0.f, 1.f);

    // определить текущую частоту срабатывания сигнала
    af_info.cur_period = item_type->freq.x + (item_type->freq.y - item_type->freq.x) * (fRelPow * fRelPow);

    float min_snd_freq = 0.9f;
    float max_snd_freq = 1.4f;

    float snd_freq = min_snd_freq + (max_snd_freq - min_snd_freq) * (1.0f - fRelPow);

    if (af_info.snd_time > af_info.cur_period)
    {
        af_info.snd_time = 0;
        PlaySound(item_type->detect_snds, Position());
        ui().Flash(true, fRelPow);
        if (!item_type->detect_snds.sounds.empty() && item_type->detect_snds.m_activeSnd)
            item_type->detect_snds.m_activeSnd->snd.set_frequency(snd_freq);
    }
    else
        af_info.snd_time += Device.fTimeDelta;
}

void CSimpleDetector::UpdateZones()
{
    if (m_zones.m_ItemInfos.empty())
        return;

    for (auto& item : m_zones.m_ItemInfos)
    { // all
        auto pZone = item.first;

        ITEM_INFO& zone_info = item.second;
        ITEM_TYPE* item_type = zone_info.curr_ref;

        CSpaceRestrictor* pSR = smart_cast<CSpaceRestrictor*>(pZone);
        float dist = pSR->distance_to(Position());
        if (dist < 0.f)
            dist = 0.f;

        float fRelPow = (dist / m_fDetectRadius);
        clamp(fRelPow, 0.f, 1.f);

        // current sound frequency
        zone_info.cur_period = item_type->freq.x + (item_type->freq.y - item_type->freq.x) * (fRelPow * fRelPow);

        // float min_snd_freq = 0.9f;
        // float max_snd_freq = 1.4f;

        // float snd_freq = min_snd_freq + (max_snd_freq - min_snd_freq) * (1.0f - fRelPow);

        if (zone_info.snd_time > zone_info.cur_period)
        {
            zone_info.snd_time = 0;
            PlaySound(item_type->detect_snds, Position());
            // if (!item_type->detect_snds.sounds.empty() && item_type->detect_snds.m_activeSnd)
            //     item_type->detect_snds.m_activeSnd->snd.set_frequency(snd_freq);
        }
        else
            zone_info.snd_time += Device.fTimeDelta;
    }
}

void CSimpleDetector::DisableUIDetection()
{
    if (m_ui)
    {
        ui().update();
        ui().Flash(false, 0.f);
    }
}

void CUIArtefactDetectorSimple::construct(CSimpleDetector* p)
{
    m_parent = p;
    m_flash_bone = BI_NONE;
    m_on_off_bone = BI_NONE;
    Flash(false, 0.0f);
}

CUIArtefactDetectorSimple::~CUIArtefactDetectorSimple()
{
    m_flash_light.destroy();
    m_on_off_light.destroy();
}

void CUIArtefactDetectorSimple::Flash(bool bOn, float fRelPower)
{
    if (!m_parent->HudItemData() || m_flash_bone == BI_NONE)
        return;

    IKinematics* K = m_parent->HudItemData()->m_model;
    R_ASSERT(K);
    if (bOn)
    {
        K->LL_SetBoneVisible(m_flash_bone, TRUE, TRUE);
        m_turn_off_flash_time = Device.dwTimeGlobal + iFloor(fRelPower * 1000.0f);
    }
    else
    {
        K->LL_SetBoneVisible(m_flash_bone, FALSE, TRUE);
        m_turn_off_flash_time = 0;
    }
    if (bOn != m_flash_light->get_active())
        m_flash_light->set_active(bOn);
}

void CUIArtefactDetectorSimple::setup_internals()
{
    R_ASSERT(!m_flash_light);
    m_flash_light = ::Render->light_create();
    m_flash_light->set_shadow(false);
    m_flash_light->set_type(IRender_Light::POINT);
    m_flash_light->set_range(pSettings->r_float(m_parent->HudItemData()->m_sect_name, "flash_light_range"));
    m_flash_light->set_hud_mode(true);

    R_ASSERT(!m_on_off_light);
    m_on_off_light = ::Render->light_create();
    m_on_off_light->set_shadow(false);
    m_on_off_light->set_type(IRender_Light::POINT);
    m_on_off_light->set_range(pSettings->r_float(m_parent->HudItemData()->m_sect_name, "onoff_light_range"));
    m_on_off_light->set_hud_mode(true);

    IKinematics* K = m_parent->HudItemData()->m_model;
    R_ASSERT(K);

    R_ASSERT(m_flash_bone == BI_NONE);
    R_ASSERT(m_on_off_bone == BI_NONE);

    m_flash_bone = K->LL_BoneID("light_bone_2");
    m_on_off_bone = K->LL_BoneID("light_bone_1");

    K->LL_SetBoneVisible(m_flash_bone, FALSE, TRUE);
    K->LL_SetBoneVisible(m_on_off_bone, TRUE, TRUE);

    R_ASSERT(m_pOnOfLAnim = LALib.FindItem("det_on_off"));
    R_ASSERT(m_pFlashLAnim = LALib.FindItem("det_flash"));
}

void CUIArtefactDetectorSimple::update()
{
    inherited::update();

    if (m_parent->HudItemData())
    {
        if (m_flash_bone == BI_NONE)
            setup_internals();

        if (m_turn_off_flash_time && m_turn_off_flash_time < Device.dwTimeGlobal)
            Flash(false, 0.0f);

        firedeps fd;
        m_parent->HudItemData()->setup_firedeps(fd);
        if (m_flash_light->get_active())
            m_flash_light->set_position(fd.vLastFP);

        m_on_off_light->set_position(fd.vLastFP2);
        if (!m_on_off_light->get_active())
            m_on_off_light->set_active(true);

        int frame = 0;
        const u32 clr = m_pOnOfLAnim->CalculateRGB(Device.fTimeGlobal, frame);
        m_on_off_light->set_color(Fcolor(clr));
    }
}