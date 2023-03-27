#pragma once

#include "UIWindow.h"

class CUIScrollView;
class CCustomOutfit;
class CUIStatic;
class CUIXml;

class CUIOutfitInfo : public CUIWindow
{
public:
    CUIOutfitInfo();
    virtual ~CUIOutfitInfo();

    void Update();
    void InitFromXml(CUIXml& xml_doc);

protected:
    CUIScrollView* m_listWnd{};

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
    CUIStatic* m_items[_max_item_index]{};
};