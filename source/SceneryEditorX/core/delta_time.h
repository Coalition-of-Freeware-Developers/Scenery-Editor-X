/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* delta_time.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{

	class DeltaTime
	{
	public:
        DeltaTime() {}
        DeltaTime(float time);

		inline float GetSeconds() const { return time_; }
		inline float GetMilliseconds() const { return time_ * 1000.0f; }

		operator float() { return time_; }
	private:
        float time_ = 0.0f;
	    float deltaTime;
	};

} // namespace SceneryEditorX
