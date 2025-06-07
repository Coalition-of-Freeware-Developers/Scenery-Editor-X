/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* layer.h
* -------------------------------------------------------
* Created: 28/3/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class Layer
	 * @brief Base Layer class representing a logical section of the application.
	 * 
	 * Layers are used to organize different functional components of the application
	 * that can be attached, detached and updated independently. Each layer can handle
	 * its own rendering, events, and update logic.
	 */
	class Layer : public RefCounted
	{
	public:
		/**
		 * @brief Construct a new Layer with the specified name.
		 * @param name The name identifier for this layer, defaults to "Layer".
		 */
		explicit Layer(const std::string &name = "Layer");
		
		/**
		 * @brief Virtual destructor for proper cleanup of derived classes.
		 */
		virtual ~Layer() override;
		
		/**
		 * @brief Called when the layer is attached to the application.
		 * 
		 * Use this for initialization of layer resources.
		 */
		virtual void OnAttach() {}
		
		/**
		 * @brief Called when the layer is detached from the application.
		 * 
		 * Use this for cleanup of layer resources.
		 */
		virtual void OnDetach() {}
		
		/**
		 * @brief Called each frame to update the layer.
		 * 
		 * Implement layer logic that needs to execute each frame.
		 */
		virtual void OnUpdate() {}
		
		/**
		 * @brief Renders ImGui elements for this layer.
		 * 
		 * Implement any ImGui rendering for debugging or UI components.
		 */
		virtual void OnImGuiRender() {}
		
		/**
		 * @brief Processes events for this layer.
		 * 
		 * Handle input or application events specific to this layer.
		 */
		virtual void OnEvent() {}
	};


} // namespace SceneryEditorX

/// -------------------------------------------------------
