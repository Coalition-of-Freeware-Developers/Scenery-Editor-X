/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui.h
* -------------------------------------------------------
* Created: 25/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/ui/colors.h>
#include <SceneryEditorX/ui/fonts.h>
#include <SceneryEditorX/ui/gui_context.h>
#include <SceneryEditorX/ui/ui_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class EditorUI : public ContextHandler
	{
	public:
	    EditorUI();
	    EditorUI(const std::string& name);
        virtual ~EditorUI();
	
	    void initUI(GLFWwindow *window, GraphicsEngine &renderer);
	
		// Stores the active command buffer
	    VkCommandBuffer activeCommandBuffer = VK_NULL_HANDLE;
	
		// Method to set the command buffer
	    // TODO: Refactor this later. This should be handled better
		//void setActiveCommandBuffer(VkCommandBuffer cmdBuffer) { activeCommandBuffer = cmdBuffer; }
        virtual void Begin() override;
        virtual void End() override;

		virtual void OnAttach(); //override;
        virtual void OnDetach(); //override;
        virtual void OnUIRender() override;

	    /**
		 * @brief Handles resizing of the window
		 * @param width New width of the window
		 * @param height New height of the window
		 */
	    void resize(const uint32_t width, const uint32_t height) const;
	
	    /**
		 * @brief Starts a new ImGui frame to be called before drawing any window
		 */
	    void newFrame();
	
	    /**
	     * @brief Cleans up ImGui resources
	     */
	    void cleanUp();
	
	    /**
		 * @brief Updates the Gui
		 * @param delta_time DeltaTime passed since last update
		 */
	    void update(const float delta_time);
	
	    /**
	     * @brief 
	     */
	    bool updateBuffers = false;
	
		/**
		 * @brief Shows an overlay top window with app info and maybe stats
		 * @param app_name Application name
		 * @param stats Statistics to show (can be null)
		 * @param debug_info Debug info to show (can be null)
		 */
	    //void show_top_window(const std::string &app_name, const Stats *stats = nullptr, DebugInfo *debug_info = nullptr);
	
	    /**
		 * @brief Shows the ImGui Demo window
		 */
	    void show_demo_window();
	
	    /**
		 * @brief Shows an child with app info
		 * @param app_name Application name
		 */
	    void show_app_info(const std::string &app_name);
	
	    void setStyle();
	    void setFonts();
	
		// The name of the default font file to use
	    static const std::string default_font;
	    // Used to show/hide the EditorUI
	    static bool visible;
	
	private:
	    GraphicsEngine *renderer = nullptr;
	    VkDescriptorPool imguiPool = VK_NULL_HANDLE;

        //Ref<RenderCommandBuffer> renderCommandBuffer;
        bool initialized = false;
        float Time_ = 0.0f;
	    //const ImGuiWindowFlags common_flags  = ImGuiWindowFlags_NoCollapse;
	    //const ImGuiWindowFlags options_flags = ImGuiWindowFlags_NoResize;
	    //const ImGuiWindowFlags info_flags    = ImGuiWindowFlags_NoMove;
	    size_t last_vertex_buffer_size;
	    size_t last_index_buffer_size;
	    //  Scale factor to apply due to a difference between the window and GL pixel sizes
	    float content_scale_factor{1.0f};
	    // Scale factor to apply to the size of gui elements (expressed in dp)
	    float dpi_factor{1.0f};
	    //std::vector<Font> fonts;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------
