/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* component.cpp
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/scene/component.h>

// -------------------------------------------------------

namespace Scene
{
	Component::Component(const std::string &name) : name{name} { }
	
	const std::string &Component::get_name() const { return name; }
} // namespace Scene

// -------------------------------------------------------
