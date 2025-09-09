/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* lights.h
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/object.h>
#include <SceneryEditorX/renderer/render_resource.h>
#include <xMath/includes/colors.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

	enum class LightType : uint8_t
	{
	    Point       = 0,
	    Spot        = 1,
	    Directional = 2,
	    MaxEnum
    };

    /// -------------------------------------------------------

	enum VolumetricType : uint8_t
    {
        Disabled = 0,
        ScreenSpace = 1,
        ShadowMap = 2,
        VolumetricLightCount = 3,
    };

    /// -------------------------------------------------------

    enum LightFlags : uint32_t
    {
        Shadows				= 1U << 0,
        ShadowsScreenSpace	= 1U << 1,
        Volumetric			= 1U << 2,
        DayNightCycle		= 1U << 3 // only affects directional lights
    };

    /// -------------------------------------------------------

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

    /// -------------------------------------------------------

    class LightNode : public Object
    {
	public:
        LightNode();
        virtual ~LightNode() override;

        //virtual void Serialize(Serializer &ser);

		/// -------------------------------------------------------

		inline static const char* typeNames[] = { "Point", "Spot", "Directional" };
		inline static const char* volumetricTypeNames[] = { "Disabled", "ScreenSpace", "ShadowMap"};

        /// -------------------------------------------------------

        // Flags
        bool GetFlag(const LightFlags flag) { return m_flags & flag; }
        void SetFlag(LightFlags flag, bool enable = true);
		
		/// -------------------------------------------------------

        // Type
        LightType GetLightType() const { return m_light_type; }
        void SetLightType(LightType type);

        // Color
        void SetTemperature(float temperature_kelvin);
        float GetTemperature() const { return m_temperature_kelvin; }
        void SetColor(const Color& rgb);
        const Color& GetColor() const { return m_color_rgb; }

        /// -------------------------------------------------------

        // Intensity
        void SetIntensity(float lumens_lux);
        void SetIntensity(LightIntensity intensity);
        float GetIntensityLumens() const    { return m_intensity_lumens_lux; }
        LightIntensity GetIntensity() const { return m_intensity; }
        float GetIntensityWatt() const;

        // Bias
        static float GetBias()            { return -0.0005f; }
        static float GetBiasSlopeScaled() { return -1.0f; }

        // Range
        void SetRange(float range);
        auto GetRange() const { return m_range; }

        // Angle
        void SetAngle(float angle_rad);
        auto GetAngle() const { return m_angle_rad; }

        // Matrices
        Matrix GetViewProjectionMatrix(uint32_t index) const { return m_matrix_view[index] * m_matrix_projection[index]; }

        // Frustum
        bool IsInViewFrustum(Renderable* renderable, uint32_t array_index, uint32_t instance_group_index = 0) const;

        // Index
        void SetIndex(const uint32_t index) { m_index = index; }
        uint32_t GetIndex() const           { return m_index; }

		// Screen Space Shadows - Slice Index
        void SetScreenSpaceShadowsSliceIndex(const uint32_t index) { m_index = index; }
        uint32_t GetScreenSpaceShadowsSliceIndex() const           { return m_index; }

        // Misc
        bool NeedsSkysphereUpdate() const;
        uint32_t GetSliceCount() const;

        // Atlas
        Vec2 GetAtlasOffset(uint32_t slice) const { return m_atlas_offsets[slice]; }
        Vec2 GetAtlasScale(uint32_t slice) const  { return m_atlas_scales[slice]; }
        const xMath::Rectangle& GetAtlasRectangle(uint32_t slice) const { return m_atlas_rectangles[slice]; }
        void SetAtlasRectangle(uint32_t slice, const xMath::Rectangle& rectangle);
        void ClearAtlasRectangles();
        BoundingBox GetBoundingBox() const { return m_bounding_box; }


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
        LightIntensity m_intensity       = LightIntensity::bulb_500_watt;
        float m_intensity_lumens_lux     = 2600.0f;
        uint32_t m_flags                 = 0;
        LightType m_light_type           = LightType::MaxEnum;
        Color m_color_rgb				 = Color::Black();
        float m_temperature_kelvin       = 0.0f;
        float m_range                    = 32.0f;
        float m_angle_rad                = ToRadians(30.0f);
        uint32_t m_index                 = 0;
        BoundingBox m_bounding_box = BoundingBox::Zero;

        // matrices/frustums per slice/face/cascade
        std::array<Frustum, 6> m_frustums;
        std::array<Matrix, 6> m_matrix_view;
        std::array<Matrix, 6> m_matrix_projection;

        // atlas entries per slice/face/cascade
        std::array<xMath::Rectangle, 6> m_atlas_rectangles;
        std::array<Vec2, 6> m_atlas_offsets;
        std::array<Vec2, 6> m_atlas_scales;
    };

} 

/// -------------------------------------------------------
