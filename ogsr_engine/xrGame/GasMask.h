#pragma once
#include "inventory_item_object.h"
class CGasMask : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CGasMask();
    virtual ~CGasMask(){};

    // коэффициент на который домножается потеря силы
    // если на персонаже надет костюм
    virtual float GetPowerLoss();

    virtual bool can_be_attached() const override;

    // инициализация свойств присоединенных аддонов
    virtual void InitAddons();
    virtual bool IsPowerOn() const;
    virtual float GetHitTypeProtection(int) const;

private:
    float m_fPowerLoss{};
};