#pragma once
#include "UIWindow.h"
#include "UIStatic.h"

class CInventoryItem;

class CUIEquipParams : public CUIWindow
{
public:
    CUIEquipParams(){};
    virtual ~CUIEquipParams(){};

    void Init();
    void SetInfo(CInventoryItem* obj);
    bool Check(CInventoryItem* obj);

protected:
    CUIStatic m_CapInfo; // метеринський статик листа інформації
    CUIStatic* SetStaticParams(CUIXml&, const char*, float);
};