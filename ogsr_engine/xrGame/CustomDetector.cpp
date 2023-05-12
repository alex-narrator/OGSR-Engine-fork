#include "StdAfx.h"
#include "CustomDetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "HUDManager.h"
#include "Inventory.h"
#include "Level.h"
#include "map_manager.h"
#include "ActorEffector.h"
#include "Actor.h"
#include "player_hud.h"
#include "Weapon.h"
#include "Grenade.h"
#include "string_table.h"
#include "UIGameSP.h"
#include "CharacterPhysicsSupport.h"

ITEM_INFO::~ITEM_INFO()
{
    if (pParticle)
        CParticlesObject::Destroy(pParticle);
}

bool CCustomDetector::CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate)
{
    if (itm == nullptr)
        return true;

    CInventoryItem& iitm = itm->item();
    bool bres = iitm.IsSingleHanded();

    if (!bres && slot_to_activate)
    {
        *slot_to_activate = NO_ACTIVE_SLOT;
        auto& Inv = smart_cast<CActor*>(H_Parent())->inventory();

        if (Inv.ItemFromSlot(BOLT_SLOT))
            *slot_to_activate = BOLT_SLOT;

        if (*slot_to_activate != NO_ACTIVE_SLOT)
            bres = true;
    }

    if (itm->GetState() != CHUDState::eShowing)
        bres = bres && !itm->IsPending();

    if (bres)
    {
        if (CWeapon* W = smart_cast<CWeapon*>(itm))
            bres = bres && (W->GetState() != CHUDState::eBore) && (W->GetState() != CWeapon::eReload) && (W->GetState() != CWeapon::eSwitch);
    }

    return bres;
}

bool CCustomDetector::CheckCompatibility(CHudItem* itm)
{
    if (!inherited::CheckCompatibility(itm))
        return false;

    if (!CheckCompatibilityInt(itm, nullptr))
    {
        HideDetector(true);
        return false;
    }
    return true;
}

void CCustomDetector::HideDetector(bool bFastMode)
{
    if (GetState() != eHidden || GetState() != eHiding)
        ToggleDetector(bFastMode);
}

void CCustomDetector::ShowDetector(bool bFastMode)
{
    if (GetState() == eHidden || GetState() == eHiding)
        ToggleDetector(bFastMode);
}

void CCustomDetector::ToggleDetector(bool bFastMode)
{
    m_bNeedActivation = false;
    m_bFastAnimMode = bFastMode;

    if (GetState() == eHidden || GetState() == eHiding)
    {
        auto actor = smart_cast<CActor*>(H_Parent());
        auto& inv = actor->inventory();
        PIItem iitem = inv.ActiveItem();
        CHudItem* itm = (iitem) ? iitem->cast_hud_item() : nullptr;
        u16 slot_to_activate = NO_ACTIVE_SLOT;

        if (CheckCompatibilityInt(itm, &slot_to_activate) && !IsBlocked())
        {
            if (slot_to_activate != NO_ACTIVE_SLOT)
            {
                inv.Activate(slot_to_activate);
                m_bNeedActivation = true;

            }
            else
            {
                SwitchState(eShowing);
            }
        }
    }
    else if (GetState() != eHidden || GetState() != eHiding)
        SwitchState(eHiding);
}

void CCustomDetector::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);

    switch (S)
    {
    case eShowing: {
        g_player_hud->attach_item(this);
        PlaySound(sndShow, Position());
        PlayHUDMotion({m_bFastAnimMode ? "anm_show_fast" : "anm_show"}, false, GetState());
        SetPending(TRUE);
        if (!IsPowerOn())
            DisableUIDetection();
    }
    break;
    case eHiding: {
        if (oldState != eHiding)
        {
            PlaySound(sndHide, Position());
            PlayHUDMotion({m_bFastAnimMode ? "anm_hide_fast" : "anm_hide"}, false, GetState());
            SetPending(TRUE);
        }
    }
    break;
    case eIdle: {
        PlayAnimIdle();
        SetPending(FALSE);
    }
    break;
    }
}

void CCustomDetector::OnAnimationEnd(u32 state)
{
    inherited::OnAnimationEnd(state);
    switch (state)
    {
    case eShowing: {
        SwitchState(eIdle);
    }
    break;
    case eHiding: {
        SwitchState(eHidden);
        g_player_hud->detach_item(this);
    }
    break;
    }
}

