#include "stdafx.h"
#include "UIInventoryWnd.h"
#include "level.h"
#include "actor.h"
#include "ActorCondition.h"
#include "hudmanager.h"
#include "inventory.h"
#include "UIInventoryUtilities.h"

#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIDragDropListEx.h"
#include "UI3tButton.h"

#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "CustomDevice.h"
#include "player_hud.h"

#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"

CUICellItem* CUIInventoryWnd::CurrentItem() { return m_pCurrentCellItem; }

PIItem CUIInventoryWnd::CurrentIItem() { return (m_pCurrentCellItem) ? (PIItem)m_pCurrentCellItem->m_pData : NULL; }

void CUIInventoryWnd::SetCurrentItem(CUICellItem* itm)
{
    if (m_pCurrentCellItem == itm)
        return;
    m_pCurrentCellItem = itm;

    //UIItemInfo.InitItem(CurrentIItem());

    if (m_pCurrentCellItem)
    {
        m_pCurrentCellItem->m_select_armament = true;
        auto script_obj = CurrentIItem()->object().lua_game_object();
        g_actor->callback(GameObject::eCellItemSelect)(script_obj);
    }
}

void CUIInventoryWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == &UIPropertiesBox && msg == PROPERTY_CLICKED)
        ProcessPropertiesBoxClicked();

    CUIWindow::SendMessage(pWnd, msg, pData);
}

void CUIInventoryWnd::InitInventory_delayed()
{
    m_b_need_reinit = true;
    m_b_need_update_stats = true;
}

void CUIInventoryWnd::InitInventory()
{
    CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(Level().CurrentEntity());
    if (!pInvOwner)
        return;

    m_pInv = &pInvOwner->inventory();

    m_pInv->RepackAmmo();

    //порядок вишиковування елементів драгдропу тепер встановлюємо тут
    //незалежно від XML-конфігу
    m_pUIBeltList->SetVerticalOrder(m_pInv->m_bBeltVertical);

    int bag_scroll = m_pUIBagList->ScrollPos();

    UIPropertiesBox.Hide();
    ClearAllLists();
    CUIWindow::Reset();
    SetCurrentItem(NULL);

    UpdateCustomDraw();

    auto show_item = [&](const auto pIItem) -> bool 
    {
        return !pIItem->m_flags.test(CInventoryItem::FIHiddenForInventory);
    };

    // Slots
    for (u8 i = 0; i < SLOTS_TOTAL; i++)
    {
        auto slot_ddlist = m_slots_array[i];
        if (!slot_ddlist)
            continue;
        auto _itm = m_pInv->m_slots[i].m_pIItem;
        if (_itm && show_item(_itm))
        {
            CUICellItem* itm = create_cell_item(_itm);
            slot_ddlist->SetItem(itm);
        }
    }

    std::sort(m_pInv->m_belt.begin(), m_pInv->m_belt.end(), InventoryUtilities::GreaterRoomInRuck);
    for (const auto& item : m_pInv->m_belt)
    {
        if (show_item(item))
        {
            CUICellItem* itm = create_cell_item(item);
            m_pUIBeltList->SetItem(itm);
        }
    }

    TIItemContainer marked_items{}, ruck_items{};
    for (const auto& item : m_pInv->m_ruck)
        item->GetMarked() ? marked_items.push_back(item) : ruck_items.push_back(item);

    int start_row = 0;
    std::sort(marked_items.begin(), marked_items.end(), InventoryUtilities::GreaterRoomInRuck);
    for (const auto& item : marked_items)
    {
        if (show_item(item))
        {
            CUICellItem* itm = create_cell_item(item);
            m_pUIBagList->SetItem(itm);
            if (itm->GetGridSize().y > start_row)
                start_row = itm->GetGridSize().y;
        }
    }
    std::sort(ruck_items.begin(), ruck_items.end(), InventoryUtilities::GreaterRoomInRuck);
    for (const auto& item : ruck_items)
    {
        if (show_item(item))
        {
            CUICellItem* itm = create_cell_item(item);
            m_pUIBagList->SetItem(itm, start_row);
        }
    }

    m_pUIBagList->SetScrollPos(bag_scroll);

    m_b_need_reinit = false;
}

void CUIInventoryWnd::DropCurrentItem(bool b_all)
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!pActor)
        return;

    if (!CurrentIItem() || CurrentIItem()->IsQuestItem())
        return;

    if (b_all)
    {
        u32 cnt = CurrentItem()->ChildsCount();
        for (u32 i = 0; i < cnt; ++i)
        {
            CUICellItem* itm = CurrentItem()->PopChild();
            PIItem iitm = (PIItem)itm->m_pData;
            iitm->Drop();
        }
    }

    CurrentIItem()->Drop();
    SetCurrentItem(NULL);

    PlaySnd(eInvDropItem);
    m_b_need_update_stats = true;
}

