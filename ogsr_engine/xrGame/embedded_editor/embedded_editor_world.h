#pragma once
#include "embedded_editor_window.h"

class CImGuiWorldEditorWnd : public CImGuiEditorWnd
{
public:
    CImGuiWorldEditorWnd() : CImGuiEditorWnd("World Editor") {}
    CImGuiWorldEditorWnd(LPSTR name) : CImGuiEditorWnd(name) {}
    void Render() override;
};