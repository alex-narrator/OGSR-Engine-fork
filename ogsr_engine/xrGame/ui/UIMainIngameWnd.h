// UIMainIngameWnd.h:  окошки-информация в игре
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UIProgressBar.h"
#include "UIGameLog.h"

#include "../alife_space.h"

#include "UICarPanel.h"
#include "../hudsound.h"
#include "../script_export_space.h"
#include "../inventory.h"

struct GAME_NEWS_DATA;

class CUIPdaMsgListItem;
class CLAItem;
class CUIZoneMap;
class CActor;

class CUIXml;
class CUIStatic;

class CUIMainIngameWnd : public CUIWindow
{
public:
    CUIMainIngameWnd();
    virtual ~CUIMainIngameWnd();

    virtual void Init();
    virtual void Draw();
    virtual void Update();

    bool OnKeyboardPress(int dik);
    bool OnKeyboardHold(int cmd);

protected:
    CUIZoneMap* UIZoneMap;

public:
    CUIZoneMap* GetUIZoneMap() { return UIZoneMap; }
    bool m_bShowZoneMap{};

    void ReceiveNews(GAME_NEWS_DATA* news);

protected:
    // для текущего активного актера и оружия
    CActor* m_pActor;

public:
    void OnConnected();
    void reset_ui();

    DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CUIMainIngameWnd)
#undef script_type_list
#define script_type_list save_type_list(CUIMainIngameWnd)