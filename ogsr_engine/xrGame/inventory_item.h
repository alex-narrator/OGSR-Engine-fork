////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item.h
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "inventory_space.h"
#include "hit_immunity.h"
#include "attachable_item.h"
#include "ui\UIIconParams.h"

class CUIInventoryCellItem;

enum EHandDependence
{
    hdNone = 0,
    hd1Hand = 1,
    hd2Hand = 2
};

class CSE_Abstract;
class CGameObject;
class CFoodItem;
class CMissile;
class CHudItem;
class CWeaponAmmo;
class CWeapon;
class CPhysicsShellHolder;
class NET_Packet;
class CEatableItem;
struct SPHNetState;
class CInventoryOwner;

struct SHit;

class CInventoryItem : public CAttachableItem,
                       public CHitImmunity
#ifdef DEBUG
    ,
                       public pureRender
#endif
{
private:
    typedef CAttachableItem inherited;

public:
    enum EIIFlags
    {
        FdropManual = (1 << 0),
        FCanTake = (1 << 1),
        FCanTrade = (1 << 2),
        Fbelt = (1 << 3),
        Fruck = (1 << 4),
        FRuckDefault = (1 << 5),
        FUsingCondition = (1 << 6),
        FAllowSprint = (1 << 7),
        Fuseful_for_NPC = (1 << 8),
        FInInterpolation = (1 << 9),
        FInInterpolate = (1 << 10),
        FIsQuestItem = (1 << 11),
        FIAlwaysUntradable = (1 << 12),
        FIUngroupable = (1 << 13),
        FIHiddenForInventory = (1 << 14),
        Fvest = (1 << 15),
    };
    const u32 ClrEquipped = READ_IF_EXISTS(pSettings, r_color, "dragdrop", "color_equipped", color_argb(255, 255, 225, 0));
    const u32 ClrUntradable = READ_IF_EXISTS(pSettings, r_color, "dragdrop", "color_untradable", color_argb(255, 124, 0, 0));
    Flags16 m_flags;
    CIconParams m_icon_params;

public:
    CInventoryItem();
    virtual ~CInventoryItem();

public:
    virtual void Load(LPCSTR section);

    virtual LPCSTR Name();
    virtual LPCSTR NameShort();
    //.	virtual LPCSTR				NameComplex			();
    shared_str ItemDescription() { return m_Description; }
    virtual void GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count) /* {}*/;
    virtual bool NeedBriefInfo() { return m_need_brief_info; };

    virtual void OnEvent(NET_Packet& P, u16 type);

    virtual bool Useful() const; // !!! Переопределить. (см. в Inventory.cpp)
    virtual bool Attach(PIItem, bool);
    virtual bool Detach(PIItem pIItem) { return false; }
    // при детаче спаунится новая вещь при заданно названии секции
    virtual bool Detach(const char* item_section_name, bool b_spawn_item, float item_condition = 1.f);
    virtual bool CanAttach(PIItem);
    virtual bool CanDetach(const char*);
    virtual void DetachAll();

    virtual EHandDependence HandDependence() const { return eHandDependence; };
    virtual bool IsSingleHanded() const { return m_bIsSingleHanded; };
    virtual bool Activate(bool = false); // !!! Переопределить. (см. в Inventory.cpp)
    virtual void Deactivate(bool = false); // !!! Переопределить. (см. в Inventory.cpp)
    virtual bool Action(s32 cmd, u32 flags) { return false; } // true если известная команда, иначе false

    virtual void OnH_B_Chield();
    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void OnH_A_Independent();

    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);
    virtual BOOL net_SaveRelevant() { return TRUE; }

    virtual void UpdateCL();

    virtual void Hit(SHit* pHDS);

    BOOL GetDropManual() const { return m_flags.test(FdropManual); }
    void SetDropManual(BOOL val) { m_flags.set(FdropManual, val); }

    BOOL IsInvalid() const;

    BOOL IsQuestItem() const { return m_flags.test(FIsQuestItem); }

    virtual u32 Cost() const; //{ return m_cost;	}
    virtual void SetCost(u32 cost) { m_cost = cost; }

    virtual float Weight() const; //	{ return m_weight;	}
    virtual void SetWeight(float w) { m_weight = w; }

    float m_fRadiationAccumFactor{}; // alpet: скорость появления вторичной радиактивности
    float m_fRadiationAccumLimit{}; // alpet: предел вторичной радиоактивности

    virtual u32 GetSlotEnabled() const { return m_uSlotEnabled; }
    virtual bool IsModule() const { return m_uSlotEnabled != NO_ACTIVE_SLOT; }
    virtual bool IsDropPouch() const { return m_uSlotEnabled == u32(-1); }

