#include "stdafx.h"

#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"
#include "../UIZoneMap.h"

#include <dinput.h>
#include "../actor.h"
#include "../HUDManager.h"
#include "../UIGameSP.h"

#include "UIMap.h"

#ifdef DEBUG
#include "../attachable_item.h"
#include "..\..\xr_3da\xr_input.h"
#endif

#include "../game_news.h"

constexpr auto DEFAULT_MAP_SCALE = 1.f;

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
}

CUIMainIngameWnd::~CUIMainIngameWnd() { xr_delete(UIZoneMap); }

void CUIMainIngameWnd::Init()
{
    CUIWindow::Init(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

    Enable(false);

    // индикаторы
    UIZoneMap->Init();
    UIZoneMap->SetScale(DEFAULT_MAP_SCALE);
}

void CUIMainIngameWnd::Draw()
{
    if (!m_pActor)
        return;

    CUIWindow::Draw();

    if (m_bShowZoneMap)
        UIZoneMap->Render();
}

void CUIMainIngameWnd::Update()
{
    m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!m_pActor)
    {
        CUIWindow::Update();
        return;
    }

    UIZoneMap->UpdateRadar(Device.vCameraPosition);
    float h, p;
    Device.vCameraDirection.getHP(h, p);
    UIZoneMap->SetHeading(-h);

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

void CUIMainIngameWnd::ReceiveNews(GAME_NEWS_DATA* news)
{
    VERIFY(news->texture_name.size());
    HUD().GetUI()->m_pMessagesWnd->AddIconedPdaMessage(*(news->texture_name), news->tex_rect, news->SingleLineText(), news->show_time);
}

void CUIMainIngameWnd::OnConnected() { UIZoneMap->SetupCurrentMap(); }

void CUIMainIngameWnd::reset_ui() { m_pActor = nullptr; }

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
        CUIStatic* src = wnd->GetUIZoneMap()->Background();
        child = src->FindChild(name, 5);

        if (!child)
        {
            src = wnd->GetUIZoneMap()->ClipFrame();
            child = src->FindChild(name, 5);
        }
    }

    if (child)
    {
        // if (test_push_window<CUIMotionIcon>  (L, child)) return;
        if (test_push_window<CUIMiniMap>(L, child))
            return;
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