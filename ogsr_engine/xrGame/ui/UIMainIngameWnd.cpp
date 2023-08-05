#include "stdafx.h"

#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"
#include "../UIZoneMap.h"

#include <dinput.h>
#include "../actor.h"
#include "../HUDManager.h"
#include "../PDA.h"
#include "CustomOutfit.h"
#include "../character_info.h"
#include "../inventory.h"
#include "../UIGameSP.h"
#include "../weaponmagazined.h"
#include "../missile.h"
#include "../Grenade.h"
#include "../xrServer_objects_ALife.h"
#include "../alife_simulator.h"
#include "../alife_object_registry.h"
#include "../game_cl_base.h"
#include "../level.h"
#include "../seniority_hierarchy_holder.h"

#include "../date_time.h"
#include "../xrServer_Objects_ALife_Monsters.h"
#include "../../xr_3da/LightAnimLibrary.h"

#include "UIInventoryUtilities.h"

#include "UIXmlInit.h"
#include "UIPdaMsgListItem.h"
#include "../alife_registry_wrappers.h"
#include "../actorcondition.h"

#include "../string_table.h"
#include "clsid_game.h"
#include "UIPanels.h"
#include "UIMap.h"

#ifdef DEBUG
#include "../attachable_item.h"
#include "..\..\xr_3da\xr_input.h"
#endif

#include "UIScrollView.h"
#include "map_hint.h"
#include "UIColorAnimatorWrapper.h"
#include "../game_news.h"

#include "CustomDetector.h"

using namespace InventoryUtilities;

constexpr auto DEFAULT_MAP_SCALE = 1.f;
constexpr auto MAININGAME_XML = "maingame.xml";

static CUIMainIngameWnd* GetMainIngameWindow()
{
    if (g_hud)
    {
        CUI* pUI = g_hud->GetUI();
        if (pUI)
            return pUI->UIMainIngameWnd;
    }
    return nullptr;
}

static CUIStatic* warn_icon_list[CUIMainIngameWnd::ewiMax]{};

// alpet: для возможности внешнего контроля иконок (используется в NLC6 вместо типичных индикаторов). Никак не влияет на игру для остальных модов.
static bool external_icon_ctrl = false;

// позволяет расцветить иконку или изменить её размер
static bool SetupGameIcon(CUIMainIngameWnd::EWarningIcons icon, u32 cl, float width, float height)
{
    auto window = GetMainIngameWindow();
    if (!window)
    {
        Msg("!![SetupGameIcon] failed due GetMainIngameWindow() returned NULL");
        return false;
    }

    R_ASSERT(icon > 0 && icon < std::size(warn_icon_list), "!!Invalid first arg for setup_game_icon!");

    CUIStatic* sIcon = warn_icon_list[icon];

    if (width > 0 && height > 0)
    {
        sIcon->SetWidth(width);
        sIcon->SetHeight(height);
        sIcon->SetStretchTexture(cl > 0);
    }
    else
        window->SetWarningIconColor(icon, cl);

    external_icon_ctrl = true;
    return true;
}

CUIMainIngameWnd::CUIMainIngameWnd()
{
    m_pActor = nullptr;
    UIZoneMap = xr_new<CUIZoneMap>();
    m_pPickUpItem = nullptr;
    m_beltPanel = xr_new<CUIBeltPanel>();
    m_slotPanel = xr_new<CUISlotPanel>();
    m_boosterPanel = xr_new<CUIBoosterPanel>();

    warn_icon_list[ewiWeaponJammed] = &UIWeaponJammedIcon;
    warn_icon_list[ewiArmor] = &UIArmorIcon;
    warn_icon_list[ewiRadiation] = &UIRadiaitionIcon;
    warn_icon_list[ewiWound] = &UIWoundIcon;
    warn_icon_list[ewiStarvation] = &UIStarvationIcon;
    warn_icon_list[ewiPsyHealth] = &UIPsyHealthIcon;
    warn_icon_list[ewiSafehouse] = &UISafehouseIcon;
    warn_icon_list[ewiInvincible] = &UIInvincibleIcon;
}

