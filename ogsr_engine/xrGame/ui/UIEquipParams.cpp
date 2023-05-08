/////////////////////////////////////////////////////////////
// різноманітна кастомна інформація про
// предмети спорядження, обвіс, магазини
/////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UIEquipParams.h"
#include "UIXmlInit.h"

#include "string_table.h"

#include "inventory_item.h"
#include "CustomOutfit.h"
#include "Helmet.h"
#include "Warbelt.h"
#include "Vest.h"
#include "WeaponAmmo.h"
#include "Addons.h"
#include "PowerBattery.h"
#include "InventoryContainer.h"
#include "Grenade.h"
#include "CustomDetector.h"

constexpr auto equip_params = "equip_params.xml";

void CUIEquipParams::Init()
{
    CUIXml uiXml;
    uiXml.Init(CONFIG_PATH, UI_PATH, equip_params);
    AttachChild(&m_CapInfo);
    CUIXmlInit::InitStatic(uiXml, "equip_params:cap_info", 0, &m_CapInfo);
}

CUIStatic* CUIEquipParams::SetStaticParams(CUIXml& _xml, const char* _path, float _h)
{
    CUIStatic* _static = xr_new<CUIStatic>();
    CUIXmlInit::InitStatic(_xml, _path, 0, _static);
    _static->SetAutoDelete(true);
    _static->SetWndPos(_static->GetPosLeft(), _static->GetPosTop() + _h);
    m_CapInfo.AttachChild(_static);
    return _static;
}

bool CUIEquipParams::Check(CInventoryItem* obj)
{
    if (smart_cast<CWeaponAmmo*>(obj) || smart_cast<CWarbelt*>(obj) || smart_cast<CVest*>(obj) || smart_cast<CCustomOutfit*>(obj) || smart_cast<CHelmet*>(obj) ||
        smart_cast<CScope*>(obj) || smart_cast<CSilencer*>(obj) || smart_cast<CStock*>(obj) || smart_cast<CExtender*>(obj) || smart_cast<CForend*>(obj) ||
        smart_cast<CInventoryContainer*>(obj) || smart_cast<CPowerBattery*>(obj) || smart_cast<CGrenade*>(obj) || smart_cast<CCustomDetector*>(obj) || 
        obj->IsPowerConsumer() || obj->GetDetailPartSection() ||
        !obj->m_repair_items.empty() || !obj->m_required_tools.empty() || !fis_zero(obj->repair_condition_gain) || obj->repair_count || !fis_zero(obj->repair_condition_threshold))
    {
        return true;
    }
    else
        return false;
}

