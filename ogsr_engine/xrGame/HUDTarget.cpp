// exxZERO Time Stamp AddIn. Document modified at : Thursday, March 07, 2002 14:13:00 , by user : Oles , from computer : OLES
// HUDCursor.cpp: implementation of the CHUDTarget class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "hudtarget.h"
#include "hudmanager.h"
#include "../xr_3da/GameMtlLib.h"

#include "..\xr_3da\Environment.h"
#include "..\xr_3da\CustomHUD.h"
#include "Actor.h"
#include "level.h"
#include "game_cl_base.h"
#include "..\xr_3da\IGame_Persistent.h"

#include "InventoryOwner.h"
#include "relation_registry.h"
#include "character_info.h"

#include "string_table.h"
#include "entity_alive.h"

#include "inventory_item.h"
#include "inventory.h"
#include "monster_community.h"
#include "HudItem.h"
#include "Weapon.h"
#include "PDA.h"

constexpr float NEAR_LIM = 0.5f;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHUDTarget::CHUDTarget()
{
    RQ.range = 0.f;
    m_real_dist = 0.f;

    RQ.set(NULL, 0.f, -1);
}

void CHUDTarget::net_Relcase(CObject* O)
{
    if (RQ.O == O)
        RQ.O = NULL;

    RQR.r_clear();
}

float CHUDTarget::GetDist() { return RQ.range; }

CObject* CHUDTarget::GetObj() { return RQ.O; }

ICF static BOOL pick_trace_callback(collide::rq_result& result, LPVOID params)
{
    collide::rq_result* RQ = (collide::rq_result*)params;
    if (result.O)
    {
        *RQ = result;
        return FALSE;
    }
    else
    {
        //получить треугольник и узнать его материал
        CDB::TRI* T = Level().ObjectSpace.GetStaticTris() + result.element;
        const auto* mtl = GMLib.GetMaterialByIdx(T->material);
        if (mtl->Flags.is(SGameMtl::flPassable))
            return TRUE;
        // возможно это сетка-рабица и через нее можно брать предметы
        else if (fsimilar(mtl->fVisTransparencyFactor, 1.0f, EPS) && fsimilar(mtl->fShootFactor, 1.0f, EPS) && mtl->Flags.is(SGameMtl::flSuppressWallmarks))
            return TRUE;
    }
    *RQ = result;
    return FALSE;
}

void CHUDTarget::CursorOnFrame()
{
    CActor* Actor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!Actor)
        return;

    Fvector p1 = Device.vCameraPosition;
    Fvector dir = Device.vCameraDirection;

    // Render cursor
    RQ.O = nullptr;
    RQ.range = g_pGamePersistent->Environment().CurrentEnv->far_plane * 0.99f;
    RQ.element = -1;
    m_real_dist = -1.f;

    collide::ray_defs RD(p1, dir, RQ.range, CDB::OPT_CULL, collide::rqtBoth);
    RQR.r_clear();
    VERIFY(!fis_zero(RD.dir.square_magnitude()));
    if (Level().ObjectSpace.RayQuery(RQR, RD, pick_trace_callback, &RQ, nullptr, Level().CurrentEntity()))
    {
        m_real_dist = RQ.range;
        clamp(RQ.range, NEAR_LIM, RQ.range);
    }
}

extern ENGINE_API BOOL g_bRendering;

void CHUDTarget::Render()
{
    VERIFY(g_bRendering);

    Fvector2 crosshair_ui_pos{UI_BASE_WIDTH * 0.5, UI_BASE_HEIGHT * 0.5};

    crosshair_pos.set(crosshair_ui_pos);

    CActor* Actor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!Actor)
        return;

    CInventoryItem* active_item = Actor->inventory().ActiveItem();

    Fvector p1 = Device.vCameraPosition;
    Fvector dir = Device.vCameraDirection;

    if (const auto Wpn = smart_cast<CHudItem*>(active_item))
        Actor->g_fireParams(Wpn, p1, dir, true);

    Fvector p2{};
    p2.mad(p1, dir, RQ.range);

    Fvector4 pt;
    Device.mFullTransform.transform(pt, p2);
    pt.y = -pt.y;

    // Convert to UI coords
    crosshair_ui_pos.x *= (pt.x + 1);
    crosshair_ui_pos.y *= (pt.y + 1);
    crosshair_pos.set(crosshair_ui_pos);
}

float CHUDTarget::GetRealDist() { return m_real_dist; }
