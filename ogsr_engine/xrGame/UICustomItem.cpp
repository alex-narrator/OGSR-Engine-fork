#include "stdafx.h"
#include "uicustomitem.h"
#include "hudmanager.h"

void CUICustomItem::Render(const Fvector2& pos, u32 color, float x1, float y1, float x2, float y2)
{
    Fvector2 ts;
    UIRender->GetActiveTextureResolution(ts);

    if (!uFlags.test(flValidRect))
    {
        SetRect(0, 0, ts.x, ts.y);
    }
    if (!uFlags.test(flValidOriginalRect))
    {
        iOriginalRect.set(0, 0, ts.x, ts.y);
        uFlags.set(flValidOriginalRect, TRUE);
    }

    Fvector2 LTp, RBp;
    Fvector2 LTt, RBt;
    //координаты на экране в пикселях
    UI()->ClientToScreenScaled(LTp, x1, y1);
    LTp.add(pos);

    UI()->ClientToScreenScaled(RBp, x2, y2);
    RBp.add(pos);

    //текстурные координаты
    LTt.set(iOriginalRect.x1 / ts.x, iOriginalRect.y1 / ts.y);
    RBt.set(iOriginalRect.x2 / ts.x, iOriginalRect.y2 / ts.y);

    float offset = -0.5f;
    if (UI()->m_currentPointType == IUIRender::pttLIT)
        offset = 0.0f;
    // clip poly
    sPoly2D S;
    S.resize(4);
    S[0].set(LTp.x + offset, LTp.y + offset, LTt.x, LTt.y); // LT
    S[1].set(RBp.x + offset, LTp.y + offset, RBt.x, LTt.y); // RT
    S[2].set(RBp.x + offset, RBp.y + offset, RBt.x, RBt.y); // RB
    S[3].set(LTp.x + offset, RBp.y + offset, LTt.x, RBt.y); // LB

    sPoly2D D;
    sPoly2D* R = NULL;

    if (UI()->m_currentPointType != IUIRender::pttLIT)
        R = UI()->ScreenFrustum().ClipPoly(S, D);
    else
        R = UI()->ScreenFrustumLIT().ClipPoly(S, D);

    if (R && R->size())
    {
        for (u32 k = 0; k < R->size() - 2; ++k)
        {
            UIRender->PushPoint((*R)[0 + 0].pt.x, (*R)[0 + 0].pt.y, 0, color, (*R)[0 + 0].uv.x, (*R)[0 + 0].uv.y);
            UIRender->PushPoint((*R)[k + 1].pt.x, (*R)[k + 1].pt.y, 0, color, (*R)[k + 1].uv.x, (*R)[k + 1].uv.y);
            UIRender->PushPoint((*R)[k + 2].pt.x, (*R)[k + 2].pt.y, 0, color, (*R)[k + 2].uv.x, (*R)[k + 2].uv.y);
        }
    }
}
//--------------------------------------------------------------------
void CUICustomItem::Render(const Fvector2& pos, u32 color) { Render(pos, color, iVisRect.x1, iVisRect.y1, iVisRect.x2, iVisRect.y2); }
//--------------------------------------------------------------------

