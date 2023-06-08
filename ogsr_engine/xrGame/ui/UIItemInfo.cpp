#include "stdafx.h"

#include "uiiteminfo.h"
#include "uistatic.h"
#include "UIXmlInit.h"

#include "UIListWnd.h"
#include "UIProgressBar.h"
#include "UIScrollView.h"
#include "UIFrameWindow.h"

#include "../string_table.h"
#include "../Inventory_Item.h"
#include "UIInventoryUtilities.h"
#include "../PhysicsShellHolder.h"
#include "UIWpnParams.h"
#include "ui_af_params.h"
#include "UIPowerParams.h"
#include "UIEatableParams.h"
#include "UIArmorParams.h"

#include "Weapon.h"

CUIItemInfo::CUIItemInfo() {}

CUIItemInfo::~CUIItemInfo()
{
    xr_delete(UIWpnParams);
    xr_delete(UIArtefactParams);
    xr_delete(UIPowerParams);
    xr_delete(UIEatableParams);
    xr_delete(UIArmorParams);
}

void CUIItemInfo::Init(LPCSTR xml_name)
{
    CUIXml uiXml;
    bool xml_result = uiXml.Init(CONFIG_PATH, UI_PATH, xml_name);
    R_ASSERT2(xml_result, "xml file not found");

    CUIXmlInit xml_init;

    if (uiXml.NavigateToNode("main_frame", 0))
    {
        Frect wnd_rect{};
        wnd_rect.x1 = uiXml.ReadAttribFlt("main_frame", 0, "x", 0);
        wnd_rect.y1 = uiXml.ReadAttribFlt("main_frame", 0, "y", 0);

        wnd_rect.x2 = uiXml.ReadAttribFlt("main_frame", 0, "width", 0);
        wnd_rect.y2 = uiXml.ReadAttribFlt("main_frame", 0, "height", 0);
        wnd_rect.x2 += wnd_rect.x1;
        wnd_rect.y2 += wnd_rect.y1;
        inherited::SetWndRect(wnd_rect);

        show_delay = uiXml.ReadAttribFlt("main_frame", 0, "delay", 1);
        info_offset.x = uiXml.ReadAttribFlt("main_frame", 0, "offset_x");
        info_offset.y = uiXml.ReadAttribFlt("main_frame", 0, "offset_y");

        min_height = uiXml.ReadAttribFlt("main_frame", 0, "min_height");

        //inherited::Init(wnd_rect.x1, wnd_rect.y1, wnd_rect.x2, wnd_rect.y2);
    }

    if (uiXml.NavigateToNode("background_frame", 0))
    {
        UIBackground = xr_new<CUIFrameWindow>();
        UIBackground->SetAutoDelete(true);
        AttachChild(UIBackground);
        xml_init.InitFrameWindow(uiXml, "background_frame", 0, UIBackground);
    }

    if (uiXml.NavigateToNode("static_name", 0))
    {
        UIName = xr_new<CUIStatic>();
        AttachChild(UIName);
        UIName->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "static_name", 0, UIName);
    }
    if (uiXml.NavigateToNode("static_weight", 0))
    {
        UIWeight = xr_new<CUIStatic>();
        AttachChild(UIWeight);
        UIWeight->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "static_weight", 0, UIWeight);
    }
    if (uiXml.NavigateToNode("static_cost", 0))
    {
        UICost = xr_new<CUIStatic>();
        AttachChild(UICost);
        UICost->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "static_cost", 0, UICost);
    }

    if (uiXml.NavigateToNode("static_condition", 0))
    {
        UICondition = xr_new<CUIStatic>();
        AttachChild(UICondition);
        UICondition->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "static_condition", 0, UICondition);
    }

    if (uiXml.NavigateToNode("condition_progress", 0))
    {
        UICondProgresBar = xr_new<CUIProgressBar>();
        AttachChild(UICondProgresBar);
        UICondProgresBar->SetAutoDelete(true);
        xml_init.InitProgressBar(uiXml, "condition_progress", 0, UICondProgresBar);
    }

    if (uiXml.NavigateToNode("descr_list", 0))
    {
        UIWpnParams = xr_new<CUIWpnParams>();
        UIWpnParams->Init();

        UIArtefactParams = xr_new<CUIArtefactParams>();
        UIArtefactParams->Init();

        UIPowerParams = xr_new<CUIPowerParams>();
        UIPowerParams->Init();

        UIEatableParams = xr_new<CUIEatableParams>();
        UIEatableParams->Init();

        UIArmorParams = xr_new<CUIArmorParams>();
        UIArmorParams->Init();

        UIDesc = xr_new<CUIScrollView>();
        AttachChild(UIDesc);
        UIDesc->SetAutoDelete(true);
        xml_init.InitScrollView(uiXml, "descr_list", 0, UIDesc);

        m_desc_info.bShowDescrText = !!uiXml.ReadAttribInt("descr_list", 0, "only_text_info", 1);
        xml_init.InitFont(uiXml, "descr_list:font", 0, m_desc_info.uDescClr, m_desc_info.pDescFont);
    }

    if (uiXml.NavigateToNode("image_static", 0))
    {
        UIItemImage = xr_new<CUIStatic>();
        AttachChild(UIItemImage);
        UIItemImage->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "image_static", 0, UIItemImage);
        UIItemImage->TextureAvailable(true);

        UIItemImage->TextureOff();
        UIItemImage->ClipperOn();
        UIItemImageSize.set(UIItemImage->GetWidth(), UIItemImage->GetHeight());
    }

    xml_init.InitAutoStaticGroup(uiXml, "auto", 0, this);
}

