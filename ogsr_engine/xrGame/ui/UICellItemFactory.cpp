#include "stdafx.h"
#include "UICellItemFactory.h"
#include "UICellCustomItems.h"

CUICellItem* create_cell_item(CInventoryItem* itm)
{
    auto pAmmo = smart_cast<CWeaponAmmo*>(itm);
    if (pAmmo)
        return xr_new<CUIAmmoCellItem>(pAmmo);

    auto pWeaponRPG = smart_cast<CWeaponRPG7*>(itm);
    if (pWeaponRPG)
        return xr_new<CUIWeaponRGP7CellItem>(pWeaponRPG);

    auto pWeapon = smart_cast<CWeapon*>(itm);
    if (pWeapon)
        return xr_new<CUIWeaponCellItem>(pWeapon);

    auto pEatable = smart_cast<CEatableItem*>(itm);
    if (pEatable)
        return xr_new<CUIEatableCellItem>(pEatable);

    auto pArtefact = smart_cast<CArtefact*>(itm);
    if (pArtefact)
        return xr_new<CUIArtefactCellItem>(pArtefact);

    return xr_new<CUIInventoryCellItem>(itm);
}
