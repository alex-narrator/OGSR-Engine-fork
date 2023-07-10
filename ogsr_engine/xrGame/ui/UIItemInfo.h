#pragma once
#include "uiwindow.h"

class CInventoryItem;
class CUIStatic;
class CUIScrollView;
class CUIProgressBar;
class CUIWpnParams;
class CUIArtefactParams;
class CUIEatableParams;
class CUIArmorParams;
class CUIFrameWindow;

// extern const char * const 		fieldsCaptionColor;

class CUIItemInfo : public CUIWindow
{
private:
    typedef CUIWindow inherited;
    struct _desc_info
    {
        CGameFont* pDescFont;
        u32 uDescClr;
        bool bShowDescrText;
    } m_desc_info{};
    CInventoryItem* m_pInvItem{};

public:
    CUIItemInfo();
    virtual ~CUIItemInfo();

    void Init(LPCSTR xml_name);
    void InitItem(CInventoryItem* pInvItem);

    void TryAddWpnInfo(CInventoryItem* obj);
    void TryAddArtefactInfo(CInventoryItem* obj);
    void TryAddEatableInfo(CInventoryItem* obj);
    void TryAddArmorInfo(CInventoryItem* obj);
    void TryAddCustomInfo(CInventoryItem* obj);

    virtual void Draw();
    bool m_b_force_drawing{};
    CUIFrameWindow* UIBackground{}; 
    CUIStatic* UIName{};
    CUIStatic* UIWeight{};
    CUIStatic* UICost{};
    CUIStatic* UICondition{};
    CUIScrollView* UIDesc{};
    CUIProgressBar* UICondProgresBar{};
    CUIWpnParams* UIWpnParams{};
    CUIArtefactParams* UIArtefactParams{};
    CUIEatableParams* UIEatableParams{};
    CUIArmorParams* UIArmorParams{};

    Fvector2 UIItemImageSize{};
    CUIStatic* UIItemImage;

    u32 show_delay{};
    Fvector2 info_offset{};
    float min_height{};
};