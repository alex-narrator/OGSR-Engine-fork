#include "stdafx.h"

#include "customoutfit.h"
#include "PhysicsShell.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Level.h"
#include "..\Include/xrRender/Kinematics.h"
#include "../Include/xrRender/RenderVisual.h"
#include "UIGameSP.h"
#include "HudManager.h"
#include "ui/UIInventoryWnd.h"
#include "player_hud.h"
#include "xrserver_objects_alife_items.h"

void CCustomOutfit::Load(LPCSTR section)
{
    inherited::Load(section);

    m_ActorVisual = READ_IF_EXISTS(pSettings, r_string, section, "actor_visual", nullptr);

    m_ef_equipment_type = pSettings->r_u32(section, "ef_equipment_type");

    m_full_icon_name = pSettings->r_string(section, "full_icon_name");

    m_bIsHelmetBuiltIn = std::find(m_slots_locked.begin(), m_slots_locked.end(), HELMET_SLOT) != m_slots_locked.end();

    m_iBeltSize = READ_IF_EXISTS(pSettings, r_u32, section, "belt_size", 0);
}

float CCustomOutfit::HitThruArmour(SHit* pHDS)
{
    auto actor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
    if (!actor || actor->IsHitToHead(pHDS) && !m_bIsHelmetBuiltIn)
        return pHDS->damage();
    return inherited::HitThruArmour(pHDS);
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
                pActor->ChangeVisual(m_ActorVisual);
            if (pSettings->line_exist(cNameSect(), "player_hud_section"))
                g_player_hud->load(pSettings->r_string(cNameSect(), "player_hud_section"));
            else
                g_player_hud->load_default();
            m_pCurrentInventory->DropBeltToRuck();
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
            m_pCurrentInventory->DropBeltToRuck();
        }
    }
}

u32 CCustomOutfit::ef_equipment_type() const { return (m_ef_equipment_type); }