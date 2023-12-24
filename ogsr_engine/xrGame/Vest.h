#pragma once
#include "inventory_item_object.h"
class CVest : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;
    xr_vector<shared_str> m_covered_bones{};

public:    
    CVest(){};
    virtual ~CVest(){};

    virtual void Load(LPCSTR section);

    float HitThruArmour(SHit* pHDS);

    virtual float GetHitTypeProtection(int) const override;

    virtual bool can_be_attached() const override;

    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CVest)
#undef script_type_list
#define script_type_list save_type_list(CVest)