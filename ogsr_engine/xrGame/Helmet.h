#pragma once
#include "inventory_item_object.h"
struct SBoneProtections;
class CHelmet : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CHelmet();
    virtual ~CHelmet();

    virtual void Load(LPCSTR section);

    float HitThruArmour(SHit* pHDS);

    virtual float GetHitTypeProtection(int) const override;

    virtual bool can_be_attached() const override;

    virtual void OnMoveToSlot(EItemPlace prevPlace);

private:
    SBoneProtections* m_boneProtection;

protected:
    shared_str bulletproof_display_bone{};
};
