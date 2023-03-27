#pragma once
#include "UIWindow.h"

#include "UIProgressBar.h"

class CUIXml;

class CInventoryItem;

class CUIWpnParams : public CUIWindow
{
public:
    CUIWpnParams();
    virtual ~CUIWpnParams();

    void Init();
    void Reinit();
    void SetInfo(CInventoryItem* obj);
    bool Check(CInventoryItem* obj);

protected:
    //
    CUIStatic m_CapInfo; // метеринський статик листа інформації про зброю
    CUIStatic* SetStaticParams(CUIXml&, const char*, float);
};