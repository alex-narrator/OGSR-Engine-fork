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

    CUIStatic* m_upgrade{};
    CUIStatic* CreateUpgradeIcon();

    CUIStatic* m_marked{};
    CUIStatic* CreateMarkedIcon();

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
    CUIStatic* m_ammo_in_box{};
    CUIStatic* CreateAmmoInBoxIcon();
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
    CUIStatic* m_addons[eMaxAddon]{};

protected:
    void CreateIcon(u32, CIconParams& params);
    void DestroyIcon(u32);
    CUIStatic* GetIcon(u32);
    void InitAddon(CUIStatic* s, CIconParams& params, Fvector2 offset, bool b_rotate);
    void InitAllAddons(CUIStatic* s_silencer, CUIStatic* s_scope, CUIStatic* s_launcher, CUIStatic* s_laser, CUIStatic* s_flashlight, CUIStatic* s_stock, CUIStatic* s_extender,
                       CUIStatic* s_forend, CUIStatic* s_magazine, bool b_vertical);
    bool is_scope();
    bool is_silencer();
    bool is_launcher();
    bool is_laser();
    bool is_flashlight();
    bool is_stock();
    bool is_extender();
    bool is_forend();
    bool is_magazine();

public:
    CUIWeaponCellItem(CWeapon* itm);
    virtual ~CUIWeaponCellItem();
    virtual void Update();
    CWeapon* object() { return (CWeapon*)m_pData; }
    virtual void OnAfterChild(CUIDragDropListEx* parent_list);
    CUIDragItem* CreateDragItem();
    virtual bool EqualTo(CUICellItem* itm);
    CUIStatic* get_addon_static(u32 idx) { return m_addons[idx]; }
    Fvector2 get_addon_offset(u32 idx) { return object()->GetAddonOffset(idx); }
};

class CUIWeaponRGP7CellItem : public CUIWeaponCellItem
{
    typedef CUIWeaponCellItem inherited;

protected:
    CUIStatic* m_grenade_loaded{};
    CUIStatic* CreateGrenadeIcon();

public:
    CUIWeaponRGP7CellItem(CWeaponRPG7* itm);
    virtual void Update();
    CWeaponRPG7* object() { return (CWeaponRPG7*)m_pData; }
    CUIDragItem* CreateDragItem();
};

class CBuyItemCustomDrawCell : public ICustomDrawCell
{
    CGameFont* m_pFont;
    string16 m_string;

public:
    CBuyItemCustomDrawCell(LPCSTR str, CGameFont* pFont);
    virtual void OnDraw(CUICellItem* cell);
};