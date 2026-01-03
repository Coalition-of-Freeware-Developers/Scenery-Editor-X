/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* layer.cpp
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#include "layer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	* @brief Constructor for layer class
	* @param name The unique name identifier for this layer
	*
	* Initializes a new layer with the specified name. The name can be used
	* for debugging and identification purposes throughout the application.
	*/
	Layer::Layer(const std::string &name) : m_DebugName(name) {}

	/**
	* @brief Destructor for layer class
	*
	* Virtual destructor to ensure proper cleanup of derived layer classes.
	* Resources specific to each layer type should be cleaned up in their
	* respective OnDetach() methods.
	*/
	Layer::~Layer() = default;

}

// -------------------------------------------------------
