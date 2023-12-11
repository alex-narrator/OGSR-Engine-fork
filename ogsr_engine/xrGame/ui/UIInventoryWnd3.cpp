#include "stdafx.h"
#include "UIInventoryWnd.h"
#include "Actor.h"
#include "Addons.h"
#include "Artifact.h"
#include "eatable_item.h"
#include "BottleItem.h"
#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "inventory.h"
#include "game_base.h"
#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "UICellItem.h"
#include "UIListBoxItem.h"
#include "CustomOutfit.h"
#include "InventoryContainer.h"
#include "string_table.h"
#include <regex>

#include "script_game_object.h"

void CUIInventoryWnd::EatItem(PIItem itm)
{
    SetCurrentItem(nullptr);
    m_b_need_reinit = true;
    if (!itm->Useful())
        return;
    GetInventory()->Eat(itm);
}

void CUIInventoryWnd::ActivatePropertiesBox()
{
    // Флаг-признак для невлючения пункта контекстного меню: Dreess Outfit, если костюм уже надет
    bool bAlreadyDressed = false;

    UIPropertiesBox.RemoveAll();

    auto pEatableItem = smart_cast<CEatableItem*>(CurrentIItem());
    auto pOutfit = smart_cast<CCustomOutfit*>(CurrentIItem());
    auto pContainer = smart_cast<CInventoryContainer*>(CurrentIItem());
    auto pWeapon = smart_cast<CWeapon*>(CurrentIItem());
    auto pAmmo = smart_cast<CWeaponAmmo*>(CurrentIItem());

    const auto& inv = m_pInv;
    auto pBackpack = pContainer && inv->CanPutInSlot(pContainer); // pContainer->GetSlot() == BACKPACK_SLOT;

    string1024 temp;

    bool b_show = false;
    bool b_many = CurrentItem()->ChildsCount();
    LPCSTR _many = b_many ? "•" : "";
    LPCSTR _addon_name{};
    LPCSTR detach_tip = CurrentIItem()->GetDetachMenuTip();
    bool b_wearable = (pOutfit || pBackpack);

     if (!b_wearable && CurrentIItem()->GetSlot() != NO_ACTIVE_SLOT)
    {
        auto& slots = CurrentIItem()->GetSlots();
        for (u8 i = 0; i < (u8)slots.size(); ++i)
        {
            auto slot = slots[i];
            if (slot != NO_ACTIVE_SLOT && inv->IsSlotAllowed(slot))
            {
                if (!inv->m_slots[slot].m_pIItem || inv->m_slots[slot].m_pIItem != CurrentIItem())
                {
                    string128 full_action_text;
                    strconcat(sizeof(full_action_text), full_action_text, "st_move_to_slot_", std::to_string(slot).c_str());
                    UIPropertiesBox.AddItem(full_action_text, (void*)(__int64)slot, INVENTORY_TO_SLOT_ACTION);
                    b_show = true;
                }
            }
        }
    }

    if (CurrentIItem()->Belt() && inv->CanPutInBelt(CurrentIItem()))
    {
        UIPropertiesBox.AddItem("st_move_on_belt", NULL, INVENTORY_TO_BELT_ACTION);
        b_show = true;
    }

    if (CurrentIItem()->Ruck() && inv->CanPutInRuck(CurrentIItem()) && (CurrentIItem()->GetSlot() == NO_ACTIVE_SLOT || !inv->m_slots[CurrentIItem()->GetSlot()].m_bPersistent))
    {
        UIPropertiesBox.AddItem(b_wearable ? "st_undress_outfit" : "st_move_to_bag", NULL, INVENTORY_TO_BAG_ACTION);

        bAlreadyDressed = true;
        b_show = true;
    }

    if (b_wearable && !bAlreadyDressed)
    {
        UIPropertiesBox.AddItem("st_dress_outfit", NULL, INVENTORY_TO_SLOT_ACTION);
        b_show = true;
    }

    const char* _addon_sect{};

    if (pAmmo && pAmmo->IsBoxReloadable())
    {
        // reload AmmoBox
        bool is_full = pAmmo->m_boxCurr == pAmmo->m_boxSize;
        for (const auto& type : pAmmo->m_ammoTypes)
        {
            if (inv->GetAmmoByLimit(type.c_str(), true))
            {
                if (is_full && type == pAmmo->m_ammoSect)
                    continue;
                auto _str = (type == pAmmo->m_ammoSect || !pAmmo->m_boxCurr) ? "st_load_ammo_type" : "st_reload_ammo_type";
                sprintf(temp, "%s%s %s", _many, CStringTable().translate(_str).c_str(),
                        CStringTable().translate(pSettings->r_string(type, "inv_name_short")).c_str());
                UIPropertiesBox.AddItem(temp, (void*)type.c_str(), INVENTORY_RELOAD_AMMO_BOX);
                b_show = true;
            }
        }
        // unload AmmoBox
        if (pAmmo->m_boxCurr)
        {
            sprintf(temp, "%s%s", _many, CStringTable().translate("st_unload_magazine").c_str());
            UIPropertiesBox.AddItem(temp, NULL, INVENTORY_UNLOAD_AMMO_BOX);
            b_show = true;
        }
    }

    // отсоединение аддонов от вещи
    if (pWeapon)
    {
        if (inv->InSlot(pWeapon))
        {
            for (u32 i = 0; i < pWeapon->m_ammoTypes.size(); ++i)
            {
                if (pWeapon->CanBeReloaded() && pWeapon->TryToGetAmmo(i))
                {
                    auto ammo_sect = pSettings->r_string(pWeapon->m_ammoTypes[i].c_str(), "inv_name_short");
                    sprintf(temp, "%s %s", CStringTable().translate("st_load_ammo_type").c_str(), CStringTable().translate(ammo_sect).c_str());
                    UIPropertiesBox.AddItem(temp, (void*)(__int64)i, INVENTORY_RELOAD_MAGAZINE);
                    b_show = true;
                }
            }
        }
        // addons detach
        for (u32 i = 0; i < eMagazine; i++)
        {
            if (pWeapon->AddonAttachable(i) && pWeapon->IsAddonAttached(i) && pWeapon->CanDetach(pWeapon->GetAddonName(i).c_str()))
            {
                _addon_sect = pWeapon->GetAddonName(i).c_str();
                _addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
                sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
                UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
                b_show = true;
            }
        }
        //
        if (pWeapon)
        {
            bool b = pWeapon->CanBeUnloaded();

            if (!b)
            {
                CUICellItem* itm = CurrentItem();
                for (u32 i = 0; i < itm->ChildsCount(); ++i)
                {
                    auto pWeaponChild = static_cast<CWeaponMagazined*>(itm->Child(i)->m_pData);
                    if (pWeaponChild->CanBeUnloaded())
                    {
                        b = true;
                        break;
                    }
                }
            }

            if (b)
            {
                sprintf(temp, "%s%s", _many, CStringTable().translate("st_unload_magazine").c_str());
                UIPropertiesBox.AddItem(temp, NULL, INVENTORY_UNLOAD_MAGAZINE);
                b_show = true;
            }
        }
    }

    for (const auto& slot : inv->m_slots)
    {
        auto tgt = slot.m_pIItem;
        if (!tgt)
            continue;
        if (tgt->CanAttach(CurrentIItem()))
        {
            sprintf(temp, "%s %s", CStringTable().translate(CurrentIItem()->GetAttachMenuTip()).c_str(), tgt->NameShort());
            UIPropertiesBox.AddItem(temp, (void*)tgt, INVENTORY_ATTACH_ADDON);
            b_show = true;
        }
    }

    if (pEatableItem)
    {
        UIPropertiesBox.AddItem(pEatableItem->GetUseMenuTip(), NULL, INVENTORY_EAT_ACTION);
        b_show = true;
    }

    for (const auto& action : CurrentIItem()->m_script_actions_map)
    {
        if (luabind::functor<bool> m_functorHasAction; ai().script_engine().functor(action.second[0].c_str(), m_functorHasAction))
        {
            if (m_functorHasAction(CurrentIItem()->object().lua_game_object()))
            {
                LPCSTR tip_text{};
                if (luabind::functor<LPCSTR> tip_func; ai().script_engine().functor(action.first.c_str(), tip_func))
                    tip_text = tip_func(CurrentIItem()->object().lua_game_object());
                else
                    tip_text = CStringTable().translate(action.first).c_str();

                UIPropertiesBox.AddItem(tip_text, (void*)action.first.c_str(), INVENTORY_SCRIPT_ACTION);
                b_show = true;
            }
        }
        else
            Msg("!Item-action-condition function [%s] not exist.", action.second[0].c_str());
    }

    if (!CurrentIItem()->IsQuestItem())
    {
        sprintf(temp, "%s%s", _many, CStringTable().translate("st_drop").c_str());
        UIPropertiesBox.AddItem(temp, NULL, INVENTORY_DROP_ACTION);
        b_show = true;
    }

    if (b_show)
    {
        UIPropertiesBox.AutoUpdateSize();
        UIPropertiesBox.BringAllToTop();

        Fvector2 cursor_pos;
        Frect vis_rect;
        GetAbsoluteRect(vis_rect);
        cursor_pos = GetUICursor()->GetCursorPosition();
        cursor_pos.sub(vis_rect.lt);
        UIPropertiesBox.Show(vis_rect, cursor_pos);
        PlaySnd(eInvProperties);
    }
}