//------------------------------------------
bool CUIInventoryWnd::ToSlot(CUICellItem* itm, u8 _slot_id, bool force_place)
{
    PIItem iitem = (PIItem)itm->m_pData;
    // Msg("~~[CUIInventoryWnd::ToSlot] Name [%s], slot [%u]", iitem->object().cName().c_str(), _slot_id);
    // LogStackTrace("ST:\n");
    iitem->SetSlot(_slot_id);
    return ToSlot(itm, force_place);
}

bool CUIInventoryWnd::ToSlot(CUICellItem* itm, bool force_place)
{
    CUIDragDropListEx* old_owner = itm->OwnerList();
    PIItem iitem = (PIItem)itm->m_pData;
    u8 _slot = iitem->GetSlot();
    bool result{};

    if (GetInventory()->CanPutInSlot(iitem))
    {
        /*Msg("CUIInventoryWnd::ToSlot [%d]", _slot);*/
        CUIDragDropListEx* new_owner = GetSlotList(_slot);

        //		if(_slot==GRENADE_SLOT && !new_owner )return true; //fake, sorry (((

        result = GetInventory()->Slot(iitem);
        VERIFY(result);

        CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

        new_owner->SetItem(i);

        GetInventory()->Slot(iitem, !iitem->cast_hud_item());
        PlaySnd(eInvItemToSlot);
        m_b_need_update_stats = true;
    }
    else
    { // in case slot is busy
        if (!force_place || _slot == NO_ACTIVE_SLOT || GetInventory()->m_slots[_slot].m_bPersistent || !GetInventory()->IsSlotAllowed(_slot))
            return false;

        CUIDragDropListEx* slot_list = GetSlotList(_slot);
        VERIFY(slot_list->ItemsCount() == 1);

        CUICellItem* slot_cell = slot_list->GetItemIdx(0);
        VERIFY(slot_cell && ((PIItem)slot_cell->m_pData) == GetInventory()->m_slots[_slot].m_pIItem);

#ifdef DEBUG
        bool _result =
#endif
        ToBag(slot_cell, false);
        VERIFY(_result);

        return ToSlot(itm, false);
    }

    if (result)
    {
        if (_slot == DETECTOR_SLOT)
            if (auto dev = smart_cast<CCustomDevice*>(iitem))
                dev->ToggleDevice(g_player_hud->attached_item(0) != nullptr);

        TryReinitLists(iitem);
    }

    return result;
}

bool CUIInventoryWnd::ToBag(CUICellItem* itm, bool b_use_cursor_pos)
{
    PIItem iitem = (PIItem)itm->m_pData;

    if (GetInventory()->CanPutInRuck(iitem))
    {
        CUIDragDropListEx* old_owner = itm->OwnerList();
        CUIDragDropListEx* new_owner = NULL;
        if (b_use_cursor_pos)
        {
            new_owner = CUIDragDropListEx::m_drag_item->BackList();
            VERIFY(new_owner == m_pUIBagList);
        }
        else
            new_owner = m_pUIBagList;

#ifdef DEBUG
        bool result =
#endif
        GetInventory()->Ruck(iitem);
        PlaySnd(eInvItemToRuck);
        m_b_need_update_stats = true;
        VERIFY(result);
        CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

        if (b_use_cursor_pos)
            new_owner->SetItem(i, old_owner->GetDragItemPosition());
        else
            new_owner->SetItem(i);

        if (old_owner != m_pUIBagList)
            TryReinitLists(iitem);

        return true;
    }
    return false;
}

bool CUIInventoryWnd::ToBelt(CUICellItem* itm, bool b_use_cursor_pos)
{
    PIItem iitem = (PIItem)itm->m_pData;

    if (GetInventory()->CanPutInBelt(iitem))
    {
        CUIDragDropListEx* old_owner = itm->OwnerList();
        CUIDragDropListEx* new_owner = NULL;
        if (b_use_cursor_pos)
        {
            new_owner = CUIDragDropListEx::m_drag_item->BackList();
            VERIFY(new_owner == m_pUIBeltList);
        }
        else
            new_owner = m_pUIBeltList;
#ifdef DEBUG
        bool result =
#endif
        GetInventory()->Belt(iitem);
        PlaySnd(eInvItemToBelt);
        m_b_need_update_stats = true;
        VERIFY(result);
        CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

        //.	UIBeltList.RearrangeItems();
        if (b_use_cursor_pos)
            new_owner->SetItem(i, old_owner->GetDragItemPosition());
        else
            new_owner->SetItem(i);

        TryReinitLists(iitem);

        return true;
    }
    return false;
}

void CUIInventoryWnd::AddItemToBag(PIItem pItem)
{
    CUICellItem* itm = create_cell_item(pItem);
    m_pUIBagList->SetItem(itm);
}

