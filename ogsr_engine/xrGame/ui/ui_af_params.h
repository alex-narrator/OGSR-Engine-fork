#pragma once
#include "UIWindow.h"

class CUIXml;
class CUIStatic;
class CInventoryItem;

class CUIArtefactParams : public CUIWindow
{
public:
    CUIArtefactParams();
    virtual ~CUIArtefactParams();
    void Init();
    void SetInfo(CInventoryItem* obj);

protected:
    enum
    {
        // restore
        _item_health_restore,
        _item_power_restore,
        _item_max_power_restore,
        _item_satiety_restore,
        _item_radiation_restore,
        _item_psy_health_restore,
        _item_alcohol_restore,
        _item_wounds_heal,
        // additional
        _item_additional_sprint,
        _item_additional_jump,
        _item_additional_weight,

        _hit_type_protection_index,

        _item_burn_immunity = _hit_type_protection_index,
        _item_shock_immunity,
        _item_strike_immunity,
        _item_wound_immunity,
        _item_radiation_immunity,
        _item_telepatic_immunity,
        _item_chemical_burn_immunity,
        _item_explosion_immunit,
        _item_fire_wound_immunity,

        _max_item_index,
    };
    CUIStatic* m_info_items[_max_item_index];
};