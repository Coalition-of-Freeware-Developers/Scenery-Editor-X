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
#include <imgui/imgui.h>
#include <SceneryEditorX/ui/ui_manager.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/renderer.h>

/// -------------------------------------------------------

struct Icons
{
    /// Core UI icons
    static constexpr const char* ARROW_RIGHT = ICON_FA_ARROW_RIGHT;
    static constexpr const char* ARROW_LEFT = ICON_FA_ARROW_LEFT;
    static constexpr const char* ARROW_UP = ICON_FA_ARROW_UP;
    static constexpr const char* ARROW_DOWN = ICON_FA_ARROW_DOWN;
    static constexpr const char* PLUS = ICON_FA_PLUS;
    static constexpr const char* MINUS = ICON_FA_MINUS;
    static constexpr const char* CHECK = ICON_FA_CHECK;
    static constexpr const char* TIMES = ICON_FA_TIMES;
    static constexpr const char* TRASH = ICON_FA_TRASH;
    static constexpr const char* SAVE = ICON_FA_SAVE;
    static constexpr const char* FOLDER = ICON_FA_FOLDER;
    static constexpr const char* FOLDER_OPEN = ICON_FA_FOLDER_OPEN;
    static constexpr const char* FILE = ICON_FA_FILE;
    static constexpr const char* FILE_ALT = ICON_FA_FILE_ALT;

    /// Editor tools
    static constexpr const char* PENCIL = ICON_FA_PENCIL_ALT;
    static constexpr const char* EDIT = ICON_FA_EDIT;
    static constexpr const char* MOVE = ICON_FA_ARROWS_ALT;
    static constexpr const char* ROTATE = ICON_FA_SYNC;
    static constexpr const char* SCALE = ICON_FA_EXPAND;
    static constexpr const char* UNDO = ICON_FA_UNDO;
    static constexpr const char* REDO = ICON_FA_REDO;
    static constexpr const char* EYE = ICON_FA_EYE;
    static constexpr const char* EYE_SLASH = ICON_FA_EYE_SLASH;
    static constexpr const char* LOCK = ICON_FA_LOCK;
    static constexpr const char* UNLOCK = ICON_FA_UNLOCK;

    /// 3D visualization
    static constexpr const char* CUBE = ICON_FA_CUBE;
    static constexpr const char* CUBES = ICON_FA_CUBES;
    static constexpr const char* MOUNTAIN = ICON_FA_MOUNTAIN;
    static constexpr const char* MAP = ICON_FA_MAP;
    static constexpr const char* COMPASS = ICON_FA_COMPASS;
    static constexpr const char* LOCATION = ICON_FA_MAP_MARKER_ALT;
    static constexpr const char* RULER = ICON_FA_RULER_COMBINED;
    static constexpr const char* CROSSHAIRS = ICON_FA_CROSSHAIRS;
    static constexpr const char* OBJECT_GROUP = ICON_FA_OBJECT_GROUP;
    static constexpr const char* OBJECT_UNGROUP = ICON_FA_OBJECT_UNGROUP;

    /// Interface elements
    static constexpr const char* COG = ICON_FA_COG;
    static constexpr const char* COGS = ICON_FA_COGS;
    static constexpr const char* BARS = ICON_FA_BARS;
    static constexpr const char* QUESTION = ICON_FA_QUESTION;
    static constexpr const char* INFO = ICON_FA_INFO_CIRCLE;
    static constexpr const char* EXCLAMATION = ICON_FA_EXCLAMATION_TRIANGLE;
    static constexpr const char* SEARCH = ICON_FA_SEARCH;
    static constexpr const char* HOME = ICON_FA_HOME;
    static constexpr const char* DOWNLOAD = ICON_FA_DOWNLOAD;
    static constexpr const char* UPLOAD = ICON_FA_UPLOAD;
    static constexpr const char* SYNC = ICON_FA_SYNC_ALT;
    static constexpr const char* PLAY = ICON_FA_PLAY;
    static constexpr const char* PAUSE = ICON_FA_PAUSE;
    static constexpr const char* STOP = ICON_FA_STOP;

    /// Helper method to get font icon
    static const char* GetIcon(const std::string& name)
    {
        /// Map common names to icon constants
        static const std::unordered_map<std::string, const char*> iconMap = {
            {"arrow_right", ARROW_RIGHT},
            {"arrow_left", ARROW_LEFT},
            {"arrow_up", ARROW_UP},
            {"arrow_down", ARROW_DOWN},
            {"plus", PLUS},
            {"minus", MINUS},
            {"check", CHECK},
            {"times", TIMES},
            {"trash", TRASH},
            {"save", SAVE},
            {"folder", FOLDER},
            {"folder_open", FOLDER_OPEN},
            {"file", FILE},
            {"file_alt", FILE_ALT},
            {"pencil", PENCIL},
            {"edit", EDIT},
            {"move", MOVE},
            {"rotate", ROTATE},
            {"scale", SCALE},
            {"undo", UNDO},
            {"redo", REDO},
            {"eye", EYE},
            {"eye_slash", EYE_SLASH},
            {"lock", LOCK},
            {"unlock", UNLOCK},
            {"cube", CUBE},
            {"cubes", CUBES},
            {"mountain", MOUNTAIN},
            {"map", MAP},
            {"compass", COMPASS},
            {"location", LOCATION},
            {"ruler", RULER},
            {"crosshairs", CROSSHAIRS},
            {"object_group", OBJECT_GROUP},
            {"object_ungroup", OBJECT_UNGROUP},
            {"cog", COG},
            {"cogs", COGS},
            {"bars", BARS},
            {"question", QUESTION},
            {"info", INFO},
            {"exclamation", EXCLAMATION},
            {"search", SEARCH},
            {"home", HOME},
            {"download", DOWNLOAD},
            {"upload", UPLOAD},
            {"sync", SYNC},
            {"play", PLAY},
            {"pause", PAUSE},
            {"stop", STOP}
        };

        auto it = iconMap.find(name);
        return it != iconMap.end() ? it->second : "";
    }
};