void CUICustomItem::Render(const Fvector2& pos_ns, u32 color, float angle)
{
    Fvector2 ts;
    Fvector2 hp;

    UIRender->GetActiveTextureResolution(ts);
    hp.set(0.5f / ts.x, 0.5f / ts.y);

    if (!uFlags.test(flValidRect))
    {
        SetRect(0, 0, ts.x, ts.y);
    }

    if (!uFlags.test(flValidOriginalRect))
    {
        iOriginalRect.set(0, 0, ts.x, ts.y);
        uFlags.set(flValidOriginalRect, TRUE);
    }

    Fvector2 pivot, offset, SZ;
    SZ.set(iVisRect.rb);

    //	UI()->ClientToScreenScaled			(SZ, iVisRect.x2, iVisRect.y2);

    float cosA = _cos(angle);
    float sinA = _sin(angle);

    // Rotation
    if (!uFlags.test(flValidHeadingPivot))
        pivot.set(iVisRect.x2 / 2.f, iVisRect.y2 / 2.f);
    else
        pivot.set(iHeadingPivot.x, iHeadingPivot.y);

    //.	UI()->ClientToScreenScaled			(pivot, pivot.x, pivot.y);
    // pivot.set							(pivot); //???
    offset.set(pos_ns);
    offset.add(iHeadingOffset);

    Fvector2 LTt, RBt;
    LTt.set(iOriginalRect.x1 / ts.x + hp.x, iOriginalRect.y1 / ts.y + hp.y);
    RBt.set(iOriginalRect.x2 / ts.x + hp.x, iOriginalRect.y2 / ts.y + hp.y);

    //	float kx = (UI()->is_widescreen())?0.8333f: 1.0f;
    const bool is_3d_ui = UI()->m_currentPointType == IUIRender::pttLIT;
    float kx = UI()->is_widescreen() && !is_3d_ui ? UI()->get_current_kx() : 1.0f;
    // clip poly
    sPoly2D S;
    S.resize(4);
    // LT
    S[0].set(0.f, 0.f, LTt.x, LTt.y);
    S[0].rotate_pt(pivot, cosA, sinA, kx);
    S[0].pt.add(offset);

    // RT
    S[1].set(SZ.x, 0.f, RBt.x, LTt.y);
    S[1].rotate_pt(pivot, cosA, sinA, kx);
    S[1].pt.add(offset);
    // RB
    S[2].set(SZ.x, SZ.y, RBt.x, RBt.y);
    S[2].rotate_pt(pivot, cosA, sinA, kx);
    S[2].pt.add(offset);
    // LB
    S[3].set(0.f, SZ.y, LTt.x, RBt.y);
    S[3].rotate_pt(pivot, cosA, sinA, kx);
    S[3].pt.add(offset);

    for (int i = 0; i < 4; ++i)
        UI()->ClientToScreenScaled(S[i].pt);

    sPoly2D D;
    sPoly2D* R = UI()->ScreenFrustum().ClipPoly(S, D);
    if (R && R->size())
    {
        for (u32 k = 0; k < R->size() - 2; k++)
        {
            UIRender->PushPoint((*R)[0 + 0].pt.x, (*R)[0 + 0].pt.y, 0, color, (*R)[0 + 0].uv.x, (*R)[0 + 0].uv.y);
            UIRender->PushPoint((*R)[k + 1].pt.x, (*R)[k + 1].pt.y, 0, color, (*R)[k + 1].uv.x, (*R)[k + 1].uv.y);
            UIRender->PushPoint((*R)[k + 2].pt.x, (*R)[k + 2].pt.y, 0, color, (*R)[k + 2].uv.x, (*R)[k + 2].uv.y);
        }
    }
}

