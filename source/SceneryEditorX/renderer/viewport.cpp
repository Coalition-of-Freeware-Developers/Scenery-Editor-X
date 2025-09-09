/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewport.cpp
* -------------------------------------------------------
* Created: 25/8/2025
* -------------------------------------------------------
*/
#include "viewport.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    // Constructors
    Viewport::Viewport(const float x, const float y, const float width, const float height, const float minDepth, const float maxDepth) :
        m_Data{
            .x = x, .y = y,
            .width = width, .height = height,
            .minDepth = minDepth, .maxDepth = maxDepth}
    {
    }

    Viewport::Viewport(const Viewport &viewport) : Resource(viewport), m_Data(viewport.m_Data)
    {
        viewportHovered = viewport.viewportHovered;
        viewportResized = viewport.viewportResized;
    }

    Viewport::Viewport(const ViewportData& data) : m_Data(data)
    {
    }

    // Operators
    bool Viewport::operator==(const Viewport &data) const
    {
        return m_Data.x == data.m_Data.x &&
               m_Data.y == data.m_Data.y &&
               m_Data.width == data.m_Data.width &&
               m_Data.height == data.m_Data.height &&
               m_Data.minDepth == data.m_Data.minDepth &&
               m_Data.maxDepth == data.m_Data.maxDepth;
    }

    // Methods
    float Viewport::GetAspectRatio() const
    {
        return (m_Data.height != 0.0f) ? (m_Data.width / m_Data.height) : 0.0f;
    }

    bool Viewport::IsDefined() const
    {
        return m_Data.x != 0.0f || m_Data.y != 0.0f ||
               m_Data.width != 0.0f || m_Data.height != 0.0f ||
               m_Data.minDepth != 0.0f || m_Data.maxDepth != 0.0f;
    }

	void Viewport::SetData(const ViewportData& data)
	{
	    // Use placement new to reconstruct the ViewportData with new values
	    m_Data.~ViewportData();
	    new (&m_Data) ViewportData(data);
	    viewportResized = true;
	}

    // Setters for mutable operations
    void Viewport::SetPosition(float x, float y)
    {
        m_Data.~ViewportData();
        new (&m_Data) ViewportData{
            .x = x, .y = y,
            .width = m_Data.width, .height = m_Data.height,
            .minDepth = m_Data.minDepth, .maxDepth = m_Data.maxDepth
        };
        viewportResized = true;
    }

    void Viewport::SetSize(float width, float height)
    {
        m_Data.~ViewportData();
        new (&m_Data) ViewportData{
            .x = m_Data.x, .y = m_Data.y,
            .width = width, .height = height,
            .minDepth = m_Data.minDepth, .maxDepth = m_Data.maxDepth
        };
        viewportResized = true;
    }

    void Viewport::SetDepthRange(float minDepth, float maxDepth)
    {
        m_Data.~ViewportData();
        new (&m_Data) ViewportData{
            .x = m_Data.x, .y = m_Data.y,
            .width = m_Data.width, .height = m_Data.height,
            .minDepth = minDepth, .maxDepth = maxDepth
        };
    }

    const Viewport& Viewport::GetUndefined() { return s_Undefined; }

    const Ref<Viewport> undefined = CreateRef<Viewport>(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

}

/// -------------------------------------------------------
