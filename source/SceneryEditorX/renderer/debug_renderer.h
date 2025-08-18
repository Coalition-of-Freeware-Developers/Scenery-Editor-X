/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_debug.h
* -------------------------------------------------------
* Created: 21/6/2025
* -------------------------------------------------------
*/
#pragma once
#include "2d_renderer.h"

/// ---------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * Utility class which queues rendering work for once-per-frame
	 * flushing by Scene (intended to be used for debug graphics)
	 */
	class DebugRenderer : public RefCounted
	{
	public:
		using RenderQueue = std::vector<std::function<void(Ref<Renderer2D>)>>;

		DebugRenderer() = default;
		~DebugRenderer() = default;

		void DrawLine(const Vec3& p0, const Vec3& p1, const Vec4& color = Vec4(1.0f), bool onTop = false);
		void DrawCircle(const Vec3& centre, const Vec3& rotation, float radius, const Vec4& color = Vec4(1.0f), bool onTop = false);
		void DrawTransform(const Mat4& transform, float scale = 1.0f, bool onTop = false);
		void DrawQuadBillboard(const Vec3& translation, const Vec2& size, const Vec4& color = Vec4(1.0f));
		void SetLineWidth(float thickness);

        /// -------------------------------------------------------

		RenderQueue& GetRenderQueue() { return m_RenderQueue; }
		void ClearRenderQueue() { m_RenderQueue.clear(); }

	    /// -------------------------------------------------------

		GLOBAL bool HasValidationLayers() { return m_validation_layer_enabled; }
		GLOBAL bool IsRenderDocEnabled() { return m_renderdoc_enabled; }

	private:
		RenderQueue m_RenderQueue;

        inline GLOBAL bool m_validation_layer_enabled = false;
        inline GLOBAL bool m_renderdoc_enabled = false;
	};

}

/// ---------------------------------------------------------
