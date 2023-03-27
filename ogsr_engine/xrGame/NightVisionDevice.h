#pragma once
#include "inventory_item_object.h"
#include "hudsound.h"
class CNightVisionDevice : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CNightVisionDevice(void);
    virtual ~CNightVisionDevice(void);

    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(CSE_Abstract* E);

    virtual void UpdateCL();
    virtual void OnH_B_Independent(bool just_before_destroy);

    virtual bool can_be_attached() const;
    virtual void afterAttach();
    virtual void afterDetach();

    virtual void DrawHUDMask();

    virtual void Switch();
    virtual void Switch(bool);
    void UpdateSwitch();
    virtual bool IsPowerOn() const { return m_bNightVisionOn; };

protected:
    bool m_bNightVisionOn{};

    HUD_SOUND sndNightVisionOn;
    HUD_SOUND sndNightVisionOff;
    HUD_SOUND sndNightVisionIdle;
    HUD_SOUND sndNightVisionBroken;

    shared_str m_NightVisionSect{};

    CUIStaticItem* m_UINightVision{};
    shared_str m_NightVisionTexture{};
};
