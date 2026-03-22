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
 * wind.h
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "component.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

/**
	 * @class Wind
	 * @brief A component that simulates wind effects in the scene, affecting particle systems, 
	 * vegetation, and other dynamic elements. 
	 * It allows for configuring wind direction, speed, and gusts to create realistic environmental interactions.
	 */
	class Wind : public Component
	{
	public:
		~Wind() = default;

		void Init() override;
		void Start() override;
		void Tick() override;
		void Stop() override;
		void Remove() override;

		static const xMath::Vec3& GetWind();
		static void SetWind(const xMath::Vec3& wind);

	private:
		float direction; // Wind direction in degrees (0-360)
		float speed;     // Wind speed in knots
		float gust;      // Gust speed in knots
	};

}

// -------------------------------------------------------
