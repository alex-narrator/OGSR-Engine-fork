#include "stdafx.h"
#include "actor.h"
#include "weapon.h"
#include "mercuryball.h"
#include "inventory.h"
#include "hudmanager.h"
#include "UsableScriptObject.h"
#include "customzone.h"
#include "../xr_3da/GameMtlLib.h"
#include "ui/UIMainIngameWnd.h"
#include "Grenade.h"
#include "clsid_game.h"

#include "game_cl_base.h"
#include "Level.h"

void CActor::feel_touch_new(CObject* O) {}

void CActor::feel_touch_delete(CObject* O)
{
    CPhysicsShellHolder* sh = smart_cast<CPhysicsShellHolder*>(O);
    if (sh && sh->character_physics_support())
        m_feel_touch_characters--;
}

BOOL CActor::feel_touch_contact(CObject* O)
{
    CInventoryItem* item = smart_cast<CInventoryItem*>(O);
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(O);

    if (item && item->Useful() && !item->object().H_Parent())
        return TRUE;

    if (inventory_owner && inventory_owner != smart_cast<CInventoryOwner*>(this))
    {
        CPhysicsShellHolder* sh = smart_cast<CPhysicsShellHolder*>(O);
        if (sh && sh->character_physics_support())
            m_feel_touch_characters++;
        return TRUE;
    }

    return (FALSE);
}

BOOL CActor::feel_touch_on_contact(CObject* O)
{
    CCustomZone* custom_zone = smart_cast<CCustomZone*>(O);
    if (!custom_zone)
        return (TRUE);

    Fsphere sphere;
    sphere.P = Position();
    sphere.R = EPS_L;
    if (custom_zone->inside(sphere))
        return (TRUE);

    return (FALSE);
}

void CActor::feel_sound_new(CObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position, float power)
{
    if (who == this)
        m_snd_noise = _max(m_snd_noise, power);
}
