#pragma once
#include "inventory_item_object.h"
class CGasMask : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CGasMask();
    virtual ~CGasMask(){};

    virtual bool can_be_attached() const override;

    float HitThruArmour(SHit* pHDS);
};