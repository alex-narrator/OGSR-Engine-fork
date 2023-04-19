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
    // коэффициент на который домножается потеря силы
    // если на персонаже надет костюм
    virtual float GetPowerLoss();

    virtual float GetHitTypeProtection(int) const override;

    virtual void OnMoveToSlot(EItemPlace prevPlace);
    virtual void OnMoveToRuck(EItemPlace prevPlace) override;

    virtual float GetExoFactor() const { return 1.f; };

private:
    float m_fPowerLoss{};

    shared_str m_ActorVisual;
    shared_str m_full_icon_name;
    SBoneProtections* m_boneProtection;

    u32 m_ef_equipment_type{};

public:
    virtual u32 ef_equipment_type() const;
    const shared_str& GetFullIconName() const { return m_full_icon_name; };

    bool m_bIsHelmetBuiltIn{};

    virtual void DrawHUDMask();
    bool HasVisor() const { return m_b_has_visor && m_bIsHelmetBuiltIn; }

protected:
    CUIStaticItem* m_UIVisor{};
    shared_str m_VisorTexture{}, bulletproof_display_bone{};
    bool m_b_has_visor{};
};