bool CUIInventoryWnd::OnItemStartDrag(CUICellItem* itm)
{
    return false; // default behaviour
}

bool CUIInventoryWnd::OnItemSelected(CUICellItem* itm)
{
    SetCurrentItem(itm);
    xr_vector<CUIDragDropListEx*> list{m_pUIBagList, m_pUIBeltList};
    for (const auto& ddlist : m_slots_array)
        if (ddlist)
            list.push_back(ddlist);
    itm->ColorizeItems(list);
    return false;
}

bool CUIInventoryWnd::OnItemDrop(CUICellItem* itm)
{
    auto old_owner = itm->OwnerList();
    auto new_owner = CUIDragDropListEx::m_drag_item->BackList();
    if (old_owner == new_owner || !old_owner || !new_owner)
        return false;

    auto t_new = GetType(new_owner);
    auto t_old = GetType(old_owner);

    if (t_new == t_old && t_new != iwSlot)
        return true;

    switch (t_new)
    {
    case iwSlot: {
        auto item = CurrentIItem();

        bool can_put = false;
        Ivector2 max_size = new_owner->CellSize();

        LPCSTR name = item->object().Name_script();
        int item_w = item->GetGridWidth();
        int item_h = item->GetGridHeight();

        if (new_owner->GetVerticalPlacement())
            std::swap(max_size.x, max_size.y);

        if (item_w <= max_size.x && item_h <= max_size.y)
        {
            for (u8 i = 0; i < SLOTS_TOTAL; i++)
                if (new_owner == GetSlotList(i))
                {
                    if (item->IsPlaceable(i, i))
                    {
                        item->SetSlot(i);
                        can_put = true;
                    }
                    else
                    {
                        if (/*!DropItem(CurrentIItem(), new_owner) && */item->GetSlotsCount() > 0)
                            Msg("! cannot put item %s into slot %d, allowed slots {%s}", name, i, item->GetSlotsSect());
                    }
                    break;
                } // for-if
        }
        else
            Msg("!#ERROR: item %s to large for slot: (%d x %d) vs (%d x %d) ", name, item_w, item_h, max_size.x, max_size.y);

        // при невозможности поместить в выбранный слот
        if (!can_put)
        {
            // восстановление не требуется, слот не был назначен
            return true;
        }

        if (GetSlotList(item->GetSlot()) == new_owner)
            ToSlot(itm, true);
    }
    break;
    case iwBag: {
        ToBag(itm, true);
    }
    break;
    case iwBelt: {
        ToBelt(itm, true);
    }
    break;
    };

    return true;
}

bool CUIInventoryWnd::OnItemDbClick(CUICellItem* itm)
{
    auto __item = (PIItem)itm->m_pData;
    auto old_owner = itm->OwnerList();

    auto t_old = GetType(old_owner);

    switch (t_old)
    {
    case iwSlot:
    case iwBelt: {
        ToBag(itm, false);
    }
    break;

    case iwBag: {
        // Пытаемся найти свободный слот из списка разрешенных.
        // Если его нету, то принудительно займет первый слот,
        // указанный в списке.
        auto& slots = __item->GetSlots();
        for (const auto& slot : slots)
        {
            __item->SetSlot(slot);
            if (ToSlot(itm, false))
                return true;
        }
        if (ToBelt(itm, false))
            return true;

        for (const auto& slot : slots)
        {
            if (m_pInv->IsSlotAllowed(slot))
            {
                __item->SetSlot(slot);
                break;
            }
        }
        if (!ToSlot(itm, false))
            ToSlot(itm, true);
    }
    break;
    };

    return true;
}

bool CUIInventoryWnd::OnItemRButtonClick(CUICellItem* itm)
{
    SetCurrentItem(itm);
    ActivatePropertiesBox();
    return false;
}

CUIDragDropListEx* CUIInventoryWnd::GetSlotList(u8 slot_idx)
{
    if (slot_idx == NO_ACTIVE_SLOT || GetInventory()->m_slots[slot_idx].m_bPersistent)
        return NULL;
    return m_slots_array[slot_idx];
}

void CUIInventoryWnd::ClearAllLists()
{
    m_pUIBagList->ClearAll(true);
    m_pUIBeltList->ClearAll(true);
    //
    for (const auto& ddlist : m_slots_array)
        if (ddlist)
            ddlist->ClearAll(true);
}

void CUIInventoryWnd::ClearSlotList(u32 slot)
{
    auto slot_list = GetSlotList(slot);
    if (!slot_list)
        return;
    if (m_pInv->IsSlotAllowed(slot))
        return;
    for (u32 i = 0; i < slot_list->ItemsCount(); ++i)
    {
        auto itm = slot_list->GetItemIdx(i);
        PIItem iitem = (PIItem)itm->m_pData;
        AddItemToBag(iitem);
    }
    slot_list->ClearAll(true);
}