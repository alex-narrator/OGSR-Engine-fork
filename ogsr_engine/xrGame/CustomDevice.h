#pragma once
#include "hud_item_object.h"
class CCustomDevice : public CHudItemObject
{
    typedef CHudItemObject inherited;

protected:
    bool m_bThrowAnm{};
    bool m_bFastAnimMode{};
    bool m_bNeedActivation{};
    bool m_bWorking{};
    bool CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate);
    void UpdateVisibility();

    virtual void UpdateWork(){};

    virtual size_t GetWeaponTypeForCollision() const override { return Detector; }
    virtual Fvector GetPositionForCollision() override;
    virtual Fvector GetDirectionForCollision() override;

    virtual u8 GetCurrentHudOffsetIdx() const override { return IsAiming(); };

    virtual bool IsBlocked();

public:
    virtual BOOL net_Spawn(CSE_Abstract* DC) override;
    virtual void Load(LPCSTR section) override;

    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    virtual void OnActiveItem() override;
    virtual void OnHiddenItem() override;
    virtual void OnStateSwitch(u32 S, u32 oldState) override;
    virtual void OnAnimationEnd(u32 state) override;
    virtual void UpdateXForm() override;

    virtual void UpdateCL() override;

    virtual bool IsPowerOn() const;
    virtual void Switch(bool);

    virtual void SwitchMode(){};

    void ToggleDevice(bool bFastMode);
    void HideDevice(bool bFastMode);
    void ShowDevice(bool bFastMode);
    virtual bool CheckCompatibility(CHudItem* itm) override;

    virtual void OnMoveToSlot(EItemPlace prevPlace) override;
    virtual void OnMoveToRuck(EItemPlace prevPlace) override;
    virtual void OnMoveToBelt(EItemPlace prevPlace) override;
    virtual void OnMoveToVest(EItemPlace prevPlace) override;

    bool IsZoomed() const override;
    bool IsAiming() const;
};