#pragma once
#include "UIWindow.h"

class CUIFrameWindow;
class CUIFrameLineWnd;
class CUIAnimatedStatic;

class CUIPdaFunctionsWnd : public CUIWindow
{
public:
    CUIPdaFunctionsWnd(){};
    virtual ~CUIPdaFunctionsWnd(){};
    void Init();

protected:
    CUIFrameWindow* UIMainFrame{};
    CUIFrameLineWnd* UIMainFrameHeader{};
    CUIAnimatedStatic* UIAnimatedIcon{};
};