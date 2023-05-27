#include "stdafx.h"

#include "customoutfit.h"
#include "PhysicsShell.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Level.h"
#include "BoneProtections.h"
#include "..\Include/xrRender/Kinematics.h"
#include "../Include/xrRender/RenderVisual.h"
#include "UIGameSP.h"
#include "HudManager.h"
#include "ui/UIInventoryWnd.h"
#include "player_hud.h"
#include "xrserver_objects_alife_items.h"

CCustomOutfit::CCustomOutfit()
{
    SetSlot(OUTFIT_SLOT);
    m_boneProtection = xr_new<SBoneProtections>();
}

CCustomOutfit::~CCustomOutfit()
{
    xr_delete(m_boneProtection);
    xr_delete(m_UIVisor);
}

void CCustomOutfit::Load(LPCSTR section)
{
    inherited::Load(section);

    m_ActorVisual = READ_IF_EXISTS(pSettings, r_string, section, "actor_visual", nullptr);

    m_ef_equipment_type = pSettings->r_u32(section, "ef_equipment_type");

    m_fPowerLoss = READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.f);

    m_full_icon_name = pSettings->r_string(section, "full_icon_name");

    m_bIsHelmetBuiltIn = READ_IF_EXISTS(pSettings, r_bool, section, "helmet_built_in", false);

    m_b_has_visor = READ_IF_EXISTS(pSettings, r_bool, section, "has_visor", true);
    m_VisorTexture = READ_IF_EXISTS(pSettings, r_string, section, "visor_texture", nullptr);

    bulletproof_display_bone = READ_IF_EXISTS(pSettings, r_string, section, "bulletproof_display_bone", "bip01_spine");
}

float CCustomOutfit::HitThruArmour(SHit* pHDS)
{
    float hit_power = pHDS->power;
    float BoneArmour = m_boneProtection->getBoneArmour(pHDS->boneID) * !fis_zero(GetCondition());

    Msg("%s %s take hit power [%.4f], hitted bone %s, bone armor [%.4f], hit AP [%.4f]", __FUNCTION__, Name(), hit_power,
        smart_cast<IKinematics*>(smart_cast<CActor*>(m_pCurrentInventory->GetOwner())->Visual())->LL_BoneName_dbg(pHDS->boneID), BoneArmour, pHDS->ap);

    if (pHDS->ap < BoneArmour)
    { // броню не пробито, хіт тільки від умовного удару в броню
        hit_power *= m_boneProtection->m_fHitFrac;

        Msg("%s %s armor is not pierced, result hit power [%.4f]", __FUNCTION__, Name(), hit_power);
    }

    return hit_power;
};

float CCustomOutfit::GetHitTypeProtection(int hit_type) const { return (hit_type == ALife::eHitTypeFireWound) ? 0.f : inherited::GetHitTypeProtection(hit_type); }

void CCustomOutfit::OnMoveToSlot(EItemPlace prevPlace)
{
    inherited::OnMoveToSlot(prevPlace);

    if (m_pCurrentInventory)
    {
        CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
        if (pActor)
        {
            if (m_ActorVisual.size())
            {
                pActor->ChangeVisual(m_ActorVisual);
            }
            if (pSettings->line_exist(cNameSect(), "bones_koeff_protection"))
            {
                m_boneProtection->reload(pSettings->r_string(cNameSect(), "bones_koeff_protection"), smart_cast<IKinematics*>(pActor->Visual()));
            }

            if (pSettings->line_exist(cNameSect(), "player_hud_section"))
                g_player_hud->load(pSettings->r_string(cNameSect(), "player_hud_section"));
            else
                g_player_hud->load_default();

            if (m_bIsHelmetBuiltIn)
                m_pCurrentInventory->DropSlotsToRuck(HELMET_SLOT);

            if (m_UIVisor)
                xr_delete(m_UIVisor);
            if (!!m_VisorTexture && m_bIsHelmetBuiltIn)
            {
                m_UIVisor = xr_new<CUIStaticItem>();
                m_UIVisor->Init(m_VisorTexture.c_str(), Core.Features.test(xrCore::Feature::scope_textures_autoresize) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
            }
        }
    }
}

void CCustomOutfit::OnMoveToRuck(EItemPlace prevPlace)
{
    inherited::OnMoveToRuck(prevPlace);

    if (m_pCurrentInventory && !Level().is_removing_objects())
    {
        CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
        if (pActor && prevPlace == eItemPlaceSlot)
        {
            if (m_ActorVisual.size())
            {
                shared_str DefVisual = pActor->GetDefaultVisualOutfit();
                if (DefVisual.size())
                {
                    pActor->ChangeVisual(DefVisual);
                }
            }

            g_player_hud->load_default();

            if (m_UIVisor)
                xr_delete(m_UIVisor);
        }
    }
}

u32 CCustomOutfit::ef_equipment_type() const { return (m_ef_equipment_type); }

float CCustomOutfit::GetPowerLoss()
{
    if (m_fPowerLoss < 1 && GetCondition() <= 0)
    {
        return 1.0f;
    };
    return m_fPowerLoss;
};

void CCustomOutfit::DrawHUDMask()
{
    if (m_UIVisor)
    {
        m_UIVisor->SetPos(0, 0);
        m_UIVisor->SetRect(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
        m_UIVisor->Render();
    }
}