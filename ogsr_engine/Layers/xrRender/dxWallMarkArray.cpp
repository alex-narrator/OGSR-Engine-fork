#include "stdafx.h"
#include "dxWallMarkArray.h"

#include "dxUIShader.h"

void dxWallMarkArray::Copy(IWallMarkArray& _in) { *this = *(dxWallMarkArray*)&_in; }

dxWallMarkArray::~dxWallMarkArray()
{
    for (auto& m_CollideMark : m_CollideMarks)
        m_CollideMark.destroy();
}

void dxWallMarkArray::AppendMark(LPCSTR s_textures)
{
    ref_shader s;
    s.create(strstr(s_textures, "wm_blood_") ? "effects\\wallmark_blood" : "effects\\wallmark", s_textures);
    m_CollideMarks.push_back(s);
}

void dxWallMarkArray::clear() { return m_CollideMarks.clear(); }

bool dxWallMarkArray::empty() { return m_CollideMarks.empty(); }

wm_shader dxWallMarkArray::GenerateWallmark()
{
    wm_shader res;
    if (!m_CollideMarks.empty())
        ((dxUIShader*)&*res)->hShader = m_CollideMarks[::Random.randI(0, m_CollideMarks.size())];
    return res;
}

ref_shader* dxWallMarkArray::dxGenerateWallmark() { return m_CollideMarks.empty() ? nullptr : &m_CollideMarks[::Random.randI(0, m_CollideMarks.size())]; }