#include "StdAfx.h"
#include "CustomDetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "ActorEffector.h"
#include "Actor.h"

ITEM_INFO::~ITEM_INFO()
{
    if (pParticle)
        CParticlesObject::Destroy(pParticle);
}

CCustomDetector::~CCustomDetector()
{
    m_artefacts.destroy();
    m_zones.destroy();
    m_creatures.destroy();

    Switch(false);
    xr_delete(m_ui);
}

//void CCustomDetector::save(NET_Packet& output_packet)
//{
//    inherited::save(output_packet);
//}
//
//void CCustomDetector::load(IReader& input_packet)
//{
//    inherited::load(input_packet);
//}

void CCustomDetector::Load(LPCSTR section)
{
    inherited::Load(section);

    m_fDetectRadius = READ_IF_EXISTS(pSettings, r_float, section, "detect_radius", 15.0f);
    m_fArtefactRadius = READ_IF_EXISTS(pSettings, r_float, section, "af_radius", m_fDetectRadius);
    m_fAfVisRadius = READ_IF_EXISTS(pSettings, r_float, section, "af_vis_radius", 2.0f);
    m_artefacts.load(section, "af");
    m_artefacts.m_af_rank = READ_IF_EXISTS(pSettings, r_u32, section, "af_rank", 0);
    m_zones.load(section, "zone");
    m_creatures.load(section, "creature");

    m_nightvision_particle = READ_IF_EXISTS(pSettings, r_string, section, "night_vision_particle", nullptr);
}

void CCustomDetector::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    if (!IsPowerOn())
    {
        DisableUIDetection();
        return;
    }

    Position().set(H_Parent()->Position());

    Fvector P{};
    P.set(H_Parent()->Position());

    m_artefacts.feel_touch_update(P, m_fArtefactRadius);
    m_zones.feel_touch_update(P, m_fDetectRadius);
    m_creatures.feel_touch_update(P, m_fDetectRadius);
}

void CCustomDetector::UpdateWork()
{
    UpdateAf();
    UpdateZones();
    UpdateNightVisionMode();
    m_ui->update();
}

void CCustomDetector::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    m_artefacts.clear();
    m_zones.clear();
    m_creatures.clear();
}

void CCustomDetector::Switch(bool turn_on)
{
    if (turn_on && !m_ui)
        CreateUI();
    if (!turn_on)
        DisableUIDetection();

    inherited::Switch(turn_on);

    UpdateNightVisionMode();
}

// void CCustomDetector::UpdateNightVisionMode(bool b_on) {}

void CCustomDetector::TryMakeArtefactVisible(CArtefact* artefact)
{
    if (artefact->H_Parent())
        return;
    if (artefact->CanBeInvisible() && GetHUDmode())
    {
        float dist = Position().distance_to(artefact->Position());
        if (dist < m_fAfVisRadius)
            artefact->SwitchVisibility(true);
    }
}

void CCustomDetector::UpdateNightVisionMode()
{
    auto pActor = smart_cast<CActor*>(H_Parent());
    if (!pActor)
        return;

    auto& actor_cam = pActor->Cameras();
    bool bNightVision = pActor && (actor_cam.GetPPEffector(EEffectorPPType(effNightvision)) || actor_cam.GetPPEffector(EEffectorPPType(effNightvisionScope)));

    bool bOn = bNightVision && pActor == Level().CurrentViewEntity() && IsPowerOn() && GetHUDmode() && // in hud mode only
        m_nightvision_particle.size();

    for (auto& item : m_zones.m_ItemInfos)
    {
        auto pZone = item.first;
        auto& zone_info = item.second;

        if (bOn)
        {
            if (!zone_info.pParticle)
                zone_info.pParticle = CParticlesObject::Create(m_nightvision_particle.c_str(), FALSE);

            zone_info.pParticle->UpdateParent(pZone->XFORM(), Fvector{});
            if (!zone_info.pParticle->IsPlaying())
                zone_info.pParticle->Play();
        }
        else
        {
            if (zone_info.pParticle)
            {
                zone_info.pParticle->Stop();
                CParticlesObject::Destroy(zone_info.pParticle);
            }
        }
    }
}

void CCustomDetector::OnMoveToSlot(EItemPlace prevPlace)
{
    inherited::OnMoveToSlot(prevPlace);
    Switch(true);
}

void CCustomDetector::OnMoveToBelt(EItemPlace prevPlace)
{
    inherited::OnMoveToBelt(prevPlace);
    Switch(true);
}

BOOL CAfList::feel_touch_contact(CObject* O)
{
    auto pAf = smart_cast<CArtefact*>(O);
    if (!pAf)
        return false;

    bool res = (m_TypesMap.find(O->cNameSect()) != m_TypesMap.end()) || (m_TypesMap.find("class_all") != m_TypesMap.end());
    if (pAf->H_Parent() || pAf->GetAfRank() > m_af_rank)
        res = false;

    return res;
}

BOOL CZoneList::feel_touch_contact(CObject* O)
{
    auto pZone = smart_cast<CCustomZone*>(O);
    if (!pZone)
        return false;

    bool res = (m_TypesMap.find(O->cNameSect()) != m_TypesMap.end()) || (m_TypesMap.find("class_all") != m_TypesMap.end());
    if (!pZone->IsEnabled() || !pZone->VisibleByDetector())
        res = false;

    return res;
}

CZoneList::~CZoneList()
{
    clear();
    destroy();
}

BOOL CCreatureList::feel_touch_contact(CObject* O)
{
    auto pCreature = smart_cast<CEntityAlive*>(O);
    if (!pCreature)
        return false;

    const auto species = pSettings->r_string(pCreature->cNameSect(), "species");

    bool res = (m_TypesMap.find(species) != m_TypesMap.end()) || (m_TypesMap.find("class_all") != m_TypesMap.end());
    if (!pCreature->g_Alive() || smart_cast<CActor*>(pCreature))
        res = false;

    return res;
}

CCreatureList::~CCreatureList()
{
    clear();
    destroy();
}