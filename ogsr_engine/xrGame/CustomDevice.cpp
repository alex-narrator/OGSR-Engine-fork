#include "stdafx.h"
#include "CustomDevice.h"
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

void CCustomDevice::Load(LPCSTR section)
{
    m_animation_slot = 7;
    inherited::Load(section);

    /*m_fZoomRotateTime = READ_IF_EXISTS(pSettings, r_float, hud_sect, "zoom_rotate_time", 0.25f);*/

    m_sounds.LoadSound(section, "snd_draw", "sndShow");
    m_sounds.LoadSound(section, "snd_holster", "sndHide");
    m_sounds.LoadSound(section, "snd_switch", "sndSwitch");

    m_bWorkIndependent = READ_IF_EXISTS(pSettings, r_bool, section, "work_independent", false);
}

BOOL CCustomDevice::net_Spawn(CSE_Abstract* DC)
{
    Switch(false);
    return inherited::net_Spawn(DC);
}

void CCustomDevice::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(GetState() == eIdle, output_packet);
}

void CCustomDevice::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_bNeedActivation, input_packet);
}

bool CCustomDevice::IsPowerOn() const { return m_bWorking && (H_Parent() && H_Parent() == Level().CurrentViewEntity() || m_bWorkIndependent); }

void CCustomDevice::Switch(bool turn_on)
{
    inherited::Switch(turn_on);
    m_bWorking = turn_on;
}

void CCustomDevice::UpdateCL()
{
    inherited::UpdateCL();

    if (H_Parent() != Level().CurrentEntity() && !m_bWorkIndependent)
        return;

    UpdateVisibility();
    if (!IsPowerOn())
        return;
    UpdateWork();
}

bool CCustomDevice::CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate)
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
            bres =
                bres /*&& (W->GetState() != CHUDState::eBore)*/ && (W->GetState() != CWeapon::eReload) && (W->GetState() != CWeapon::eSwitch) && (W->GetState() != CWeapon::eShutter);
    }

    return bres;
}

bool CCustomDevice::CheckCompatibility(CHudItem* itm)
{
    if (!inherited::CheckCompatibility(itm))
        return false;

    if (!CheckCompatibilityInt(itm, nullptr))
    {
        HideDevice(true);
        return false;
    }
    return true;
}

void CCustomDevice::HideDevice(bool bFastMode)
{
    if (GetState() != eHidden || GetState() != eHiding)
        ToggleDevice(bFastMode);
}

void CCustomDevice::ShowDevice(bool bFastMode)
{
    if (GetState() == eHidden || GetState() == eHiding)
        ToggleDevice(bFastMode);
}

void CCustomDevice::ToggleDevice(bool bFastMode)
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

