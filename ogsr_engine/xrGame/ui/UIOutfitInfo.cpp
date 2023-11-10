#include "StdAfx.h"
#include "UIOutfitInfo.h"
#include "UIXmlInit.h"
#include "UIStatic.h"
#include "UIScrollView.h"
#include "Actor.h"
#include "Inventory.h"
#include "Artifact.h"
#include "CustomOutfit.h"
#include "InventoryContainer.h"
#include "Helmet.h"
#include "Vest.h"
#include "ActorCondition.h"
#include "../string_table.h"

CUIOutfitInfo::CUIOutfitInfo() {}

CUIOutfitInfo::~CUIOutfitInfo()
{
    for (u32 i = 0; i < _max_item_index; ++i)
    {
        CUIStatic* _s = m_items[i];
        xr_delete(_s);
    }
}

LPCSTR _imm_names[] = {
    "health_restore_speed",
    "power_restore_speed",
    "max_power_restore_speed",
    "satiety_restore_speed",
    "radiation_restore_speed",
    "psy_health_restore_speed",
    "alcohol_restore_speed",
    "wounds_heal_speed",
    //
    "additional_sprint",
    "additional_jump",
    "additional_weight",
    //
    "burn_immunity",
    "shock_immunity",
    "strike_immunity",
    "wound_immunity",
    "radiation_immunity",
    "telepatic_immunity",
    "chemical_burn_immunity",
    "explosion_immunity",
    "fire_wound_immunity",
};

LPCSTR _imm_st_names[] = {
    "ui_inv_health_boost",
    "ui_inv_power",
    "ui_inv_max_power",
    "ui_inv_satiety",
    "ui_inv_radiation",
    "ui_inv_psy_health",
    "ui_inv_alcohol",
    "ui_inv_wounds_heal",
    //
    "ui_inv_sprint",
    "ui_inv_jump",
    "ui_inv_weight",
    //
    "ui_inv_burn_protection",
    "ui_inv_shock_protection",
    "ui_inv_strike_protection",
    "ui_inv_wound_protection",
    "ui_inv_radiation_protection",
    "ui_inv_telepatic_protection",
    "ui_inv_chemical_burn_protection",
    "ui_inv_explosion_protection",
    "ui_inv_fire_wound_protection",
};

void CUIOutfitInfo::InitFromXml(CUIXml& xml_doc)
{
    LPCSTR _base = "outfit_info";

    string256 _buff;
    CUIXmlInit::InitWindow(xml_doc, _base, 0, this);

    m_listWnd = xr_new<CUIScrollView>();
    m_listWnd->SetAutoDelete(true);
    AttachChild(m_listWnd);
    strconcat(sizeof(_buff), _buff, _base, ":scroll_view");
    CUIXmlInit::InitScrollView(xml_doc, _buff, 0, m_listWnd);

    for (u32 i = 0; i < _max_item_index; ++i)
    {
        strconcat(sizeof(_buff), _buff, _base, ":static_", _imm_names[i]);

        if (xml_doc.NavigateToNode(_buff, 0))
        {
            m_items[i] = xr_new<CUIStatic>();
            CUIStatic* _s = m_items[i];
            _s->SetAutoDelete(false);
            CUIXmlInit::InitStatic(xml_doc, _buff, 0, _s);
        }
        else
            Msg("! can't find static name %s", _buff);
    }
}

#include "script_game_object.h"

void CUIOutfitInfo::Update()
{
    string128 _buff;

    const auto actor = Actor();
    auto outfit = actor->GetOutfit();
    auto backpack = actor->GetBackpack();
    auto helmet = actor->GetHelmet();
    auto vest = actor->GetVest();

    auto& cond = actor->conditions();

    m_listWnd->Clear(false); // clear existing items and do not scroll to top

    for (u32 i = 0; i < _max_item_index; ++i)
    {
        CUIStatic* _s = m_items[i];
        R_ASSERT2(_s, make_string("%s static %s not described", __FUNCTION__, _imm_names[i]));

        float _val{}, _val_boost{cond.GetBoostedParams(i)};

        if (i < _hit_type_protection_index)
        {
            if (i < _item_additional_sprint)
            {
                _val += Actor()->GetItemBoostedParams(i);
            }
            else
            {
                _val += Actor()->GetTotalArtefactsEffect(i);
                if (outfit)
                    _val += outfit->GetItemEffect(i);
                if (vest)
                    _val += vest->GetItemEffect(i);
                if (backpack)
                    _val += backpack->GetItemEffect(i);
                if (helmet)
                    _val += helmet->GetItemEffect(i);
            }
        }
        else
        {
            _val += Actor()->GetArtefactsProtection(i - _hit_type_protection_index);
            if (outfit)
                _val += outfit->GetHitTypeProtection(i - _hit_type_protection_index);
            if (vest)
                _val += vest->GetHitTypeProtection(i - _hit_type_protection_index);
            // рюкзак захищає актора за досить специфічних умов (див. CActor::IsHitToBackPack)
            // тож не будемо відображати його захисні хар-ки тут
            //if (backpack)
            //    _val += backpack->GetHitTypeProtection(i - _hit_type_protection_index);
            if (helmet)
                _val += helmet->GetHitTypeProtection(i - _hit_type_protection_index);
        }

        if (fis_zero(_val) && fis_zero(_val_boost))
            continue;

        LPCSTR _sn{"%"}, _sn_boost{"%"};

        _val *= 100.0f;
        _val_boost *= 100.f;

        if (i == _item_radiation_restore)
        {
            _val /= 100.0f;
            _val_boost /= 100.f;
            _sn = _sn_boost = CStringTable().translate("st_rad").c_str();
        }

        if (i == _item_alcohol_restore)
        {
            _val *= -1.0f;
            _val_boost *= -0.1f;
        }

        LPCSTR _color{(_val > 0) ? "%c[green]" : "%c[red]"};
        LPCSTR _color_boost{(_val_boost > 0) ? "%c[green]" : "%c[red]"};

        if (i == _item_radiation_restore || i == _item_alcohol_restore || i == _item_max_power_restore)
        {
            _color = (_val > 0) ? "%c[red]" : "%c[green]";
            _color_boost = (_val_boost > 0) ? "%c[red]" : "%c[green]";
        }

        auto _name{CStringTable().translate(_imm_st_names[i]).c_str()};
        auto _val_time{cond.GetBoostedTime(i)};
        auto _st_time{CStringTable().translate("st_time_minute").c_str()};

        if (!fis_zero(_val) && !fis_zero(_val_boost))
            sprintf_s(_buff, "%s %s%+.f%s %s%+.f%s [%.f %s]", _name, _color, _val, _sn, _color_boost, _val_boost, _sn_boost, _val_time, _st_time);
        else if (!fis_zero(_val))
            sprintf_s(_buff, "%s %s%+.f%s", _name, _color, _val, _sn);
        else
            sprintf_s(_buff, "%s %s%+.f%s [%.f %s]", _name, _color_boost, _val_boost, _sn_boost, _val_time, _st_time);

        _s->SetText(_buff);

        m_listWnd->AddWindow(_s, false);
    }

    if (pSettings->line_exist("engine_callbacks", "ui_actor_info_callback"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "ui_actor_info_callback");
        if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function))
        {
            lua_function(m_listWnd, outfit ? outfit->lua_game_object() : nullptr);
        }
    }
}