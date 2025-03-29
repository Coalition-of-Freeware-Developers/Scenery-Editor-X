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
#include <IconsFontAwesome5.h>
#include <SceneryEditorX/renderer/vk_core.h>

// -------------------------------------------------------


struct Icons
{

};


//struct Font
//{
//  /**
//	 * @brief Constructor
//	 * @param name The name of the font file that exists within 'assets/fonts' (without extension)
//	 * @param size The font size, scaled by DPI
//	 */
//    Font(const std::string &name, float size)
//        : name{name}, data{SceneryEditorX::fs::read_asset("fonts/" + name + ".ttf")}, size{size}
//    {
//        // Keep ownership of the font data to avoid a double delete
//        ImFontConfig font_config{};
//        font_config.FontDataOwnedByAtlas = false;
//
//        if (size < 1.0f)
//        {
//            size = 20.0f;
//        }
//
//        ImGuiIO &io = ImGui::GetIO();
//        handle = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
//    }
//
//    ImFont *handle{nullptr};
//
//    std::string name;
//
//    std::vector<uint8_t> data;
//
//    float size{};
//};

// -------------------------------------------------------

class GUI
{
public:
    GUI();
    ~GUI();
    void init(GLFWwindow *window, SceneryEditorX::GraphicsEngine &renderer);

    /**
	 * @brief Handles resizing of the window
	 * @param width New width of the window
	 * @param height New height of the window
	 */
    void resize(const uint32_t width, const uint32_t height) const;

    /**
	 * @brief Starts a new ImGui frame
	 *        to be called before drawing any window
	 */
    inline void new_frame();

    /**
	 * @brief Updates the Gui
	 * @param delta_time Time passed since last update
	 */
    void update(const float delta_time);

    bool update_buffers();

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

	/**
	 * @brief 
	 * @param input_event 
	 * @return 
	 */
	//bool input_event(const InputEvent &input_event);

    void setStyle();
    void setFonts();

	// The name of the default font file to use
    static const std::string default_font;
    // Used to show/hide the GUI
    static bool visible;

	//Font &get_font(const std::string &font_name = Gui::default_font);

private:
    SceneryEditorX::GraphicsEngine *renderer = nullptr;
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

void initImGuiExtensions();

// -------------------------------------------------------
