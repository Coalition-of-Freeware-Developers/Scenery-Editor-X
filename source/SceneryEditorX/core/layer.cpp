/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* layer.cpp
* -------------------------------------------------------
* Created: 28/3/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/layer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	* @brief Constructor for Layer class
	* @param name The unique name identifier for this layer
	* 
	* Initializes a new layer with the specified name. The name can be used
	* for debugging and identification purposes throughout the application.
	*/
	Layer::Layer(const std::string &name) {}

	/**
	* @brief Destructor for Layer class
	* 
	* Virtual destructor to ensure proper cleanup of derived layer classes.
	* Resources specific to each layer type should be cleaned up in their
	* respective OnDetach() methods.
	*/
	Layer::~Layer() {}


} // namespace SceneryEditorX

/// -------------------------------------------------------
