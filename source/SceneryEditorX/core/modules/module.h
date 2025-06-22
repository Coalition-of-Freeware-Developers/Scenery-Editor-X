/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* module.h
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class Module
	 * @brief Base Module class representing a logical section of the application.
	 * 
	 * Module are used to organize different functional components of the application
	 * that can be attached, detached and updated independently. Each module can handle
	 * its own rendering, events, and update logic.
	 */
	class Module : public RefCounted
	{
	public:
		/**
		 * @brief Construct a modular component that can be plugged into the application.
		 * @param name The name of the module.
		 */
		explicit Module(const std::string &name = "Module");
		virtual ~Module() override;
		
		/**
		 * @brief Called when the module is attached to the application.
		 * Use this for initialization of module resources.
		 */
		virtual void OnAttach() {}
		
		/**
		 * @brief Called when the module is detached from the application.
		 * Use this for cleanup of module resources.
		 */
		virtual void OnDetach() {}
		
		/**
		 * @brief Called each frame to update the module.
		 * Implement module logic that needs to execute each frame.
		 */
		virtual void OnUpdate() {}
		
		/**
		 * @brief Renders ImGui elements for this module.
		 * Implement any ImGui rendering for debugging or UI components.
		 */
		virtual void OnUIRender() {}
		
		/**
		 * @brief Processes events for this module.
		 * Handle input or application events specific to this module.
		 */
		virtual void OnEvent() {}

	private:
        std::string m_Name; ///< The name of the module.
	};

} // namespace SceneryEditorX

/// -------------------------------------------------------
