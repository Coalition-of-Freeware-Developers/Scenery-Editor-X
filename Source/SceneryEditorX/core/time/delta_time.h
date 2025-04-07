/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* delta_time.h
* -------------------------------------------------------
* Created: 5/4/2025
* -------------------------------------------------------
*/

#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Timestep
	{
	public:
		Timestep() {}
		Timestep(float time);

		inline float GetSeconds() const { return m_Time; }
		inline float GetMilliseconds() const { return m_Time * 1000.0f; }

		operator float() { return m_Time; }
	private:
		float m_Time = 0.0f;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------
