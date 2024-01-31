#pragma once

#include "uiframewindow.h"
#include "uilistbox.h"

#include "../script_export_space.h"
#include "script_game_object.h"

class CUIPropertiesBox : public CUIFrameWindow
{
private:
    typedef CUIFrameWindow inherited;

public:
    CUIPropertiesBox();
    virtual ~CUIPropertiesBox();

    virtual void Init(float x, float y, float width, float height);

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
    virtual bool OnMouse(float x, float y, EUIMessages mouse_action);
    virtual bool OnKeyboard(int dik, EUIMessages keyboard_action);

    bool AddItem(const char* str, void* pData = NULL, u32 tag_value = 0);
    bool AddItem_script(const char* str) { return AddItem(str); };
    u32 GetItemsCount() { return m_UIListWnd.GetSize(); };
    void RemoveItemByTAG(u32 tag_value);
    void RemoveAll();

    virtual void Show(const Frect& parent_rect, const Fvector2& point);
    virtual void Hide();

    virtual void Update();
    virtual void Draw();

    CUIListBoxItem* GetClickedItem();

    void AutoUpdateSize();
    
    string_unordered_map<shared_str, xr_vector<shared_str>> m_custom_actions_item;
    string_unordered_map<shared_str, xr_vector<shared_str>> m_custom_actions_map_spot;
    void CheckCustomActionsItem(CScriptGameObject* obj);
    void ProcessCustomActionsItem(CScriptGameObject* obj);
    void CheckCustomActionsMapSpot(u16 id, LPCSTR spot_type, LPCSTR location_name, Fvector pos);
    void ProcessCustomActionsMapSpot(u16 id, LPCSTR spot_type, LPCSTR location_name, Fvector pos);

protected:
    CUIListBox m_UIListWnd;

    DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CUIPropertiesBox)
#undef script_type_list
#define script_type_list save_type_list(CUIPropertiesBox)