void CCustomDevice::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);

    switch (S)
    {
    case eShowing: {
        g_player_hud->attach_item(this);
        PlaySound("sndShow", Position());
        IsZoomed() && AnimationExist("anm_zoom_show") ? PlayHUDMotion("anm_zoom_show", false, GetState()) :
                                                        PlayHUDMotion({m_bFastAnimMode ? "anm_show_fast" : "anm_show"}, false, GetState());
        SetPending(TRUE);
    }
    break;
    case eHiding: {
        if (oldState != eHiding)
        {
            PlaySound("sndHide", Position());
            IsZoomed() && AnimationExist(m_bFastAnimMode ? "anm_zoom_hide_fast" : "anm_zoom_hide") ?
                PlayHUDMotion({m_bFastAnimMode ? "anm_zoom_hide_fast" : "anm_zoom_hide"}, false, GetState()) :
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
    case eThrowStart: {
        if (AnimationExist("anm_throw_start"))
        {
            PlayHUDMotion("anm_throw_start", true, GetState());
            SetPending(TRUE);
        }
        else
            SwitchState(eThrow);
    }
    break;
    case eThrow: {
        PlayHUDMotion("anm_throw", true, GetState());
        SetPending(FALSE);
    }
    break;
    case eThrowEnd: {
        if (AnimationExist("anm_throw_end"))
        {
            PlayHUDMotion("anm_throw_end", true, GetState());
            SetPending(TRUE);
        }
        else
            SwitchState(eIdle);
    }
    break;
    case eIdleZoom: {
        if (AnimationExist("anm_idle_zoom"))
            PlayHUDMotion("anm_idle_zoom", true, GetState());
        else
            SwitchState(eIdle);
    }
    break;
    case eIdleZoomIn: {
        if (AnimationExist("anm_zoom_in"))
            PlayHUDMotion("anm_zoom_in", true, GetState());
        else
            SwitchState(eIdleZoom);
    }
    break;
    case eIdleZoomOut: {
        if (AnimationExist("anm_zoom_out"))
            PlayHUDMotion("anm_zoom_out", true, GetState());
        else
            SwitchState(eIdle);
    }
    break;
    }
}

void CCustomDevice::OnAnimationEnd(u32 state)
{
    inherited::OnAnimationEnd(state);
    switch (state)
    {
    case eShowing: {
        if (IsZoomed() && AnimationExist("anm_idle_zoom"))
            SwitchState(eIdleZoom);
        else
            SwitchState(eIdle);
    }
    break;
    case eHiding: {
        SwitchState(eHidden);
        g_player_hud->detach_item(this);
    }
    break;
    case eThrowStart: {
        SwitchState(eThrow);
    }
    break;
    case eThrowEnd: {
        SwitchState(eIdle);
    }
    break;
    case eIdleZoomIn: SwitchState(eIdleZoom); break;
    case eIdleZoomOut: SwitchState(eIdle); break;
    }
}

void CCustomDevice::UpdateVisibility()
{
    if (!H_Parent() || !smart_cast<CActor*>(H_Parent()))
        return;
    // check visibility
    bool bClimb = ((Actor()->MovingState() & mcClimb) != 0);
    attachable_hud_item* i0 = g_player_hud->attached_item(0);
    if (GetState() == eIdle || GetState() == eIdleZoom || GetState() == eShowing || GetState() == eThrow || GetState() == eThrowStart)
    {
        if (bClimb || IsBlocked())
        {
            HideDevice(true);
            m_bNeedActivation = true;
        }
        else if (i0 && HudItemData())
        {
            if (i0->m_parent_hud_item)
            {
                u32 state = i0->m_parent_hud_item->GetState();
                if (smart_cast<CMissile*>(i0->m_parent_hud_item) && AnimationExist("anm_throw"))
                {
                    if ((state == eThrowStart || state == eReady) && GetState() == eIdle)
                        SwitchState(eThrowStart);
                    else if (state == eThrow && GetState() == eThrow)
                        SwitchState(eThrowEnd);
                    else if (state == eHiding && (GetState() == eThrowStart || GetState() == eThrow))
                        SwitchState(eIdle);
                }
                if (auto wpn = smart_cast<CWeapon*>(i0->m_parent_hud_item) && AnimationExist("anm_idle_zoom"))
                {
                    if (IsZoomed() && GetState() != eIdleZoom)
                        SwitchState(eIdleZoomIn);
                    else if (!IsZoomed() && GetState() == eIdleZoom)
                        SwitchState(eIdleZoomOut);
                }
                if (state == eReload || state == eSwitch || state == eShutter)
                {
                    if (GetState() == eThrowStart || GetState() == eThrow)
                        SwitchState(eIdle);

                    HideDevice(true);
                    m_bNeedActivation = true;
                }
            }
        }
    }
    else if (m_bNeedActivation)
    {
        if (!bClimb)
        {
            CHudItem* huditem = (i0) ? i0->m_parent_hud_item : nullptr;
            bool bChecked = !huditem || CheckCompatibilityInt(huditem, 0);

            if (bChecked && !IsBlocked())
                ShowDevice(true);
        }
    }
}

void CCustomDevice::UpdateXForm() { CInventoryItem::UpdateXForm(); }

void CCustomDevice::OnActiveItem() {}

void CCustomDevice::OnHiddenItem() {}

void CCustomDevice::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    if (GetState() != eHidden)
    {
        // Detaching hud item and animation stop in OnH_A_Independent
        if (!m_bWorkIndependent)
            Switch(false);
        SwitchState(eHidden);
    }
}

void CCustomDevice::OnMoveToRuck(EItemPlace prevPlace)
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

void CCustomDevice::OnMoveToBelt(EItemPlace prevPlace)
{
    inherited::OnMoveToBelt(prevPlace);
    if (prevPlace == eItemPlaceSlot)
    {
        SwitchState(eHidden);
        g_player_hud->detach_item(this);
    }
    // Switch(false);
    StopCurrentAnimWithoutCallback();
}

Fvector CCustomDevice::GetPositionForCollision()
{
    Fvector det_pos{}, det_dir{};
    // Офсет подобрал через худ аждаст, это скорее всего временно, но такое решение подходит всем детекторам более-менее.
    GetBoneOffsetPosDir("wpn_body", det_pos, det_dir, Fvector{-0.247499f, -0.810510f, 0.178999f});
    return det_pos;
}

Fvector CCustomDevice::GetDirectionForCollision()
{
    // Пока и так нормально, в будущем мб придумаю решение получше.
    return Device.vCameraDirection;
}

bool CCustomDevice::IsZoomed() const
{
    attachable_hud_item* i0 = g_player_hud->attached_item(0);
    if (i0 && HudItemData())
    {
        return i0->m_parent_hud_item->IsZoomed();
    }
    return false;
}

bool CCustomDevice::IsAiming() const
{
    attachable_hud_item* i0 = g_player_hud->attached_item(0);
    if (i0 && HudItemData())
    {
        auto wpn = smart_cast<CWeapon*>(i0->m_parent_hud_item);
        return wpn && wpn->IsAiming();
    }
    return false;
}

bool CCustomDevice::IsBlocked()
{
    auto actor = smart_cast<CActor*>(H_Parent());
    if (!actor)
        return false;
    if (actor->inventory().m_bBlockDevice || !m_pCurrentInventory->InSlot(this))
        return true;
    return false;
}