#include "stdafx.h"

#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"

#include <dinput.h>
#include "../HUDManager.h"

#include "../game_news.h"
#include "../xr_3da/xr_input.h"

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

void CUIMainIngameWnd::Init()
{
    CUIWindow::Init(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
    Enable(false);
}

bool CUIMainIngameWnd::OnKeyboardPress(int dik)
{
    const auto bind = get_binded_action(dik);

    if (bind == kHIDEHUD)
    {
        HUD().GetUI()->HideGameIndicators();
        HUD().GetUI()->hud_disabled_by_user = true;
        return true;
    }
    else if (bind == kSHOWHUD)
    {
        HUD().GetUI()->ShowGameIndicators();
        HUD().GetUI()->hud_disabled_by_user = false;
        return true;
    }

    return false;
}

void CUIMainIngameWnd::ReceiveNews(GAME_NEWS_DATA* news)
{
    VERIFY(news->texture_name.size());

    HUD().GetUI()->m_pMessagesWnd->AddIconedPdaMessage(*(news->texture_name), news->tex_rect, news->SingleLineText(), news->show_time);
}

using namespace luabind;

void CUIMainIngameWnd::script_register(lua_State* L)
{
    module(L)[(
        class_<CUIMainIngameWnd, CUIWindow>("CUIMainIngameWnd"),
            def("get_main_window", &GetMainIngameWindow) // get_mainingame_window better??
    )];
}