void CUICustomItem::Render(const Fvector2& pos, u32 color, float angle, u32 segments_count)
{
    Fvector2 ts;
    Fvector2 hp;

    UIRender->GetActiveTextureResolution(ts);
    hp.set(0.5f / ts.x, 0.5f / ts.y);

    if (!uFlags.test(flValidRect))
    {
        SetRect(0, 0, ts.x, ts.y);
    }

    if (!uFlags.test(flValidOriginalRect))
    {
        iOriginalRect.set(0, 0, ts.x, ts.y);
        uFlags.set(flValidOriginalRect, TRUE);
    }

    Fvector2 pivot, offset;
    if (!uFlags.test(flValidHeadingPivot))
        pivot.set(iVisRect.x2 * 0.5f, iVisRect.y2 * 0.5f);
    else
        pivot.set(iHeadingPivot.x, iHeadingPivot.y);

    offset.set(pos);
    offset.add(iHeadingOffset);

    Fvector2 radius_pos{iVisRect.x2 * 0.5f, iVisRect.y2 * 0.5f};

    Frect tex_rect = iOriginalRect;

    float uv_x1 = tex_rect.x1 / ts.x + hp.x;
    float uv_y1 = tex_rect.y1 / ts.y + hp.y;
    float uv_w = tex_rect.width() / ts.x;
    float uv_h = tex_rect.height() / ts.y;

    Fvector2 center_tex{uv_x1 + uv_w * 0.5f, uv_y1 + uv_h * 0.5f};

    Fvector2 radius_tex{uv_w * 0.5f, uv_h * 0.5f};

    Fvector2 uv_pivot_offset{(pivot.x - (iVisRect.x2 * 0.5f)) / ts.x, (pivot.y - (iVisRect.y2 * 0.5f)) / ts.y};

    const bool is_3d_ui = UI()->m_currentPointType == IUIRender::pttLIT;
    float kx = UI()->is_widescreen() && !is_3d_ui ? UI()->get_current_kx() : 1.0f;

    float angle_step = PI_MUL_2 / float(segments_count);

    Fvector2 lt_clip{offset.x, offset.y};
    Fvector2 rb_clip{offset.x + iVisRect.x2, offset.y + iVisRect.y2};

    UI()->ClientToScreenScaled(lt_clip);
    UI()->ClientToScreenScaled(rb_clip);

    Frect scissor_rect = {lt_clip.x, lt_clip.y, rb_clip.x, rb_clip.y};
    UI()->PushScissor(scissor_rect);

    UIRender->StartPrimitive(segments_count * 3, IUIRender::ptTriList, UI()->m_currentPointType);

    xr_vector<Fvector2> screen_pts(segments_count);
    xr_vector<Fvector2> texture_uvs(segments_count);

    for (u32 i = 0; i < segments_count; ++i)
    {
        float ch_angle = angle_step * float(i);

        screen_pts[i].set(pivot.x + radius_pos.x * _cos(ch_angle) * kx, pivot.y - radius_pos.y * _sin(ch_angle));
        screen_pts[i].add(offset);
        UI()->ClientToScreenScaled(screen_pts[i]);

        texture_uvs[i].x = center_tex.x + radius_tex.x * _cos(ch_angle - angle) + uv_pivot_offset.x;
        texture_uvs[i].y = center_tex.y - radius_tex.y * _sin(ch_angle - angle) + uv_pivot_offset.y;
    }

    Fvector2 final_center_pos{pivot.x, pivot.y};
    final_center_pos.add(offset);
    UI()->ClientToScreenScaled(final_center_pos);

    Fvector2 final_center_tex{center_tex.x + uv_pivot_offset.x, center_tex.y + uv_pivot_offset.y};

    for (u32 i = 0; i < segments_count; ++i)
    {
        u32 next_i = (i + 1) % segments_count;

        UIRender->PushPoint(final_center_pos.x, final_center_pos.y, 0, color, final_center_tex.x, final_center_tex.y);
        UIRender->PushPoint(screen_pts[next_i].x, screen_pts[next_i].y, 0, color, texture_uvs[next_i].x, texture_uvs[next_i].y);
        UIRender->PushPoint(screen_pts[i].x, screen_pts[i].y, 0, color, texture_uvs[i].x, texture_uvs[i].y);
    }

    UIRender->FlushPrimitive();

    UI()->PopScissor();
}

Frect CUICustomItem::GetOriginalRectScaled()
{
    Frect rect = iOriginalRect;

    rect.x2 = rect.x1 + rect.width();
    rect.y2 = rect.y1 + rect.height();

    return rect;
}

Frect CUICustomItem::GetOriginalRect() const { return iOriginalRect; }

void CUICustomItem::SetOriginalRect(float x, float y, float width, float height)
{
    iOriginalRect.set(x, y, x + width, y + height);
    uFlags.set(flValidOriginalRect, TRUE);
}

void CUICustomItem::ResetHeadingPivot()
{
    uFlags.set(flValidHeadingPivot, FALSE);
    uFlags.set(flFixedLTWhileHeading, FALSE);
}

void CUICustomItem::SetHeadingPivot(const Fvector2& p, const Fvector2& offset, bool fixedLT)
{
    iHeadingPivot = p;
    iHeadingOffset = offset;
    uFlags.set(flValidHeadingPivot, TRUE);
    if (fixedLT)
        uFlags.set(flFixedLTWhileHeading, TRUE);
    else
        uFlags.set(flFixedLTWhileHeading, FALSE);
}