#include "UIProgressShape.h"
extern CUIProgressShape* g_MissileForceShape;

CUIMainIngameWnd::~CUIMainIngameWnd()
{
    DestroyFlashingIcons();
    xr_delete(UIZoneMap);
    xr_delete(m_beltPanel);
    xr_delete(m_slotPanel);
    xr_delete(m_boosterPanel);
    HUD_SOUND::DestroySound(m_contactSnd);
    xr_delete(g_MissileForceShape);
}

void CUIMainIngameWnd::Init()
{
    CUIXml uiXml;
    uiXml.Init(CONFIG_PATH, UI_PATH, MAININGAME_XML);

    CUIXmlInit xml_init;
    CUIWindow::Init(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

    Enable(false);

    AttachChild(&UIWeaponBack);
    xml_init.InitStatic(uiXml, "static_weapon", 0, &UIWeaponBack);

    UIWeaponBack.AttachChild(&UIWeaponSignAmmo);
    xml_init.InitStatic(uiXml, "static_ammo", 0, &UIWeaponSignAmmo);
    UIWeaponSignAmmo.SetElipsis(CUIStatic::eepEnd, 2);
    ammo_icon_scale = uiXml.ReadAttribFlt("static_ammo", 0, "icon_scale", 1.f);

    UIWeaponBack.AttachChild(&UIWeaponIcon);
    xml_init.InitStatic(uiXml, "static_wpn_icon", 0, &UIWeaponIcon);
    UIWeaponIcon.SetShader(GetEquipmentIconsShader());
    UIWeaponIcon_rect = UIWeaponIcon.GetWndRect();
    //---------------------------------------------------------
    AttachChild(&UIPickUpItemIcon);
    xml_init.InitStatic(uiXml, "pick_up_item", 0, &UIPickUpItemIcon);
    UIPickUpItemIcon.SetShader(GetEquipmentIconsShader());
    //	UIPickUpItemIcon.ClipperOn	();
    UIPickUpItemIcon.Show(false);

    m_iPickUpItemIconWidth = UIPickUpItemIcon.GetWidth();
    m_iPickUpItemIconHeight = UIPickUpItemIcon.GetHeight();
    m_iPickUpItemIconX = UIPickUpItemIcon.GetWndRect().left;
    m_iPickUpItemIconY = UIPickUpItemIcon.GetWndRect().top;
    //---------------------------------------------------------

    UIWeaponIcon.Enable(false);

    // индикаторы
    UIZoneMap->Init();
    UIZoneMap->SetScale(DEFAULT_MAP_SCALE);

    xml_init.InitStatic(uiXml, "static_pda_online", 0, &UIPdaOnline);
    UIZoneMap->Background().AttachChild(&UIPdaOnline);

    // Подсказки, которые возникают при наведении прицела на объект
    AttachChild(&UIStaticQuickHelp);
    xml_init.InitStatic(uiXml, "quick_info", 0, &UIStaticQuickHelp);

    uiXml.SetLocalRoot(uiXml.GetRoot());

    m_UIIcons = xr_new<CUIStatic>();
    m_UIIcons->SetAutoDelete(true);
    xml_init.InitStatic(uiXml, "icons_back", 0, m_UIIcons);
    b_horz = uiXml.ReadAttrib("icons_back", 0, "horz", 0);
    AttachChild(m_UIIcons);

    // Загружаем иконки
    xml_init.InitStatic(uiXml, "starvation_static", 0, &UIStarvationIcon);
    UIStarvationIcon.Show(false);

    xml_init.InitStatic(uiXml, "psy_health_static", 0, &UIPsyHealthIcon);
    UIPsyHealthIcon.Show(false);

    xml_init.InitStatic(uiXml, "weapon_jammed_static", 0, &UIWeaponJammedIcon);
    UIWeaponJammedIcon.Show(false);

    xml_init.InitStatic(uiXml, "armor_static", 0, &UIArmorIcon);
    UIArmorIcon.Show(false);

    xml_init.InitStatic(uiXml, "radiation_static", 0, &UIRadiaitionIcon);
    UIRadiaitionIcon.Show(false);

    xml_init.InitStatic(uiXml, "wound_static", 0, &UIWoundIcon);
    UIWoundIcon.Show(false);

    xml_init.InitStatic(uiXml, "safehouse_static", 0, &UISafehouseIcon);
    UISafehouseIcon.Show(false);

    xml_init.InitStatic(uiXml, "invincible_static", 0, &UIInvincibleIcon);
    UIInvincibleIcon.Show(false);

    constexpr const char* warningStrings[] = {
        "jammed",
        "armor",
        "radiation", 
        "wounds", 
        "starvation", 
        "psy",
        //"invincible",
        //"safehouse",
    };

    // Загружаем пороговые значения для индикаторов
    for (int i = 0; i < ewiThresholdMax; ++i)
    {
        // Читаем данные порогов для каждого индикатора
        const char* cfgRecord = pSettings->r_string("main_ingame_indicators_thresholds", warningStrings[i]);
        u32 count = _GetItemCount(cfgRecord);

        char singleThreshold[5];
        float f{};
        for (u32 k = 0; k < count; ++k)
        {
            _GetItem(cfgRecord, k, singleThreshold);
            sscanf(singleThreshold, "%f", &f);

            m_Thresholds[(EWarningIcons)i].push_back(f);
        }
    }

    // Flashing icons initialize
    uiXml.SetLocalRoot(uiXml.NavigateToNode("flashing_icons"));
    InitFlashingIcons(&uiXml);

    uiXml.SetLocalRoot(uiXml.GetRoot());

    AttachChild(&UICarPanel);
    xml_init.InitWindow(uiXml, "car_panel", 0, &UICarPanel);

    AttachChild(&UIMotionIcon);
    UIMotionIcon.Init();

    m_beltPanel->InitFromXML(uiXml, "belt_panel", 0);
    AttachChild(m_beltPanel);

    m_slotPanel->InitFromXML(uiXml, "slot_panel", 0);
    AttachChild(m_slotPanel);

    m_boosterPanel->InitFromXML(uiXml, "booster_panel", 0);
    AttachChild(m_boosterPanel);

    HUD_SOUND::LoadSound("maingame_ui", "snd_new_contact", m_contactSnd, SOUND_TYPE_IDLE);
}

void CUIMainIngameWnd::Draw()
{
    if (!m_pActor)
        return;

    CUIWindow::Draw();

    if (m_bShowZoneMap)
        UIZoneMap->Render();

    RenderQuickInfos();
}

void CUIMainIngameWnd::SetAmmoIcon(const shared_str& sect_name)
{
    if (!sect_name.size())
    {
        UIWeaponIcon.Show(false);
        return;
    };

    UIWeaponIcon.Show(true);
    // properties used by inventory menu
    CIconParams icon_params(sect_name);

    icon_params.set_shader(&UIWeaponIcon);

    float iGridWidth = icon_params.grid_width;
    float iGridHeight = icon_params.grid_height;

    float w = std::clamp(iGridWidth, 1.f, 2.f) * INV_GRID_WIDTH;
    float h = iGridHeight * INV_GRID_HEIGHT;
    w *= UI()->get_current_kx();

    float x = UIWeaponIcon_rect.x1;
    if (iGridWidth < 2.f)
        x += w / 2.0f;

    UIWeaponIcon.SetWndPos(x, UIWeaponIcon_rect.y1);

    w *= ammo_icon_scale;
    h *= ammo_icon_scale;

    UIWeaponIcon.SetWidth(w);
    UIWeaponIcon.SetHeight(h);
};

void CUIMainIngameWnd::Update()
{
    m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!m_pActor)
    {
        CUIWindow::Update();
        return;
    }

    if (!(Device.dwFrame % 30))
    {
        string256 text_str;
        auto pda = m_pActor->GetPDA();
        u32 _cn = 0;
        if (pda && pda->IsPowerOn() && 0 != (_cn = pda->ActiveContactsNum()))
        {
            sprintf_s(text_str, "%d", _cn);
            UIPdaOnline.SetText(text_str);
        }
        else
        {
            UIPdaOnline.SetText("");
        }
    };

    if (!(Device.dwFrame % 5))
    {
        //if (!(Device.dwFrame % 30))
        {
            if (GodMode())
                SetWarningIconColor(ewiInvincible, 0xffffffff);
            else if (!external_icon_ctrl)
                TurnOffWarningIcon(ewiInvincible);

            if (m_pActor->InSafeHouse())
                SetWarningIconColor(ewiSafehouse, 0xffffffff);
            else
                TurnOffWarningIcon(ewiSafehouse);
        }

        UpdateActiveItemInfo();

        auto cond = &m_pActor->conditions();

        for (int i = 0; i < ewiThresholdMax && !external_icon_ctrl; ++i)
        {
            float value{};
            switch (i)
            {
            case ewiRadiation: 
                value = cond->GetRadiation();
                break;
            case ewiWound: 
                value = cond->BleedingSpeed(); 
                break;
            case ewiWeaponJammed:
                if (auto act_item = m_pActor->inventory().ActiveItem())
                    value = 1.f - act_item->GetCondition();
                break;
            case ewiArmor:
                if (auto outfit = m_pActor->GetOutfit())
                    value = 1.f - outfit->GetCondition();
                break;
            case ewiStarvation: 
                value = 1 - cond->GetSatiety(); 
                break;
            case ewiPsyHealth: 
                value = 1 - cond->GetPsyHealth(); 
                break;
            default: R_ASSERT(!"Unknown type of warning icon");
            }

            auto icon = (EWarningIcons)i;
            auto& threshold = m_Thresholds[icon];
            // Минимальное и максимальное значения границы
            float min = threshold.front();
            float max = threshold.back();

            if (threshold.size() > 1)
            {
                xr_vector<float>::reverse_iterator rit;

                // Сначала проверяем на точное соответсвие
                rit = std::find(threshold.rbegin(), threshold.rend(), value);

                // Если его нет, то берем последнее меньшее значение ()
                if (rit == threshold.rend())
                    rit = std::find_if(threshold.rbegin(), threshold.rend(), std::bind(std::less<float>(), std::placeholders::_1, value));

                if (rit != threshold.rend())
                {
                    float v = *rit;
                    SetWarningIconColor(icon,
                                        color_argb(0xFF, clampr<u32>(static_cast<u32>(255 * ((v - min) / (max - min) * 2)), 0, 255),
                                                   clampr<u32>(static_cast<u32>(255 * (2.0f - (v - min) / (max - min) * 2)), 0, 255), 0));
                }
                else
                    TurnOffWarningIcon(icon);
            }
            else
            {
                float val = 1 - value;
                float treshold = 1 - min;
                clamp<float>(treshold, 0.01, 1.f);

                if (val <= treshold)
                {
                    float v = val / treshold;
                    clamp<float>(v, 0.f, 1.f);
                    SetWarningIconColor(icon, color_argb(0xFF, 255, clampr<u32>(static_cast<u32>(255 * v), 0, 255), 0));
                }
                else
                    TurnOffWarningIcon(icon);
            }
        }
    }

    UIZoneMap->UpdateRadar(Device.vCameraPosition);
    float h, p;
    Device.vCameraDirection.getHP(h, p);
    UIZoneMap->SetHeading(-h);

    UpdatePickUpItem();

    m_beltPanel->Show(m_bShowGearInfo); // панель поясу та розгрузки
    m_slotPanel->Show(m_bShowGearInfo); // панель слотів

    UpdateFlashingIcons(); // обновляем состояние мигающих иконок

    CUIWindow::Update();
}

