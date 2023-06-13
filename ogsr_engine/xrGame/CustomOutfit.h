#pragma once

#include "inventory_item_object.h"

struct SBoneProtections;

class CCustomOutfit : public CInventoryItemObject
{
    friend class COutfitScript;

private:
    typedef CInventoryItemObject inherited;

public:
    CCustomOutfit(void);
    virtual ~CCustomOutfit(void);

    virtual void Load(LPCSTR section);

    float HitThruArmour(SHit* pHDS);

    virtual float GetHitTypeProtection(int) const override;

    virtual void OnMoveToSlot(EItemPlace prevPlace) override;
    virtual void OnMoveToRuck(EItemPlace prevPlace) override;

    virtual float GetExoFactor() const { return 1.f; };

private:
    shared_str m_ActorVisual;
    shared_str m_full_icon_name;
    SBoneProtections* m_boneProtection;

    u32 m_ef_equipment_type{};

public:
    virtual u32 ef_equipment_type() const;
    const shared_str& GetFullIconName() const { return m_full_icon_name; };

    bool m_bIsHelmetBuiltIn{};

protected:
    shared_str bulletproof_display_bone{};
};