public:
    CInventory* m_pCurrentInventory{};

    u32 m_cost;
    float m_weight;
    shared_str m_Description{};
    CUIInventoryCellItem* m_cell_item{};

    shared_str m_name{};
    shared_str m_nameShort{};
    shared_str m_nameComplex;

    EItemPlace m_eItemPlace{};

    bool b_breakable{};

    virtual void OnMoveToSlot(EItemPlace prevPlace);
    virtual void OnMoveToBelt(EItemPlace prevPlace);
    virtual void OnMoveToVest(EItemPlace prevPlace);
    virtual void OnMoveToRuck(EItemPlace prevPlace);
    virtual void OnMoveOut(EItemPlace prevPlace);

    int GetGridWidth() const;
    int GetGridHeight() const;
    const shared_str& GetIconName() const { return m_icon_name; };
    int GetIconIndex() const;
    int GetXPos() const;
    int GetYPos() const;

    bool GetInvShowCondition() const;

    float GetCondition() const { return m_fCondition; }
    void ChangeCondition(float fDeltaCondition);
    virtual void SetCondition(float fNewCondition)
    {
        m_fCondition = fNewCondition;
        ChangeCondition(0.0f);
    }

    u8 selected_slot;
    const xr_vector<u8>& GetSlots() { return m_slots; }
    const char* GetSlotsSect() { return m_slots_sect; }
    void SetSlot(u8 slot); // alpet: реально это SelectSlot
    virtual u8 GetSlot() const;
    u8 BaseSlot() const { return m_slots.empty() ? NO_ACTIVE_SLOT : m_slots.front(); }
    u32 GetSlotsCount() const { return m_slots.size(); }
    bool IsPlaceable(u8 min_slot, u8 max_slot);

    bool Belt() { return !!m_flags.test(Fbelt); }
    void Belt(bool on_belt) { m_flags.set(Fbelt, on_belt); }
    bool Vest() { return !!m_flags.test(Fvest); }
    void Vest(bool on_vest) { m_flags.set(Fvest, on_vest); }
    bool Ruck() { return !!m_flags.test(Fruck); }
    void Ruck(bool on_ruck) { m_flags.set(Fruck, on_ruck); }
    bool RuckDefault() { return !!m_flags.test(FRuckDefault); }

    virtual bool CanTake() const { return !!m_flags.test(FCanTake); }
    virtual bool CanTrade() const;
    virtual bool IsNecessaryItem(CInventoryItem* item);
    virtual bool IsNecessaryItem(const shared_str& item_sect) { return false; };

    virtual void SetDropTime(bool);

protected:
    xr_vector<u8> m_slots;
    LPCSTR m_slots_sect;
    float m_fCondition{1.f};

    float m_fControlInertionFactor;
    shared_str m_icon_name;
    bool m_need_brief_info;

    // 0-используется без участия рук, 1-одна рука, 2-две руки
    EHandDependence eHandDependence;
    bool m_bIsSingleHanded;

    u32 m_uSlotEnabled{NO_ACTIVE_SLOT};

    ////////// network //////////////////////////////////////////////////
public:
    virtual void net_Export(CSE_Abstract* E);

public:
    virtual void activate_physic_shell();

    virtual bool IsSprintAllowed() const { return !!m_flags.test(FAllowSprint); };

    virtual float GetControlInertionFactor() const { return m_fControlInertionFactor; };

protected:
    virtual void UpdateXForm();

