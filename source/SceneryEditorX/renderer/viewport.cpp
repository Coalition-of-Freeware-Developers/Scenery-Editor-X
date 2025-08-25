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

	bool Viewport::operator==(const Viewport &data) const
	{
	    return x == data.x && y == data.y && width == data.width && height == data.height && minDepth == data.minDepth && maxDepth == data.maxDepth;
	}
	
	bool Viewport::IsDefined() const
	{
	    return x != 0.0f || y != 0.0f || width != 0.0f || height != 0.0f || minDepth != 0.0f || maxDepth != 0.0f;
	}

    const Ref<Viewport> undefined = CreateRef<Viewport>(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

}

/// -------------------------------------------------------
