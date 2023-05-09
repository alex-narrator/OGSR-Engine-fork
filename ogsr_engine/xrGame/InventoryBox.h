#pragma once
#include "inventory_space.h"
#include "GameObject.h"

class IInventoryBox
{
protected:
    xr_vector<u16> m_items;

    void ProcessEvent(CGameObject* O, NET_Packet& P, u16 type);
    bool b_opened{true};
    int m_iItemsLimit{};

public:
    bool m_in_use{};
    IInventoryBox();
    void AddAvailableItems(TIItemContainer& items_container) const;
    bool IsEmpty() const;
    u32 GetSize() const;
    CScriptGameObject* GetObjectByName(LPCSTR);
    CScriptGameObject* GetObjectByIndex(u32);

    virtual CGameObject* cast_game_object() { return NULL; };
    virtual CInventoryItem* cast_inventory_item() { return NULL; }
    virtual CGameObject& object() = 0;
    virtual bool IsOpened() const { return b_opened; }
    virtual void SetOpened(bool opened) { b_opened = opened; }

    virtual bool CanTakeItem(CInventoryItem*) const;

    virtual xr_vector<u16> GetItems() const { return m_items; };
    virtual int GetItemsLimit() const { return m_iItemsLimit; };

    virtual void RepackAmmo();
};

template <class Based>
class CCustomInventoryBox : public Based, public IInventoryBox
{
    typedef Based inherited;
    typedef IInventoryBox inherited2;

public:
    virtual void OnEvent(NET_Packet& P, u16 type)
    {
        inherited::OnEvent(P, type);
        ProcessEvent(smart_cast<CGameObject*>(this), P, type);
    };

    virtual BOOL net_Spawn(CSE_Abstract* DC)
    {
        inherited::net_Spawn(DC);
        inherited::setVisible(TRUE);
        inherited::setEnabled(TRUE);
        return TRUE;
    }

    virtual CGameObject& object() { return *smart_cast<CGameObject*>(this); }
};

class CInventoryBox : public CCustomInventoryBox<CGameObject> // CBasicInventoryBox
{
public:
    CInventoryBox(){};

    virtual void shedule_Update(u32 dt);

protected:
    void UpdateDropTasks();
    void UpdateDropItem(PIItem pIItem);
};