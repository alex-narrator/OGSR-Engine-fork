#include "common.h"

uniform float4x4 m_texgen;

v2p_volume main(float4 P : POSITION)
{
    v2p_volume O;
    O.hpos = mul(m_WVP, P);
    O.tc = mul(m_texgen, P);

	// Dance Maniac: Объём не использует проверку глубину и его надо держать в пределах диапазона отсечения камеры, иначе будут чёрные квадраты при низком far_plane
    O.hpos.z = O.hpos.w * 0.5f;
    return O;
}
