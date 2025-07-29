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

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

	enum LightType : uint8_t
	{
	    Point = 0,
	    Spot = 1,
	    Directional = 2,
	    LightTypeCount = 3,
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

    class LightNode : public Object
    {
	public:
        LightNode();
        //virtual void Serialize(Serializer &ser);

		/// -------------------------------------------------------

		inline static const char* typeNames[] = { "Point", "Spot", "Directional" };
		inline static const char* volumetricTypeNames[] = { "Disabled", "ScreenSpace", "ShadowMap"};

		Vec3 color = Vec3(1);
		float intensity = 10.0f;
		LightType lightType = Point;
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
    };

} 

/// -------------------------------------------------------