/// -------------------------------------------------------

struct Image
{
    //std::shared_ptr<ImageResource> resource;
    uint32_t width = 0;
    uint32_t height = 0;
    //ImageUsageFlags usage;
    //Format format;
    //Layout::ImageLayout layout;
    //AspectFlags aspect;
    uint32_t layers = 1;
    uint32_t RID();
    ImTextureID ImGuiRID();
    ImTextureID ImGuiRID(uint32_t layer);
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
namespace SceneryEditorX::UI
{
    /**
     * @brief Main GUI class responsible for ImGui integration with Vulkan
     * 
     * This class manages the lifecycle of ImGui resources and provides
     * utilities for common UI operations in the editor.
     */
	class GUI
	{
	public:
	    GUI();
	    ~GUI();

		/**
         * @brief Initialize ImGui with the provided Vulkan renderer and window
         * @param window GLFW window handle
         * @param renderer Graphics engine reference
         * @return True if initialization was successful
         */
        bool InitGUI(GLFWwindow *window, Renderer &renderer);
	
        /**
         * @brief Set the command buffer for rendering ImGui
         * @param cmdBuffer Vulkan command buffer to render into
         */
        void SetActiveCommandBuffer(const VkCommandBuffer cmdBuffer) { activeCommandBuffer = cmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetActiveCommandBuffer() const { return activeCommandBuffer; }
	
	    /**
	     * @brief Handles resizing of the window
	     * @param width New width of the window
	     * @param height New height of the window
	     */
	    void Resize(uint32_t width, uint32_t height);
	
        /**
         * @brief Begin a new ImGui frame
         * 
         * This should be called at the start of each frame before any ImGui drawing
         */
        void BeginFrame() const;

        /**
         * @brief End the ImGui frame and render it to the active command buffer
         * 
         * This should be called after all ImGui drawing is complete
         */
        void EndFrame() const;
	
        /**
         * @brief Clean up ImGui resources
         * 
         * This should be called during application shutdown
         */
        void CleanUp();

        /**
         * @brief Update the GUI state
         * @param deltaTime Time elapsed since last update
         */
        void Update(float deltaTime) const;
	
        /**
         * @brief Show the ImGui demo window
         * @param open Pointer to boolean controlling window visibility
         */
        void ShowDemoWindow(bool *open = nullptr) const;

        /**
         * @brief Shows application info in an ImGui window
         * @param appName Application name
         */
        void ShowAppInfo(const std::string &appName) const;

        /**
         * @brief Initialize a viewport window for rendering
         * @param size Initial viewport size
         * @param imageView Vulkan image view to render into
         * @return True if initialization was successful
         */
        bool InitViewport(const Viewport &size, VkImageView imageView);

        /**
         * @brief Display the viewport window with the scene rendering
         * @param size Reference that will be updated with new viewport size
         * @param hovered Will be set to true if mouse is hovering the viewport
         * @param imageView Vulkan image view containing the rendered scene
         */
        void ViewportWindow(Viewport &size, bool &hovered, VkImageView imageView);

        /**
         * @brief Set ImGui style (colors, sizes, etc.)
         */
        static void SetStyle();

        /**
         * @brief Configure and load fonts for ImGui
         */
        void SetFonts() const;

        /**
         * @brief Get a descriptor set for an image to use in ImGui
         * @param imageView Vulkan image view
         * @param sampler Vulkan sampler
         * @param layout Image layout
         * @return ImTextureID that can be used with ImGui::Image functions
         */
        ImTextureID GetTextureID(VkImageView imageView, VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;

        static bool visible;                 /// Used to show/hide the GUI
        static const std::string defaultFont; /// Default font name

	private:
        /// Vulkan resources
        //Window *window = nullptr;
        GLFWwindow *window = nullptr;
        SwapChain *swapchain = nullptr;
        VulkanDevice *device = nullptr;
        //VkDevice device = VK_NULL_HANDLE;
        Renderer *renderer = nullptr;
        VkDescriptorPool imguiPool = VK_NULL_HANDLE;
        VkCommandBuffer activeCommandBuffer = VK_NULL_HANDLE;

	    /// State tracking
	    bool initialized = false;
		float contentScaleFactor = 1.0f; ///  Scale factor to apply due to a difference between the window and GL pixel sizes
        float dpiFactor = 1.0f;          /// Scale factor to apply to the size of gui elements (expressed in dp)
        bool viewportInitialized = false;

		/// ImGui window flags
        const ImGuiWindowFlags commonFlags = ImGuiWindowFlags_NoCollapse;
        const ImGuiWindowFlags optionsFlags = ImGuiWindowFlags_NoResize;
        const ImGuiWindowFlags infoFlags = ImGuiWindowFlags_NoMove;

        /// Helper methods
        bool CreateDescriptorPool();
        void UpdateDpiScale();

	};

    /**
     * @brief Initialize custom ImGui extensions
     */
	void initImGuiExtensions();

}

/// -------------------------------------------------------
