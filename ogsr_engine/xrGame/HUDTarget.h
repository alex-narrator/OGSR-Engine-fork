#pragma once

#include "..\xrcdb\xr_collide_defs.h"

constexpr u32 C_ON_ENEMY = D3DCOLOR_XRGB(0xff, 0, 0), C_ON_NEUTRAL = D3DCOLOR_XRGB(0xff, 0xff, 0x80), C_ON_FRIEND = D3DCOLOR_XRGB(0, 0xff, 0),
              C_DEFAULT = D3DCOLOR_XRGB(0xff, 0xff, 0xff);

class CHUDManager;

class CHUDTarget
{
private:
    friend class CHUDManager;

private:
    typedef collide::rq_result rq_result;
    typedef collide::rq_results rq_results;

private:
    rq_result RQ;
    rq_results RQR;
    float m_real_dist;

private:
    Fvector2 crosshair_pos{};

private:
    void net_Relcase(CObject* O);

public:
    CHUDTarget();
    void CursorOnFrame();
    void Render();
    float GetDist();
    float GetRealDist();
    CObject* GetObj();
    Fvector2 GetCrosshairPos() const { return crosshair_pos; };
};
