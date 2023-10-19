////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_container.h
//	Created 	: 12.11.2014
//  Modified 	: 11.12.2014
//	Author		: Alexander Petrov
//	Description : Mobile container class, based on inventory item
////////////////////////////////////////////////////////////////////////////
#pragma once
#include "InventoryBox.h"
#include "inventory_item.h"
#include "inventory_item_object.h"

// CInventoryContainer
class CInventoryContainer : public CCustomInventoryBox<CInventoryItemObject>
{
private:
    typedef CCustomInventoryBox<CInventoryItemObject> inherited;

public:
    CInventoryContainer(){};
    virtual ~CInventoryContainer(){};
    virtual void Load(LPCSTR section);
    //virtual bool CanTrade() const;
    virtual u32 Cost() const;
    virtual float Weight() const;

    virtual void shedule_Update(u32 dt);

    virtual float GetItemEffect(int) const;
    // окремий підрахунок діючих параметрів від артефактів у контейнері
    virtual float GetContainmentArtefactEffect(int) const;
    virtual float GetContainmentArtefactProtection(int) const;

    virtual bool can_be_attached() const override;

    virtual void Hit(SHit* pHDS);
    void HitItemsInBackPack(SHit* pHDS);
    void HitItemsInContainer(SHit* pHDS);

    virtual u32 GetSameItemCount(shared_str) const;

    void AddUniqueItems(TIItemContainer& items_container) const;

    virtual bool CanTakeItem(CInventoryItem*) const;

    float HitThruArmour(SHit* pHDS);

protected:
    void UpdateDropTasks();
    void UpdateDropItem(PIItem pIItem);
    xr_vector<shared_str> m_allowed_classes{};
};