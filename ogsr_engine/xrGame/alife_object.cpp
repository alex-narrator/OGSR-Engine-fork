////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_object.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife object class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife.h"
#include "alife_simulator.h"
#include "xrServer_Objects_ALife_Items.h"

void CSE_ALifeObject::spawn_supplies() { spawn_supplies(*m_ini_string); }

void CSE_ALifeObject::spawn_supplies(LPCSTR ini_string)
{
    if (!ini_string)
        return;

    if (!xr_strlen(ini_string))
        return;

    IReader r((void*)(ini_string), strlen(ini_string));
    CInifile ini(&r, FS.get_path("$game_config$")->m_Path);

    if (ini.section_exist("spawn"))
    {
        LPCSTR N, V;
        float p;
        for (u32 k = 0, j; ini.r_line("spawn", k, &N, &V); k++)
        {
            VERIFY(xr_strlen(N));

            float f_cond{1.0f};
            bool bScope{}, bSilencer{}, bLauncher{}, bLaser{}, bFlashlight{}, bStock{}, bExtender{}, bForend{}, bMagazine{}, bAmmo{};

            u32 cur_scope{}, cur_silencer{}, cur_launcher{}, cur_laser{}, cur_flashlight{}, cur_stock{}, cur_extender{}, cur_forend{}, cur_magazine{}, cur_ammo_type{};

            j = 1;
            p = 1.f;

            if (V && xr_strlen(V))
            {
                string64 buf;
                j = atoi(_GetItem(V, 0, buf));
                if (!j)
                    j = 1;

                bScope = (NULL != strstr(V, "scope"));
                bSilencer = (NULL != strstr(V, "silencer"));
                bLauncher = (NULL != strstr(V, "launcher"));
                bLaser = (NULL != strstr(V, "laser"));
                bFlashlight = (NULL != strstr(V, "flashlight"));
                bStock = (NULL != strstr(V, "stock"));
                bExtender = (NULL != strstr(V, "extender"));
                bForend = (NULL != strstr(V, "forend"));
                bMagazine = (NULL != strstr(V, "magazine"));
                bAmmo = (NULL != strstr(V, "ammo"));
                // preloaded ammo type
                if (NULL != strstr(V, "ammo="))
                    cur_ammo_type = (u32)atof(strstr(V, "ammo=") + 5);
                // custom multi-addon to install
                if (NULL != strstr(V, "scope="))
                    cur_scope = (u32)atof(strstr(V, "scope=") + 6);
                if (NULL != strstr(V, "silencer="))
                    cur_silencer = (u32)atof(strstr(V, "silencer=") + 9);
                if (NULL != strstr(V, "launcher="))
                    cur_launcher = (u32)atof(strstr(V, "launcher=") + 9);
                if (NULL != strstr(V, "laser="))
                    cur_laser = (u32)atof(strstr(V, "laser=") + 6);
                if (NULL != strstr(V, "flashlight="))
                    cur_flashlight = (u32)atof(strstr(V, "flashlight=") + 11);
                if (NULL != strstr(V, "stock="))
                    cur_stock = (u32)atof(strstr(V, "stock=") + 6);
                if (NULL != strstr(V, "extender="))
                    cur_extender = (u32)atof(strstr(V, "extender=") + 9);
                if (NULL != strstr(V, "forend="))
                    cur_forend = (u32)atof(strstr(V, "forend=") + 7);
                if (NULL != strstr(V, "magazine="))
                    cur_magazine = (u32)atof(strstr(V, "magazine=") + 9);
                // probability
                if (NULL != strstr(V, "prob="))
                    p = (float)atof(strstr(V, "prob=") + 5);
                if (fis_zero(p))
                    p = 1.0f;
                if (NULL != strstr(V, "cond="))
                    f_cond = (float)atof(strstr(V, "cond=") + 5);
            }
            for (u32 i = 0; i < j; ++i)
            {
                if (randF(1.f) < p)
                {
                    CSE_Abstract* E = alife().spawn_item(N, o_Position, m_tNodeID, m_tGraphID, ID);

                    CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);
                    if (IItem)
                        IItem->m_fCondition = f_cond;

                    if (auto ammo = smart_cast<CSE_ALifeItemAmmo*>(E); bAmmo && ammo && !ammo->m_ammoTypes.empty())
                    {
                        ammo->m_cur_ammo_type = cur_ammo_type;
                        ammo->a_elapsed = ammo->m_boxSize;
                    }

                    // подсоединить аддоны к оружию, если включены соответствующие флажки
                    CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);
                    if (W)
                    {
                        W->ammo_type = cur_ammo_type;

                        if (W->m_scope_status == CSE_ALifeItemWeapon::eAddonAttachable)
                        {
                            if (pSettings->line_exist(N, "scope_installed"))
                            {
                                bScope = true;
                                cur_scope = pSettings->r_u8(N, "scope_installed");
                            }
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
                            W->m_cur_scope = cur_scope;
                        }
                        if (W->m_silencer_status == CSE_ALifeItemWeapon::eAddonAttachable)
                        {
                            if (pSettings->line_exist(N, "silencer_installed"))
                            {
                                bSilencer = true;
                                cur_silencer = pSettings->r_u8(N, "silencer_installed");
                            }
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
                            W->m_cur_silencer = cur_silencer;
                        }
                        if (W->m_grenade_launcher_status == CSE_ALifeItemWeapon::eAddonAttachable)
                        {
                            if (pSettings->line_exist(N, "launcher_installed"))
                            {
                                bLauncher = true;
                                cur_launcher = pSettings->r_u8(N, "launcher_installed");
                            }
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);
                            W->m_cur_glauncher = cur_launcher;
                        }
                        if (W->m_laser_status == CSE_ALifeItemWeapon::eAddonAttachable)
                        {
                            if (pSettings->line_exist(N, "laser_installed"))
                            {
                                bLaser = true;
                                cur_laser = pSettings->r_u8(N, "laser_installed");
                            }
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonLaser, bLaser);
                            W->m_cur_laser = cur_laser;
                        }
                        if (W->m_flashlight_status == CSE_ALifeItemWeapon::eAddonAttachable)
                        {
                            if (pSettings->line_exist(N, "flashlight_installed"))
                            {
                                bFlashlight = true;
                                cur_flashlight = pSettings->r_u8(N, "flashlight_installed");
                            }
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonFlashlight, bFlashlight);
                            W->m_cur_flashlight = cur_flashlight;
                        }
                        if (W->m_stock_status == CSE_ALifeItemWeapon::eAddonAttachable)
                        {
                            if (pSettings->line_exist(N, "stock_installed"))
                            {
                                bStock = true;
                                cur_stock = pSettings->r_u8(N, "stock_installed");
                            }
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonStock, bStock);
                            W->m_cur_stock = cur_stock;
                        }
                        if (W->m_extender_status == CSE_ALifeItemWeapon::eAddonAttachable)
                        {
                            if (pSettings->line_exist(N, "extender_installed"))
                            {
                                bExtender = true;
                                cur_extender = pSettings->r_u8(N, "extender_installed");
                            }
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonExtender, bExtender);
                            W->m_cur_extender = cur_extender;
                        }
                        if (W->m_forend_status == CSE_ALifeItemWeapon::eAddonAttachable)
                        {
                            if (pSettings->line_exist(N, "forend_installed"))
                            {
                                bForend = true;
                                cur_forend = pSettings->r_u8(N, "forend_installed");
                            }
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonForend, bForend);
                            W->m_cur_forend = cur_forend;
                        }
                        if (pSettings->line_exist(N, "magazine_class"))
                        {
                            W->m_weapon_flags.set(CSE_ALifeItemWeapon::eWeaponMagazineAttached, true);
                            if (bMagazine)
                                W->m_cur_magazine = cur_magazine;
                        }
                    }
                }
            }
        }
    }
}

bool CSE_ALifeObject::keep_saved_data_anyway() const { return (false); }