public:
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void reload(LPCSTR section);
    virtual void reinit();
    virtual bool can_kill() const;
    virtual CInventoryItem* can_kill(CInventory* inventory) const;
    virtual const CInventoryItem* can_kill(const xr_vector<const CGameObject*>& items) const;
    virtual CInventoryItem* can_make_killing(const CInventory* inventory) const;
    virtual bool ready_to_kill() const;
    IC bool useful_for_NPC() const;

    virtual bool can_be_attached() const override;
#ifdef DEBUG
    virtual void OnRender();
#endif

public:
    virtual DLL_Pure* _construct();
    IC CPhysicsShellHolder& object() const
    {
        VERIFY(m_object);
        return (*m_object);
    }
    virtual void on_activate_physic_shell() { R_ASSERT(0); } // sea

protected:
    float m_holder_range_modifier;
    float m_holder_fov_modifier;

public:
    virtual void modify_holder_params(float& range, float& fov) const;

protected:
    IC CInventoryOwner& inventory_owner() const;

private:
    CPhysicsShellHolder* m_object;

public:
    virtual CInventoryItem* cast_inventory_item() { return this; }
    virtual CAttachableItem* cast_attachable_item() { return this; }
    virtual CPhysicsShellHolder* cast_physics_shell_holder() { return 0; }
    virtual CEatableItem* cast_eatable_item() { return 0; }
    virtual CWeapon* cast_weapon() { return 0; }
    virtual CFoodItem* cast_food_item() { return 0; }
    virtual CMissile* cast_missile() { return 0; }
    virtual CHudItem* cast_hud_item() { return 0; }
    virtual CWeaponAmmo* cast_weapon_ammo() { return 0; }
    virtual CGameObject* cast_game_object() { return 0; };

    bool m_highlight_equipped{};
    bool m_always_ungroupable{};

    virtual void BreakItem();

    string_unordered_map<shared_str, xr_vector<shared_str>> m_script_actions_map;

protected:
    // партікли знищення
    shared_str m_sBreakParticles;
    // звук знищення
    ref_sound sndBreaking;

public:
    enum ItemEffects
    {
        // restore
        eHealthRestoreSpeed,
        ePowerRestoreSpeed,
        eMaxPowerRestoreSpeed,
        eSatietyRestoreSpeed,
        eRadiationRestoreSpeed,
        ePsyHealthRestoreSpeed,
        eAlcoholRestoreSpeed,
        eWoundsHealSpeed,
        // additional
        eAdditionalSprint,
        eAdditionalJump,
        eAdditionalWeight,

        eEffectMax,
    };

    virtual float GetItemEffect(int) const;
    virtual float GetHitTypeProtection(int) const;

    virtual void SetItemEffect(int, float);
    virtual void SetHitTypeProtection(int, float);

    virtual void InitAddons();

    virtual void Switch(bool);
    virtual void Switch();
    virtual bool IsPowerOn() const;

    LPCSTR GetAttachMenuTip() const { return m_sAttachMenuTip; };
    LPCSTR GetDetachMenuTip() const { return m_sDetachMenuTip; };

    virtual LPCSTR GetBoneName(int);
    virtual float GetArmorByBone(int);
    virtual float GetArmorHitFraction();
    virtual bool HasArmorToDisplay(int);

    virtual float GetPowerLoss();

    virtual void Drop();
    void Transfer(u16 from_id, u16 to_id = u16(-1));

    shared_str m_upgrade_icon_sect{};
    Fvector2 m_upgrade_icon_offset{};

protected:
    HitImmunity::HitTypeSVec m_HitTypeProtection;

    svector<float, eEffectMax> m_ItemEffect;

    LPCSTR m_sAttachMenuTip{};
    LPCSTR m_sDetachMenuTip{};

    float m_fPowerLoss{};

public:
    DECLARE_SCRIPT_REGISTER_FUNCTION
};

#include "inventory_item_inline.h"

add_to_type_list(CInventoryItem)
#undef script_type_list
#define script_type_list save_type_list(CInventoryItem)