void CUIEquipParams::SetInfo(CInventoryItem* obj)
{
    m_CapInfo.DetachAll();

    const shared_str& item_section = obj->object().cNameSect();

    string1024 text_to_show{};

    // динамічний лист інформації
    CUIXml _uiXml;

    // елемент списку
    _uiXml.Init(CONFIG_PATH, UI_PATH, equip_params);
    auto marker_ = _uiXml.ReadAttrib("equip_params:list_item", 0, "marker", "• ");
    float list_item_h = _uiXml.ReadAttribFlt("equip_params:list_item", 0, "height");
    const char* _path = "equip_params:list_item";

    float _h{}, _val{};
    LPCSTR _param_name{}, _sn = "%";

    if (obj->IsPowerConsumer() && obj->IsPowerSourceAttached() || smart_cast<CPowerBattery*>(obj))
    {
        _val = obj->GetPowerLevelToShow();
        _param_name = CStringTable().translate("st_power_level").c_str();
        sprintf_s(text_to_show, "%s %.0f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;

        _val = obj->GetPowerCapacity();
        _param_name = CStringTable().translate("st_power_capacity").c_str();
        _sn = CStringTable().translate("st_power_capacity_units").c_str();
        sprintf_s(text_to_show, "%s %.0f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    if (obj->IsPowerConsumer())
    {
        _val = obj->m_fPowerConsumption;
        _param_name = CStringTable().translate("st_power_consumption").c_str();
        _sn = CStringTable().translate("st_power_consumption_units").c_str();
        sprintf_s(text_to_show, "%s %.0f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }

    if (auto det_cop = smart_cast<CCustomDetector*>(obj))
    {
        _val = det_cop->m_fDetectRadius;
        _param_name = CStringTable().translate("st_detect_radius").c_str();
        _sn = CStringTable().translate("st_m").c_str();
        sprintf_s(text_to_show, "%s %.0f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }

    _sn = "%";
    auto pSilencer = smart_cast<CSilencer*>(obj);
    if (pSilencer)
    {
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "bullet_hit_power_k", 0.f);
        if (!fis_zero(_val))
        {
            _val *= 100.f;
            _param_name = CStringTable().translate("st_damage").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "bullet_speed_k", 0.f);
        if (!fis_zero(_val))
        {
            _val *= 100.f;
            _param_name = CStringTable().translate("st_bullet_speed").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        float _val = READ_IF_EXISTS(pSettings, r_float, item_section, "fire_dispersion_base_k", 0.f);
        if (!fis_zero(_val))
        {
            _val *= 100.f;
            _param_name = CStringTable().translate("st_dispersion").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "cam_dispersion_k", 0.f);
        if (!fis_zero(_val))
        {
            _val *= 100.f;
            _param_name = CStringTable().translate("st_recoil").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "condition_shot_dec_silencer", 0.f);
        if (!fis_zero(_val))
        {
            _val *= 100.f;
            _param_name = CStringTable().translate("st_weapon_dec").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "condition_shot_dec", 0.f);
        if (!fis_zero(_val))
        {
            _sn = CStringTable().translate("st_resource_units").c_str();
            _val = 1.f / _val;
            _param_name = CStringTable().translate("st_resource").c_str();
            sprintf_s(text_to_show, "%s %.0f %s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    auto pScope = smart_cast<CScope*>(obj);
    if (pScope)
    {
        _param_name = CStringTable().translate("st_current_scope").c_str();
        SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
        _h += list_item_h;

        auto scope_zoom_line = READ_IF_EXISTS(pSettings, r_string, item_section, "scope_zoom_factor", nullptr);
        if (scope_zoom_line)
        {
            _param_name = CStringTable().translate("st_scope_zoom").c_str();
             int _count = _GetItemCount(scope_zoom_line);
            string128 tmp;

             _val = atof(_GetItem(scope_zoom_line, 0, tmp));

            if (_count > 1)
            {
                float max_zoom_factor = atof(_GetItem(scope_zoom_line, 1, tmp));
                sprintf_s(text_to_show, "%s%s %.1f-%.1fx", marker_, _param_name, _val, max_zoom_factor);
            }
            else
                sprintf_s(text_to_show, "%s%s %.1fx", marker_, _param_name, _val);

            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }

        bool has_night_vision = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "night_vision", false);
        if (has_night_vision)
        {
            _param_name = CStringTable().translate("st_scope_night_vision").c_str();
            sprintf_s(text_to_show, "%s%s", marker_, _param_name);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }

        bool vision_present = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "vision_present", false);
        if (vision_present)
        {
            _param_name = CStringTable().translate("st_scope_vision_present").c_str();
            sprintf_s(text_to_show, "%s%s", marker_, _param_name);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }

        bool has_range_meter = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "range_meter", false);
        if (has_range_meter)
        {
            _param_name = CStringTable().translate("st_scope_range_meter").c_str();
            sprintf_s(text_to_show, "%s%s", marker_, _param_name);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    if (smart_cast<CStock*>(obj) || smart_cast<CForend*>(obj))
    {
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "cam_dispersion_k", 0.f);
        if (!fis_zero(_val))
        {
            _val *= 100.f;
            _param_name = CStringTable().translate("st_recoil").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "control_inertion_k", 0.f);
        if (!fis_zero(_val))
        {
            _val *= 100.f;
            _param_name = CStringTable().translate("st_bulkiness").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "aim_inertion_k", 0.f);
        if (!fis_zero(_val))
        {
            _val *= -100.f;
            _param_name = CStringTable().translate("st_aiming_controllability").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "zoom_rotate_time_k", 0.f);
        if (!fis_zero(_val))
        {
            _val *= 100.f;
            _param_name = CStringTable().translate("st_aim_time").c_str();
            sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    auto pExtender = smart_cast<CExtender*>(obj);
    if (pExtender)
    {
        _val = READ_IF_EXISTS(pSettings, r_float, item_section, "ammo_mag_size", 0.f);
        if (!fis_zero(_val))
        {
            _param_name = CStringTable().translate("st_ammo_mag_size").c_str();
            sprintf_s(text_to_show, "%s %+.0f", _param_name, _val);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    auto pOutfit = smart_cast<CCustomOutfit*>(obj);
    if (pOutfit)
    {
        if (pOutfit->m_bIsHelmetBuiltIn)
        {
            _param_name = CStringTable().translate("st_inbuild_helmet").c_str();
            sprintf_s(text_to_show, "%s", _param_name);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }

        _val = pOutfit->GetExoFactor();
        if (_val > 1.f)
        {
            _param_name = CStringTable().translate("st_exo_factor").c_str();
            sprintf_s(text_to_show, "%s x%.1f", _param_name, _val);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    auto pWarbelt = smart_cast<CWarbelt*>(obj);
    auto pVest = smart_cast<CVest*>(obj);
    if (pWarbelt || pVest)
    {
        u32 cells_width = pWarbelt ? pWarbelt->GetBeltWidth() : pVest->GetVestWidth();
        u32 cells_height = pWarbelt ? pWarbelt->GetBeltHeight() : pVest->GetVestHeight();
        _param_name = CStringTable().translate("st_cells_available").c_str();
        sprintf_s(text_to_show, "%s [%dx%d]", _param_name, cells_width, cells_height);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }

    if (pVest && pVest->m_plates.size())
    {
        if (pVest->IsPlateInstalled())
        {
            _param_name = CStringTable().translate("st_installed_plate").c_str();
            sprintf_s(text_to_show, "%s %s", _param_name, pVest->GetPlateName().c_str());
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }

        _param_name = CStringTable().translate("st_plates").c_str();
        SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
        _h += list_item_h;

        for (const auto& plate : pVest->m_plates)
        {
            auto plate_name = pSettings->r_string(plate, "inv_name");
            sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(plate_name).c_str());
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    auto pHelmet = smart_cast<CHelmet*>(obj);
    if (pOutfit || pVest || pHelmet)
    {
        _val = obj->GetPowerLoss();
        if (_val > 1.f)
        {
            _param_name = CStringTable().translate("st_power_loss").c_str();
            sprintf_s(text_to_show, "%s x%.1f", _param_name, _val);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    // сумісні набої магазинів
    auto pAmmo = smart_cast<CWeaponAmmo*>(obj);
    if (pAmmo)
    {
        if (pAmmo->IsBoxReloadable())
        {
            //споряджений набій
            if (pAmmo->m_boxCurr)
            {
                _param_name = CStringTable().translate("st_current_ammo_type").c_str();
                auto ammo_name = pSettings->r_string(pAmmo->m_ammoSect, "inv_name");
                sprintf_s(text_to_show, "%s %s", _param_name, CStringTable().translate(ammo_name).c_str());
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
            // сумісні набої магазинів - заголовок
            _param_name = CStringTable().translate("st_compatible_ammo").c_str();
            SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
            _h += list_item_h;
            // сумісні набої магазинів - список
            for (const auto& ammo : pAmmo->m_ammoTypes)
            {
                auto ammo_name = pSettings->r_string(ammo, "inv_name");
                sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(ammo_name).c_str());
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
        }
        else
        {
            // дальність вогню
            _sn = "%";
            _val = pAmmo->m_kDist;
            if (!fis_zero(_val))
            {
                _val *= 100.f;
                _param_name = CStringTable().translate("st_fire_distance").c_str();
                sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
            // розкид
            _val = pAmmo->m_kDisp;
            if (!fis_zero(_val))
            {
                _val *= 100.f;
                _param_name = CStringTable().translate("st_dispersion").c_str();
                sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
            // ушкодження
            _val = pAmmo->m_kHit;
            if (!fis_zero(_val))
            {
                _val *= 100.f;
                _param_name = CStringTable().translate("st_damage").c_str();
                sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
            // швидкість кулі
            _val = pAmmo->m_kSpeed;
            if (!fis_zero(_val))
            {
                _val *= 100.f;
                _param_name = CStringTable().translate("st_bullet_speed").c_str();
                sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
            // бронебійність
            _val = pAmmo->m_kAP;
            if (!fis_zero(_val))
            {
                _param_name = CStringTable().translate("st_ap").c_str();
                sprintf_s(text_to_show, "%s %.1f", _param_name, _val);
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
            // зношування зброї
            _val = pAmmo->m_impair;
            if (!fis_zero(_val))
            {
                _val *= 100.f;
                _param_name = CStringTable().translate("st_weapon_dec").c_str();
                sprintf_s(text_to_show, "%s %+.1f%s", _param_name, _val, _sn);
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
            // кількість шротин у набої
            _val = pAmmo->m_buckShot;
            if (_val > 1)
            {
                _param_name = CStringTable().translate("st_buck_count").c_str();
                sprintf_s(text_to_show, "%s %.0f", _param_name, _val);
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
        }
    }

    auto pGrenade = smart_cast<CGrenade*>(obj);
    if (pGrenade || pSettings->line_exist(item_section, "fake_grenade_name"))
    {
        const shared_str& explosive_sect = pGrenade ? item_section : pSettings->r_string(item_section, "fake_grenade_name");
        // хіт вибуху
        _val = pSettings->r_float(explosive_sect, "blast");
        _param_name = CStringTable().translate("st_explosion_hit").c_str();
        sprintf_s(text_to_show, "%s %.1f", _param_name, _val);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
        // радіус вибуху
        _val = pSettings->r_float(explosive_sect, "blast_r");
        _param_name = CStringTable().translate("st_explosion_radius").c_str();
        _sn = CStringTable().translate("st_m").c_str();
        sprintf_s(text_to_show, "%s %.1f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
        // хіт уламків
        _val = pSettings->r_float(explosive_sect, "frag_hit");
        _param_name = CStringTable().translate("st_frags_hit").c_str();
        sprintf_s(text_to_show, "%s %.1f", _param_name, _val);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
        // радіус уламків
        _val = pSettings->r_float(explosive_sect, "frags_r");
        _param_name = CStringTable().translate("st_frags_radius").c_str();
        _sn = CStringTable().translate("st_m").c_str();
        sprintf_s(text_to_show, "%s %.1f %s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
        // хіт уламків
        _val = pSettings->r_u8(explosive_sect, "frags") * 2;
        _param_name = CStringTable().translate("st_frags_count").c_str();
        sprintf_s(text_to_show, "%s %.0f", _param_name, _val);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
        // затримка до вибуху
        if (pSettings->line_exist(explosive_sect, "destroy_time"))
        {
            LPCSTR str = pSettings->r_string(explosive_sect, "destroy_time");
            _param_name = CStringTable().translate("st_destroy_time").c_str();
            _sn = CStringTable().translate("st_time_second").c_str();
            if (_GetItemCount(str) > 1)
            {
                string128 tmp{};
                _val = (float)atof(_GetItem(str, 0, tmp));
                _val /= 1000.f;
                float _val1 = (float)atof(_GetItem(str, 1, tmp));
                _val1 /= 1000.f;
                sprintf_s(text_to_show, "%s %.1f-%.1f %s", _param_name, _val, _val1, _sn);
            }
            else
            {
                _val = pSettings->r_float(explosive_sect, "destroy_time");
                _val /= 1000.f;
                sprintf_s(text_to_show, "%s %.1f %s", _param_name, _val, _sn);
            }
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    if (auto pContainer = smart_cast<CInventoryContainer*>(obj))
    {
        if (pContainer->HasQuickDrop())
        {
            _param_name = CStringTable().translate("st_quick_drop").c_str();
            sprintf_s(text_to_show, "%s", _param_name);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        if (pContainer->GetItemsLimit())
        {
            _param_name = CStringTable().translate("st_items_limit").c_str();
            _val = pContainer->GetItemsLimit();
            sprintf_s(text_to_show, "%s: %.0f", _param_name, _val);
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
        if (!pContainer->IsEmpty())
        {
            _param_name = CStringTable().translate("st_containment").c_str();
            SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
            _h += list_item_h;

            TIItemContainer container_list;
            pContainer->AddUniqueItems(container_list);

            for (const auto& item : container_list)
            {
                u32 count = pContainer->GetSameItemCount(item->object().cNameSect());
                if (count > 1)
                    sprintf_s(text_to_show, "%s%s x%d", marker_, CStringTable().translate(item->Name()).c_str(), count);
                else
                    sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(item->Name()).c_str());
                SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
                _h += list_item_h;
            }
        }
    }

    if (!obj->m_required_tools.empty())
    {
        _param_name = CStringTable().translate("st_required_tools").c_str();
        SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
        _h += list_item_h;

        for (const auto& tool_sect : obj->m_required_tools)
        {
            auto tool_name = pSettings->r_string(tool_sect, "inv_name");
            sprintf(text_to_show, "%s%s", marker_, CStringTable().translate(tool_name).c_str());
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    if (obj->GetDetailPartSection())
    {
        _param_name = CStringTable().translate("st_detail_parts").c_str();
        SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
        _h += list_item_h;

        string128 item_sect;
        LPCSTR detail_part_sect = obj->GetDetailPartSection();
        int count = _GetItemCount(detail_part_sect);
        for (int i = 0; i < count; i += 2)
        {
            _GetItem(detail_part_sect, i, item_sect);
            string128 tmp;
            int item_count = atoi(_GetItem(detail_part_sect, i + 1, tmp));
            auto detail_name = pSettings->r_string(item_sect, "inv_name");
            sprintf(text_to_show, "%sx%d %s", marker_, item_count, CStringTable().translate(detail_name).c_str());
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    if (!obj->m_repair_items.empty())
    {
        _param_name = CStringTable().translate("st_repair_items").c_str();
        SetStaticParams(_uiXml, _path, _h)->SetText(_param_name);
        _h += list_item_h;

        for (const auto& item_sect : obj->m_repair_items)
        {
            auto rep_item_name = pSettings->r_string(item_sect, "inv_name");
            sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate(rep_item_name).c_str());
            SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
            _h += list_item_h;
        }
    }

    _sn = "%";
    _val = obj->repair_condition_gain;
    if (!fis_zero(_val))
    {
        _val *= 100.f;
        _param_name = CStringTable().translate("st_repair_condition_gain").c_str();
        sprintf_s(text_to_show, "%s %.0f%s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    _val = obj->repair_condition_threshold;
    if (!fis_zero(_val))
    {
        _val *= 100.f;
        _param_name = CStringTable().translate("st_repair_condition_threshold").c_str();
        sprintf_s(text_to_show, "%s %.0f%s", _param_name, _val, _sn);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }
    _val = obj->repair_count;
    if (!fis_zero(_val))
    {
        _param_name = CStringTable().translate("st_repair_count").c_str();
        sprintf_s(text_to_show, "%s %.0f", _param_name, _val);
        SetStaticParams(_uiXml, _path, _h)->SetText(text_to_show);
        _h += list_item_h;
    }

    SetHeight(_h);
}