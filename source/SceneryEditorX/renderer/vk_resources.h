/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_resources.h
* -------------------------------------------------------
* Created: 9/4/2025
* -------------------------------------------------------
*/

#pragma once
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct Resource
	{
	    std::string name;
	    int32_t rid = -1;
	    virtual ~Resource() {};
	};

} // namespace SceneryEditorX

// -------------------------------------------------------
