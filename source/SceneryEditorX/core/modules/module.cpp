/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* module.cpp
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#include "module.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	* @brief Constructor for Module class
	* @param name The unique name identifier for this module
	*
	* Initializes a new module with the specified name. The name can be used
	* for debugging and identification purposes throughout the application.
	*/
	Module::Module(const std::string &name) : m_Name(name) {}

	/**
	* @brief Destructor for Module class
	*
	* Virtual destructor to ensure proper cleanup of derived module classes.
	* Resources specific to each module type should be cleaned up in their
	* respective OnDetach() methods.
	*/
	Module::~Module() {}

}

/// -------------------------------------------------------
