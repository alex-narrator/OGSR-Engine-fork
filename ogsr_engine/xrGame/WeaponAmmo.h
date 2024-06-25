#pragma once
#include "inventory_item_object.h"
#include "script_export_space.h"

class CCartridge
{
public:
    CCartridge();
    void Load(LPCSTR section, u8 LocalAmmoType);
    virtual float Weight() const;

    shared_str m_ammoSect{};
    enum
    {
        cfTracer = (1 << 0),
        cfRicochet = (1 << 1),
        cfCanBeUnlimited = (1 << 2),
        cfExplosive = (1 << 3),
    };
    float m_kDist{}, m_kDisp{}, m_kHit{}, m_kImpulse{}, m_kPierce{}, m_kAP{}, m_kAirRes{}, m_kSpeed{}, m_impair{}, fWallmarkSize{}, m_misfireProbability{};

    int m_buckShot{1};

    u8 m_u8ColorID{};
    u8 m_LocalAmmoType{};

    u16 bullet_material_idx{u16(-1)};
    Flags8 m_flags;

    shared_str m_InvShortName;
    RStringVec m_ExplodeParticles;
};

class CWeaponAmmo : public CInventoryItemObject
{
    typedef CInventoryItemObject inherited;

public:
    CWeaponAmmo(void);
    virtual ~CWeaponAmmo(void);

    virtual CWeaponAmmo* cast_weapon_ammo() { return this; }
    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(CSE_Abstract* E);
    virtual void OnH_B_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void UpdateCL();
    virtual void renderable_Render();

    virtual bool Useful() const;
    virtual float Weight() const;

    virtual u32 Cost() const;
    bool Get(CCartridge& cartridge);

    float m_kDist{}, m_kDisp{}, m_kHit{}, m_kImpulse{}, m_kPierce{}, m_kAP{}, m_kAirRes{}, m_kSpeed{}, m_impair{}, fWallmarkSize{},
        // вірогідність затримки набою
        m_misfireProbability{},
        // вірогідність затримки магазину
        m_misfireProbabilityBox{};

    int m_buckShot{1};

    u8 m_u8ColorID{};

    u16 m_boxSize{};
    u16 m_boxCurr{};
    bool m_tracer{};
    //
    shared_str m_ammoSect{}, m_InvShortName{};

    xr_vector<shared_str> m_ammoTypes{};
    u8 m_cur_ammo_type{};
    virtual bool IsBoxReloadable() const;
    void ReloadBox(LPCSTR ammo_sect);
    void UnloadBox();
    void SpawnAmmo(u32 boxCurr = 0xffffffff, LPCSTR ammoSect = NULL);
    bool IsDirectReload(CWeaponAmmo*);

    LPCSTR GetAmmoSect() { return m_ammoSect.c_str(); }

public:
    virtual CInventoryItem* can_make_killing(const CInventory* inventory) const;

    bool UsefulForReload() const;

public:
    DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CWeaponAmmo)
#undef script_type_list
#define script_type_list save_type_list(CWeaponAmmo)