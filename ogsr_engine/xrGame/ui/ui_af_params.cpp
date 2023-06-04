#include "stdafx.h"
#include "ui_af_params.h"
#include "UIStatic.h"
#include "object_broker.h"
#include "UIXmlInit.h"

#include "string_table.h"

#include "inventory_item.h"
#include "Artifact.h"
#include "Actor.h"
#include "ActorCondition.h"

#include "Vest.h"

constexpr auto AF_PARAMS = "af_params.xml";

CUIArtefactParams::CUIArtefactParams() { Memory.mem_fill(m_info_items, 0, sizeof(m_info_items)); }

CUIArtefactParams::~CUIArtefactParams()
{
    for (u32 i = 0; i < _max_item_index; ++i)
    {
        CUIStatic* _s = m_info_items[i];
        xr_delete(_s);
    }
}

LPCSTR af_item_sect_names[] = {
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
    //
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

LPCSTR af_item_param_names[] = {
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
    //
    "ui_inv_weight",
    //
    "ui_inv_burn_protection", // "(burn_imm)",
    "ui_inv_shock_protection", // "(shock_imm)",
    "ui_inv_strike_protection", // "(strike_imm)",
    "ui_inv_wound_protection", // "(wound_imm)",
    "ui_inv_radiation_protection", // "(radiation_imm)",
    "ui_inv_telepatic_protection", // "(telepatic_imm)",
    "ui_inv_chemical_burn_protection", // "(chemical_burn_imm)",
    "ui_inv_explosion_protection", // "(explosion_imm)",
    "ui_inv_fire_wound_protection", // "(fire_wound_imm)",
};

bool CUIArtefactParams::Check(CInventoryItem* obj) 
{ 
    for (int i = 0; i < _max_item_index; ++i)
    {
        if (i < _hit_type_protection_index)
        {
            if (!fis_zero(obj->GetItemEffect(i)))
                return true;
        }
        else
        {
            if (!fis_zero(obj->GetHitTypeProtection(i - _hit_type_protection_index)))
                return true;
        }
    }
    return false;
}

void CUIArtefactParams::Init()
{
    CUIXml uiXml;
    uiXml.Init(CONFIG_PATH, UI_PATH, AF_PARAMS);

    LPCSTR _base = "af_params";
    if (!uiXml.NavigateToNode(_base, 0))
        return;

    string256 _buff;
    CUIXmlInit::InitWindow(uiXml, _base, 0, this);

    for (u32 i = 0; i < _max_item_index; ++i)
    {
        strconcat(sizeof(_buff), _buff, _base, ":static_", af_item_sect_names[i]);

        if (uiXml.NavigateToNode(_buff, 0))
        {
            m_info_items[i] = xr_new<CUIStatic>();
            CUIStatic* _s = m_info_items[i];
            _s->SetAutoDelete(false);
            CUIXmlInit::InitStatic(uiXml, _buff, 0, _s);
        }
    }
}

void CUIArtefactParams::SetInfo(CInventoryItem* obj)
{
    if (!obj)
        return;

    CActor* pActor = Actor();
    if (!pActor)
        return;

    string128 _buff;
    float _h{};
    DetachAll();

    for (int i = 0; i < _max_item_index; ++i)
    {
        CUIStatic* _s = m_info_items[i];
        R_ASSERT2(_s, make_string("%s static %s not described", __FUNCTION__, af_item_sect_names[i]));

        float _val{};

        if (i < _hit_type_protection_index)
        {
            _val = obj->GetItemEffect(i);
        }
        else
        {
            _val = obj->GetHitTypeProtection(i - _hit_type_protection_index);
        }

        if (fis_zero(_val))
            continue;

        _val *= 100.0f;

        LPCSTR _sn{"%"};

        if (i == _item_radiation_restore)
        {
            _val /= 100.0f;
            _sn = CStringTable().translate("st_rad").c_str();
        }

        LPCSTR _color{(_val > 0) ? "%c[green]" : "%c[red]"};

        if (i == _item_alcohol_restore)
            _val *= -1.0f;

        if (i == _item_radiation_restore || i == _item_alcohol_restore || i == _item_max_power_restore)
            _color = (_val > 0) ? "%c[red]" : "%c[green]";

        sprintf_s(_buff, "%s %s %+.1f %s", CStringTable().translate(af_item_param_names[i]).c_str(), _color, _val, _sn);
        _s->SetText(_buff);
        _s->SetWndPos(_s->GetWndPos().x, _h);
        _h += _s->GetWndSize().y;
        AttachChild(_s);
    }
    SetHeight(_h);
}