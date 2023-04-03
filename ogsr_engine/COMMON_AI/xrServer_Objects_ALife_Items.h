////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects items for ALife simulator
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_ALife_ItemsH
#define xrServer_Objects_ALife_ItemsH

#include "xrServer_Objects_ALife.h"
#include "PHSynchronize.h"
#include "inventory_space.h"

#include "character_info_defs.h"
#include "infoportiondefs.h"

#pragma warning(push)
#pragma warning(disable : 4005)

class CSE_ALifeItemAmmo;

SERVER_ENTITY_DECLARE_BEGIN0(CSE_ALifeInventoryItem)
public:
enum
{
    inventory_item_state_enabled = u8(1) << 0,
    inventory_item_angular_null = u8(1) << 1,
    inventory_item_linear_null = u8(1) << 2,
};

union mask_num_items
{
    struct
    {
        u8 num_items : 5;
        u8 mask : 3;
    };
    u8 common;
};

public:
float m_fCondition{1.f};
float m_fMass;
u32 m_dwCost;
s32 m_iHealthValue{};
s32 m_iFoodValue{};
float m_fDeteriorationValue{};
CSE_ALifeObject* m_self{};
u32 m_last_update_time{};
//
float m_fRadiationRestoreSpeed{};
// статус джерела живлення
enum EPowerSourceStatus
{
    ePowerSourceDisabled = 0, // без джерела живлення
    ePowerSourcePermanent = 1, // незнімне джерело
    ePowerSourceAttachable = 2 // від'ємне джерело
};
EPowerSourceStatus m_power_source_status{};
u8 m_cur_power_source{};
float m_fLastTimeCalled{};
bool m_bIsPowerSourceAttached{true};
float m_fPowerLevel{};

CSE_ALifeInventoryItem(LPCSTR caSection);
virtual ~CSE_ALifeInventoryItem();
// we need this to prevent virtual inheritance :-(
virtual CSE_Abstract* base() = 0;
virtual const CSE_Abstract* base() const = 0;
virtual CSE_Abstract* init();
virtual CSE_Abstract* cast_abstract() { return 0; };
virtual CSE_ALifeInventoryItem* cast_inventory_item() { return this; };
virtual u32 update_rate() const;
// end of the virtual inheritance dependant code

IC bool attached() const { return (base()->ID_Parent < 0xffff); }
virtual bool bfUseful();

/////////// network ///////////////
u8 m_u8NumItems{};
SPHNetState State;
///////////////////////////////////
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeInventoryItem)
#define script_type_list save_type_list(CSE_ALifeInventoryItem)

    SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeItem, CSE_ALifeDynamicObjectVisual, CSE_ALifeInventoryItem) bool m_physics_disabled{};

CSE_ALifeItem(LPCSTR caSection);
virtual ~CSE_ALifeItem();
virtual CSE_Abstract* base();
virtual const CSE_Abstract* base() const;
virtual CSE_Abstract* init();
virtual CSE_Abstract* cast_abstract() { return this; };
virtual CSE_ALifeInventoryItem* cast_inventory_item() { return this; };
virtual BOOL Net_Relevant();
virtual void OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItem)
#define script_type_list save_type_list(CSE_ALifeItem)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemTorch, CSE_ALifeItem)
    // флаги
    enum EStats {
        eTorchActive = (1 << 0),
        eAttached = (1 << 1)
    };
bool m_active{};
bool m_attached{};
CSE_ALifeItemTorch(LPCSTR caSection);
virtual ~CSE_ALifeItemTorch();
virtual BOOL Net_Relevant();

SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemTorch)
#define script_type_list save_type_list(CSE_ALifeItemTorch)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemAmmo, CSE_ALifeItem) 
u16 a_elapsed{};
u16 m_boxSize{};
bool m_bNeedFindPlace{};
u8 m_cur_ammo_type{};
xr_vector<shared_str> m_ammoTypes{};

CSE_ALifeItemAmmo(LPCSTR caSection);
virtual ~CSE_ALifeItemAmmo();
virtual CSE_ALifeItemAmmo* cast_item_ammo() { return this; };
virtual bool can_switch_online() const;
virtual bool can_switch_offline() const;
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemAmmo)
#define script_type_list save_type_list(CSE_ALifeItemAmmo)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemWeapon, CSE_ALifeItem)

    //возможность подключения аддонов
    enum EWeaponAddonStatus {
        eAddonDisabled = 0, //нельзя присоеденить
        eAddonPermanent = 1, //постоянно подключено по умолчанию
        eAddonAttachable = 2 //можно присоединять
    };

