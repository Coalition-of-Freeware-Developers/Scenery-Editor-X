/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewport.h
* -------------------------------------------------------
* Created: 5/9/2025
* -------------------------------------------------------
*/
#pragma once
#include "ui_panel.h"

/// -------------------------------------------------------

class Viewport : public UI_Panel
{
public:
    Viewport(Editor *editor);

    void OnTickVisible() override;

private:
    //Editor *m_Editor = nullptr;
    Vec2 m_offset = Vec2(0.0f, 0.0f);
    float m_padding = 4.0f;
};

/// -------------------------------------------------------