bool CUIMainIngameWnd::OnKeyboardPress(int dik)
{
    if (Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION)))
    {
        switch (dik)
        {
        case DIK_NUMPADMINUS:
            UIZoneMap->ZoomOut();
            return true;
            break;
        case DIK_NUMPADPLUS:
            UIZoneMap->ZoomIn();
            return true;
            break;
        }
    }
    else
    {
        switch (dik)
        {
        case DIK_NUMPADMINUS:
            //.HideAll();
            HUD().GetUI()->HideGameIndicators();
            return true;
            break;
        case DIK_NUMPADPLUS:
            //.ShowAll();
            HUD().GetUI()->ShowGameIndicators();
            return true;
            break;
        }
    }

    return false;
}

void CUIMainIngameWnd::RenderQuickInfos()
{
    if (!m_pActor)
        return;

    static CGameObject* pObject = NULL;
    LPCSTR actor_action = m_pActor->GetDefaultActionForObject();
    UIStaticQuickHelp.Show(NULL != actor_action);

    if (NULL != actor_action)
    {
        if (stricmp(actor_action, UIStaticQuickHelp.GetText()))
            UIStaticQuickHelp.SetTextST(actor_action);
    }

    if (pObject != m_pActor->ObjectWeLookingAt())
    {
        UIStaticQuickHelp.SetTextST(actor_action);
        UIStaticQuickHelp.ResetClrAnimation();
        pObject = m_pActor->ObjectWeLookingAt();
    }
}

