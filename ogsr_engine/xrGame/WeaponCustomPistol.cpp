#include "stdafx.h"

#include "Entity.h"
#include "WeaponCustomPistol.h"
#include "game_object_space.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CWeaponCustomPistol::CWeaponCustomPistol(LPCSTR name) : CWeaponMagazined(name, SOUND_TYPE_WEAPON_PISTOL) {}

void CWeaponCustomPistol::switch2_Fire()
{
    if (GetCurrentFireMode() == 1)
        SetPending(TRUE);
    inherited::switch2_Fire();
}