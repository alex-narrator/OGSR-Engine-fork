#pragma once
#include "UIWindow.h"

#include "UIProgressBar.h"

class CUIXml;

struct SLuaWpnParams;
class CInventoryItem;

class CUIWpnParams : public CUIWindow
{
public:
    CUIWpnParams();
    virtual ~CUIWpnParams();

    void Init();
    void SetInfo(CInventoryItem* obj);
    bool Check(CInventoryItem* obj);

protected:
    CUIProgressBar m_progressAccuracy;
    CUIProgressBar m_progressHandling;
    CUIProgressBar m_progressDamage;
    CUIProgressBar m_progressRPM;
    CUIProgressBar m_progressRange;
    CUIProgressBar m_progressReliability;

    CUIStatic m_textAccuracy;
    CUIStatic m_textHandling;
    CUIStatic m_textDamage;
    CUIStatic m_textRPM;
    CUIStatic m_textRange;
    CUIStatic m_textReliability;
};