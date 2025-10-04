#pragma once
#include "CustomDevice.h"
class CFlashlight : public CCustomDevice
{
    typedef CCustomDevice inherited;

protected:
    float fBrightness{1.f};
    CLAItem* lanim{};

    float m_delta_h;
    Fvector2 m_prev_hp;
    bool m_switched_on{};
    ref_light light_render;
    ref_light light_omni;
    Fvector m_focus;
    Fcolor m_color;

    float m_fRange{};

    virtual void UpdateWork();

private:
    bool useVolumetric{}, useVolumetricForActor{};
    Fvector light_direction{};

public:
    CFlashlight();
    virtual ~CFlashlight();

    virtual void Load(LPCSTR section);

    void LoadLightDefinitions(shared_str light_sect);

    virtual void Switch(bool);

    // alpet: управление светом фонаря
    IRender_Light* GetLight(int target = 0) const;

    void SetAnimation(LPCSTR name);
    void SetBrightness(float brightness);
    void SetColor(const Fcolor& color, int target = 0);
    void SetRGB(float r, float g, float b, int target = 0);
    void SetAngle(float angle, int target = 0);
    void SetRange(float range, int target = 0);
    void SetTexture(LPCSTR texture, int target = 0);
    void SetVirtualSize(float size, int target = 0);

    DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CFlashlight)
#undef script_type_list
#define script_type_list save_type_list(CFlashlight)