void CCustomDetector::UpdateXForm() { CInventoryItem::UpdateXForm(); }

void CCustomDetector::OnActiveItem() {}

void CCustomDetector::OnHiddenItem() {}

CCustomDetector::~CCustomDetector()
{
    HUD_SOUND::DestroySound(sndShow);
    HUD_SOUND::DestroySound(sndHide);
    HUD_SOUND::DestroySound(sndSwitch);

    m_artefacts.destroy();
    m_zones.destroy();
    m_creatures.destroy();

    Switch(false);
    xr_delete(m_ui);
}

BOOL CCustomDetector::net_Spawn(CSE_Abstract* DC)
{
    Switch(false);
    return inherited::net_Spawn(DC);
}

void CCustomDetector::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(m_bAfMode, output_packet);
    save_data(GetState() == eIdle, output_packet);
}

void CCustomDetector::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_bAfMode, input_packet);
    load_data(m_bNeedActivation, input_packet);
}

void CCustomDetector::Load(LPCSTR section)
{
    m_animation_slot = 7;
    inherited::Load(section);

    m_fDetectRadius = READ_IF_EXISTS(pSettings, r_float, section, "detect_radius", 15.0f);
    m_fAfVisRadius = READ_IF_EXISTS(pSettings, r_float, section, "af_vis_radius", 2.0f);
    m_artefacts.load(section, "af");
    m_artefacts.m_af_rank = READ_IF_EXISTS(pSettings, r_u32, section, "af_rank", 0);
    m_zones.load(section, "zone");
    m_creatures.load(section, "creature");

    m_bCanSwitchModes = m_artefacts.not_empty() && m_zones.not_empty();

    m_nightvision_particle = READ_IF_EXISTS(pSettings, r_string, section, "night_vision_particle", nullptr);
    m_fZoomRotateTime = READ_IF_EXISTS(pSettings, r_float, hud_sect, "zoom_rotate_time", 0.25f);

    HUD_SOUND::LoadSound(section, "snd_draw", sndShow, SOUND_TYPE_ITEM_TAKING);
    HUD_SOUND::LoadSound(section, "snd_holster", sndHide, SOUND_TYPE_ITEM_HIDING);
    if (pSettings->line_exist(section, "snd_switch"))
        HUD_SOUND::LoadSound(section, "snd_switch", sndSwitch);
}

void CCustomDetector::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    if (!IsPowerOn())
        return;

    Position().set(H_Parent()->Position());

    Fvector P{};
    P.set(H_Parent()->Position());

    m_artefacts.feel_touch_update(P, m_fDetectRadius);
    m_zones.feel_touch_update(P, m_fDetectRadius);
    m_creatures.feel_touch_update(P, m_fDetectRadius);
}

bool CCustomDetector::IsPowerOn() const { return m_bWorking && H_Parent() && H_Parent() == Level().CurrentViewEntity(); }

void CCustomDetector::UpdateWork()
{
    UpdateAf();
    UpdateZones();
    UpdateNightVisionMode();
    m_ui->update();
}

void CCustomDetector::UpdateVisibility()
{
    // check visibility
    attachable_hud_item* i0 = g_player_hud->attached_item(0);
    if (GetState() == eIdle || GetState() == eShowing)
    {
        bool bClimb = ((Actor()->MovingState() & mcClimb) != 0);
        if (bClimb || IsBlocked())
        {
            HideDetector(true);
            m_bNeedActivation = true;
        }
        else if (i0 && HudItemData())
        {
            if (i0->m_parent_hud_item)
            {
                u32 state = i0->m_parent_hud_item->GetState();
                if (state == eReload || state == eSwitch || smart_cast<CGrenade*>(i0->m_parent_hud_item) && state == eThrowStart)
                {
                    HideDetector(true);
                    m_bNeedActivation = true;
                }
            }
        }
    }
    else if (m_bNeedActivation)
    {
        attachable_hud_item* i0 = g_player_hud->attached_item(0);
        bool bClimb = ((Actor()->MovingState() & mcClimb) != 0);
        if (!bClimb)
        {
            CHudItem* huditem = (i0) ? i0->m_parent_hud_item : nullptr;
            bool bChecked = !huditem || CheckCompatibilityInt(huditem, 0);

            if (bChecked && !IsBlocked())
                ShowDetector(true);
        }
    }
}

