#pragma once
#include "UICellItem.h"
#include "Weapon.h"
#include "WeaponRPG7.h"
#include "eatable_item.h"
#include "Artifact.h"
#include "Warbelt.h"
#include "Vest.h"
#include "GasMask.h"
#include "InventoryContainer.h"

class CUIInventoryCellItem : public CUICellItem
{
    typedef CUICellItem inherited;

protected:
    bool b_auto_drag_childs;

    CUIStatic* m_text_add{};
    void init_add();

    CUIStatic* m_upgrade{};
    CUIStatic* CreateUpgradeIcon();

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

class CUIWarbeltCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

protected:
    virtual void UpdateItemText();

public:
    CUIWarbeltCellItem(CWarbelt* itm);
    virtual void Update();
    CWarbelt* object() { return (CWarbelt*)m_pData; }
};

class CUIVestCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

protected:
    virtual void UpdateItemText();

public:
    CUIVestCellItem(CVest* itm);
    virtual void Update();
    CVest* object() { return (CVest*)m_pData; }
};

class CUIContainerCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

protected:
    virtual void UpdateItemText();

public:
    CUIContainerCellItem(CInventoryContainer* itm);
    virtual void Update();
    CInventoryContainer* object() { return (CInventoryContainer*)m_pData; }
};

class CUIEatableCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

protected:
    virtual void UpdateItemText();

public:
    CUIEatableCellItem(CEatableItem* itm);
    virtual void Update();
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

class CUIGasMaskCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

protected:
    CUIStatic* m_filter{};
    CUIStatic* CreateFilterIcon();

public:
    CUIGasMaskCellItem(CGasMask* itm);
    virtual void Update();
    CGasMask* object() { return (CGasMask*)m_pData; }
    virtual bool EqualTo(CUICellItem* itm);
};

class CUIAmmoCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

protected:
    virtual void UpdateItemText();
    virtual void UpdateItemTextCustom();
    CUIStatic* m_ammo_in_box{};
    CUIStatic* CreateAmmoInBoxIcon();

public:
    CUIAmmoCellItem(CWeaponAmmo* itm);
    virtual void Update();
    virtual bool EqualTo(CUICellItem* itm);
    CWeaponAmmo* object() { return (CWeaponAmmo*)m_pData; }
};

class CUIWeaponCellItem : public CUIInventoryCellItem
{
    typedef CUIInventoryCellItem inherited;

public:
    CUIStatic* m_addons[eMaxAddon]{};

protected:
    virtual void UpdateItemText();
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