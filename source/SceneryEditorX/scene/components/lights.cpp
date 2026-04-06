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
 * lights.cpp
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#include "lights.h"
#include <SceneryEditorX/asset/standards.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// Most of the color/temperature values are derived from: https://physicallybased.info/ 
	// Might get inaccurate over 40000 K (which is really the limit that you should be using)
	static void TemperatureToColor(const float temperature_kelvin, float& r, float& g, float& b)
	{
		// Constants for color temperature to RGB conversion
		const float A_R = 329.698727446f;
		const float B_R = -0.1332047592f;
		const float A_G = 288.1221695283f;
		const float B_G = -0.0755148492f;

		// Ensure temperature is above absolute zero
		if (temperature_kelvin < 0) {
			// Handle error
		}

		float temp = temperature_kelvin / 100.0f;

		r = 0.0f;
		g = 0.0f;
		b = 0.0f;

		if (temp <= 66)
		{
			r = 255;
			g = temp;
			g = 99.4708025861f * log(g) - 161.1195681661f;

			if (temp <= 19)
			{
				b = 0;
			}
			else
			{
				b = temp - 10.0f;
				b = 138.5177312231f * log(b) - 305.0447927307f;
			}
		}
		else
		{
			r = temp - 60.0f;
			r = A_R * pow(r, B_R);
			g = temp - 60.0f;
			g = A_G * pow(g, B_G);
			b = 255;
		}

		// clamp rgb values to [0, 1]
		r = xMath::Clamp(r / 255.0f, 0.0f, 1.0f);
		g = xMath::Clamp(g / 255.0f, 0.0f, 1.0f);
		b = xMath::Clamp(b / 255.0f, 0.0f, 1.0f);
	}

	/**
	 * @brief 
	 * @param temperatureKelvin 
	 * @param a 
	 * @return 
	 */
	static float LightColor(const float temperatureKelvin, const float a /*= 1.0f*/)
	{
		float r, g, b;
		TemperatureToColor(temperatureKelvin, r, g, b);
		return a;
	}

	// directional matrix parameters
	const float CASCADE_NEAR_EXTENT    = 20.0f;
	const float CASCADE_FAR_EXTENT     = 300.0f;
	const float CASCADE_DEPTH          = 1000.0f;
	const float CASCADE_FAR_MAX_EXTENT = FLT_MAX;

	/**
	 * @brief 
	 * @param type 
	 * @return 
	 */
	static float GetSensibleRange(const LightType type)
	{
		if (type == LightType::Directional)
		{
			return std::numeric_limits<float>::max();
		}
		if (type == LightType::Point)
		{
			return 15.0f;
		}
		if (type == LightType::Spot)
		{
			return 15.0f;
		}
		if (type == LightType::Area)
		{
			return 20.0f;
		}

		return 0.0f;
	}

	/**
	 * @brief 
	 * @param type 
	 * @return 
	 */
	static Standard GetSensibleColor(const LightType type)
	{
		if (type == LightType::Directional)
		{
			return Standard::LIGHT_SKY_CLEAR;
		}
		if (type == LightType::Point)
		{
			return Standard::LIGHT_LIGHT_BULB;
		}
		if (type == LightType::Spot)
		{
			return Standard::LIGHT_LIGHT_BULB;
		}
		if (type == LightType::Area)
		{
			return Standard::LIGHT_LIGHT_BULB;
		}

		return Standard::LIGHT_DIRECT_SUNLIGHT;
	}

	Light::Light(Entity* entity) : Component(entity)
	{
		m_Matrix_View.fill(Matrix::IDENTITY);
		m_Matrix_Projection.fill(Matrix::IDENTITY);

		SetLightColor(GetSensibleColor(m_LightType));
		SetIntensity(LightIntensity::bulb_500_watt);
		SetRange(GetSensibleRange(m_LightType));
		SetFlag(LightFlags::Shadows);
		SetFlag(LightFlags::ShadowsScreenSpace);
	}
	
	void Light::Tick()
	{
		// detect transform change
		bool update_matrices = false;
		if (GetEntity()->GetTimeSinceLastTransform() <= 0.1f)
		{
			update_matrices = true;
		}

		// detect day night cycle change
		if (m_LightType == LightType::Directional)
		{
			// day night cycle
			if (GetFlag(LightFlags::DayNightCycle))
			{
				const float rotation_angle_rad = (Scene::GetTimeOfDay() * 360.0f - 90.0f) * xMath::DEG_TO_RAD;
				const Quat rotation = Quat::AngleAxisRadians(rotation_angle_rad, Vec3(1.0f, 0.0f, 0.0f));

				GetEntity()->SetRotation(rotation.ToEulerRadians());
				update_matrices = true;
			}

			// it follows the camera, so it also need to updated if it moves
			if (Camera* camera = Scene::GetCamera())
			{
				update_matrices = camera->GetEntity()->GetTimeSinceLastTransform() < 0.1f ? true : update_matrices;
			}
		}

		// detect active state change
		if (m_IsActive_PreviousFrame != GetEntity()->GetActive())
		{
			m_IsActive_PreviousFrame = GetEntity()->GetActive();
			update_matrices = true;
		}

		if (update_matrices)
		{
			UpdateMatrices();
		}
	}

	void Light::SetFlag(LightFlags flag, bool enable)
	{
		bool enabled      = false;
		bool disabled     = false;
		bool flag_present = m_Flags & flag;

		if (enable && !flag_present)
		{
			m_Flags |= static_cast<uint32_t>(flag);
			enabled  = true;
		}
		else if (!enable && flag_present)
		{
			m_Flags  &= ~static_cast<uint32_t>(flag);
			disabled  = true;
		}

		if (enabled || disabled)
		{
			if (disabled)
			{
				// if the shadows have been disabled, disable properties which rely on them
				if (flag & LightFlags::Shadows)
				{
					m_Flags &= ~static_cast<uint32_t>(LightFlags::ShadowsScreenSpace);
					m_Flags &= ~static_cast<uint32_t>(LightFlags::Volumetric);
				}
			}
		}
	}
	
	void Light::SetLightType(LightType type)
	{
		if (m_LightType == type)
			return;

		m_LightType = type;

		SetLightColor(GetSensibleColor(m_LightType));
		SetRange(GetSensibleRange(m_LightType));
		UpdateMatrices();
	}
	
	void Light::SetTemperature(float temperature_kelvin)
	{
		m_TemperatureKelvin = temperature_kelvin;
		m_Color = Standard(temperature_kelvin);
	}
	
	void Light::SetLightColor(const Standard &rgb)
	{
		m_Color = rgb;

		if (rgb == Standard::LIGHT_SKY_CLEAR)
			m_TemperatureKelvin = 15000.0f;
		else if (rgb == Standard::LIGHT_SKY_DAYLIGHT_OVERCAST)
			m_TemperatureKelvin = 6500.0f;
		else if (rgb == Standard::LIGHT_SKY_MOONLIGHT)
			m_TemperatureKelvin = 4000.0f;
		else if (rgb == Standard::LIGHT_SKY_SUNRISE)
			m_TemperatureKelvin = 2000.0f;
		else if (rgb == Standard::LIGHT_CANDLE_FLAME)
			m_TemperatureKelvin = 1850.0f;
		else if (rgb == Standard::LIGHT_DIRECT_SUNLIGHT)
			m_TemperatureKelvin = 5778.0f;
		else if (rgb == Standard::LIGHT_DIGITAL_DISPLAY)
			m_TemperatureKelvin = 6500.0f;
		else if (rgb == Standard::LIGHT_FLUORESCENT_TUBE_LIGHT)
			m_TemperatureKelvin = 5000.0f;
		else if (rgb == Standard::LIGHT_KEROSENE_LAMP)
			m_TemperatureKelvin = 1850.0f;
		else if (rgb == Standard::LIGHT_LIGHT_BULB)
			m_TemperatureKelvin = 2700.0f;
		else if (rgb == Standard::LIGHT_PHOTO_FLASH)
			m_TemperatureKelvin = 5500.0f;
	}
	
	void Light::SetIntensity(float lumens_lux)
	{
		m_IntensityLumens_Lux = lumens_lux;
		m_Intensity           = LightIntensity::custom;
	}
	
	void Light::SetIntensity(LightIntensity intensity)
	{
		m_Intensity = intensity;

		if (intensity == LightIntensity::bulb_stadium)
		{
			m_IntensityLumens_Lux = 200000.0f;
		}
		else if (intensity == LightIntensity::bulb_500_watt)
		{
			m_IntensityLumens_Lux = 8500.0f;
		}
		else if (intensity == LightIntensity::bulb_150_watt)
		{
			m_IntensityLumens_Lux = 2600.0f;
		}
		else if (intensity == LightIntensity::bulb_100_watt)
		{
			m_IntensityLumens_Lux = 1600.0f;
		}
		else if (intensity == LightIntensity::bulb_60_watt)
		{
			m_IntensityLumens_Lux = 800.0f;
		}
		else if (intensity == LightIntensity::bulb_25_watt)
		{
			m_IntensityLumens_Lux = 200.0f;
		}
		else if (intensity == LightIntensity::bulb_flashlight)
		{
			m_IntensityLumens_Lux = 100.0f;
		}
		else // black hole
		{
			m_IntensityLumens_Lux = 0.0f;
		}
	}
	
	float Light::GetIntensityWatt() const
	{
		// ideal luminous efficacy of monochromatic radiation at 555 nm (lm/w).
		// note: for broad spectrum white light, ~250-400 is more accurate,
		// but 683 is the standard "ideal" definition used in engines like ue5/frostbite
		const float luminous_efficacy = 683.0f;

		// 1. convert photometric (lumens/lux) to radiometric (watts)
		float radiant_flux = m_IntensityLumens_Lux / luminous_efficacy;

		if (m_LightType == LightType::Directional)
		{
			// directional: input is lux (lm/m^2), output is irradiance (w/m^2)
			// no solid angle conversion needed
			return radiant_flux;
		}
		else
		{
			// point/spot: input is lumens (lm) -> flux (watts)
			// we need radiant intensity (watts/sr)
			// divide by 4pi to distribute flux over the sphere
			return radiant_flux / (4.0f * 3.14159265359f);
		}
	}
	
	void Light::SetRange(float range)
	{
		range = xMath::Clamp(range, 0.0f, std::numeric_limits<float>::max());
		if (range == m_Range)
			return;

		m_Range = range;
		UpdateMatrices();
	}
	
	void Light::SetAreaWidth(float width)
	{
		width = xMath::Clamp(width, 0.01f, 100.0f);
		if (width == m_AreaWidth)
			return;

		m_AreaWidth = width;
		UpdateMatrices();
	}
	
	void Light::SetAreaHeight(float height)
	{
		height = xMath::Clamp(height, 0.01f, 100.0f);
		if (height == m_AreaHeight)
			return;

		m_AreaHeight = height;
		UpdateMatrices();
	}
	
	void Light::SetAngle(float angle_rad)
	{
		angle_rad = xMath::Clamp(angle_rad, 0.0f, xMath::TWO_PI);
		if (angle_rad == m_Angle_Rad)
			return;

		m_Angle_Rad = angle_rad;
		UpdateMatrices();
	}
	
	bool Light::IsInViewFrustum(Renderable *renderable, uint32_t array_index, uint32_t instance_group_index) const
	{
		const BoundingBox& bounding_box = renderable->GetBoundingBox();
		const Vec3 center            = bounding_box.GetCenter();
		const Vec3 extents           = bounding_box.GetExtents();
		const bool ignore_depth         = m_LightType == LightType::Directional; // orthographic

		return m_Frustums[array_index].IsVisible(center, extents, ignore_depth);
	}
	
	bool Light::NeedsSkysphereUpdate() const
	{
		if (m_LightType != LightType::Directional)
			return false;

		static Vec3 last_rotation					= Vec3(0.0f, 0.0f, 0.0f);
		static Standard last_color_rgb				= {0.0f, 0.0f, 0.0f, 0.0f};
		static float last_intensity_lumens_lux		= std::numeric_limits<float>::max();

		Vec3 current_rotation = GetEntity() ? GetEntity()->GetRotation() : Vec3(0.0f, 0.0f, 0.0f);
		bool rotation_changed  = current_rotation != last_rotation;
		bool color_changed     = m_Color != last_color_rgb;
		bool intensity_changed = abs(m_IntensityLumens_Lux - last_intensity_lumens_lux) > 0.01f;

		if (rotation_changed || color_changed || intensity_changed)
		{
			last_rotation             = current_rotation;
			last_color_rgb            = m_Color;
			last_intensity_lumens_lux = m_IntensityLumens_Lux;
			return true;
		}

		return false;
	}
	
	uint32_t Light::GetSliceCount() const
	{
		if (m_LightType == LightType::Directional) return 2;
		if (m_LightType == LightType::Point)       return 6;
		return 1; // spot and area lights use a single slice
	}
	
	void Light::SetAtlasRectangle(uint32_t slice, const xMath::Rectangle &rectangle)
	{
		m_Atlas_Rectangles[slice] = rectangle;
		float atlas_w             = static_cast<float>(Renderer::GetRenderTarget(Renderer_RenderTarget::shadow_atlas)->GetWidth());
		float atlas_h             = static_cast<float>(Renderer::GetRenderTarget(Renderer_RenderTarget::shadow_atlas)->GetHeight());
		m_Atlas_Offsets[slice]    = Vec2(rectangle.x / atlas_w, rectangle.y / atlas_h);
		m_Atlas_Scales[slice]     = Vec2(rectangle.width / atlas_w, rectangle.height / atlas_h);
	}
	
	void Light::ClearAtlasRectangles()
	{
		m_Atlas_Rectangles.fill(xMath::Rectangle::ZERO);
		m_Atlas_Offsets.fill(Vec2::ZERO);
		m_Atlas_Scales.fill(Vec2::ZERO);
	}
	
	void Light::UpdateMatrices()
	{
		UpdateViewMatrix();
		UpdateProjectionMatrix();
		UpdateBoundingBox();
	}
	
	void Light::UpdateViewMatrix()
	{
		const Vec3 position = GetEntity()->GetPosition(); // light�s base position (arbitrary for directional)
		Matrix matrix_builder;

		if (m_LightType == LightType::Directional)
		{
			Camera* camera = Scene::GetCamera();
			if (!camera)
				return;

			// both cascades follow the camera
			Vec3 camera_pos = camera->GetEntity()->GetPosition();
			Vec3 position   = camera_pos - GetEntity()->GetForward() * CASCADE_DEPTH * 0.5f;
			m_Matrix_View[0]   = matrix_builder.CreateLookAtLH(position, camera_pos, Vec3(0.0f, 1.0f, 0.0f));
			m_Matrix_View[1]   = m_Matrix_View[0];

			// move the light in words units per texel to avoid shimmering
			{
				// compute shadow extents (both fixed sizes)
				float extents[2];
				extents[0] = CASCADE_NEAR_EXTENT; // near cascade: fixed
				extents[1] = CASCADE_FAR_EXTENT;  // far cascade: fixed, bigger than near

				float atlas_width = static_cast<float>(Renderer::GetRenderTarget(Renderer_RenderTarget::shadow_atlas)->GetWidth());
				for (uint32_t i  = 0; i < 2; i++)
				{
					float rect_width           = m_Atlas_Rectangles[i].width;                // cascade rectangle width in atlas
					float atlas_scale          = rect_width / atlas_width;                   // proportion of atlas used by cascade
					float effective_resolution = atlas_width * atlas_scale;                  // effective resolution for cascade
					float texel_size_world     = (2.0f * extents[i]) / effective_resolution; // world units per texel
					m_Matrix_View[i].m30       = round(m_Matrix_View[i].m30 / texel_size_world) * texel_size_world; // snap x
					m_Matrix_View[i].m31       = round(m_Matrix_View[i].m31 / texel_size_world) * texel_size_world; // snap y
					// z-translation (m32) remains unchanged for orthographic projection
				}
			}
		}
		else if (m_LightType == LightType::Spot)
		{
			m_Matrix_View[0] = matrix_builder.CreateLookAtLH(position, position + GetEntity()->GetForward(), Vec3(0.0f, 1.0f, 0.0f));
		}
		else if (m_LightType == LightType::Area)
		{
			// area light looks along its forward direction
			m_Matrix_View[0] = matrix_builder.CreateLookAtLH(position, position + GetEntity()->GetForward(), Vec3(0.0f, 1.0f, 0.0f));
		}
		else if (m_LightType == LightType::Point)
		{
			// +X (right)
			m_Matrix_View[0] = matrix_builder.CreateLookAtLH(position, position + Vec3(1.0f, 0.0f, 0.0f),  Vec3(0.0f, 1.0f, 0.0f));
			// -X (left)
			m_Matrix_View[1] = matrix_builder.CreateLookAtLH(position, position + Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
			// +Y (up)
			m_Matrix_View[2] = matrix_builder.CreateLookAtLH(position, position + Vec3(0.0f, 1.0f, 0.0f),  Vec3(0.0f, 0.0f, -1.0f));
			// -Y (down)
			m_Matrix_View[3] = matrix_builder.CreateLookAtLH(position, position + Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
			// +Z (forward)
			m_Matrix_View[4] = matrix_builder.CreateLookAtLH(position, position + Vec3(0.0f, 0.0f, 1.0f),  Vec3(0.0f, 1.0f, 0.0f));
			// -Z (backward)
			m_Matrix_View[5] = matrix_builder.CreateLookAtLH(position, position + Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, 1.0f, 0.0f));
		}
	}
	
	void Light::UpdateProjectionMatrix()
	{
		Matrix matrix_builder;

		if (m_LightType == LightType::Directional)
		{
			// near cascade (tight, camera following)
			m_Matrix_Projection[0] = matrix_builder.CreateOrthoOffCenterLH(
				-CASCADE_NEAR_EXTENT, CASCADE_NEAR_EXTENT,
				-CASCADE_NEAR_EXTENT, CASCADE_NEAR_EXTENT,
				CASCADE_DEPTH, 0.0f
			);

			// far cascade (camera following, fixed size, bigger than near cascade)
			m_Matrix_Projection[1] = matrix_builder.CreateOrthoOffCenterLH(
				-CASCADE_FAR_EXTENT, CASCADE_FAR_EXTENT,
				-CASCADE_FAR_EXTENT, CASCADE_FAR_EXTENT,
				CASCADE_DEPTH, 0.0f
			);

			m_Frustums[0] = Frustum(m_Matrix_View[0], m_Matrix_Projection[0]);
			m_Frustums[1] = Frustum(m_Matrix_View[1], m_Matrix_Projection[1]);
		}
		else if (m_LightType == LightType::Area)
		{
			// area lights use orthographic projection based on their dimensions
			float half_width  = m_AreaWidth * 0.5f;
			float half_height = m_AreaHeight * 0.5f;

			m_Matrix_Projection[0] = matrix_builder.CreateOrthoOffCenterLH(
				-half_width, half_width,
				-half_height, half_height,
				m_Range, 0.05f
			);
			m_Frustums[0] = Frustum(m_Matrix_View[0], m_Matrix_Projection[0]);
		}
		else // spot/point
		{
			const float aspectRatio  = 1;
			const float fovYRadians = m_LightType == LightType::Spot ? m_Angle_Rad * 2.0f : xMath::TWO_PI + 0.02f; // small epsilon to hide face seems

			for (uint32_t i = 0; i < GetSliceCount(); i++)
			{
				m_Matrix_Projection[i] = matrix_builder.CreatePerspectiveFieldOfViewLH(fovYRadians, aspectRatio, m_Range, 0.05f);
				m_Frustums[i]          = Frustum(m_Matrix_View[i], m_Matrix_Projection[i]);
			}
		}
	}
	
	void Light::UpdateBoundingBox()
	{
		const Vec3 position = GetEntity()->GetPosition();

		if (m_LightType == LightType::Point)
		{
			const float radius = m_Range;
			m_BoundingBox = xMath::BoundingBox(
				position - Vec3(radius, radius, radius),
				position + Vec3(radius, radius, radius)
			);
		}
		else if (m_LightType == LightType::Spot)
		{
			const float opposite = m_Range * tan(m_Angle_Rad);

			const Vec3 pos_tip    = position;
			const Vec3 pos_center = pos_tip    + GetEntity()->GetForward() * m_Range;
			const Vec3 pos_up     = pos_center + GetEntity()->GetUp()      * opposite;
			const Vec3 pos_down   = pos_center + GetEntity()->GetDown()    * opposite;
			const Vec3 pos_right  = pos_center + GetEntity()->GetRight()   * opposite;
			const Vec3 pos_left   = pos_center + GetEntity()->GetLeft()    * opposite;

			Vec3 min = pos_tip;
			Vec3 max = pos_tip;

			auto expand = [&](const Vec3& p)
			{
				min.x = xMath::Min(min.x, p.x);
				min.y = xMath::Min(min.y, p.y);
				min.z = xMath::Min(min.z, p.z);

				max.x = xMath::Max(max.x, p.x);
				max.y = xMath::Max(max.y, p.y);
				max.z = xMath::Max(max.z, p.z);
			};

			expand(pos_center);
			expand(pos_up);
			expand(pos_down);
			expand(pos_right);
			expand(pos_left);

			m_BoundingBox = xMath::BoundingBox(min, max);
		}
		else if (m_LightType == LightType::Area)
		{
			// area light bounding box extends from the light rectangle to its range
			const float half_width  = m_AreaWidth * 0.5f;
			const float half_height = m_AreaHeight * 0.5f;

			// corners of the area light rectangle
			const Vec3 right   = GetEntity()->GetRight();
			const Vec3 up      = GetEntity()->GetUp();
			const Vec3 forward = GetEntity()->GetForward();

			Vec3 min = position;
			Vec3 max = position;

			auto expand = [&](const Vec3& p)
			{
				min.x = xMath::Min(min.x, p.x);
				min.y = xMath::Min(min.y, p.y);
				min.z = xMath::Min(min.z, p.z);

				max.x = xMath::Max(max.x, p.x);
				max.y = xMath::Max(max.y, p.y);
				max.z = xMath::Max(max.z, p.z);
			};

			// expand by corners of the light rectangle
			expand(position + right * half_width + up * half_height);
			expand(position - right * half_width + up * half_height);
			expand(position + right * half_width - up * half_height);
			expand(position - right * half_width - up * half_height);

			// expand by the range in the forward direction
			expand(position + forward * m_Range + right * half_width + up * half_height);
			expand(position + forward * m_Range - right * half_width + up * half_height);
			expand(position + forward * m_Range + right * half_width - up * half_height);
			expand(position + forward * m_Range - right * half_width - up * half_height);

			m_BoundingBox = xMath::BoundingBox(min, max);
		}
		else // directional
		{
			m_BoundingBox = xMath::BoundingBox::BB_INFINITE;
		}
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
