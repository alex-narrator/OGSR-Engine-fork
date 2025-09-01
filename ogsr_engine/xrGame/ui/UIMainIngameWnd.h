// UIMainIngameWnd.h:  окошки-информация в игре
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UIGameLog.h"

#include "../alife_space.h"

#include "../script_export_space.h"

struct GAME_NEWS_DATA;

class CUIZoneMap;
class CActor;

class CUIMainIngameWnd : public CUIWindow
{
public:
    CUIMainIngameWnd();
    virtual ~CUIMainIngameWnd();

    virtual void Init();
    virtual void Draw();
    virtual void Update();

    bool OnKeyboardPress(int dik);

    CUIZoneMap* GetUIZoneMap() { return UIZoneMap; }
    bool m_bShowZoneMap{};

    void ReceiveNews(GAME_NEWS_DATA* news);

    void OnConnected();
    void reset_ui();

protected:
    CUIZoneMap* UIZoneMap{};
    CActor* m_pActor{};

    DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CUIMainIngameWnd)
#undef script_type_list
#define script_type_list save_type_list(CUIMainIngameWnd)
