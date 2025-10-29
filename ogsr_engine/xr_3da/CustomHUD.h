#pragma once

#include "iinputreceiver.h"

ENGINE_API extern Flags32 psHUD_Flags;

enum HUD_Flags : u32
{
    HUD_DRAW            = 1 << 0,
    HUD_CROSSHAIR_HARD  = 1 << 1, // Усложненный режим прицела - оружие от бедра будет стрелять не по центру камеры, а по реальному направлению ствола
    HUD_DRAW_RT         = 1 << 2,
};

class CUI;

class ENGINE_API CCustomHUD : public DLL_Pure, public IEventReceiver
{
public:
    CCustomHUD();
    virtual ~CCustomHUD();

    virtual void Load() { }

    virtual void Render_SMAP(u32 context_id) {}
    virtual void Render_MAIN(u32 context_id) {}

    virtual void OnFrame() { }
    virtual void OnEvent(EVENT E, u64 P1, u64 P2) { ; }

    virtual IC CUI* GetUI() = 0;
    virtual void OnScreenRatioChanged() = 0;
    virtual void OnDisconnected() = 0;
    virtual void OnConnected() = 0;
    virtual void RenderActiveItemUI() = 0;
    virtual bool RenderActiveItemUIQuery() = 0;
    virtual void net_Relcase(CObject* object) = 0;
};

extern ENGINE_API CCustomHUD* g_hud;