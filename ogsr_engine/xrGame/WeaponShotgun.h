#pragma once

#include "weaponcustompistol.h"
#include "script_export_space.h"

class CWeaponShotgun : public CWeaponCustomPistol
{
    typedef CWeaponCustomPistol inherited;

public:
    CWeaponShotgun(void);
    virtual ~CWeaponShotgun(void) {};

    virtual void Load(LPCSTR section);

    virtual void Reload();
    virtual void Fire2Start();
    virtual void Fire2End();
    virtual void OnShot();
    virtual void OnShotBoth();
    virtual void switch2_Fire();
    virtual void switch2_Fire2();

    virtual void UpdateCL();

    virtual bool Action(s32 cmd, u32 flags);

    virtual bool Attach(PIItem pIItem, bool b_send_event);
    virtual bool Detach(const char* item_section_name, bool b_spawn_item, float item_condition = 1.f);
    virtual bool CanAttach(PIItem pIItem);
    virtual bool CanDetach(const char* item_section_name);
    virtual void InitAddons();

    virtual bool CanBeReloaded();

protected:
    virtual void OnAnimationEnd(u32 state);
    void TriStateReload();
    virtual void OnStateSwitch(u32 S, u32 oldState);

    bool HaveCartridgeInInventory(u8 cnt);
    virtual u8 AddCartridge(u8 cnt);
    virtual void ReloadMagazine();

    virtual void PlayAnimShutter();
    virtual void PlayAnimShutterMisfire();

    bool m_stop_triStateReload{};
    bool has_anm_reload_jammed{};
    bool SecondCartridge{};
    bool StartCartridge{};
    bool m_bIsTriStateReloadPartly{};

    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponShotgun)
#undef script_type_list
#define script_type_list save_type_list(CWeaponShotgun)
