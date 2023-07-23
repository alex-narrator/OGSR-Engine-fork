#include "stdafx.h"
#include "UIPdaFunctionsWnd.h"
#include "UIXmlInit.h"
#include "UIFrameWindow.h"
#include "UIFrameLineWnd.h"
#include "UIAnimatedStatic.h"

constexpr auto PDA_FUNCTIONS_XML = "pda_functions.xml";

void CUIPdaFunctionsWnd::Init()
{
    CUIXml uiXml;
    bool xml_result = uiXml.Init(CONFIG_PATH, UI_PATH, PDA_FUNCTIONS_XML);
    R_ASSERT3(xml_result, "xml file not found: pda_functions.xml", PDA_FUNCTIONS_XML);

    CUIXmlInit xml_init;

    UIMainFrame = xr_new<CUIFrameWindow>();
    UIMainFrame->SetAutoDelete(true);
    xml_init.InitFrameWindow(uiXml, "main_frame", 0, UIMainFrame);
    AttachChild(UIMainFrame);

    UIMainFrameHeader = xr_new<CUIFrameLineWnd>();
    UIMainFrameHeader->SetAutoDelete(true);
    xml_init.InitFrameLine(uiXml, "main_frame_header", 0, UIMainFrameHeader);
    UIMainFrame->AttachChild(UIMainFrameHeader);

    UIAnimatedIcon = xr_new<CUIAnimatedStatic>();
    UIAnimatedIcon->SetAutoDelete(true);
    xml_init.InitAnimatedStatic(uiXml, "a_static", 0, UIAnimatedIcon);
    UIMainFrameHeader->AttachChild(UIAnimatedIcon);

    // Элементы автоматического добавления
    xml_init.InitAutoStatic(uiXml, "auto_static", UIMainFrame);
}