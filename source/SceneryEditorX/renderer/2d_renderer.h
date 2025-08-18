/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* 2d_renderer.h
* -------------------------------------------------------
* Created: 24/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <Math/includes/aabb.h>

#include "texture.h"
#include "buffers/index_buffer.h"
#include "buffers/uniform_buffer.h"
#include "fonts/font.h"
#include "vulkan/vk_cmd_buffers.h"
#include "vulkan/vk_render_pass.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// Forward declarations to avoid circular dependencies
    class CommandBuffer;
    class Material;
    class Pipeline;

    /// -------------------------------------------------------

	struct Renderer2DSpecification
	{
		bool SwapChainTarget = false;
		uint32_t MaxQuads = 5000;
		uint32_t MaxLines = 1000;
	};

    /// -------------------------------------------------------

	class Renderer2D : public RefCounted
	{
	public:
		Renderer2D(const Renderer2DSpecification& specification = Renderer2DSpecification());
		virtual ~Renderer2D();

		void Init();
		void Shutdown() const;

		void BeginScene(const Mat4& viewProj, const Mat4& view, bool depthTest = true);
		void EndScene();

		Ref<RenderPass> GetTargetRenderPass();
		void SetTargetFramebuffer(Ref<Framebuffer> &framebuffer);
		void OnRecreateSwapchain();

        /// -------------------------------------------------------

		///< Primitives
		void DrawQuad(const Mat4 &transform, const Vec4 &color);
		void DrawQuad(const Mat4 &transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const Vec4& tintColor = Vec4(1.0f), Vec2 uv0 = Vec2(0.0f), Vec2 uv1 = Vec2(1.0f));

        /// -------------------------------------------------------

		void DrawQuad(const Vec2 &position, const Vec2& size, const Vec4& color);
		void DrawQuad(const Vec3 &position, const Vec2& size, const Vec4& color);
		void DrawQuad(const Vec2 &position, const Vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const Vec4& tintColor = Vec4(1.0f), Vec2 uv0 = Vec2(0.0f), Vec2 uv1 = Vec2(1.0f));
		void DrawQuad(const Vec3 &position, const Vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const Vec4& tintColor = Vec4(1.0f), Vec2 uv0 = Vec2(0.0f), Vec2 uv1 = Vec2(1.0f));

        /// -------------------------------------------------------

		void DrawQuadBillboard(const Vec3& position, const Vec2& size, const Vec4& color);
		void DrawQuadBillboard(const Vec3& position, const Vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const Vec4& tintColor = Vec4(1.0f));

        /// -------------------------------------------------------

		void DrawRotatedQuad(const Vec2& position, const Vec2& size, float rotation, const Vec4& color);
		void DrawRotatedQuad(const Vec3& position, const Vec2& size, float rotation, const Vec4& color);
		void DrawRotatedQuad(const Vec2& position, const Vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const Vec4& tintColor = Vec4(1.0f));
		void DrawRotatedQuad(const Vec3& position, const Vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const Vec4& tintColor = Vec4(1.0f));

        /// -------------------------------------------------------

		void DrawRotatedRect(const Vec2& position, const Vec2& size, float rotation, const Vec4& color, bool onTop = false);
		void DrawRotatedRect(const Vec3& position, const Vec2& size, float rotation, const Vec4& color, bool onTop = false);

        /// -------------------------------------------------------

		///< Thickness is between 0 and 1
		void DrawCircle(const Vec3& position, const Vec3& rotation, float radius, const Vec4& color, bool onTop = false);
		void DrawCircle(const Mat4& transform, const Vec4& color, bool onTop = false);
		void FillCircle(const Vec2& position, float radius, const Vec4& color, float thickness = 0.05f);
		void FillCircle(const Vec3& position, float radius, const Vec4& color, float thickness = 0.05f);

        /// -------------------------------------------------------

		void DrawLine(const Vec3& p0, const Vec3& p1, const Vec4& color = Vec4(1.0f), bool onTop = false);
		void DrawTransform(const Mat4& transform, float scale = 1.0f, bool onTop = true);
		void DrawAABB(const AABB& aabb, const Mat4& transform, const Vec4& color = Vec4(1.0f), bool onTop = false);
		void DrawString(const std::string& string, const Vec3& position, float maxWidth, const Vec4& color = Vec4(1.0f));
		void DrawString(const std::string& string, const Ref<Font>& font, const Vec3& position, float maxWidth, const Vec4& color = Vec4(1.0f));
		void DrawString(const std::string& string, const Ref<Font>& font, const Mat4& transform, float maxWidth, const Vec4& color = Vec4(1.0f), float lineHeightOffset = 0.0f, float kerningOffset = 0.0f);

        /// -------------------------------------------------------

		float GetLineWidth() const;
		void SetLineWidth(float lineWidth);

        /// -------------------------------------------------------

		///< Stats
		struct DrawStatistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t LineCount = 0;
            [[nodiscard]] uint32_t GetTotalVertexCount() const { return QuadCount * 4 + LineCount * 2; }
            [[nodiscard]] uint32_t GetTotalIndexCount() const { return QuadCount * 6 + LineCount * 2; }
		};

        /// -------------------------------------------------------

		struct MemoryStatistics
		{
			uint64_t Used = 0;
			uint64_t TotalAllocated = 0;
            [[nodiscard]] uint64_t GetAllocatedPerFrame() const;
		};

        /// -------------------------------------------------------

		void ResetStats();
		DrawStatistics GetDrawStats() const;
        MemoryStatistics GetMemoryStats() const;
		const Renderer2DSpecification& GetSpecification() const { return m_Specification; }

	private:
		void Flush();
		void AddQuadBuffer();
		void AddLineBuffer(bool onTop);
		void AddTextBuffer();
		void AddCircleBuffer();

        /// -------------------------------------------------------

		struct QuadVertex
		{
			Vec3 Position;
			Vec4 Color;
			Vec2 TexCoord;
			float TexIndex;
			float TilingFactor;
		};

        /// -------------------------------------------------------

		struct TextVertex
		{
			Vec3 Position;
			Vec4 Color;
			Vec2 TexCoord;
			float TexIndex;
		};

        /// -------------------------------------------------------

		struct LineVertex
		{
			Vec3 Position;
			Vec4 Color;
		};

        /// -------------------------------------------------------

		struct CircleVertex
		{
			Vec3 WorldPosition;
			float Thickness;
			Vec2 LocalPosition;
			Vec4 Color;
		};

        /// -------------------------------------------------------

		QuadVertex* &GetWriteableQuadBuffer();
		LineVertex* &GetWriteableLineBuffer(bool onTop);
		TextVertex* &GetWriteableTextBuffer();
		CircleVertex* &GetWriteableCircleBuffer();

        /// -------------------------------------------------------

		static constexpr uint32_t MaxTextureSlots = 32; ///< TODO: RenderCaps

		const uint32_t c_MaxVertices;
		const uint32_t c_MaxIndices;
		const uint32_t c_MaxLineVertices;
		const uint32_t c_MaxLineIndices;

		Renderer2DSpecification m_Specification;
		Ref<CommandBuffer> m_RenderCommandBuffer;
		Ref<Texture2D> m_WhiteTexture;

		using VertexBufferPerFrame = std::vector<Ref<VertexBuffer>>;

        /// -------------------------------------------------------

		///< Quads
		Ref<RenderPass> m_QuadPass;
		std::vector<VertexBufferPerFrame> m_QuadVertexBuffers;
		Ref<IndexBuffer> m_QuadIndexBuffer;
		Ref<Material> m_QuadMaterial;

		uint32_t m_QuadIndexCount = 0;
		using QuadVertexBasePerFrame = std::vector<QuadVertex*>;
		std::vector<QuadVertexBasePerFrame> m_QuadVertexBufferBases;
		std::vector<QuadVertex*> m_QuadVertexBufferPtr;
		uint32_t m_QuadBufferWriteIndex = 0;

		Ref<Pipeline> m_CirclePipeline;
		Ref<Material> m_CircleMaterial;
		std::vector<VertexBufferPerFrame> m_CircleVertexBuffers;
		uint32_t m_CircleIndexCount = 0;
		using CircleVertexBasePerFrame = std::vector<CircleVertex*>;
		std::vector<CircleVertexBasePerFrame> m_CircleVertexBufferBases;
		std::vector<CircleVertex*> m_CircleVertexBufferPtr;
		uint32_t m_CircleBufferWriteIndex = 0;

		std::array<Ref<Texture2D>, MaxTextureSlots> m_TextureSlots;
		uint32_t m_TextureSlotIndex = 1; ///< 0 = white texture

		Vec4 m_QuadVertexPositions[4];

        /// -------------------------------------------------------

		///< Lines
		Ref<RenderPass> m_LinePass;
		std::vector<VertexBufferPerFrame> m_LineVertexBuffers;
		std::vector<VertexBufferPerFrame> m_LineOnTopVertexBuffers;
		Ref<IndexBuffer> m_LineIndexBuffer;
		Ref<IndexBuffer> m_LineOnTopIndexBuffer;
		Ref<Material> m_LineMaterial;

		uint32_t m_LineIndexCount = 0;
		uint32_t m_LineOnTopIndexCount = 0;
		using LineVertexBasePerFrame = std::vector<LineVertex*>;
		std::vector<LineVertexBasePerFrame> m_LineVertexBufferBases;
		std::vector<LineVertexBasePerFrame> m_LineOnTopVertexBufferBases;
		std::vector<LineVertex*> m_LineVertexBufferPtr;
		std::vector<LineVertex*> m_LineOnTopVertexBufferPtr;
		uint32_t m_LineBufferWriteIndex = 0;
		uint32_t m_LineOnTopBufferWriteIndex = 0;

        /// -------------------------------------------------------

		///< Text
		Ref<RenderPass> m_TextPass;
		std::vector<VertexBufferPerFrame> m_TextVertexBuffers;
		Ref<IndexBuffer> m_TextIndexBuffer;
		Ref<Material> m_TextMaterial;
		std::array<Ref<Texture2D>, MaxTextureSlots> m_FontTextureSlots;
		uint32_t m_FontTextureSlotIndex = 0;

		uint32_t m_TextIndexCount = 0;
		using TextVertexBasePerFrame = std::vector<TextVertex*>;
		std::vector<TextVertexBasePerFrame> m_TextVertexBufferBases;
		std::vector<TextVertex*> m_TextVertexBufferPtr;
		uint32_t m_TextBufferWriteIndex = 0;

		Mat4 m_CameraViewProj;
		Mat4 m_CameraView;
		bool m_DepthTest = true;
		float m_LineWidth = 1.0f;

		DrawStatistics m_DrawStats;
		MemoryStatistics m_MemoryStats;

		Ref<UniformBufferSet> m_UBSCamera;

        /// -------------------------------------------------------

	    struct CameraUniformBuffer
		{
			Mat4 ViewProjection;
		};
	};


}

/// -------------------------------------------------------
