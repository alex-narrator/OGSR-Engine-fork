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
#include "CustomDetector.h"
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
    m_pUIVestList->SetVerticalOrder(m_pInv->m_bVestVertical);

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
    PIItem _itm = m_pInv->m_slots[KNIFE_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIKnifeList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[FIRST_WEAPON_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIFirstWeaponList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[SECOND_WEAPON_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUISecondWeaponList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[APPARATUS_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIBinocularList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[GRENADE_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIGrenadeList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[ARTEFACT_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIArtefactList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[BOLT_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIBoltList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[DETECTOR_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIDetectorList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[TORCH_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIOnHeadList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[PDA_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIPdaList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[HELMET_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIHelmetList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[GASMASK_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIGasMaskList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[WARBELT_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIWarBeltList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[BACKPACK_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUIBackPackList->SetItem(itm);
    }

    _itm = m_pInv->m_slots[VEST_SLOT].m_pIItem;
    if (_itm && show_item(_itm))
    {
        CUICellItem* itm = create_cell_item(_itm);
        m_pUITacticalVestList->SetItem(itm);
    }

    PIItem _outfit = m_pInv->m_slots[OUTFIT_SLOT].m_pIItem;
    CUICellItem* outfit = (_outfit && show_item(_outfit)) ? create_cell_item(_outfit) : NULL;
    m_pUIOutfitList->SetItem(outfit);

    std::sort(m_pInv->m_belt.begin(), m_pInv->m_belt.end(), InventoryUtilities::GreaterRoomInRuck);
    for (const auto& item : m_pInv->m_belt)
    {
        if (show_item(item))
        {
            CUICellItem* itm = create_cell_item(item);
            m_pUIBeltList->SetItem(itm);
        }
    }

    std::sort(m_pInv->m_vest.begin(), m_pInv->m_vest.end(), InventoryUtilities::GreaterRoomInRuck);
    for (const auto& item : m_pInv->m_vest)
    {
        if (show_item(item))
        {
            CUICellItem* itm = create_cell_item(item);
            m_pUIVestList->SetItem(itm);
        }
    }

    std::sort(m_pInv->m_ruck.begin(), m_pInv->m_ruck.end(), InventoryUtilities::GreaterRoomInRuck);
    TIItemContainer marked_items{};
    for (const auto& item : m_pInv->m_ruck)
    {
        if (show_item(item))
        {
            if (!item->GetMarked() || !pSettings->line_exist("engine_callbacks", "can_move_to_marked_list"))
            {
                CUICellItem* itm = create_cell_item(item);
                m_pUIBagList->SetItem(itm);
            }
            else
                marked_items.push_back(item);
        }
    }    
    std::sort(marked_items.begin(), marked_items.end(), InventoryUtilities::GreaterRoomInRuck);
    for (const auto& item : marked_items)
    {
        if (show_item(item))
        {
            CUICellItem* itm = create_cell_item(item);
            m_pUIMarkedList->SetItem(itm);
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
            if (auto det = smart_cast<CCustomDetector*>(iitem))
                det->ToggleDetector(g_player_hud->attached_item(0) != nullptr);

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
    else if (iitem->GetMarked() && itm->OwnerList() == m_pUIMarkedList)
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

        m_b_need_update_stats = true;
        VERIFY(result);
        CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

        if (b_use_cursor_pos)
            new_owner->SetItem(i, old_owner->GetDragItemPosition());
        else
            new_owner->SetItem(i);

        OnFromMarked(iitem);

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
        
        if (old_owner == m_pUIMarkedList)
            OnFromMarked(iitem);

        return true;
    }
    return false;
}

bool CUIInventoryWnd::ToVest(CUICellItem* itm, bool b_use_cursor_pos)
{
    PIItem iitem = (PIItem)itm->m_pData;

    if (GetInventory()->CanPutInVest(iitem))
    {
        CUIDragDropListEx* old_owner = itm->OwnerList();
        CUIDragDropListEx* new_owner = NULL;
        if (b_use_cursor_pos)
        {
            new_owner = CUIDragDropListEx::m_drag_item->BackList();
            VERIFY(new_owner == m_pUIVestList);
        }
        else
            new_owner = m_pUIVestList;
#ifdef DEBUG
        bool result =
#endif
        GetInventory()->Vest(iitem);
        PlaySnd(eInvItemToVest);
        m_b_need_update_stats = true;
        VERIFY(result);
        CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

        //.	UIBeltList.RearrangeItems();
        if (b_use_cursor_pos)
            new_owner->SetItem(i, old_owner->GetDragItemPosition());
        else
            new_owner->SetItem(i);

        TryReinitLists(iitem);
        
        if (old_owner == m_pUIMarkedList)
            OnFromMarked(iitem);

        return true;
    }
    return false;
}

bool CUIInventoryWnd::CanMoveToMarked(PIItem pItem)
{
    bool can_move_to_marked_list = false;
    if (pSettings->line_exist("engine_callbacks", "can_move_to_marked_list"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "can_move_to_marked_list");
        if (luabind::functor<bool> lua_function; ai().script_engine().functor(callback, lua_function))
            can_move_to_marked_list = lua_function(pItem->object().lua_game_object());
    }
    return can_move_to_marked_list;
}

bool CUIInventoryWnd::OnToMarked(CUICellItem* itm, bool b_use_cursor_pos)
{
    PIItem iitem = (PIItem)itm->m_pData;

    if (CanMoveToMarked(iitem))
    {
        if (pSettings->line_exist("engine_callbacks", "on_move_to_marked_list"))
        {
            const char* callback = pSettings->r_string("engine_callbacks", "on_move_to_marked_list");
            if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function))
                lua_function(iitem->object().lua_game_object());
        }

        CUIDragDropListEx* old_owner = itm->OwnerList();
        CUIDragDropListEx* new_owner = NULL;
        if (b_use_cursor_pos)
        {
            new_owner = CUIDragDropListEx::m_drag_item->BackList();
            VERIFY(new_owner == m_pUIMarkedList);
        }
        else
            new_owner = m_pUIMarkedList;

#ifdef DEBUG
        bool result =
#endif
        m_b_need_update_stats = true;
        VERIFY(result);
        CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

        if (b_use_cursor_pos)
            new_owner->SetItem(i, old_owner->GetDragItemPosition());
        else
            new_owner->SetItem(i);

        return true;
    }
    return false;
}

void CUIInventoryWnd::OnFromMarked(PIItem pItem)
{
    if (pSettings->line_exist("engine_callbacks", "on_move_from_marked_list"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "on_move_from_marked_list");
        if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function))
            lua_function(pItem->object().lua_game_object());
    }
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

    itm->ColorizeItems({m_pUIBagList, m_pUIBeltList, m_pUIVestList,
                        //
                        m_pUIOutfitList, m_pUIHelmetList, m_pUIGasMaskList, m_pUIWarBeltList, m_pUIBackPackList, m_pUITacticalVestList,
                        //
                        m_pUIKnifeList, m_pUIFirstWeaponList, m_pUISecondWeaponList, m_pUIBinocularList,
                        //
                        m_pUIGrenadeList, m_pUIArtefactList, m_pUIBoltList,
                        //
                        m_pUIDetectorList, m_pUIOnHeadList, m_pUIPdaList,
                        //
                        m_pUIMarkedList});
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
        ////спробуємо тут атач адонів, заряджання набоїв та тому подібне
        //if (DropItem(CurrentIItem(), new_owner))
        //    return true;

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
    case iwVest: {
        ToVest(itm, true);
    }
    case iwMarked: {
        OnToMarked(itm, true);
    }
    break;
    };

    //DropItem(CurrentIItem(), new_owner);

    return true;
}

bool CUIInventoryWnd::OnItemDbClick(CUICellItem* itm)
{
    auto __item = (PIItem)itm->m_pData;
    auto old_owner = itm->OwnerList();

    if (TryUseItem(__item))
        return true;

    auto t_old = GetType(old_owner);

    switch (t_old)
    {
    case iwSlot:
    case iwBelt:
    case iwVest:
    case iwMarked: {
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
        if (ToVest(itm, false))
            return true;
        if (ToBelt(itm, false))
            return true;
        if (OnToMarked(itm, false))
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
    m_pUIVestList->ClearAll(true);
    //
    m_pUIOutfitList->ClearAll(true);
    m_pUIHelmetList->ClearAll(true);
    m_pUIGasMaskList->ClearAll(true);
    m_pUIWarBeltList->ClearAll(true);
    m_pUIBackPackList->ClearAll(true);
    m_pUITacticalVestList->ClearAll(true);
    //
    m_pUIKnifeList->ClearAll(true);
    m_pUIFirstWeaponList->ClearAll(true);
    m_pUISecondWeaponList->ClearAll(true);
    m_pUIBinocularList->ClearAll(true);
    //
    m_pUIGrenadeList->ClearAll(true);
    m_pUIArtefactList->ClearAll(true);
    m_pUIBoltList->ClearAll(true);
    //
    m_pUIDetectorList->ClearAll(true);
    m_pUIOnHeadList->ClearAll(true);
    m_pUIPdaList->ClearAll(true);
    //
    m_pUIMarkedList->ClearAll(true);
}

void CUIInventoryWnd::ReinitBeltList()
{
    for (u32 i = 0; i < m_pUIBeltList->ItemsCount(); ++i)
    {
        auto itm = m_pUIBeltList->GetItemIdx(i);
        PIItem iitem = (PIItem)itm->m_pData;
        if (!m_pInv->InBelt(iitem))
            AddItemToBag(iitem);
    }
    m_pUIBeltList->ClearAll(true);
    std::sort(m_pInv->m_belt.begin(), m_pInv->m_belt.end(), InventoryUtilities::GreaterRoomInRuck);
    for (const auto& item : m_pInv->m_belt)
    {
        CUICellItem* itm = create_cell_item(item);
        m_pUIBeltList->SetItem(itm);
    }
}

void CUIInventoryWnd::ReinitVestList()
{
    for (u32 i = 0; i < m_pUIVestList->ItemsCount(); ++i)
    {
        auto itm = m_pUIVestList->GetItemIdx(i);
        PIItem iitem = (PIItem)itm->m_pData;
        if (!m_pInv->InVest(iitem))
            AddItemToBag(iitem);
    }
    m_pUIVestList->ClearAll(true);
    std::sort(m_pInv->m_vest.begin(), m_pInv->m_vest.end(), InventoryUtilities::GreaterRoomInRuck);
    for (const auto& item : m_pInv->m_vest)
    {
        CUICellItem* itm = create_cell_item(item);
        m_pUIVestList->SetItem(itm);
    }
}

void CUIInventoryWnd::ReinitMarkedList()
{
    m_pUIMarkedList->ClearAll(true);
    for (const auto& item : m_pInv->m_all)
    {
        if (item->GetMarked())
        {
            CUICellItem* itm = create_cell_item(item);
            m_pUIMarkedList->SetItem(itm);
        }
    }
}

void CUIInventoryWnd::ReinitSlotList(u32 slot)
{
    auto slot_list = GetSlotList(slot);
    if (!slot_list)
        return;

    if(!m_pInv->IsSlotAllowed(slot))
        for (u32 i = 0; i < slot_list->ItemsCount(); ++i)
        {
            auto itm = slot_list->RemoveItem(slot_list->GetItemIdx(i), false);
            m_pUIBagList->SetItem(itm);
            if (slot == WARBELT_SLOT)
                ReinitBeltList();
            if (slot == VEST_SLOT)
                ReinitVestList();
        }

    slot_list->ClearAll(true);

    PIItem _itm = m_pInv->m_slots[slot].m_pIItem;
    if (_itm)
    {
        CUICellItem* itm = create_cell_item(_itm);
        slot_list->SetItem(itm);
    }
}