/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ecs.h
* -------------------------------------------------------
* Created: 20/7/2025
* -------------------------------------------------------
*/
#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <limits>
#include <SceneryEditorX/core/identifiers/uuid.h>
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/scene/material.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    struct EntityID
    {
        ///< Use explicit UUID constructor for zero value
        UUID ID = UUID(0);
    };

	struct EntityTag
	{
		std::string Tag;

		EntityTag() = default;
		EntityTag(const EntityTag& other) = default;
        explicit EntityTag(std::string tag) : Tag(std::move(tag)) {}

        explicit operator std::string& () { return Tag; }
        explicit operator const std::string& () const { return Tag; }
	};


	struct TextComponent
    {
        std::string TextString;
        size_t TextHash = 0;

        ///< Font
        UUID32 FontHandle;
        Vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
        float LineSpacing = 0.0f;
        float Kerning = 0.0f;

        ///< Layout
        float MaxWidth = 10.0f;

        bool ScreenSpace = false;
        bool DropShadow = false;
        float ShadowDistance = 0.0f;
        Vec4 ShadowColor = {0.0f, 0.0f, 0.0f, 1.0f};

        TextComponent() = default;
        TextComponent(const TextComponent &other) = default;
    };

	struct StaticMeshComponent
	{
		AssetHandle StaticMesh;
        Ref<MaterialTable> MaterialTable = CreateRef<SceneryEditorX::MaterialTable>();
		bool Visible = true;

		StaticMeshComponent() = default;
		StaticMeshComponent(const StaticMeshComponent& other) : StaticMesh(other.StaticMesh), MaterialTable(CreateRef<SceneryEditorX::MaterialTable>(other.MaterialTable)), Visible(other.Visible) {}
        explicit StaticMeshComponent(const AssetHandle &staticMesh) : StaticMesh(staticMesh) {}
	};

	struct SkyLightComponent
	{
		AssetHandle SceneEnvironment;
		float Intensity = 1.0f;
		float Lod = 0.0f;

		bool DynamicSky = false;
		Vec3 TurbidityAzimuthInclination = { 2.0, 0.0, 0.0 };
	};

}

/// -------------------------------------------------------
