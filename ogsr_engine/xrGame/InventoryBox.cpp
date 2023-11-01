#include "stdafx.h"
#include "InventoryBox.h"
#include "level.h"
#include "actor.h"
#include "HUDManager.h"
#include "UIGameSP.h"
#include "game_object_space.h"

#include "script_callback_ex.h"
#include "script_game_object.h"

#include "inventory_item.h"
#include "WeaponAmmo.h"

IInventoryBox::IInventoryBox() : m_items() { m_items.clear(); }

void IInventoryBox::ProcessEvent(CGameObject* O, NET_Packet& P, u16 type)
{
    switch (type)
    {
    case GE_OWNERSHIP_TAKE:
    case GE_TRANSFER_TAKE: {
        u16 id;
        P.r_u16(id);
        CObject* itm = Level().Objects.net_Find(id);
        VERIFY(itm);

        // штатно таке повинно траплятись тільки при спробі запхати предмети в лімітний контейнер стаком через карбоді
        auto iitem = smart_cast<PIItem>(itm);
        if (!CanTakeItem(iitem))
        {
            iitem->Transfer(object().ID(), m_in_use ? Actor()->ID() : u16(-1));
            return;
        }

        m_items.push_back(id);
        itm->H_SetParent(O);
        itm->setVisible(FALSE);
        itm->setEnabled(FALSE);

        // Real Wolf: Коллбек для ящика на получение предмета. 02.08.2014.
        if (auto obj = smart_cast<CGameObject*>(itm))
        {
            O->callback(GameObject::eOnInvBoxItemTake)(obj->lua_game_object());

            if (m_in_use)
            {
                Actor()->callback(GameObject::eInvBoxItemPlace)(O->lua_game_object(), obj->lua_game_object());
            }
        }
        smart_cast<PIItem>(itm)->SetDropTime(false);
    }
    break;
    case GE_OWNERSHIP_REJECT:
    case GE_TRANSFER_REJECT: {
        u16 id;
        P.r_u16(id);
        CObject* itm = Level().Objects.net_Find(id);

        auto it = std::find(m_items.begin(), m_items.end(), id);
        if (it != m_items.end())
            m_items.erase(it);
        else
            Msg("!![%s.GE_TRANSFER_REJECT] object with id [%u] not found! itm: [%p] [%s]", __FUNCTION__, id, itm, itm->Name_script());

        bool just_before_destroy = !P.r_eof() && P.r_u8();
        bool dont_create_shell = (type == GE_TRADE_SELL) || (type == GE_TRANSFER_REJECT) || just_before_destroy;

        if (!itm)
            return;
        /*Msg("~ [%s: REJECT] object [%s] with id [%u] parent [%s]", __FUNCTION__, itm->cName().c_str(), id, itm->H_Parent()->cName().c_str());*/
        itm->H_SetParent(NULL, dont_create_shell);

        if (Actor() && HUD().GetUI() && HUD().GetUI()->UIGame())
            HUD().GetUI()->UIGame()->ReInitShownUI();

        if (auto obj = smart_cast<CGameObject*>(itm))
        {
            O->callback(GameObject::eOnInvBoxItemDrop)(obj->lua_game_object());

            if (m_in_use)
            {
                Actor()->callback(GameObject::eInvBoxItemTake)(O->lua_game_object(), obj->lua_game_object());
            }
        }
        smart_cast<PIItem>(itm)->SetDropTime(true);
    }
    break;
    };
}

void IInventoryBox::AddAvailableItems(TIItemContainer& items_container) const
{
    for (const auto& item_id : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        VERIFY(itm);
        items_container.push_back(itm);
    }
}

bool IInventoryBox::CanTakeItem(CInventoryItem* inventory_item) const { return true; }

CScriptGameObject* IInventoryBox::GetObjectByName(LPCSTR name)
{
    const shared_str s_name(name);
    CObjectList& objects = Level().Objects;
    CObject* result = objects.FindObjectByName(name);
    if (result)
    {
        CObject* self = this->object().dcast_CObject();
        if (result->H_Parent() != self)
            return NULL; // объект существует, но не принадлежит сему контейнеру
    }
    else
    {
        for (auto it = m_items.begin(); it != m_items.end(); ++it)
            if (auto obj = objects.net_Find(*it))
            {
                if (obj->cName() == s_name)
                    return smart_cast<CGameObject*>(obj)->lua_game_object();
                if (obj->cNameSect() == s_name)
                    result = obj; // поиск по секции в качестве резерва
            }
    }
    return result ? smart_cast<CGameObject*>(result)->lua_game_object() : NULL;
}

CScriptGameObject* IInventoryBox::GetObjectByIndex(u32 id)
{
    if (id < m_items.size())
    {
        u32 obj_id = u32(m_items[id]);
        if (auto obj = smart_cast<CGameObject*>(Level().Objects.net_Find(obj_id)); obj && !obj->getDestroy())
            return obj->lua_game_object();
    }
    return nullptr;
}

u32 IInventoryBox::GetSize() const { return m_items.size(); }

bool IInventoryBox::IsEmpty() const { return m_items.empty(); }

void IInventoryBox::RepackAmmo()
{
    xr_vector<CWeaponAmmo*> _ammo;
    // заполняем массив неполными пачками
    for (const auto& item_id : m_items)
    {
        PIItem _pIItem = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
        CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(_pIItem);
        if (pAmmo && !pAmmo->IsBoxReloadable() && pAmmo->m_boxCurr < pAmmo->m_boxSize)
            _ammo.push_back(pAmmo);
    }
    while (!_ammo.empty())
    {
        shared_str asect = _ammo[0]->cNameSect(); // текущая секция
        u16 box_size = _ammo[0]->m_boxSize; // размер пачки
        u32 cnt = 0;
        u16 cart_cnt = 0;
        // считаем кол=во патронов текущей секции
        for (CWeaponAmmo* ammo : _ammo)
        {
            if (asect == ammo->cNameSect())
            {
                cnt = cnt + ammo->m_boxCurr;
                cart_cnt++;
            }
        }
        // если больше одной неполной пачки, то перепаковываем
        if (cart_cnt > 1)
        {
            for (CWeaponAmmo* ammo : _ammo)
            {
                if (asect == ammo->cNameSect())
                {
                    if (cnt > 0)
                    {
                        if (cnt > box_size)
                        {
                            ammo->m_boxCurr = box_size;
                            cnt = cnt - box_size;
                        }
                        else
                        {
                            ammo->m_boxCurr = (u16)cnt;
                            cnt = 0;
                        }
                    }
                    else
                        ammo->DestroyObject();
                }
            }
        }
        // чистим массив от обработанных пачек
        _ammo.erase(std::remove_if(_ammo.begin(), _ammo.end(), [asect](CWeaponAmmo* a) { return a->cNameSect() == asect; }), _ammo.end());
    }
}

void CInventoryBox::shedule_Update(u32 dt)
{
    CGameObject::shedule_Update(dt);
    UpdateDropTasks();
}

void CInventoryBox::UpdateDropTasks()
{
    for (const auto& item : m_items)
    {
        PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item));
        VERIFY(itm);
        UpdateDropItem(itm);
    }
}

void CInventoryBox::UpdateDropItem(PIItem pIItem)
{
    if (pIItem->GetDropManual())
    {
        pIItem->SetDropManual(FALSE);
        NET_Packet P;
        pIItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
        P.w_u16(u16(pIItem->object().ID()));
        pIItem->object().u_EventSend(P);
    } // dropManual
}