//текущее состояние аддонов
enum EWeaponAddonState : u16
{
    eWeaponAddonScope = 1 << 0,
    eWeaponAddonGrenadeLauncher = 1 << 1,
    eWeaponAddonSilencer = 1 << 2,
    eWeaponAddonLaser = 1 << 3,
    eWeaponAddonFlashlight = 1 << 4,
    eWeaponAddonStock = 1 << 5,
    eWeaponAddonExtender = 1 << 6,
    eWeaponAddonForend = 1 << 7,
    //
    eWeaponMisfire = 1 << 8,
    eWeaponMagazineAttached = 1 << 9,
    eWeaponLaserOn = 1 << 10,
    eWeaponFlashlightOn = 1 << 11,
    eWeaponGrenadeMode = 1 << 12,
};

EWeaponAddonStatus m_scope_status{};
EWeaponAddonStatus m_silencer_status{};
EWeaponAddonStatus m_grenade_launcher_status{};
EWeaponAddonStatus m_laser_status{};
EWeaponAddonStatus m_flashlight_status{};
EWeaponAddonStatus m_stock_status{};
EWeaponAddonStatus m_extender_status{};
EWeaponAddonStatus m_forend_status{};

u32 timestamp{};
u8 wpn_flags{};
u8 wpn_state{};
u8 ammo_type{};
u16 a_current{90};
u16 a_elapsed{};
float m_fHitPower;
ALife::EHitType m_tHitType;
LPCSTR m_caAmmoSections;
u32 m_dwAmmoAvailable{};
Flags16 m_weapon_flags;
u8 m_bZoom{};
u32 m_ef_main_weapon_type;
u32 m_ef_weapon_type;

u8 m_cur_scope{};
u8 m_cur_silencer{};
u8 m_cur_glauncher{};
u8 m_cur_laser{};
u8 m_cur_flashlight{};
u8 m_cur_stock{};
u8 m_cur_extender{};
u8 m_cur_forend{};
u8 m_cur_magazine{};

CSE_ALifeItemWeapon(LPCSTR caSection);
virtual ~CSE_ALifeItemWeapon();
virtual void OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender);
virtual u32 ef_main_weapon_type() const;
virtual u32 ef_weapon_type() const;
u8 get_slot();
u16 get_ammo_limit();
u16 get_ammo_total();
u16 get_ammo_elapsed();
u16 get_ammo_magsize();

virtual BOOL Net_Relevant();

virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemWeapon)
#define script_type_list save_type_list(CSE_ALifeItemWeapon)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemWeaponMagazined, CSE_ALifeItemWeapon) 
u8 m_u8CurFireMode{};
float m_fAttachedScopeCondition{1.f};
float m_fAttachedGrenadeLauncherCondition{1.f};
float m_fAttachedSilencerCondition{1.f};
xr_vector<u8> m_AmmoIDs;
//u8 m_cur_magazine{};
CSE_ALifeItemWeaponMagazined(LPCSTR caSection);
virtual ~CSE_ALifeItemWeaponMagazined();

virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemWeaponMagazined)
#define script_type_list save_type_list(CSE_ALifeItemWeaponMagazined)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemWeaponMagazinedWGL, CSE_ALifeItemWeaponMagazined) 
u8 ammo_type2{};
u16 a_elapsed2{};
CSE_ALifeItemWeaponMagazinedWGL(LPCSTR caSection);
virtual ~CSE_ALifeItemWeaponMagazinedWGL();

virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemWeaponMagazinedWGL)
#define script_type_list save_type_list(CSE_ALifeItemWeaponMagazinedWGL)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemWeaponShotGun, CSE_ALifeItemWeaponMagazined) 
CSE_ALifeItemWeaponShotGun(LPCSTR caSection);
virtual ~CSE_ALifeItemWeaponShotGun();

virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemWeaponShotGun)
#define script_type_list save_type_list(CSE_ALifeItemWeaponShotGun)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemDetector, CSE_ALifeItem) u32 m_ef_detector_type;
CSE_ALifeItemDetector(LPCSTR caSection);
virtual ~CSE_ALifeItemDetector();
virtual u32 ef_detector_type() const;
virtual CSE_ALifeItemDetector* cast_item_detector() { return this; }
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemDetector)
#define script_type_list save_type_list(CSE_ALifeItemDetector)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemArtefact, CSE_ALifeItem) 
float m_fAnomalyValue{100.f};
float m_fRandomK{1.f};
CSE_ALifeItemArtefact(LPCSTR caSection);
virtual ~CSE_ALifeItemArtefact();
virtual BOOL Net_Relevant();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemArtefact)
#define script_type_list save_type_list(CSE_ALifeItemArtefact)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemPDA, CSE_ALifeItem) 
u16 m_original_owner{0xffff};
shared_str m_specific_character{};
shared_str m_info_portion{};