void CUIMainIngameWnd::ReceiveNews(GAME_NEWS_DATA* news)
{
    VERIFY(news->texture_name.size());
    auto pda = m_pActor->GetPDA();
    if (pda && pda->IsPowerOn() || !m_pActor->g_Alive())
        HUD().GetUI()->m_pMessagesWnd->AddIconedPdaMessage(*(news->texture_name), news->tex_rect, news->SingleLineText(), news->show_time);
}

void CUIMainIngameWnd::SetWarningIconColor(CUIStatic* s, const u32 cl)
{
    int bOn = (cl >> 24);
    bool bIsShown = s->IsShown();

    if (bOn)
        s->SetColor(cl);

    if (bOn && !bIsShown)
    {
        //m_UIIcons->AddWindow(s, false);
        m_UIIcons->AttachChild(s);
        s->Show(true);
    }

    if (!bOn && bIsShown)
    {
        //m_UIIcons->RemoveWindow(s);
        m_UIIcons->DetachChild(s);
        s->Show(false);
    }
}

void CUIMainIngameWnd::SetWarningIconColor(EWarningIcons icon, const u32 cl)
{
    // Задаем цвет требуемой иконки
    switch (icon)
    {
    case ewiWeaponJammed: SetWarningIconColor(&UIWeaponJammedIcon, cl); break;
    case ewiArmor: SetWarningIconColor(&UIArmorIcon, cl); break;
    case ewiRadiation: SetWarningIconColor(&UIRadiaitionIcon, cl); break;
    case ewiWound: SetWarningIconColor(&UIWoundIcon, cl); break;
    case ewiStarvation: SetWarningIconColor(&UIStarvationIcon, cl); break;
    case ewiPsyHealth: SetWarningIconColor(&UIPsyHealthIcon, cl); break;
    case ewiSafehouse: SetWarningIconColor(&UISafehouseIcon, cl); break;
    case ewiInvincible: SetWarningIconColor(&UIInvincibleIcon, cl); break;

    default: R_ASSERT(!"Unknown warning icon type"); break;
    }

    Fvector2 pos{};
    for (int i = 0; i < icon; ++i)
    {
        auto icon = warn_icon_list[i];
        if (icon->IsShown())
        {
            if (b_horz)
                pos.x += icon->GetWidth();
            else
                pos.y += icon->GetHeight();
        }
    }
    warn_icon_list[icon]->SetWndPos(pos);
}

