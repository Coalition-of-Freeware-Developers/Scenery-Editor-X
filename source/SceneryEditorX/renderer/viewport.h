/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewport.h
* -------------------------------------------------------
* Created: 25/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "vulkan/resource.h"
#include "vulkan/vk_data.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class Viewport : public Resource
    {
    public:
        explicit Viewport(const float x = 0.0f, const float y = 0.0f, const float width = 0.0f, const float height = 0.0f, const float minDepth = 0.0f, const float maxDepth = 1.0f);
        Viewport(const Viewport &viewport);
        Viewport(const ViewportData& data);
        virtual ~Viewport() = default;

        bool operator==(const Viewport &data) const;
        bool operator!=(const Viewport &data) const { return !(*this == data); }

        float GetAspectRatio() const;
        bool IsDefined() const;

        // Access to viewport data
        const ViewportData& GetData() const { return m_Data; }
        void SetData(const ViewportData &data);

        // Convenience accessors
        float GetX() const { return m_Data.x; }
        float GetY() const { return m_Data.y; }
        float GetWidth() const { return m_Data.width; }
        float GetHeight() const { return m_Data.height; }
        float GetMinDepth() const { return m_Data.minDepth; }
        float GetMaxDepth() const { return m_Data.maxDepth; }

        // Setters for mutable operations
        void SetPosition(float x, float y);
        void SetSize(float width, float height);
        void SetDepthRange(float minDepth, float maxDepth);

        /** @brief Flag indicating whether the mouse is currently hovering over the viewport */
        bool viewportHovered = false;

        /** @brief Flag indicating whether the viewport has been resized and needs updating */
        bool viewportResized = false;

        // Static undefined viewport instance
        static const Viewport& GetUndefined();

    private:
        /** @brief Core viewport data using the existing ViewportData struct */
        ViewportData m_Data;

        // Static undefined instance - declared here, defined in source
        static Viewport s_Undefined;
    };

}

/// -------------------------------------------------------
