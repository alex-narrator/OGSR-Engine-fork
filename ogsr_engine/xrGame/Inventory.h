#pragma once
#include "inventory_item.h"

class CInventory;
class CInventoryItem;
class CHudItem;
class CInventoryOwner;

class CInventorySlot
{
public:
    CInventorySlot(){};
    virtual ~CInventorySlot(){};

    bool CanBeActivated() const;
    bool IsBlocked() const;
    bool maySwitchFast() const;
    void setSwitchFast(bool);

    PIItem m_pIItem{};
    bool m_bPersistent{};
    bool m_bVisible{true};
    int m_blockCounter{};
    bool m_maySwitchFast{};
    bool m_bNeedModule{};
};
enum EActivationReason
{
    eGeneral,
    eKeyAction,
    eImportUpdate,
};

typedef xr_vector<CInventorySlot> TISlotArr;

class CInventory
{
    friend class CInventoryScript;

public:
    CInventory();
    virtual ~CInventory(){};

    float TotalWeight() const;
    float CalcTotalWeight();

    void Take(CGameObject* pObj, bool bNotActivate, bool strict_placement);
    bool DropItem(CGameObject* pObj);
    void Clear();

    bool Slot(PIItem pIItem, bool bNotActivate = false);
    bool Belt(PIItem pIItem);
    bool Vest(PIItem pIItem);
    bool Ruck(PIItem pIItem);

    bool InSlot(PIItem pIItem) const;
    bool InBelt(PIItem pIItem) const;
    bool InVest(PIItem pIItem) const;
    bool InRuck(PIItem pIItem) const;

    bool CanPutInSlot(PIItem pIItem) const;
    bool CanPutInSlot(PIItem pIItem, u8 slot) const;
    bool CanPutInBelt(PIItem pIItem) const;
    bool CanPutInVest(PIItem pIItem) const;
    bool CanPutInRuck(PIItem pIItem) const;

    bool CanTakeItem(CInventoryItem* inventory_item) const;

    bool Activate(u32 slot, EActivationReason reason = eGeneral, bool bForce = false, bool now = false);
    void Activate_deffered(u32 slot, u32 _frame);
    PIItem ActiveItem() const { return m_iActiveSlot == NO_ACTIVE_SLOT ? NULL : m_slots[m_iActiveSlot].m_pIItem; }
    PIItem ItemFromSlot(u32 slot) const;
    bool Action(s32 cmd, u32 flags);
    void Update();
    // Ищет на поясе аналогичный IItem
    PIItem Same(const PIItem pIItem, bool bSearchRuck) const;
    // Ищет на поясе IItem для указанного слота
    PIItem SameSlot(const u32 slot, PIItem pIItem, bool bSearchRuck) const;
    PIItem SameGrenade(PIItem pIItem, bool bSearchRuck) const;
    // Ищет на поясе или в рюкзаке IItem с указанным именем (cName())
    PIItem Get(const char* name, bool bSearchRuck) const;
    // Ищет на поясе или в рюкзаке IItem с указанным именем (id)
    PIItem Get(const u16 id, bool bSearchRuck) const;
    // Ищет на поясе или в рюкзаке IItem с указанным CLS_ID
    PIItem Get(CLASS_ID cls_id, bool bSearchRuck) const;
    PIItem GetAny(const char* name) const;

    void Iterate(bool, std::function<bool(const PIItem)>) const;
    void IterateAmmo(bool, std::function<bool(const PIItem)>) const;
    PIItem GetAmmoByLimit(const char* sect, bool forActor, bool limit_max = false, bool include_magazines = false) const;

    // search both (ruck and belt)
    PIItem item(CLASS_ID cls_id) const;

    // get all the items with the same object id
    virtual bool bfCheckForObject(ALife::_OBJECT_ID tObjectID);
    PIItem get_object_by_id(ALife::_OBJECT_ID tObjectID);

    u32 dwfGetObjectCount();
    PIItem tpfGetObjectByIndex(int iIndex);
    PIItem GetItemFromInventory(LPCSTR caItemName);

    bool Eat(PIItem pIItem, CInventoryOwner* eater = nullptr);

    u32 GetActiveSlot() const { return m_iActiveSlot; }

    void SetPrevActiveSlot(u32 ActiveSlot) { m_iPrevActiveSlot = ActiveSlot; }
    u32 GetPrevActiveSlot() const { return m_iPrevActiveSlot; }
    u32 GetNextActiveSlot() const { return m_iNextActiveSlot; }