void CUIMainIngameWnd::TurnOffWarningIcon(EWarningIcons icon) { SetWarningIconColor(icon, 0x00ffffff); }

void CUIMainIngameWnd::SetFlashIconState_(EFlashingIcons type, bool enable)
{
    // Включаем анимацию требуемой иконки
    FlashingIcons_it icon = m_FlashingIcons.find(type);
    R_ASSERT2(icon != m_FlashingIcons.end(), "Flashing icon with this type not existed");
    icon->second->Show(enable);
}

void CUIMainIngameWnd::InitFlashingIcons(CUIXml* node)
{
    const char* const flashingIconNodeName = "flashing_icon";
    int staticsCount = node->GetNodesNum("", 0, flashingIconNodeName);

    CUIXmlInit xml_init;
    CUIStatic* pIcon = NULL;
    // Пробегаемся по всем нодам и инициализируем из них статики
    for (int i = 0; i < staticsCount; ++i)
    {
        pIcon = xr_new<CUIStatic>();
        xml_init.InitStatic(*node, flashingIconNodeName, i, pIcon);
        shared_str iconType = node->ReadAttrib(flashingIconNodeName, i, "type", "none");

        // Теперь запоминаем иконку и ее тип
        EFlashingIcons type = efiPdaTask;

        if (iconType == "pda")
            type = efiPdaTask;
        else if (iconType == "mail")
            type = efiMail;
        else
            R_ASSERT(!"Unknown type of mainingame flashing icon");

        R_ASSERT2(m_FlashingIcons.find(type) == m_FlashingIcons.end(), "Flashing icon with this type already exists");

        CUIStatic*& val = m_FlashingIcons[type];
        val = pIcon;

        AttachChild(pIcon);
        pIcon->Show(false);
    }
}

