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
#include "WeaponBinoculars.h"
#include "WeaponMagazinedWGrenade.h"
#include "../string_table.h"
#include "UIInventoryUtilities.h"
#include "UI.h"

constexpr auto WPN_PARAMS = "wpn_params.xml";

struct SLuaWpnParams
{
    luabind::functor<float> m_functorRPM;
    luabind::functor<float> m_functorAccuracy;
    luabind::functor<float> m_functorDamage;
    luabind::functor<float> m_functorHandling;
    luabind::functor<float> m_functorRange;
    luabind::functor<float> m_functorReliability;
    SLuaWpnParams();
    ~SLuaWpnParams();
};

SLuaWpnParams::SLuaWpnParams()
{
    bool functor_exists;
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetRPM", m_functorRPM);
    VERIFY(functor_exists);
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetDamage", m_functorDamage);
    VERIFY(functor_exists);
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetHandling", m_functorHandling);
    VERIFY(functor_exists);
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetAccuracy", m_functorAccuracy);
    VERIFY(functor_exists);
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetRange", m_functorRange);
    VERIFY(functor_exists);
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetReliability", m_functorReliability);
    VERIFY(functor_exists);
}

SLuaWpnParams::~SLuaWpnParams() {}

SLuaWpnParams* g_lua_wpn_params{};

void destroy_lua_wpn_params()
{
    if (g_lua_wpn_params)
        xr_delete(g_lua_wpn_params);
}

CUIWpnParams::CUIWpnParams() 
{
    AttachChild(&m_textAccuracy);
    AttachChild(&m_textDamage);
    AttachChild(&m_textHandling);
    AttachChild(&m_textRPM);
    AttachChild(&m_textRange);
    AttachChild(&m_textReliability);

    AttachChild(&m_progressAccuracy);
    AttachChild(&m_progressDamage);
    AttachChild(&m_progressHandling);
    AttachChild(&m_progressRPM);
    AttachChild(&m_progressRange);
    AttachChild(&m_progressReliability);
}

CUIWpnParams::~CUIWpnParams() {}

void CUIWpnParams::Init()
{
    CUIXml xml_doc;
    if (!xml_doc.Init(CONFIG_PATH, UI_PATH, WPN_PARAMS))
        return;

    CUIXmlInit xml_init;

    xml_init.InitWindow(xml_doc, "wpn_params", 0, this);

    xml_init.InitStatic(xml_doc, "wpn_params:cap_accuracy", 0, &m_textAccuracy);
    xml_init.InitStatic(xml_doc, "wpn_params:cap_damage", 0, &m_textDamage);
    xml_init.InitStatic(xml_doc, "wpn_params:cap_handling", 0, &m_textHandling);
    xml_init.InitStatic(xml_doc, "wpn_params:cap_rpm", 0, &m_textRPM);
    xml_init.InitStatic(xml_doc, "wpn_params:cap_range", 0, &m_textRange);
    xml_init.InitStatic(xml_doc, "wpn_params:cap_reliability", 0, &m_textReliability);

    xml_init.InitProgressBar(xml_doc, "wpn_params:progress_accuracy", 0, &m_progressAccuracy);
    xml_init.InitProgressBar(xml_doc, "wpn_params:progress_damage", 0, &m_progressDamage);
    xml_init.InitProgressBar(xml_doc, "wpn_params:progress_handling", 0, &m_progressHandling);
    xml_init.InitProgressBar(xml_doc, "wpn_params:progress_rpm", 0, &m_progressRPM);
    xml_init.InitProgressBar(xml_doc, "wpn_params:progress_range", 0, &m_progressRange);
    xml_init.InitProgressBar(xml_doc, "wpn_params:progress_reliability", 0, &m_progressReliability);

    xml_init.InitAutoStatic(xml_doc, "auto_static", this);
}

bool CUIWpnParams::Check(CInventoryItem* obj)
{
    if (!READ_IF_EXISTS(pSettings, r_bool, obj->object().cNameSect(), "show_wpn_properties", true)) // allow to suppress default wpn params
    {
        return false;
    }

    return smart_cast<CWeapon*>(obj) && !smart_cast<CWeaponBinoculars*>(obj);
}

void ShowIcon(CUIStatic& icon, shared_str sect, float scale, float& pos_x, bool autoscale)
{
    icon.Show(true);
    CIconParams params(sect);
    params.set_shader(&icon);

    Fvector2 size{}, pos{};
    Frect rect{};
    icon.GetAbsoluteRect(rect);
    pos.set(rect.left, rect.top);

    const auto& r = params.original_rect();
    size.set(r.width(), r.height());
    size.mul(scale);
    if (autoscale)
        size.mul(1 / params.grid_height);
    size.x *= UI()->get_current_kx();

    icon.SetWndRect(0, 0, size.x, size.y);
    icon.SetWndPos(/*pos.x*/ pos_x, pos.y);
    pos_x += icon.GetWidth();
}

void CUIWpnParams::SetInfo(CInventoryItem* obj)
{
    if (!g_lua_wpn_params)
        g_lua_wpn_params = xr_new<SLuaWpnParams>();

    const auto& object = obj->object();
    const auto wpn_section = object.cNameSect();

    m_progressRPM.SetProgressPos(g_lua_wpn_params->m_functorRPM(wpn_section.c_str(), object.lua_game_object()));
    m_progressAccuracy.SetProgressPos(g_lua_wpn_params->m_functorAccuracy(wpn_section.c_str(), object.lua_game_object()));
    m_progressDamage.SetProgressPos(g_lua_wpn_params->m_functorDamage(wpn_section.c_str(), object.lua_game_object()));
    m_progressHandling.SetProgressPos(g_lua_wpn_params->m_functorHandling(wpn_section.c_str(), object.lua_game_object()));
    m_progressRange.SetProgressPos(g_lua_wpn_params->m_functorRange(wpn_section.c_str(), object.lua_game_object()));
    m_progressReliability.SetProgressPos(g_lua_wpn_params->m_functorReliability(wpn_section.c_str(), object.lua_game_object()));
}