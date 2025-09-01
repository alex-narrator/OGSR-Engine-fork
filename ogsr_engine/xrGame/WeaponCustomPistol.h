#pragma once

#include "WeaponMagazined.h"

class CWeaponCustomPistol : public CWeaponMagazined
{
private:
    typedef CWeaponMagazined inherited;

public:
    CWeaponCustomPistol(LPCSTR name);
    virtual ~CWeaponCustomPistol();

protected:
    virtual void FireEnd();
    virtual void switch2_Fire();
};