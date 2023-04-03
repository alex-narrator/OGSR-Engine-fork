#include "StdAfx.h"
#include "UIWpnParams.h"
#include "UIXmlInit.h"
#include "../Level.h"
#include "../ai_space.h"
#include "../script_engine.h"
#include "inventory_item.h"
#include "clsid_game.h"
#include "script_game_object.h"
//
#include "../string_table.h"
#include "WeaponMagazinedWGrenade.h"
#include "WeaponBinoculars.h"
#include "WeaponKnife.h"

constexpr auto WPN_PARAMS = "wpn_params.xml";

CUIWpnParams::CUIWpnParams() {}

CUIWpnParams::~CUIWpnParams() {}

void CUIWpnParams::Init()
{
    CUIXml uiXml;
    uiXml.Init(CONFIG_PATH, UI_PATH, WPN_PARAMS);

    CUIXmlInit::InitWindow(uiXml, "wpn_params", 0, this);
    //
    AttachChild(&m_CapInfo);
    CUIXmlInit::InitStatic(uiXml, "wpn_params:cap_info", 0, &m_CapInfo);
}

void CUIWpnParams::Reinit()
{
    m_CapInfo.DetachAll();
    DetachAll();
    Init();
}

CUIStatic* CUIWpnParams::SetStaticParams(CUIXml& _xml, const char* _path, float _h)
{
    CUIStatic* _static = xr_new<CUIStatic>();
    CUIXmlInit::InitStatic(_xml, _path, 0, _static);
    _static->SetAutoDelete(true);
    _static->SetWndPos(_static->GetPosLeft(), _static->GetPosTop() + _h);
    m_CapInfo.AttachChild(_static);
    return _static;
}

