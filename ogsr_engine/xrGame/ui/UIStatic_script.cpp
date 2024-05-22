#include "stdafx.h"
#include "UIStatic.h"
#include "UIMap.h"

void UIMiniMapZoom(CUIMiniMap* wnd, float scale)
{
    Fvector2 wnd_size;
    float zoom_factor = float(wnd->GetParent()->GetWndRect().width()) / 100.0f;
    wnd_size.x = wnd->BoundRect().width() * zoom_factor * scale /*m_fScale*/;
    wnd_size.y = wnd->BoundRect().height() * zoom_factor * scale /*m_fScale*/;
    wnd->SetWndSize(wnd_size);
}

#include "Level.h"
void UIMiniMapInit(CUIMiniMap* wnd)
{
    CUIWindow* parent = wnd->GetParent();

    CInifile* pLtx = pGameIni;

    if (!pLtx->section_exist(Level().name()))
        pLtx = Level().pLevel;

    wnd->Init(Level().name(), *pLtx, "hud\\default");

    Frect r;
    parent->GetAbsoluteRect(r);
    wnd->SetClipRect(r);
    wnd->WorkingArea().set(r);

    UIMiniMapZoom(wnd, 1.f);
}

void CUIStatic__set_color_a(CUIStatic* self, u8 alpha) { self->SetColor(subst_alpha(self->GetColor(), alpha)); }

using namespace luabind;

#pragma optimize("s", on)
void CUIStatic::script_register(lua_State* L)
{
    module(L)[class_<CUIStatic, CUIWindow>("CUIStatic")
                  .def(constructor<>())

                  .def("SetText", (void(CUIStatic::*)(LPCSTR))(&CUIStatic::SetText))
                  .def("SetTextST", (void(CUIStatic::*)(LPCSTR))(&CUIStatic::SetTextST))
                  .def("GetText", &CUIStatic::GetText)

                  .def("SetTextX", &CUIStatic::SetTextX)
                  .def("SetTextY", &CUIStatic::SetTextY)
                  .def("GetTextX", &CUIStatic::GetTextX)
                  .def("GetTextY", &CUIStatic::GetTextY)

                  .def("SetColor", &CUIStatic::SetColor)
                  .def("GetColor", &CUIStatic::GetColor)
                  .def("SetColorA", &CUIStatic__set_color_a)
                  .def("SetTextColor", &CUIStatic::SetTextColor_script)
                  .def("SetTextColor", (void(CUIStatic::*)(u32)) & CUIStatic::SetTextColor)
                  .def("GetTextColor", &CUIStatic::GetTextColor)
                  .def("Init", (void(CUIStatic::*)(float, float, float, float)) & CUIStatic::Init)
                  .def("Init", (void(CUIStatic::*)(LPCSTR, float, float, float, float)) & CUIStatic::Init)
                  .def("InitTexture", &CUIStatic::InitTexture)
                  .def("InitTextureEx", &CUIStatic::InitTextureEx)
                  .def("SetTextureOffset", &CUIStatic::SetTextureOffset)

                  .def("GetOriginalRect", &CUIStatic::GetOriginalRect)
                  .def("SetOriginalRect", (void(CUIStatic::*)(float, float, float, float)) & CUIStatic::SetOriginalRect)
                  .def("ResetOriginalRect", &CUIStatic::ResetOriginalRect)
                  .def("SetStretchTexture", &CUIStatic::SetStretchTexture)
                  .def("GetStretchTexture", &CUIStatic::GetStretchTexture)

                  .def("SetTextAlign", &CUIStatic::SetTextAlign_script)
                  .def("GetTextAlign", &CUIStatic::GetTextAlign_script)

                  .def("SetHeading", &CUIStatic::SetHeading)
                  .def("GetHeading", &CUIStatic::GetHeading)
                  .def("EnableHeading", &CUIStatic::EnableHeading)
                  .def("SetHeadingPivot", &CUIStatic::SetHeadingPivot)

                  .def("ClipperOn", &CUIStatic::ClipperOn)
                  .def("ClipperOff", (void(CUIStatic::*)(void)) & CUIStatic::ClipperOff)
                  .def("GetClipperState", &CUIStatic::GetClipperState)
                  .def("SetClipRect", &CUIStatic::SetClipRect)
                  .def("GetClipRect", &CUIStatic::GetClipperRect)

                  .def("SetTextComplexMode", &CUIStatic::SetTextComplexMode)
                  .def("AdjustWidthToText", &CUIStatic::AdjustWidthToText)
                  .def("AdjustHeightToText", &CUIStatic::AdjustHeightToText)
                  .def("SetVTextAlign", &CUIStatic::SetVTextAlignment)
                  .def("SetTextPos", &CUIStatic::SetTextPos),

            class_<CUIMiniMap, CUIStatic>("CUIMiniMap")
                  .def(constructor<>())
                  .def("SetRounded", &CUIMiniMap::SetRounded)
                  .def("SetLocked", &CUIMiniMap::SetLocked)
                  .def("Init", &UIMiniMapInit)
                  .def("Zoom", &UIMiniMapZoom)
                  .def("SetActivePoint", &CUIMiniMap::SetActivePoint)
                  .def("GetActivePointDist", &CUIMiniMap::GetPointerDistance)
    ];
}