void CUIMainIngameWnd::DestroyFlashingIcons()
{
    for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
    {
        DetachChild(it->second);
        xr_delete(it->second);
    }

    m_FlashingIcons.clear();
}

void CUIMainIngameWnd::UpdateFlashingIcons()
{
    for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
    {
        auto pda = m_pActor->GetPDA();
        if (pda && pda->IsPowerOn())
            it->second->Update();
        else
            it->second->Show(false);
    }
}

void CUIMainIngameWnd::AnimateContacts(bool b_snd)
{
    if (!m_pActor->GetPDA() || !m_pActor->GetPDA()->IsPowerOn())
        return;

    UIPdaOnline.ResetClrAnimation();

    if (b_snd)
        HUD_SOUND::PlaySound(m_contactSnd, Fvector().set(0, 0, 0), 0, true);
}

void CUIMainIngameWnd::SetPickUpItem(CInventoryItem* PickUpItem)
{
    //	m_pPickUpItem = PickUpItem;
    if (m_pPickUpItem != PickUpItem)
    {
        m_pPickUpItem = PickUpItem;
        UIPickUpItemIcon.Show(false);
        UIPickUpItemIcon.DetachAll();
    }
};

#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../Actor.h"

void CUIMainIngameWnd::UpdatePickUpItem()
{
    if (!m_pPickUpItem || !Level().CurrentViewEntity() || Level().CurrentViewEntity()->CLS_ID != CLSID_OBJECT_ACTOR)
    {
        if (UIPickUpItemIcon.IsShown())
        {
            UIPickUpItemIcon.Show(false);
        }

        return;
    };
    if (UIPickUpItemIcon.IsShown())
        return;

    // properties used by inventory menu
    CIconParams& params = m_pPickUpItem->m_icon_params;
    Frect rect = params.original_rect();

    float scale_x = m_iPickUpItemIconWidth / rect.width();

    float scale_y = m_iPickUpItemIconHeight / rect.height();

    scale_x = (scale_x > 1) ? 1.0f : scale_x;
    scale_y = (scale_y > 1) ? 1.0f : scale_y;

    float scale = scale_x < scale_y ? scale_x : scale_y;

    params.set_shader(&UIPickUpItemIcon);

    UIPickUpItemIcon.SetWidth(rect.width() * scale * UI()->get_current_kx());
    UIPickUpItemIcon.SetHeight(rect.height() * scale);

    UIPickUpItemIcon.SetWndPos(m_iPickUpItemIconX + (m_iPickUpItemIconWidth - UIPickUpItemIcon.GetWidth()) / 2,
                               m_iPickUpItemIconY + (m_iPickUpItemIconHeight - UIPickUpItemIcon.GetHeight()) / 2);

    UIPickUpItemIcon.SetColor(color_rgba(255, 255, 255, 192));

    TryAttachIcons(&UIPickUpItemIcon, m_pPickUpItem, scale);

    // Real Wolf: Колбек для скриптового добавления своих иконок. 10.08.2014.
    g_actor->callback(GameObject::eUIPickUpItemShowing)(m_pPickUpItem->object().lua_game_object(), &UIPickUpItemIcon);

    UIPickUpItemIcon.Show(true);
};

