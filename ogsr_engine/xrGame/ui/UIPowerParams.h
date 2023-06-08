#pragma once
#include "UIWindow.h"
#include "UIStatic.h"
#include "UIProgressBar.h"

class CInventoryItem;

class CUIPowerParams : public CUIWindow
{
public:
    CUIPowerParams();
    virtual ~CUIPowerParams(){};

    void Init();
    void SetInfo(CInventoryItem* obj);
    bool Check(CInventoryItem* obj);

protected:
    CUIStatic m_textPower;
    CUIStatic m_textWorkTime;
    CUIProgressBar m_progressPower;
};