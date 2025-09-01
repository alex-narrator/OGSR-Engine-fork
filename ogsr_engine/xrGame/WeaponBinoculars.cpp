#include "stdafx.h"
#include "WeaponBinoculars.h"

bool CWeaponBinoculars::Action(s32 cmd, u32 flags)
{
    switch (cmd)
    {
    case kWPN_FIRE: return inherited::Action(kWPN_ZOOM, flags);
    }
    return inherited::Action(cmd, flags);
}