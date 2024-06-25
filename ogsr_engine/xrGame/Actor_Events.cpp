#include "stdafx.h"
#include "actor.h"
#include "customdetector.h"
#include "uigamesp.h"
#include "hudmanager.h"
#include "weapon.h"
#include "artifact.h"
#include "inventory.h"
#include "level.h"
#include "xr_level_controller.h"
#include "FoodItem.h"
#include "ActorCondition.h"
#include "Grenade.h"

#include "CameraLook.h"
#include "CameraFirstEye.h"
#include "holder_custom.h"
#include "game_base_space.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif

#pragma todo("KRodin: заменить на прямые вызовы вместо этих всратых ивентов!")

void CActor::OnEvent(NET_Packet& P, u16 type)
{
    inherited::OnEvent(P, type);
    CInventoryOwner::OnEvent(P, type);

    u16 id;
    switch (type)
    {
    case GE_TRADE_BUY:
    case GE_OWNERSHIP_TAKE:
    case GE_TRANSFER_TAKE: {
        P.r_u16(id);
        CObject* O = Level().Objects.net_Find(id);
        if (!O)
        {
            Msg("! Error: No object to take/buy [%d]", id);
            break;
        }

        CGameObject* _GO = smart_cast<CGameObject*>(O);

        if (inventory().CanTakeItem(smart_cast<CInventoryItem*>(_GO)))
        {
            O->H_SetParent(smart_cast<CObject*>(this));

            inventory().Take(_GO, false, true);
        }
        else
        {
            NET_Packet _P;
            u_EventGen(_P, GE_OWNERSHIP_REJECT, ID());
            _P.w_u16(u16(O->ID()));
            u_EventSend(_P);
        }
    }
    break;
    case GE_TRADE_SELL:
    case GE_OWNERSHIP_REJECT:
    case GE_TRANSFER_REJECT: {
        P.r_u16(id);
        CObject* O = Level().Objects.net_Find(id);

        if (!O)
        {
            Msg("! [%s] Error: No object to reject/sell [%u]", __FUNCTION__, id);
            break;
        }

        bool just_before_destroy = !P.r_eof() && P.r_u8();
        bool dont_create_shell = (type == GE_TRADE_SELL) || (type == GE_TRANSFER_REJECT) || just_before_destroy;

        O->SetTmpPreDestroy(just_before_destroy);
        if (inventory().DropItem(smart_cast<CGameObject*>(O)) && !O->getDestroy())
        {
            O->H_SetParent(0, dont_create_shell);
            //.				feel_touch_deny(O,2000);
            Level().m_feel_deny.feel_touch_deny(O, 1000);
        }
    }
    break;
    }
}

void CActor::MoveActor(Fvector NewPos, Fvector NewDir)
{
    Fmatrix M = XFORM();
    M.translate(NewPos);
    r_model_yaw = NewDir.y;
    r_torso.yaw = NewDir.y;
    r_torso.pitch = -NewDir.x;
    unaffected_r_torso.yaw = r_torso.yaw;
    unaffected_r_torso.pitch = r_torso.pitch;
    unaffected_r_torso.roll = 0; // r_torso.roll;

    r_torso_tgt_roll = 0;
    cam_Active()->Set(-unaffected_r_torso.yaw, unaffected_r_torso.pitch, unaffected_r_torso.roll);
    ForceTransform(M);
}