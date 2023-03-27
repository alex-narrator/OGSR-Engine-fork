#pragma once
#include "inventory_item_object.h"
class CPowerBattery : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CPowerBattery(){};
    virtual ~CPowerBattery(){};

    virtual void Load(LPCSTR section);

    virtual bool Useful() const;

    virtual bool CanBeCharged() const override;

    bool m_bRechargeable{};
};