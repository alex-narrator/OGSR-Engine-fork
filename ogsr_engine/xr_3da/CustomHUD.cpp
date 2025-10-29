#include "stdafx.h"
#include "CustomHUD.h"

Flags32 psHUD_Flags{HUD_DRAW | HUD_DRAW_RT};

ENGINE_API CCustomHUD* g_hud = nullptr;

CCustomHUD::CCustomHUD()
{
    // g_hud = this; ???
}

CCustomHUD::~CCustomHUD()
{
    g_hud = nullptr;
}