using namespace InventoryUtilities;

void CUIItemInfo::InitItem(CInventoryItem* pInvItem)
{
    m_pInvItem = pInvItem;
    if (!m_pInvItem)
        return;

    string256 str;
    if (UIName)
    {
        UIName->SetText(pInvItem->Name());
    }
    if (UIWeight)
    {
        sprintf_s(str, "%3.2f %s", pInvItem->Weight(), CStringTable().translate("st_kg").c_str());
        UIWeight->SetText(str);
    }
    if (UICost)
    {
        sprintf_s(str, "%d %s", pInvItem->Cost(), CStringTable().translate("ui_st_money_regional").c_str()); // will be owerwritten in multiplayer
        UICost->SetText(str);
    }
    if (UICondition)
    {
        sprintf_s(str, "%.0f%s", pInvItem->GetCondition() * 100.0f, "%");
        UICondition->SetText(str);
    }

    if (UICondProgresBar)
    {
        float cond = pInvItem->GetCondition();
        UICondProgresBar->Show(true);
        UICondProgresBar->SetProgressPos(cond * 100.0f + 1.0f - EPS);
    }

    if (UIDesc)
    {
        UIDesc->Clear();
        VERIFY(0 == UIDesc->GetSize());
        TryAddWpnInfo(pInvItem);
        TryAddArtefactInfo(pInvItem);
        TryAddPowerInfo(pInvItem);
        TryAddEatableInfo(pInvItem);
        TryAddArmorInfo(pInvItem);
        TryAddCustomInfo(pInvItem);
        if (m_desc_info.bShowDescrText)
        {
            CUIStatic* pItem = xr_new<CUIStatic>();
            pItem->SetTextColor(m_desc_info.uDescClr);
            pItem->SetFont(m_desc_info.pDescFont);
            pItem->SetWidth(UIDesc->GetDesiredChildWidth());
            pItem->SetTextComplexMode(true);
            pItem->SetText(*pInvItem->ItemDescription());
            pItem->AdjustHeightToText();
            UIDesc->AddWindow(pItem, true);
        }
        //
        UIDesc->SetWndSize(Fvector2().set(UIDesc->GetWndSize().x, UIDesc->GetPadSize().y));
        if (UIBackground)
        {
            float _height = _max(min_height, UIDesc->GetWndPos().y + UIDesc->GetWndSize().y);
            UIBackground->SetHeight(_height);
            SetHeight(_height);
        }
        //
        UIDesc->ScrollToBegin();
    }
    if (UIItemImage)
    {
        // Загружаем картинку
        pInvItem->m_icon_params.set_shader(UIItemImage);

        Frect rect = pInvItem->m_icon_params.original_rect();
        UIItemImage->TextureOn();
        UIItemImage->ClipperOn();
        Frect v_r = {0.0f, 0.0f, rect.width(), rect.height()};
        v_r.x2 *= UI()->get_current_kx();

        UIItemImage->GetUIStaticItem().SetRect(v_r);
        UIItemImage->SetWidth(_min(v_r.width(), UIItemImageSize.x));
        UIItemImage->SetHeight(_min(v_r.height(), UIItemImageSize.y));
        
        float scale_x = UIItemImageSize.x / v_r.width();
        float scale_y = UIItemImageSize.y / v_r.height();
        scale_x = (scale_x > 1) ? 1.0f : scale_x;
        scale_y = (scale_y > 1) ? 1.0f : scale_y;
        float scale = scale_x < scale_y ? scale_x : scale_y;

        TryAttachIcons(UIItemImage, pInvItem, scale);
    }
}