void CUIMainIngameWnd::UpdateActiveItemInfo()
{
    PIItem item = m_pActor->inventory().ActiveItem();
    bool show_info = item && item->NeedBriefInfo() && (m_bShowActiveItemInfo || m_bShowGearInfo);

    UIWeaponBack.Show(show_info);
    UIWeaponSignAmmo.Show(show_info);

    if (show_info)
    {
        xr_string str_name;
        xr_string icon_sect_name;
        xr_string str_count;

        item->GetBriefInfo(str_name, icon_sect_name, str_count);

        UIWeaponBack.SetText(str_name.c_str());
        UIWeaponSignAmmo.SetText(str_count.c_str());
        SetAmmoIcon(icon_sect_name.c_str());
    }
}

void CUIMainIngameWnd::OnConnected() { UIZoneMap->SetupCurrentMap(); }

void CUIMainIngameWnd::reset_ui()
{
    m_pActor = NULL;
    m_pPickUpItem = NULL;
    UIMotionIcon.ResetVisibility();
}

#include "../xr_3da/XR_IOConsole.h"
bool CUIMainIngameWnd::OnKeyboardHold(int cmd)
{
    if (Console->bVisible)
        return false;
    return Level().IR_GetKeyState(cmd);
}

using namespace luabind::detail;

template <typename T>
bool test_push_window(lua_State* L, CUIWindow* wnd)
{
    T* derived = smart_cast<T*>(wnd);
    if (derived)
    {
        convert_to_lua<T*>(L, derived);
        return true;
    }
    return false;
}

void GetStaticRaw(CUIMainIngameWnd* wnd, lua_State* L)
{
    // wnd->GetChildWndList();
    shared_str name = lua_tostring(L, 2);
    CUIWindow* child = wnd->FindChild(name, 2);
    if (!child)
    {
        CUIStatic* src = &wnd->GetUIZoneMap()->Background();
        child = src->FindChild(name, 5);

        if (!child)
        {
            src = &wnd->GetUIZoneMap()->ClipFrame();
            child = src->FindChild(name, 5);
        }
        if (!child)
        {
            src = &wnd->GetUIZoneMap()->Compass();
            child = src->FindChild(name, 5);
        }
    }

    if (child)
    {
        // if (test_push_window<CUIMotionIcon>  (L, child)) return;
        if (test_push_window<CUIProgressBar>(L, child))
            return;
        if (test_push_window<CUIStatic>(L, child))
            return;
        if (test_push_window<CUIWindow>(L, child))
            return;
    }
    lua_pushnil(L);
}

using namespace luabind;

#pragma optimize("s", on)
void CUIMainIngameWnd::script_register(lua_State* L)
{
    module(L)[

        class_<CUIMainIngameWnd, CUIWindow>("CUIMainIngameWnd")
            .def("GetStatic", &GetStaticRaw, raw<2>())
            .def_readwrite("show_zone_map", &CUIMainIngameWnd::m_bShowZoneMap)
            .def_readwrite("show_active_item_info", &CUIMainIngameWnd::m_bShowActiveItemInfo)
            .def_readwrite("show_gear_info", &CUIMainIngameWnd::m_bShowGearInfo)
        ,
        def("get_main_window", &GetMainIngameWindow) // get_mainingame_window better??
        ,
        def("setup_game_icon", &SetupGameIcon)];
}