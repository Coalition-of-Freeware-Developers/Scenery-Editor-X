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
 * gizmos.h
 * -------------------------------------------------------
 * Created: 15/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <ImGuizmo.h>
#include <xMath/includes/vector.h>

// -------------------------------------------------------

/**
 * @brief 
 */
enum class ManipulatorType : uint32_t
{
	Translate = ImGuizmo::OPERATION::TRANSLATE,
	Rotate    = ImGuizmo::OPERATION::ROTATE,
	Scale     = ImGuizmo::OPERATION::SCALE,
	MaxEnum   = 0xFFFFFFFF
};

/**
 * @class Gizmo
 * @brief Represents a gizmo for manipulating objects in the scene.
 */
class Gizmo
{
public:
	Gizmo() = default;
	Gizmo(ManipulatorType type);

	static void Tick();
	static void DrawGizmo(const xMath::Vec3 &position, const xMath::Vec3 &rotation, const xMath::Vec3 &scale);
	static bool AllowObjectSelection();
	static bool EnableSnapping();

private:
	ManipulatorType m_Type = ManipulatorType::Translate;
};


// -------------------------------------------------------
