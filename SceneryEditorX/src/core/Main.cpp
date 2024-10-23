#include "../xpeditorpch.h"
//#include "iniparser.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> 

#include "../ui/UI.h"

using namespace SceneryEditorX::UI;

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

/*
##########################################################
			IMGUI UI PANEL RENDERING FUNCTIONS
##########################################################
*/

void RenderMainMenu() 
{
	MainMenuBar();
    
}

void RenderPanels()
{
    LayerStack();
    AssetBrowser();
    SettingsPanel();
}

void RenderModals()
{
    AboutModal();
}

/*
##########################################################
			MAIN PROGRAM ENTRY POINT
##########################################################
*/

int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return -1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 330";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Scenery Editor X", nullptr, nullptr);
	if (window == nullptr)
        return -1;

    spdlog::info("Application Rendered");

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

#define IMGUI_ENABLE_FREETYPE

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

    /*
    ##########################################################
    			    IMGUI FONT LOADERS
    ##########################################################
    */

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//ImFont* font1 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Black.otf", 15.0f);
	//ImFont* font2 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BlackItalic.otf", 15.0f);
	//ImFont* font3 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Bold.otf", 15.0f);
	//ImFont* font4 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BoldItalic.otf", 15.0f);
	//ImFont* font5 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Book.otf", 15.0f);
	//ImFont* font6 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BookItalic.otf", 15.0f);
	ImFont* font7 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Demi.otf", 15.0f);
	//ImFont* font8 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-DemiItalic.otf", 15.0f);
	//ImFont* font9 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Light.otf", 15.0f);
	//ImFont* font10 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-LightItalic.otf", 15.0f);
	//ImFont* font11 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Medium.otf", 15.0f);
	//ImFont* font12 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-MediumItalic.otf", 15.0f);
	//ImFont* font13 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Thin.otf", 15.0f);
	//ImFont* font14 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-ThinItalic.otf", 15.0f);
	//ImFont* font15 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Ultra.otf", 15.0f);
	//ImFont* font16 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-UltraItalic.otf", 15.0f);

    /*
    ##########################################################
    			    IMGUI CONFIG FLAGS
    ##########################################################
    */

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	io.ConfigFlags |= ImGuiWindowFlags_NoCollapse;
	io.ConfigFlags |= ImGuiWindowFlags_AlwaysAutoResize;
	io.ConfigViewportsNoAutoMerge = true;
	io.ConfigViewportsNoTaskBarIcon = true;

	// ImGui Style
    SetDarkThemeColors();
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
	ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
	ImGui_ImplOpenGL3_Init(glsl_version);

    /*
    ##########################################################
    			    IMGUI PANEL STATES
    ##########################################################
    */

	bool show_demo_window = true;
	bool show_another_window = false;

	ImVec4 clear_color = ImVec4(0.037f, 0.039f, 0.039f, 1.000f); // Background Window Color
	static float padding = 16.0f;

	/*
    ##########################################################
    			    IMGUI MAIN WHILE LOOP
    ##########################################################
    */

#ifdef __EMSCRIPTEN__
	// For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
	// You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
	io.IniFilename = nullptr;
	EMSCRIPTEN_MAINLOOP_BEGIN
#else
	while (!glfwWindowShouldClose(window))
#endif
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.


		glfwPollEvents();
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
		{
			ImGui_ImplGlfw_Sleep(10);
			continue;
		}

		// Start the ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
        /*
        ##########################################################
        			    RENDER CALLS FROM UI.H
        ##########################################################
        */

		RenderMainMenu();
        ImGui::DockSpaceOverViewport(ImGui::GetID("CoreDockSpace"));


		RenderModals();
        SettingsPanel();
        LayerStack();
        AssetBrowser();
        
        // Call the exit confirmation modal
        ExitConfirmationModal(window); // Pass the window pointer

		ImGui::Begin("3D Viewport");
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImGui::End();


		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

        /*
        ##########################################################
        			        IMGUI RENDERING
        ##########################################################
        */

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);
	}

#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

    /*
    ##########################################################
    			    IMGUI FRAME CLEANUP
    ##########################################################
    */

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
