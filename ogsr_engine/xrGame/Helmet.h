#pragma once
#include "inventory_item_object.h"
class CHelmet : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CHelmet(){};
    virtual ~CHelmet(){};

    virtual float GetHitTypeProtection(int) const override;

    virtual bool can_be_attached() const override;
};
