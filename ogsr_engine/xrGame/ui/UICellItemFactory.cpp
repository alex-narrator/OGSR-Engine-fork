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

    auto pWarbelt = smart_cast<CWarbelt*>(itm);
    if (pWarbelt)
        return xr_new<CUIWarbeltCellItem>(pWarbelt);

    auto pVest = smart_cast<CVest*>(itm);
    if (pVest)
        return xr_new<CUIVestCellItem>(pVest);

    auto pContainer = smart_cast<CInventoryContainer*>(itm);
    if (pContainer)
        return xr_new<CUIContainerCellItem>(pContainer);

    return xr_new<CUIInventoryCellItem>(itm);
}
