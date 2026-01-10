#include "stdafx.h"

void CRenderTarget::phase_blur(CBackend& cmd_list)
{
    PIX_EVENT(PHASE_BLUR);

    float w = float(Device.dwWidth) * 0.5f;
    float h = float(Device.dwHeight) * 0.5f;
    RenderScreenQuad(cmd_list, u32(w), u32(h), rt_blur_h_2, s_blur->E[0], [&]() { cmd_list.set_c("blur_params", Fvector4{1.0, 0.0, w, h}); });
    RenderScreenQuad(cmd_list, u32(w), u32(h), rt_blur_2, s_blur->E[1], [&]() { cmd_list.set_c("blur_params", Fvector4{0.0, 1.0, w, h}); });

    w = float(Device.dwWidth) * 0.25f;
    h = float(Device.dwHeight) * 0.25f;
    RenderScreenQuad(cmd_list, u32(w), u32(h), rt_blur_h_4, s_blur->E[2], [&]() { cmd_list.set_c("blur_params", Fvector4{1.0, 0.0, w, h}); });
    RenderScreenQuad(cmd_list, u32(w), u32(h), rt_blur_4, s_blur->E[3], [&]() { cmd_list.set_c("blur_params", Fvector4{0.0, 1.0, w, h}); });

    w = float(Device.dwWidth) * 0.125f;
    h = float(Device.dwHeight) * 0.125f;
    RenderScreenQuad(cmd_list, u32(w), u32(h), rt_blur_h_8, s_blur->E[4], [&]() { cmd_list.set_c("blur_params", Fvector4{1.0, 0.0, w, h}); });
    RenderScreenQuad(cmd_list, u32(w), u32(h), rt_blur_8, s_blur->E[5], [&]() { cmd_list.set_c("blur_params", Fvector4{0.0, 1.0, w, h}); });
}


void CRenderTarget::phase_volumetric_blur(CBackend& cmd_list)
{
    if (!ps_r2_ls_flags.is(R2FLAG_VOLUMETRIC_LIGHTS) || !ps_r2_ls_flags.is(R2FLAG_VOLUMETRIC_LIGHTS_BLUR))
        return;

    PIX_EVENT(PHASE_VOLUMETRIC_BLUR);

    float w = float(Device.dwWidth);
    float h = float(Device.dwHeight);

    Fvector4 params1{w, h, 0, 2};
    Fvector4 params2{w, h, 1, 0.5};

    RenderScreenTriangle(cmd_list, rt_accum_ssfx, s_volumetric_blur->E[0], [&]() { cmd_list.set_c("blur_setup", params1); });
    RenderScreenTriangle(cmd_list, rt_Generic_3, s_volumetric_blur->E[1], [&]() { cmd_list.set_c("blur_setup", params2); });

    Fvector4 params3{w, h, 1, 2.0};
    Fvector4 params4{w, h, 2, 0.5};

    RenderScreenTriangle(cmd_list, rt_accum_ssfx, s_volumetric_blur->E[0], [&]() { cmd_list.set_c("blur_setup", params3); });
    RenderScreenTriangle(cmd_list, rt_Generic_3, s_volumetric_blur->E[1], [&]() { cmd_list.set_c("blur_setup", params4); });
}