CSE_ALifeItemPDA(LPCSTR caSection);
virtual ~CSE_ALifeItemPDA();
virtual CSE_ALifeItemPDA* cast_item_pda() { return this; };
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemPDA)
#define script_type_list save_type_list(CSE_ALifeItemPDA)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemDocument, CSE_ALifeItem) shared_str m_wDoc{};
CSE_ALifeItemDocument(LPCSTR caSection);
virtual ~CSE_ALifeItemDocument();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemDocument)
#define script_type_list save_type_list(CSE_ALifeItemDocument)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemGrenade, CSE_ALifeItem) 
u32 m_ef_weapon_type;
u32 m_dwDestroyTimeMax{}; // время до взырва гранаты
CSE_ALifeItemGrenade(LPCSTR caSection);
virtual ~CSE_ALifeItemGrenade();
virtual u32 ef_weapon_type() const;
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemGrenade)
#define script_type_list save_type_list(CSE_ALifeItemGrenade)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemExplosive, CSE_ALifeItem) CSE_ALifeItemExplosive(LPCSTR caSection);
virtual ~CSE_ALifeItemExplosive();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemExplosive)
#define script_type_list save_type_list(CSE_ALifeItemExplosive)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemBolt, CSE_ALifeItem) u32 m_ef_weapon_type;
CSE_ALifeItemBolt(LPCSTR caSection);
virtual ~CSE_ALifeItemBolt();
virtual bool can_save() const;
virtual bool used_ai_locations() const;
virtual u32 ef_weapon_type() const;
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemBolt)
#define script_type_list save_type_list(CSE_ALifeItemBolt)

    SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemCustomOutfit, CSE_ALifeItem) u32 m_ef_equipment_type;
CSE_ALifeItemCustomOutfit(LPCSTR caSection);
virtual ~CSE_ALifeItemCustomOutfit();
virtual u32 ef_equipment_type() const;
virtual BOOL Net_Relevant();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemCustomOutfit)
#define script_type_list save_type_list(CSE_ALifeItemCustomOutfit)

    class CSE_InventoryContainer : public CSE_InventoryBoxAbstract,
                                   public CSE_ALifeItem
{
public:
    CSE_InventoryContainer(LPCSTR caSection) : CSE_ALifeItem(caSection){};
    virtual ~CSE_InventoryContainer(){};
    virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries)
    {
        add_offline_impl(smart_cast<CSE_ALifeDynamicObjectVisual*>(this), saved_children, update_registries);
        CSE_ALifeItem::add_offline(saved_children, update_registries);
    }
    virtual void add_online(const bool& update_registries)
    {
        add_online_impl(smart_cast<CSE_ALifeDynamicObjectVisual*>(this), update_registries);
        CSE_ALifeItem::add_online(update_registries);
    };
};

// KRodin: Закомментировал, попытка предотвратить повторную регистрацию cse_alife_item в луабинде.
// По идее, оно и не нужно, ведь у класса CSE_InventoryContainer нету метода ::script_register()
// add_to_type_list(CSE_InventoryContainer)
//#define script_type_list save_type_list(CSE_InventoryContainer)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemNightVisionDevice, CSE_ALifeItem)
// флаги
enum EStats
{
    eNightVisionActive = (1 << 0),
    eAttached = (1 << 1)
};
bool m_nightvision_active{};
bool m_attached{};
CSE_ALifeItemNightVisionDevice(LPCSTR caSection);
virtual ~CSE_ALifeItemNightVisionDevice();
virtual BOOL Net_Relevant();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemNightVisionDevice)
#define script_type_list save_type_list(CSE_ALifeItemNightVisionDevice)
SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemEatable, CSE_ALifeItem) CSE_ALifeItemEatable(LPCSTR);
virtual ~CSE_ALifeItemEatable();
virtual BOOL Net_Relevant();
public:
s32 m_portions_num{1};
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemEatable)
#define script_type_list save_type_list(CSE_ALifeItemEatable)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemVest, CSE_ALifeItem) 
bool m_bIsPlateInstalled{};
u8 m_cur_plate{};
CSE_ALifeItemVest(LPCSTR caSection);
virtual ~CSE_ALifeItemVest();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemVest)
#define script_type_list save_type_list(CSE_ALifeItemVest)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemPowerBattery, CSE_ALifeItem)
CSE_ALifeItemPowerBattery(LPCSTR caSection);
virtual ~CSE_ALifeItemPowerBattery();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemPowerBattery)
#define script_type_list save_type_list(CSE_ALifeItemPowerBattery)

#pragma warning(pop)

#endif