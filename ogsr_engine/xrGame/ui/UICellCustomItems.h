#pragma once
#include "UICellItem.h"
#include "Weapon.h"
#include "WeaponRPG7.h"
#include "eatable_item.h"
#include "Artifact.h"

class CUIInventoryCellItem : public CUICellItem
{
    typedef CUICellItem inherited;

protected:
    bool b_auto_drag_childs;

public:
    CUIInventoryCellItem(CInventoryItem* itm);
    virtual void Update();
    virtual bool EqualTo(CUICellItem* itm);
    virtual CUIDragItem* CreateDragItem();
    CInventoryItem* object() { return (CInventoryItem*)m_pData; }

    // Real Wolf: Для коллбеков. 25.07.2014.
    virtual void OnFocusReceive();
    virtual void OnFocusLost();
    virtual bool OnMouse(float, float, EUIMessages);
    // Real Wolf: Для метода get_cell_item(). 25.07.2014.
    virtual ~CUIInventoryCellItem();
};

class CUIEatableCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

public:
    CUIEatableCellItem(CEatableItem* itm);
    virtual bool EqualTo(CUICellItem* itm);
    CEatableItem* object() { return (CEatableItem*)m_pData; }
};

class CUIArtefactCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

public:
    CUIArtefactCellItem(CArtefact* itm);
    virtual bool EqualTo(CUICellItem* itm);
    CArtefact* object() { return (CArtefact*)m_pData; }
};

class CUIAmmoCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

protected:
    virtual void UpdateItemText();

public:
    CUIAmmoCellItem(CWeaponAmmo* itm);
    virtual bool EqualTo(CUICellItem* itm);
    CWeaponAmmo* object() { return (CWeaponAmmo*)m_pData; }
    virtual void Update();
};

class CUIWeaponCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

public:
    CUIWeaponCellItem(CWeapon* itm);
    virtual ~CUIWeaponCellItem(){};
    CWeapon* object() { return (CWeapon*)m_pData; }
    virtual bool EqualTo(CUICellItem* itm);
};