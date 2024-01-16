#include "stdafx.h"

#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"
#include "../UIZoneMap.h"

#include <dinput.h>
#include "../actor.h"
#include "../HUDManager.h"
#include "../PDA.h"
#include "WeaponMagazined.h"
#include "../character_info.h"
#include "../inventory.h"
#include "../UIGameSP.h"
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
#include "UIMap.h"

#ifdef DEBUG
#include "../attachable_item.h"
#include "..\..\xr_3da\xr_input.h"
#endif

#include "map_hint.h"
#include "UIColorAnimatorWrapper.h"
#include "../game_news.h"

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

CUIMainIngameWnd::CUIMainIngameWnd()
{
    m_pActor = nullptr;
    UIZoneMap = xr_new<CUIZoneMap>();
    m_pPickUpItem = nullptr;
}

CUIMainIngameWnd::~CUIMainIngameWnd()
{
    DestroyFlashingIcons();
    xr_delete(UIZoneMap);
    HUD_SOUND::DestroySound(m_contactSnd);
}

void CUIMainIngameWnd::Init()
{
    CUIXml uiXml;
    uiXml.Init(CONFIG_PATH, UI_PATH, MAININGAME_XML);

    CUIXmlInit xml_init;
    CUIWindow::Init(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

    Enable(false);

    //---------------------------------------------------------
    AttachChild(&UIPickUpItemIcon);
    xml_init.InitStatic(uiXml, "pick_up_item", 0, &UIPickUpItemIcon);
    UIPickUpItemIcon.SetShader(GetEquipmentIconsShader());
    UIPickUpItemIcon.Show(false);

    m_iPickUpItemIconWidth = UIPickUpItemIcon.GetWidth();
    m_iPickUpItemIconHeight = UIPickUpItemIcon.GetHeight();
    m_iPickUpItemIconX = UIPickUpItemIcon.GetWndRect().left;
    m_iPickUpItemIconY = UIPickUpItemIcon.GetWndRect().top;
    //---------------------------------------------------------


    // индикаторы
    UIZoneMap->Init();
    UIZoneMap->SetScale(DEFAULT_MAP_SCALE);

    xml_init.InitStatic(uiXml, "static_pda_online", 0, &UIPdaOnline);
    UIZoneMap->Background().AttachChild(&UIPdaOnline);

    // Подсказки, которые возникают при наведении прицела на объект
    AttachChild(&UIStaticQuickHelp);
    xml_init.InitStatic(uiXml, "quick_info", 0, &UIStaticQuickHelp);

    uiXml.SetLocalRoot(uiXml.GetRoot());

    // Flashing icons initialize
    uiXml.SetLocalRoot(uiXml.NavigateToNode("flashing_icons"));
    InitFlashingIcons(&uiXml);

    uiXml.SetLocalRoot(uiXml.GetRoot());

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

    UIZoneMap->UpdateRadar(Device.vCameraPosition);
    float h, p;
    Device.vCameraDirection.getHP(h, p);
    UIZoneMap->SetHeading(-h);

    UpdatePickUpItem();

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

void CUIMainIngameWnd::OnConnected() { UIZoneMap->SetupCurrentMap(); }

void CUIMainIngameWnd::reset_ui()
{
    m_pActor = NULL;
    m_pPickUpItem = NULL;
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
            .def_readwrite("show_zone_map", &CUIMainIngameWnd::m_bShowZoneMap),
        def("get_main_window", &GetMainIngameWindow) // get_mainingame_window better??
    ];
}