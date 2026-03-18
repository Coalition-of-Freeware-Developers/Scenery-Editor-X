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
 * lights.h
 * -------------------------------------------------------
 * Created: 16/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include "components/component.h"
#include <xMath/includes/colors.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Renderable;

	// -------------------------------------------------------

	enum class LightType : uint8_t
	{
		Point       = 0,
		Spot        = 1,
		Directional = 2,
		MaxEnum
	};

	// -------------------------------------------------------

	enum VolumetricType : uint8_t
	{
		Disabled = 0,
		ScreenSpace = 1,
		ShadowMap = 2,
		VolumetricLightCount = 3,
	};

	// -------------------------------------------------------

	enum LightFlags : uint32_t
	{
		Shadows				= 1U << 0,
		ShadowsScreenSpace	= 1U << 1,
		Volumetric			= 1U << 2,
		DayNightCycle		= 1U << 3 // only affects directional lights
	};

	// -------------------------------------------------------

	enum class LightIntensity : uint8_t
	{
		bulb_stadium,    // intense light used in stadiums for sports events, comparable to sunlight
		bulb_500_watt,   // a very bright domestic bulb or small industrial light
		bulb_150_watt,   // a bright domestic bulb, equivalent to an old-school incandescent bulb
		bulb_100_watt,   // a typical bright domestic bulb
		bulb_60_watt,    // a medium intensity domestic bulb
		bulb_25_watt,    // a low intensity domestic bulb, used for mood lighting or as a night light
		bulb_flashlight, // light emitted by an average flashlight, portable and less intense
		black_hole,      // no light emitted
		custom           // custom intensity
	};

	// -------------------------------------------------------

	class Light : public Component
	{
	public:
		Light();
		virtual ~Light() = default;

		//virtual void Serialize(Serializer &ser);

		// -------------------------------------------------------

		inline static const char* typeNames[] = { "Point", "Spot", "Directional" };
		inline static const char* volumetricTypeNames[] = { "Disabled", "ScreenSpace", "ShadowMap"};

		// -------------------------------------------------------

		// Flags
	    bool GetFlag(const LightFlags flag) { return m_Flags & flag; }
	    void SetFlag(LightFlags flag, bool enable = true);
		
		// -------------------------------------------------------

		// Type
		LightType GetLightType() const { return m_LightType; }
		void SetLightType(LightType type);

		// Color
		void SetTemperature(float temperature_kelvin);
		float GetTemperature() const { return m_TemperatureKelvin; }
		void SetColor(const Color& rgb);
		const Color& GetColor() const { return m_Color; }

		// -------------------------------------------------------

		// Intensity
		void SetIntensity(float lumens_lux);
		void SetIntensity(LightIntensity intensity);
		float GetIntensityLumens() const    { return m_IntensityLumens_Lux; }
		LightIntensity GetIntensity() const { return m_Intensity; }
		float GetIntensityWatt() const;

		// Bias
		static float GetBias()            { return -0.0005f; }
		static float GetBiasSlopeScaled() { return -1.0f; }

		// Range
		void SetRange(float range);
		auto GetRange() const { return m_Range; }

		// Angle
		void SetAngle(float angle_rad);
		auto GetAngle() const { return m_Angle_Rad; }

		// Matrices
		Matrix GetViewProjectionMatrix(uint32_t index) const { return m_Matrix_View[index] * m_Matrix_Projection[index]; }

		// Frustum
		bool IsInViewFrustum(Renderable* renderable, uint32_t array_index, uint32_t instance_group_index = 0) const;

		// Index
		void SetIndex(const uint32_t index) { m_Index = index; }
		uint32_t GetIndex() const           { return m_Index; }

		// Screen Space Shadows - Slice Index
	    void SetScreenSpaceShadowsSliceIndex(const uint32_t index) { m_Index = index; }
	    uint32_t GetScreenSpaceShadowsSliceIndex() const           { return m_Index; }

		// Misc
		bool NeedsSkysphereUpdate() const;
		uint32_t GetSliceCount() const;

		// Atlas
		Vec2 GetAtlasOffset(uint32_t slice) const { return m_Atlas_Offsets[slice]; }
		Vec2 GetAtlasScale(uint32_t slice) const  { return m_Atlas_Scales[slice]; }
		const xMath::Rectangle& GetAtlasRectangle(uint32_t slice) const { return m_Atlas_Rectangles[slice]; }
		void SetAtlasRectangle(uint32_t slice, const xMath::Rectangle& rectangle);
		void ClearAtlasRectangles();
		BoundingBox GetBoundingBox() const { return m_BoundingBox; }

		Vec3 color = Vec3(1);
		float intensity = 10.0f;
		LightType lightType = LightType::Point;
		float radius = 2.0f;
		float innerAngle = 60.f;
		float outerAngle = 50.f;

		float shadowMapRange = 3.0f;
		float shadowMapFar = 2000.0f;

		struct VolumetricScreenSpaceParams
		{
			float absorption = 0.5f;
			int samples = 128;
		} volumetricScreenSpaceParams;

		struct VolumetricShadowMapParams
		{
			float weight = 0.0001f;
			float absorption = 1.0f;
			float density = 1.094f;
			int samples = 128;
		} volumetricShadowMapParams;

		VolumetricType volumetricType = ScreenSpace;

	private:
		friend class AssetManager;

		void UpdateMatrices();
		void UpdateViewMatrix();
		void UpdateProjectionMatrix();
		void UpdateBoundingBox();

		// Properties
		LightIntensity m_Intensity       = LightIntensity::bulb_500_watt;
		float m_IntensityLumens_Lux      = 2600.0f;
		uint32_t m_Flags                 = 0;
		LightType m_LightType            = LightType::MaxEnum;
		Color m_Color					 = Color::Black();
		float m_TemperatureKelvin        = 0.0f;
		float m_Range                    = 32.0f;
		float m_Angle_Rad                = ToRadians(30.0f);
		uint32_t m_Index                 = 0;
		BoundingBox m_BoundingBox		 = BoundingBox::Zero;

		// matrices/frustums per slice/face/cascade
		std::array<Frustum, 6> m_Frustums;
		std::array<Matrix, 6>  m_Matrix_View;
		std::array<Matrix, 6>  m_Matrix_Projection;

		// atlas entries per slice/face/cascade
		std::array<xMath::Rectangle, 6> m_Atlas_Rectangles;
		std::array<Vec2, 6> m_Atlas_Offsets;
		std::array<Vec2, 6> m_Atlas_Scales;
	};

} 

// -------------------------------------------------------
