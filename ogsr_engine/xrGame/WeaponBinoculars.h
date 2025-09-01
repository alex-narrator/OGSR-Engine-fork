#pragma once

#include "WeaponCustomPistol.h"
#include "script_export_space.h"

class CWeaponBinoculars : public CWeaponCustomPistol
{
private:
    typedef CWeaponCustomPistol inherited;

public:
    CWeaponBinoculars() : CWeaponCustomPistol("BINOCULARS") {};
    virtual ~CWeaponBinoculars() {};

    virtual bool Action(s32 cmd, u32 flags);

    virtual bool use_crosshair() const { return false; }
    virtual bool CanBeReloaded() { return false; };

protected:
    virtual size_t GetWeaponTypeForCollision() const override { return Binocular; }
    virtual Fvector GetPositionForCollision() override { return Device.vCameraPosition; }
    virtual Fvector GetDirectionForCollision() override { return Device.vCameraDirection; }

    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponBinoculars)
#undef script_type_list
#define script_type_list save_type_list(CWeaponBinoculars)