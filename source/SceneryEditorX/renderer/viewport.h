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

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class Viewport : public Resource
    {
    public:
        explicit Viewport::Viewport(const float x = 0.0f, const float y = 0.0f, const float width = 0.0f, const float height = 0.0f, const float minDepth = 0.0f, const float maxDepth = 1.0f)
		{
		    this->x			= x;
		    this->y			= y;
		    this->width		= width;
		    this->height	= height;
		    this->minDepth  = minDepth;
		    this->maxDepth  = maxDepth;
		}

		Viewport::Viewport(const Viewport &viewport) : Resource(viewport)
        {
		    x		 = viewport.x;
		    y		 = viewport.y;
		    width	 = viewport.width;
		    height	 = viewport.height;
		    minDepth = viewport.minDepth;
		    maxDepth = viewport.maxDepth;
		}

        virtual ~Viewport() = default;

        bool operator==(const Viewport &data) const;
        bool operator!=(const Viewport &data) const { return !(*this == data); }
		float GetAspectRatio() const { return width / height; }
        bool IsDefined() const;

        /** @brief Flag indicating whether the mouse is currently hovering over the viewport */
        bool viewportHovered = false;

        /** @brief Flag indicating whether the viewport has been resized and needs updating */
        bool viewportResized = false;

        float x		 = 0.0f;
        float y		 = 0.0f;
        float width	 = 0.0f;
        float height	 = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;

        GLOBAL const Viewport undefined;
    };

}

/// -------------------------------------------------------
