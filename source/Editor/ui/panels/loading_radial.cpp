/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * loading_radial.cpp
 * -------------------------------------------------------
 * Created: 29/3/2025
 * -------------------------------------------------------
 */

/*
#include <cmath>
#include <imgui/imgui.h>
#include <imgui/imgui_widgets.cpp>

using namespace ImGui;

namespace SceneryEditorX::UI  
{  
   void LoadingIndicatorCircle(const char *label,
       const float indicator_radius,
       const ImVec4 &main_color,
       const ImVec4 &backdrop_color,
       const int circle_count,
       const float speed) {

       ImGuiWindow *window = GetCurrentWindow();
       if (window->SkipItems) {
           return;
       }

       ImGuiContext &g = *GImGui;
       const ImGuiID id = window->GetID(label);

       const ImVec2 pos = window->DC.CursorPos;
       const float circle_radius = indicator_radius / 15.0f;
       const float updated_indicator_radius = indicator_radius - 4.0f * circle_radius;
       const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
       ItemSize(bb);
       if (!ItemAdd(bb, id)) {
           return;
       }
       const float t = g.Time;
       const auto degree_offset = 2.0f * IM_PI / circle_count;
       for (int i = 0; i < circle_count; ++i) {
           const auto x = updated_indicator_radius * std::sin(degree_offset * i);
           const auto y = updated_indicator_radius * std::cos(degree_offset * i);
           const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));
           ImVec4 color;
           color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
           color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
           color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
           color.w = 1.0f;
           window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x, pos.y + indicator_radius - y),
                                             circle_radius + growth * circle_radius,
                                             GetColorU32(color));
       }
   }
}
*/