void CUIWpnParams::SetInfo(CInventoryItem* obj)
{
    Reinit();

    const shared_str& item_section = obj->object().cNameSect();

    string1024 text_to_show;
    auto pWeapon = smart_cast<CWeapon*>(obj);
    auto pWeaponMag = smart_cast<CWeaponMagazined*>(obj);
    auto pWeaponMagWGren = smart_cast<CWeaponMagazinedWGrenade*>(obj);
    auto pWeaponKnife = smart_cast<CWeaponKnife*>(obj);
    auto pWeaponBinoc = smart_cast<CWeaponBinoculars*>(obj);

    // динамічний лист інформації про зброю
    CUIXml _uiXml;

    // елемент списку
    _uiXml.Init(CONFIG_PATH, UI_PATH, WPN_PARAMS);
    auto marker_ = _uiXml.ReadAttrib("wpn_params:list_item", 0, "marker", "• ");
    float list_item_h = _uiXml.ReadAttribFlt("wpn_params:list_item", 0, "height");
    const char* _path = "wpn_params:list_item";

    LPCSTR _param_name{}, _sn = "";
    float _h{}, _val{};
    // ушкодження
    if (!pWeaponBinoc)
    {
        _val = pWeapon->GetHitPowerForActor();
        _param_name = CStringTable().translate("st_damage").c_str();
        sprintf_s(text_to_show, "%s %.2f", _param_name, _val);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    // дальність вогню
    if (!pWeaponBinoc)
    {
        _val = pSettings->r_float(item_section, "fire_distance");
        _param_name = CStringTable().translate("st_fire_distance").c_str();
        _sn = CStringTable().translate("st_m").c_str();
        sprintf_s(text_to_show, "%s %.1f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    // швидкість кулі
    if (!pWeaponKnife && !pWeaponBinoc)
    {
        _val = pWeapon->GetStartBulletSpeed();
        _param_name = CStringTable().translate("st_bullet_speed").c_str();
        _sn = CStringTable().translate("st_bullet_speed_units").c_str();
        sprintf_s(text_to_show, "%s %.f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    // швидкострільність
    if (!pWeaponKnife && !pWeaponBinoc)
    {
        _val = pSettings->r_float(item_section, "rpm");
        _param_name = CStringTable().translate("st_rpm").c_str();
        _sn = CStringTable().translate("st_rpm_units").c_str();
        sprintf_s(text_to_show, "%s %.f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    // розкид
    if (!pWeaponKnife && !pWeaponBinoc)
    {
        _val = pWeapon->GetFireDispersion(true, false);
        _val = rad2deg(_val) * 60.f; // convert to MOA
        _param_name = CStringTable().translate("st_dispersion").c_str();
        _sn = CStringTable().translate("st_dispersion_units").c_str();
        sprintf_s(text_to_show, "%s %.1f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    if (!pWeaponKnife && !pWeaponBinoc)
    {
        _val = pWeapon->camDispersion * 10.f;
        _param_name = CStringTable().translate("st_recoil").c_str();
        sprintf_s(text_to_show, "%s %.2f", _param_name, _val);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    if (!pWeaponKnife && !pWeaponBinoc)
    {
        _val = pWeapon->GetZoomRotationTime();
        _param_name = CStringTable().translate("st_aim_time").c_str();
        _sn = CStringTable().translate("st_time_second").c_str();
        sprintf_s(text_to_show, "%s %.2f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    // громіздкість
    _val = pWeapon->GetControlInertionFactor();
    _param_name = CStringTable().translate("st_bulkiness").c_str();
    sprintf_s(text_to_show, "%s %.1f", _param_name, _val);
    SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
    _h += list_item_h;
    // ресурс на відмову
    if (!pWeaponBinoc)
    {
        _val = pWeaponKnife ? pWeapon->GetCondDecPerShotOnHit() : pWeapon->GetCondDecPerShotToShow();
        if (!fis_zero(_val))
        {
            _val = 1.f / _val;
            _param_name = CStringTable().translate("st_resource").c_str();
            _sn = pWeaponKnife ? CStringTable().translate("st_resource_on_hit_units").c_str() : CStringTable().translate("st_resource_units").c_str();
            sprintf_s(text_to_show, "%s %.0f %s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }
    // тип спорядженого боєприпасу
    if (!pWeaponKnife && !pWeaponBinoc && (pWeapon->AddonAttachable(eMagazine) && pWeaponMag->IsAddonAttached(eMagazine) || pWeapon->GetAmmoElapsed() || pWeapon->IsGrenadeMode()))
    {
        _param_name = CStringTable().translate("st_current_ammo_type").c_str();
        if (pWeapon->AddonAttachable(eMagazine) && pWeaponMag->IsAddonAttached(eMagazine) && pWeapon->GetAmmoElapsed())
            sprintf_s(text_to_show, "%s %s, %s", _param_name, pWeapon->GetCurrentAmmo_ShortName(), pWeaponMag->GetCurrentMagazine_ShortName(true));
        else if (pWeapon->GetAmmoElapsed())
            sprintf_s(text_to_show, "%s %s", _param_name, pWeapon->GetCurrentAmmo_ShortName());
        else
            sprintf_s(text_to_show, "%s %s", _param_name, pWeaponMag->GetCurrentMagazine_ShortName(true));
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    // поточний приціл
    if (pWeapon->IsAddonAttached(eScope))
    {
        // поточний приціл - заголовок
        _param_name = CStringTable().translate("st_current_scope").c_str();
        SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
        _h += list_item_h;
        // поточний приціл - параметри
        // зум
        _param_name = CStringTable().translate("st_scope_zoom").c_str();
        if (pWeapon->IsScopeDynamicZoom())
        {
            sprintf_s(text_to_show, "%s%s %.1f-%.1fx", marker_, _param_name, pWeapon->GetScopeZoomFactor(), pWeapon->GetMaxScopeZoomFactor());
        }
        else
            sprintf_s(text_to_show, "%s%s %.1fx", marker_, _param_name, pWeapon->GetScopeZoomFactor());
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
        // нічне бачення
        if (pWeaponMag->IsNightVisionEnabled())
        {
            _param_name = CStringTable().translate("st_scope_night_vision").c_str();
            sprintf_s(text_to_show, "%s%s", marker_, _param_name);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        // автозахоплення цілей
        if (pWeaponMag->IsVisionPresent())
        {
            _param_name = CStringTable().translate("st_scope_vision_present").c_str();
            sprintf_s(text_to_show, "%s%s", marker_, _param_name);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        // далекомір
        if (pWeaponMag->HasRangeMeter())
        {
            _param_name = CStringTable().translate("st_scope_range_meter").c_str();
            sprintf_s(text_to_show, "%s%s", marker_, _param_name);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }
    if (!pWeaponKnife && !pWeaponBinoc)
    {
        // сумісні набої - заголовок
        _param_name = CStringTable().translate("st_compatible_ammo").c_str();
        SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
        _h += list_item_h;
        // сумісні набої - список
        for (const auto& ammo : pWeapon->m_ammoTypes)
        {
            if (pSettings->line_exist(ammo, "empty_box")) // магазини покажемо окремо нижче
                continue;
            auto ammo_name = pSettings->r_string(ammo, "inv_name");
            sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(ammo_name).c_str());
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        // сумісні набої грантометів - список
        if (pWeapon->IsAddonAttached(eLauncher))
        {
            for (const auto& ammo : pWeaponMagWGren->m_ammoTypes2)
            {
                auto ammo_name = pSettings->r_string(ammo, "inv_name");
                sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(ammo_name).c_str());
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
        }
        // сумісні магазини
        if (pWeapon->AddonAttachable(eMagazine))
        {
            // сумісні магазини - заголовок
            _param_name = CStringTable().translate("st_compatible_magazine").c_str();
            SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
            _h += list_item_h;
            // сумісні магазини - список
            for (const auto& magazine : pWeapon->m_magazines)
            {
                auto magazine_name = pSettings->r_string(magazine, "inv_name");
                sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(magazine_name).c_str());
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
        }

        bool has_addon = pWeapon->AddonAttachable(eScope) || pWeapon->AddonAttachable(eSilencer) || pWeapon->AddonAttachable(eLauncher) || pWeapon->AddonAttachable(eLaser) ||
            pWeapon->AddonAttachable(eFlashlight) || pWeapon->AddonAttachable(eStock) || pWeapon->AddonAttachable(eExtender);

        if (has_addon)
        {
            // адони - заголовок
            _param_name = CStringTable().translate("st_compatible_addon").c_str();
            SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
            _h += list_item_h;
            if (pWeapon->AddonAttachable(eScope))
            {
                // сумісні приціли - список
                for (const auto& scope : pWeapon->m_scopes)
                {
                    auto scope_name = pSettings->r_string(scope, "inv_name");
                    sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(scope_name).c_str());
                    SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                    _h += list_item_h;
                }
            }
            // сумісні глушники
            if (pWeapon->AddonAttachable(eSilencer))
            {
                // глушник - список
                for (const auto& silencer : pWeapon->m_silencers)
                {
                    auto silencer_name = pSettings->r_string(silencer, "inv_name");
                    sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(silencer_name).c_str());
                    SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                    _h += list_item_h;
                }
            }
            // сумісні гранатомети
            if (pWeapon->AddonAttachable(eLauncher))
            {
                // сумісні гранатомети - список
                for (const auto& glauncher : pWeapon->m_glaunchers)
                {
                    auto glauncher_name = pSettings->r_string(glauncher, "inv_name");
                    sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(glauncher_name).c_str());
                    SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                    _h += list_item_h;
                }
            }
            // сумісні ЛЦВ
            if (pWeapon->AddonAttachable(eLaser))
            {
                // сумісні ЛЦВ - список
                for (const auto& laser : pWeapon->m_lasers)
                {
                    auto laser_name = pSettings->r_string(laser, "inv_name");
                    sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(laser_name).c_str());
                    SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                    _h += list_item_h;
                }
            }
            // сумісні ліхтарі
            if (pWeapon->AddonAttachable(eFlashlight))
            {
                // сумісні ліхтарі - список
                for (const auto& flashlight : pWeapon->m_flashlights)
                {
                    auto flashlight_name = pSettings->r_string(flashlight, "inv_name");
                    sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(flashlight_name).c_str());
                    SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                    _h += list_item_h;
                }
            }
            // сумісні приклади
            if (pWeapon->AddonAttachable(eStock))
            {
                // сумісні приклади - список
                for (const auto& stock : pWeapon->m_stocks)
                {
                    auto stock_name = pSettings->r_string(stock, "inv_name");
                    sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(stock_name).c_str());
                    SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                    _h += list_item_h;
                }
            }
            // сумісні подовжувачі магазину
            if (pWeapon->AddonAttachable(eExtender))
            {
                // сумісні приклади - список
                for (const auto& extender : pWeapon->m_extenders)
                {
                    auto extender_name = pSettings->r_string(extender, "inv_name");
                    sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(extender_name).c_str());
                    SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                    _h += list_item_h;
                }
            }
        }
    }

    SetHeight(_h);
}

bool CUIWpnParams::Check(CInventoryItem* obj)
{
    if (!READ_IF_EXISTS(pSettings, r_bool, obj->object().cNameSect(), "show_wpn_properties", true)) // allow to suppress default wpn params
    {
        return false;
    }

    return obj->cast_weapon();
}