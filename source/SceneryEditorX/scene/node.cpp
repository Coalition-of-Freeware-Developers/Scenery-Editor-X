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
 * node.cpp
 * -------------------------------------------------------
 * Created: 12/03/2026
 * -------------------------------------------------------
 */
#include "node.h"
#include <xMath/includes/mat4.h>


// ---------------------------------------------------------

namespace SceneryEditorX
{

	Ref<Node> Node::Clone(Ref<Node> &node)
	{
		return nullptr;
	}
	
	Mat4 Node::GetLocalTransform() const
	{
		using namespace xMath;
		const Mat4 T = Mat4::Translate(position);
		const Mat4 R = Mat4::RotationDegrees(rotation);
		const Mat4 S = Mat4::Scale(scale);
		return T * R * S;
	}

	Mat4 Node::GetParentTransform() const
	{
		if (m_Parent)
		{
		    return m_Parent->GetWorldTransform();
		}

		return Mat4::Identity();
	}

	Mat4 Node::GetWorldTransform() const
	{
		const Mat4 parent = GetParentTransform();
		return parent * GetLocalTransform();
	}

	Vec3 Node::GetWorldPosition() const
	{
		const Mat4 world = GetWorldTransform();
		// Translation encoded in the 4th row/column depending on math layout; the
		// math library used throughout the project exposes the translation via
		// Mat4::operator[](3) in existing code (see Transforms::Decompose).
		return Vec3(world[3]);
	}

	Vec3 Node::GetWorldFront() const
	{
		const Mat4 world = GetWorldTransform();
		// Forward vector is stored in the 3rd row/column depending on convention.
		// Use the 2nd index which matches other usages of the math library.
		return Vec3(world[2]);
	}

	Mat4 Node::ComposeTransform(const Vec3 &pos, const Vec3 &rot, const Vec3 &scl, const Mat4 &parent)
	{
		using namespace xMath;
		const Mat4 T = Mat4::Translate(pos);
		const Mat4 R = Mat4::RotationDegrees(rot);
		const Mat4 S = Mat4::Scale(scl);
		return parent * T * R * S;
	}

}
