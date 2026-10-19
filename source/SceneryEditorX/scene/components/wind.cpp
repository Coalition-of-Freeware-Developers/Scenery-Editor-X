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
 * wind.cpp
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#include "wind.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	static Vec3 s_Wind = {0.0f,0.0f,0.0f};

	void Wind::Init()
	{
		float rotation_y      = 120.0f * xMath::DEG_TO_RAD;
		const float intensity = 3.0f;
		s_Wind = Vec3(sin(rotation_y), 0.0f, cos(rotation_y)) * intensity;
	}
	
	void Wind::Start()
	{
		Component::Start();
	}
	
	void Wind::Tick()
	{
		Component::Tick();
	}
	
	void Wind::Stop()
	{
		Component::Stop();
	}
	
	void Wind::Remove()
	{
		Component::Remove();
	}

	const xMath::Vec3 &Wind::GetWind()
	{
		return s_Wind;
	}

	void Wind::SetWind(const xMath::Vec3 &wind)
	{
		s_Wind = wind;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
