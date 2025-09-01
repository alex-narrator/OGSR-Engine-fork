#pragma once

#include "UIWindow.h"
#include "UIOptionsItem.h"

class CUI3tButton;
class CUIFrameLineWnd;
class CUITrackButton;

class CUITrackBar : public CUIWindow, public CUIOptionsItem
{
    friend class CUITrackButton;

public:
    CUITrackBar();
    // CUIOptionsItem
    virtual void SetCurrentValue();
    virtual void SaveValue();
    virtual bool IsChanged();
    virtual void SeveBackUpValue();
    virtual void Undo();
    virtual void Draw();
    virtual bool OnMouse(float x, float y, EUIMessages mouse_action);
    virtual void OnMessage(const char* message);
    // CUIWindow
    virtual void Init(float x, float y, float width, float height);
    virtual void Enable(bool status);
    void SetInvert(bool v) { m_b_invert = v; }
    bool GetInvert() const { return m_b_invert; };
    void SetStep(float step);
    void SetType(bool b_float) { m_b_is_float = b_float; };
    bool GetCheck();
    void SetCheck(bool b);
    float GetTrackValue();
    void SetTrackValue(float val);

    void SetMin(float v) { m_f_min_xml = v; }
    void SetMax(float v) { m_f_max_xml = v; }

    void SetOptionsItem(bool val) { is_options_item = val; };
    bool IsOptionsItem() { return is_options_item; };

    void SetShowValOnSlider(bool val) { show_val_on_slider = val; };

protected:
    void UpdatePos();
    void UpdatePosRelativeToMouse();

    CUI3tButton* m_pSlider;
    CUIFrameLineWnd* m_pFrameLine;
    CUIFrameLineWnd* m_pFrameLine_d;
    bool m_b_invert{};
    bool m_b_is_float{true};

    float m_f_max_xml{};
    float m_f_min_xml{};

    bool is_options_item{};
    bool show_val_on_slider{};

    float m_f_val{};
    float m_f_max{1.f};
    float m_f_min{};
    float m_f_step{0.1f};
    float m_f_back_up{};

    int m_i_val{};
    int m_i_max{1};
    int m_i_min{};
    int m_i_step{1};
    int m_i_back_up{};
};