void CUIInventoryWnd::ProcessPropertiesBoxClicked()
{
    if (UIPropertiesBox.GetClickedItem())
    {
        bool for_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
        auto itm = CurrentItem();
        auto item = CurrentIItem();
        switch (UIPropertiesBox.GetClickedItem()->GetTAG())
        {
        case INVENTORY_TO_SLOT_ACTION: {
            // Явно указали слот в меню
            void* d = UIPropertiesBox.GetClickedItem()->GetData();
            if (d)
            {
                auto slot = (u8)(__int64)d;
                item->SetSlot(slot);
                if (ToSlot(itm, true))
                    return;
            }
            // Пытаемся найти свободный слот из списка разрешенных.
            // Если его нету, то принудительно займет первый слот,
            // указанный в списке.
            auto& slots = item->GetSlots();
            for (u8 i = 0; i < (u8)slots.size(); ++i)
            {
                item->SetSlot(slots[i]);
                if (ToSlot(itm, false))
                    return;
            }
            item->SetSlot(slots.size() ? slots[0] : NO_ACTIVE_SLOT);
            ToSlot(itm, true);
        }
        break;
        case INVENTORY_TO_BELT_ACTION: ToBelt(itm, false); break;
        case INVENTORY_TO_BAG_ACTION: ToBag(itm, false); break;
        case INVENTORY_DROP_ACTION: {
            DropCurrentItem(for_all);
        }
        break;
        case INVENTORY_EAT_ACTION: EatItem(CurrentIItem()); break;
        case INVENTORY_ATTACH_ADDON: AttachAddon((PIItem)(UIPropertiesBox.GetClickedItem()->GetData())); break;
        case INVENTORY_DETACH_ADDON: DetachAddon((LPCSTR)(UIPropertiesBox.GetClickedItem()->GetData()), for_all); break;
        case INVENTORY_RELOAD_MAGAZINE: {
            void* d = UIPropertiesBox.GetClickedItem()->GetData();
            auto Wpn = smart_cast<CWeapon*>(item);
            Wpn->m_set_next_ammoType_on_reload = (u32)(__int64)d;
            Wpn->ReloadWeapon();
        }
        break;
        case INVENTORY_UNLOAD_MAGAZINE: {
            auto wpn = smart_cast<CWeaponMagazined*>(item);
            wpn->UnloadWeaponFull();
            for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i)
            {
                auto wpn = static_cast<CWeaponMagazined*>(itm->Child(i)->m_pData);
                wpn->UnloadWeaponFull();
            }
        }
        break;
        case INVENTORY_RELOAD_AMMO_BOX: {
            auto sect_to_load = (LPCSTR)UIPropertiesBox.GetClickedItem()->GetData();
            auto ammobox = smart_cast<CWeaponAmmo*>(item);
            ammobox->ReloadBox(sect_to_load);
            for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i)
            {
                auto ammobox = static_cast<CWeaponAmmo*>(itm->Child(i)->m_pData);
                ammobox->ReloadBox(sect_to_load);
            }
            InitInventory_delayed();
        }
        break;
        case INVENTORY_UNLOAD_AMMO_BOX: {
            auto ammobox = smart_cast<CWeaponAmmo*>(item);
            ammobox->UnloadBox();
            for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i)
            {
                auto ammobox = static_cast<CWeaponAmmo*>(itm->Child(i)->m_pData);
                ammobox->UnloadBox();
            }
            InitInventory_delayed();
        }
        break;
        case INVENTORY_SCRIPT_ACTION: {
            auto it = CurrentIItem()->m_script_actions_map.find((LPCSTR)UIPropertiesBox.GetClickedItem()->GetData());
            if (luabind::functor<void> m_functorDoAction; ai().script_engine().functor(it->second[1].c_str(), m_functorDoAction))
                m_functorDoAction(CurrentIItem()->object().lua_game_object());
            else
                Msg("!Item-action function [%s] not exist.", it->second[1].c_str());
        }
        break;
        }
    }
}