    void SetActiveSlot(u32 ActiveSlot) { m_iActiveSlot = m_iNextActiveSlot = ActiveSlot; }

    bool IsSlotsUseful() const { return m_bSlotsUseful; }
    void SetSlotsUseful(bool slots_useful) { m_bSlotsUseful = slots_useful; }
    bool IsBeltUseful() const { return m_bBeltUseful; }
    void SetBeltUseful(bool belt_useful) { m_bBeltUseful = belt_useful; }
    bool IsVestUseful() const { return m_bVestUseful; }
    void SetVestUseful(bool vest_useful) { m_bVestUseful = vest_useful; }

    void SetSlotsBlocked(u16 mask, bool bBlock, bool now = false);
    TIItemContainer m_all;
    TIItemContainer m_ruck, m_belt, m_vest;
    TISlotArr m_slots;

    TIItemContainer GetActiveArtefactPlace() const;

    // возвращает все кроме PDA в слоте и болта
    void AddAvailableItems(TIItemContainer& items_container, bool for_trade) const;

    float GetTakeDist() const { return m_fTakeDist; }

    float GetMaxWeight() const { return m_fMaxWeight; }
    void SetMaxWeight(float weight) { m_fMaxWeight = weight; }

    inline CInventoryOwner* GetOwner() const { return m_pOwner; }

    // Объект на который наведен прицел
    PIItem m_pTarget{};

    friend class CInventoryOwner;

    u32 ModifyFrame() const { return m_dwModifyFrame; }
    void InvalidateState() { m_dwModifyFrame = Device.dwFrame; }
    void Items_SetCurrentEntityHud(bool current_entity);
    bool isBeautifulForActiveSlot(CInventoryItem* pIItem);
    bool IsActiveSlotBlocked() const;

    // максимальный вес инвентаря
    float m_fMaxWeight;

protected:
    void UpdateDropTasks();
    void UpdateDropItem(PIItem pIItem);

    // Активный слот и слот который станет активным после смены
    // значения совпадают в обычном состоянии (нет смены слотов)
    u32 m_iActiveSlot{NO_ACTIVE_SLOT};
    u32 m_iNextActiveSlot{NO_ACTIVE_SLOT};
    u32 m_iPrevActiveSlot{NO_ACTIVE_SLOT};
    u32 m_iLoadActiveSlot{NO_ACTIVE_SLOT};
    u32 m_iLoadActiveSlotFrame{u32(-1)};
    EActivationReason m_ActivationSlotReason{};

    CInventoryOwner* m_pOwner{};

    // флаг, показывающий наличие пояса в инвенторе
    bool m_bBeltUseful{};
    bool m_bVestUseful{};
    // флаг, допускающий использование слотов
    bool m_bSlotsUseful{true};

    // текущий вес в инвентаре
    float m_fTotalWeight{-1.f};

    // Максимальное расстояние на котором можно подобрать объект
    float m_fTakeDist;

    // кадр на котором произошло последнее изменение в инвенторе
    u32 m_dwModifyFrame{};

    bool m_drop_last_frame{};

private:
    bool m_bUpdated{};

public:
    PIItem GetSame(const PIItem pIItem, bool bSearchRuck) const; // получаем айтем из всего инвентаря или с пояса
    PIItem GetSameEatable(const PIItem pIItem, bool bSearchRuck) const; // получаем айтем из всего инвентаря или с пояса
    // считаем предметы в рюкзаке или на поясе + в слотах
    virtual u32 GetSameItemCount(LPCSTR caSection, bool SearchRuck);
    // размещение патронов на поясе при разрядке оружия в руках
    void TryAmmoCustomPlacement(CInventoryItem* pIItem);

    u32 BeltWidth() const;
    u32 BeltHeight() const;

    u32 VestWidth() const;
    u32 VestHeight() const;

    void DropBeltToRuck();
    void DropVestToRuck();
    void DropSlotsToRuck(u32 min_slot, u32 max_slot = NO_ACTIVE_SLOT);
    void BackpackItemsTransfer(CInventoryItem*, bool);
    bool IsSlotAllowed(u32) const;
    bool HasModuleForSlot(u32) const;
    bool HasSameModuleEquiped(PIItem) const;
    bool HasDropPouch() const;

    bool activate_slot(u32 slot);
    bool IsAllItemsLoaded() const;
    bool OwnerIsActor() const;
    void RepackAmmo();
};