void CCustomDetector::UpdateCL()
{
    inherited::UpdateCL();

    if (H_Parent() != Level().CurrentEntity())
        return;

    UpdateVisibility();
    if (!IsPowerOn())
    {
        return;
    }
    UpdateWork();
}

void CCustomDetector::OnH_A_Chield() { inherited::OnH_A_Chield(); }

void CCustomDetector::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    m_artefacts.clear();
    m_zones.clear();
    m_creatures.clear();

    if (GetState() != eHidden)
    {
        // Detaching hud item and animation stop in OnH_A_Independent
        Switch(false);
        SwitchState(eHidden);
    }
}

void CCustomDetector::OnMoveToRuck(EItemPlace prevPlace)
{
    inherited::OnMoveToRuck(prevPlace);
    if (prevPlace == eItemPlaceSlot)
    {
        SwitchState(eHidden);
        g_player_hud->detach_item(this);
    }
    Switch(false);
    StopCurrentAnimWithoutCallback();
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

void CCustomDetector::OnMoveToVest(EItemPlace prevPlace)
{
    inherited::OnMoveToVest(prevPlace);
    Switch(true);
}

void CCustomDetector::Switch(bool turn_on)
{
    if (turn_on && !m_ui)
        CreateUI();
    if (!turn_on)
        DisableUIDetection();

    if (turn_on && fis_zero(GetPowerLevel()))
        return;
    inherited::Switch(turn_on);

    m_bWorking = turn_on;

    UpdateNightVisionMode();
}

// void CCustomDetector::UpdateNightVisionMode(bool b_on) {}

Fvector CCustomDetector::GetPositionForCollision()
{
    Fvector det_pos{}, det_dir{};
    // Офсет подобрал через худ аждаст, это скорее всего временно, но такое решение подходит всем детекторам более-менее.
    GetBoneOffsetPosDir("wpn_body", det_pos, det_dir, Fvector{-0.247499f, -0.810510f, 0.178999f});
    return det_pos;
}

Fvector CCustomDetector::GetDirectionForCollision()
{
    // Пока и так нормально, в будущем мб придумаю решение получше.
    return Device.vCameraDirection;
}

void CCustomDetector::TryMakeArtefactVisible(CArtefact* artefact)
{
    if (artefact->CanBeInvisible() && GetHUDmode() && (!CanSwitchModes() || IsAfMode()))
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

bool CCustomDetector::IsZoomed() const
{
    attachable_hud_item* i0 = g_player_hud->attached_item(0);
    if (i0 && HudItemData())
    {
        return i0->m_parent_hud_item->IsZoomed();
    }
    return false;
}

bool CCustomDetector::IsAiming() const
{
    attachable_hud_item* i0 = g_player_hud->attached_item(0);
    if (i0 && HudItemData())
    {
        auto wpn = smart_cast<CWeapon*>(i0->m_parent_hud_item);
        return wpn && wpn->IsAiming();
    }
    return false;
}

void CCustomDetector::SwitchMode() 
{
    if (!CanSwitchModes())
        return;

    m_bAfMode = !m_bAfMode;

    AnimationExist("anm_switch_mode") ? PlayHUDMotion("anm_switch_mode", true, GetState()) : PlayHUDMotion({"anm_show_fast"}, true, GetState(), false);
    PlaySound(sndSwitch, Position());
    ShowCurrentModeMsg();
}

void CCustomDetector::ShowCurrentModeMsg()
{
    string1024 str;
    if (CanSwitchModes())
        sprintf(str, "%s: %s [%.0f%s]", NameShort(), CStringTable().translate(m_bAfMode ? "st_af_mode" : "st_zone_mode").c_str(), GetPowerLevelToShow(), "%");
    else
        sprintf(str, "%s: [%.0f%s]", NameShort(), GetPowerLevelToShow(), "%");
    HUD().GetUI()->AddInfoMessage("item_usage", str, false);
}

bool CCustomDetector::IsBlocked()
{ 
    auto actor = smart_cast<CActor*>(H_Parent());
    if (!actor)
        return false;
    if (actor->character_physics_support()->movement()->PHCapture())
        return true;
    if (auto pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame()))
        return pGameSP->IsDialogsShown();
    return false;
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
    if (!res && pCreature->g_Alive() && !smart_cast<CActor*>(pCreature))
        Msg("~%s %s", __FUNCTION__, species);

    return res;
}

CCreatureList::~CCreatureList()
{
    clear();
    destroy();
}