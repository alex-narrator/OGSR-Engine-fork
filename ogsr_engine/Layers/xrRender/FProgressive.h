// FProgressive.h: interface for the FProgressive class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "FVisual.h"
struct FSlideWindowItem;

class FProgressive : public Fvisual
{
protected:
    FSlideWindowItem nSWI;
    FSlideWindowItem* xSWI;

public:
    FProgressive();
    virtual ~FProgressive();

    void Render(CBackend& cmd_list, float lod, bool use_fast_geo) override; // LOD - Level Of Detail  [0.0f - min, 1.0f - max], -1 = Ignored

    virtual void Load(const char* N, IReader* data, u32 dwFlags);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();

private:
    FProgressive(const FProgressive& other);
    void operator=(const FProgressive& other);
};
