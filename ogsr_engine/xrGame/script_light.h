#pragma once
#include "script_object.h"
#include "script_export_space.h"

class CLAItem;

class CScriptLight
{
protected:
    ref_light light_render{};
    CLAItem* lanim{};

    float fBrightness{};
    Fcolor m_color{};

public:
    CScriptLight();
    virtual ~CScriptLight();

    void SetAnimation(LPCSTR name);

    void SetPosition(Fvector pos);
    void SetPosition(float x, float y, float z);
    void SetDirection(Fvector dir);

    void SetDirection(float x, float y, float z);
    void SetDirection(Fvector dir, Fvector right);

    void SetAngle(float angle);

    void Enable(bool state);
    bool IsEnabled() const;

    void SetHudMode(bool b);

    void SetBrightness(float br);

    void SetColor(Fcolor color);
    void SetRGB(float r, float g, float b);

    void SetShadow(bool state);

    void SetRange(float range);

    void SetType(int type);

    void SetTexture(LPCSTR texture);

    void SetVirtualSize(float size);

    void SetVolumetric(bool state);

    void SetVolumetricDistance(float val);

    void SetVolumetricIntensity(float val);

    void SetVolumetricQuality(float val);

    void Update();

    DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CScriptLight)
#undef script_type_list
#define script_type_list save_type_list(CScriptLight)