void CUIItemInfo::TryAddWpnInfo(CInventoryItem* obj)
{
    if (UIWpnParams->Check(obj))
    {
        UIWpnParams->SetInfo(obj);
        UIDesc->AddWindow(UIWpnParams, false);
    }
}

void CUIItemInfo::TryAddArtefactInfo(CInventoryItem* obj)
{
    if (UIArtefactParams->Check(obj))
    {
        UIArtefactParams->SetInfo(obj);
        UIDesc->AddWindow(UIArtefactParams, false);
    }
}

void CUIItemInfo::TryAddPowerInfo(CInventoryItem* obj)
{
    if (UIPowerParams->Check(obj))
    {
        UIPowerParams->SetInfo(obj);
        UIDesc->AddWindow(UIPowerParams, false);
    }
}

void CUIItemInfo::TryAddEatableInfo(CInventoryItem* obj)
{
    if (UIEatableParams->Check(obj))
    {
        UIEatableParams->SetInfo(obj);
        UIDesc->AddWindow(UIEatableParams, false);
    }
}

void CUIItemInfo::TryAddArmorInfo(CInventoryItem* obj)
{
    if (UIArmorParams->Check(obj))
    {
        UIArmorParams->SetInfo(obj);
        UIDesc->AddWindow(UIArmorParams, false);
    }
}

#include "script_game_object.h"

void CUIItemInfo::TryAddCustomInfo(CInventoryItem* obj)
{
    if (pSettings->line_exist("engine_callbacks", "ui_item_info_callback"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "ui_item_info_callback");
        if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function))
        {
            lua_function(UIDesc, obj->object().lua_game_object());
        }
    }
}

void CUIItemInfo::Draw()
{
    if (m_pInvItem || m_b_force_drawing)
        inherited::Draw();
}

void CUIItemInfo::Update()
{
    if (m_pInvItem && m_pInvItem->NeedForcedDescriptionUpdate() && !m_pInvItem->GetDropManual())
    {
        if (UICondProgresBar)
        {
            float cond = m_pInvItem->GetCondition();
            if (!UICondProgresBar->IsShown())
                UICondProgresBar->Show(true);
            UICondProgresBar->SetProgressPos(cond * 100.0f + 1.0f - EPS);
        }

        if (UICondition)
        {
            string256 str;
            sprintf_s(str, "%.0f%s", m_pInvItem->GetCondition() * 100.0f, "%");
            UICondition->SetText(str);
        }

        if (UIArtefactParams)
        {
            UIArtefactParams->SetInfo(m_pInvItem);
        }

        if (UIPowerParams)
        {
            UIPowerParams->SetInfo(m_pInvItem);
        }
    }

    inherited::Update();
}