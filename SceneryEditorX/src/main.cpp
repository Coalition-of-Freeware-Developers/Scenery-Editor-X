#include <iostream>
//#include "iniparser.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

/*
// User-defined structure that can be loaded to\from INI
struct test_val
{
	test_val() :a(0), b(0.0), c(0) {}
	test_val(int pa, double pb, int pc) :a(pa), b(pb), c(pc) {}
	int a;
	double b;
	int c;
	bool operator== (const test_val& rt) const
	{
		return (a == rt.a && b == rt.b && c == rt.c);
	}
	bool operator!= (const test_val& rt) const { return !(*this == rt); }
};

// You need to declare these 2 operators to use your classes with Leksys' INIParser
std::ostream& operator<< (std::ostream& stream, const test_val& value)
{
	stream << '{' << value.a << ',' << value.b << ',' << value.c << '}';
	return stream;
}
std::istream& operator>> (std::istream& stream, test_val& value)
{
	char sb;
	stream >> sb;
	if (sb != '{') return stream;
	stream >> value.a >> sb >> value.b >> sb >> value.c >> sb;
	if (sb != '}') { value.a = 0; value.b = 0.0; value.c = 0; }
	return stream;
}

void show_help()
{
	std::cout << "Leksys' INIParser test program\n"
		"Usage: test_app [-i input_file] [-o output_file]\n"
		"Pass [input_file] for optional loading file test (TEST11)\n"
		"Pass [output_file] for optional saving file test (TEST12)\n"
		"---------------------------------------------------------"
		<< std::endl;
}


int configMain(int argc, char** argv)
{
	INI::File ft, ft2;
	std::string input_file;
	std::string output_file;
	show_help();
	// Bad way to parse input parameters, but why bother
	for (int i = 1; i < argc; i += 2)
	{
		std::string param = argv[i];
		if (param.size() != 2 || param[0] != '-' || i == argc - 1)
		{
			std::cerr << "ERROR! Wrong parameter " << param << std::endl;
			return -1;
		}
		if (param[1] == 'i')
			input_file = argv[i + 1];
		else if (param[1] == 'o')
			output_file = argv[i + 1];
		else
		{
			std::cerr << "ERROR! Unknown parameter " << param << std::endl;
			return -1;
		}
	}

	// [TEST1] Get & Set INT value through section pointer
	ft.GetSection("MainProg")->SetValue("value1", 456);
	if (ft.GetSection("MainProg")->GetValue("value1").AsInt() != 456 ||
		ft.GetSection("MainProg")->Name() != "MainProg")
	{
		std::cerr << "Failed to pass [TEST1]" << std::endl;
		return 1;
	}
	std::cout << "[TEST1] passed" << std::endl;

	// [TEST2] Get & Set boolean value through File object
	ft.SetValue("MainProg:value2_0", false);
	ft.SetValue("MainProg:value2_1", true);
	ft.SetValue("MainProg:value2_2", 0);
	ft.SetValue("MainProg:value2_3", 1);
	ft.SetValue("MainProg:value2_4", "FALSE");
	ft.SetValue("MainProg:value2_5", "TRUE");
	ft.SetValue("MainProg:value2_6", "false");
	ft.SetValue("MainProg:value2_7", "true");
	if (ft.GetValue("MainProg:value2_0").AsBool() || !ft.GetValue("MainProg:value2_1").AsBool() ||
		ft.GetValue("MainProg:value2_2").AsBool() || !ft.GetValue("MainProg:value2_3").AsBool() ||
		ft.GetValue("MainProg:value2_4").AsBool() || !ft.GetValue("MainProg:value2_5").AsBool() ||
		ft.GetValue("MainProg:value2_6").AsBool() || !ft.GetValue("MainProg:value2_7").AsBool())
	{
		std::cerr << "Failed to pass [TEST2]" << std::endl;
		return 2;
	}
	std::cout << "[TEST2] passed" << std::endl;

	// [TEST3] Set array value
	ft.SetValue("MainProg:value3", INI::Array() << true << 100 << 200.55 << "String");
	INI::Array value3 = ft.GetSection("MainProg")->GetValue("value3").AsArray();
	if (!value3[0].AsBool() || value3[1].AsInt() != 100 ||
		value3[2].AsDouble() != 200.55 || value3[3].AsString() != "String")
	{
		std::cerr << "Failed to pass [TEST3]" << std::endl;
		return 3;
	}
	std::cout << "[TEST3] passed" << std::endl;

	// [TEST4] Direct access to array values
	ft.SetArrayValue("MainProg:value3", 1, 105);
	ft.GetSection("MainProg")->SetArrayValue("value3", 6, 700.67);
	if (ft.GetValue("MainProg:value3").AsArray()[1].AsInt() != 105 ||
		ft.GetSection("MainProg")->GetValue("value3").AsArray().GetValue(6).AsDouble() != 700.67)
	{
		std::cerr << "Failed to pass [TEST4]" << std::endl;
		return 4;
	}
	std::cout << "[TEST4] passed" << std::endl;

	// [TEST5] Comments
	ft.GetSection("MainProg")->SetComment("value3", "Test array");
	ft.SetValue("MainProg2:value1", 1, "Comment 2");
	if (ft.GetSection("MainProg")->GetComment("value3") != "Test array" ||
		ft.GetSection("MainProg2")->GetComment("value1") != "Comment 2")
	{
		std::cerr << "Failed to pass [TEST5]" << std::endl;
		return 5;
	}
	std::cout << "[TEST5] passed" << std::endl;

	// [TEST6] Section iteration using iterator
	// Add key "value_test" with value "1" to all of the sections, existing so far (MainProg & MainProg2)
	for (INI::File::sections_iter it = ft.SectionsBegin(); it != ft.SectionsEnd(); it++)
		it->second->SetValue("value_test", 1, "Testing value");
	// Read those keys
	for (INI::File::sections_iter it = ft.SectionsBegin(); it != ft.SectionsEnd(); it++)
	{
		if (it->second->GetValue("value_test", 0).AsInt() != 1)
		{
			std::cerr << "Failed to pass [TEST6]" << std::endl;
			return 6;
		}
	}
	std::cout << "[TEST6] passed" << std::endl;

	// [TEST7] Parent & Child Sections
	ft.GetSection("MainProg2")->GetSubSection("Sub1")->GetSubSection("SubSub1")->SetValue("value1", 120);
	if (ft.GetValue("MainProg2.Sub1.SubSub1:value1").AsInt() != 120 ||
		ft.GetSection("MainProg2.Sub1")->GetSubSection("SubSub1")->GetValue("value1").AsInt() != 120)
	{
		std::cerr << "Failed to pass [TEST7]" << std::endl;
		return 7;
	}
	std::cout << "[TEST7] passed" << std::endl;

	// [TEST8] User-defined class storage
	test_val tp(1, 120.555, 3);
	ft.GetSection("MainProg2.Sub1")->SetValue("test_val", tp, "User-defined class");
	if (tp != ft.GetValue("MainProg2.Sub1:test_val").AsT<test_val>())
	{
		std::cerr << "Failed to pass [TEST8]" << std::endl;
		return 8;
	}
	std::cout << "[TEST8] passed" << std::endl;

	// [TEST9] Saving and loading files + values and section iteration (main test)
	std::stringstream stream;
	stream << ft;
	stream >> ft2;
	if (ft2.SectionsSize() != ft.SectionsSize())
	{
		std::cerr << "Failed to pass [TEST9]" << std::endl;
		return 9;
	}
	for (INI::File::sections_iter it = ft2.SectionsBegin(); it != ft2.SectionsEnd(); it++)
	{
		INI::Section* sect1 = it->second;
		INI::Section* sect2 = ft.GetSection(sect1->FullName());
		if (sect1->ValuesSize() != sect2->ValuesSize() || sect1->Comment() != sect2->Comment())
		{
			std::cerr << "Failed to pass [TEST9]" << std::endl;
			return 9;
		}
		for (INI::Section::values_iter vit = sect1->ValuesBegin(); vit != sect1->ValuesEnd(); vit++)
		{
			if (vit->second != sect2->GetValue(vit->first) || sect1->GetComment(vit->first) != sect2->GetComment(vit->first))
			{
				std::cerr << "Failed to pass [TEST9]" << std::endl;
				return 9;
			}
		}
	}
	std::cout << "[TEST9] passed" << std::endl;

	// [TEST10] Saving and loading sections + Unload feature
	stream << ft.GetSection("MainProg2.Sub1");
	ft2.Unload();
	stream >> ft2;
	// Only 1 section was saved (parent is not created automatically as well as child sections)
	if (ft2.SectionsSize() != 1)
	{
		std::cerr << "Failed to pass [TEST10]" << std::endl;
		return 10;
	}
	INI::Section* sect = ft2.GetSection("MainProg2")->GetSubSection("Sub1");
	if (sect->ValuesSize() != 1 || sect->GetValue("test_val").AsT<test_val>() != tp)
	{
		std::cerr << "Failed to pass [TEST10]" << std::endl;
		return 10;
	}
	std::cout << "[TEST10] passed" << std::endl;

	// [TEST11] Complex arrays test
	ft.SetValue("MainProg2:cmp_array1", "{12,3}, {13,5}, {18,9} ");
	ft.SetValue("MainProg2:cmp_array2", "Str1, Str2, {{Str3,,3}}, Str4\\,\\,\\{\\,\\,\\}, {Str5\\,,\\{\\{}");
	{
		INI::Value val = ft.GetSection("MainProg2")->GetValue("cmp_array1");
		if (val.AsArray()[1].AsArray()[0].AsInt() != 13 || val.AsArray()[2].AsArray()[1].AsInt() != 9)
		{
			std::cerr << "Failed to pass [TEST11] (check 1)" << std::endl;
			return 11;
		}
		val = ft.GetSection("MainProg2")->GetValue("cmp_array2");
		val = val.AsArray().ToValue();
		if (val.AsArray()[1].AsString() != "Str2" || val.AsArray()[2].AsString() != "{Str3,,3}"
			|| val.AsArray()[3].AsString() != "Str4,,{,,}" || val.AsArray()[4].AsString() != "Str5,,{{")
		{
			std::cerr << "Failed to pass [TEST11] (check 2)" << std::endl;
			return 11;
		}
	}
	std::cout << "[TEST11] passed" << std::endl;

	// [TEST12] Map test
	ft.SetValue("MainProg2:map1", "1:5, 1:3, 2:9, 3:10");
	ft.SetValue("MainProg2:map2", "Str1:5, Str2:3, Str3:{:,,:}");
	ft.SetValue("MainProg2:map3", "Str1:{Sub1,{\\,,\\\\{\\\\{Sub2,},{,:Sub3}}, Str2:1");
	{
		INI::Value val = ft.GetSection("MainProg2")->GetValue("map1");
		val = val.AsMap().ToValue();
		if (val.AsMap()[2].AsInt() != 9 || val.AsMap()[1].AsInt() != 3)
		{
			std::cerr << "Failed to pass [TEST12] (check 1)" << std::endl;
			return 12;
		}
		val = ft.GetSection("MainProg2")->GetValue("map2");
		INI::Map mp = val.AsMap();
		if (val.AsMap()["Str2"].AsInt() != 3 || val.AsMap()["Str3"].AsString() != ":,,:")
		{
			std::cerr << "Failed to pass [TEST12] (check 2)" << std::endl;
			return 12;
		}
		val = ft.GetSection("MainProg2")->GetValue("map3");
		mp = val.AsMap().ToValue().AsMap();
		std::string str = val.AsMap()["Str1"].AsArray()[1].AsString();
		if (val.AsMap()["Str1"].AsArray()[1].AsString() != ",,{{Sub2," ||
			val.AsMap()["Str1"].AsArray()[2].AsString() != ",:Sub3" ||
			val.AsMap()["Str2"].AsInt() != 1)
		{
			std::cerr << "Failed to pass [TEST12] (check 3)" << std::endl;
			return 12;
		}
	}
	std::cout << "[TEST12] passed" << std::endl;

	// [TEST13] Optional. File loading
	if (!input_file.empty())
	{
		if (!ft.Load(input_file))
		{
			std::cerr << "Failed to pass optional [TEST13] (file loading). Error: "
				<< ft.LastResult().GetErrorDesc() << std::endl;
			return 13;
		}
		std::cout << "[TEST13] passed!" << std::endl;
	}
	else
		std::cout << "Optional [TEST13] (file loading) SKIPPED" << std::endl;

	// [TEST14] Optional. File saving
	if (!output_file.empty())
	{
		if (!ft.Save(output_file))
		{
			std::cerr << "Failed to pass optional [TEST14] (file saving). Wrong output file. File contents comes next"
				<< std::endl << ft;
			return 14;
		}
		std::cout << "[TEST14] passed! (Look in " << output_file << " for your file)!" << std::endl;
	}
	else
		std::cout << "Optional [TEST14] (file saving) SKIPPED" << std::endl;

	// [SUCCESS]
	std::cout << "!!! [SUCCESS] !!! " << std::endl;
	return 0;
}
*/

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

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
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Scenery Editor X", nullptr, nullptr);
	if (window == nullptr)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

#define IMGUI_ENABLE_FREETYPE

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImFont* font11 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Black.otf", 15.0f);
	ImFont* font2 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BlackItalic.otf", 15.0f);
	ImFont* font3 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Bold.otf", 15.0f);
	ImFont* font4 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BoldItalic.otf", 15.0f);
	ImFont* font5 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Book.otf", 15.0f);
	ImFont* font6 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BookItalic.otf", 15.0f);
	ImFont* font7 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Demi.otf", 15.0f);
	ImFont* font8 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-DemiItalic.otf", 15.0f);
	ImFont* font9 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Light.otf", 15.0f);
	ImFont* font10 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-LightItalic.otf", 15.0f);
	ImFont* font1 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Medium.otf", 15.0f);
	ImFont* font12 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-MediumItalic.otf", 15.0f);
	ImFont* font13 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Thin.otf", 15.0f);
	ImFont* font14 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-ThinItalic.otf", 15.0f);
	ImFont* font15 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Ultra.otf", 15.0f);
	ImFont* font16 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-UltraItalic.otf", 15.0f);


	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	io.ConfigFlags |= ImGuiWindowFlags_NoCollapse;
	io.ConfigFlags |= ImGuiWindowFlags_AlwaysAutoResize;
	io.ConfigViewportsNoAutoMerge = true;
	io.ConfigViewportsNoTaskBarIcon = true;

	// Setup ImGui style
	ImGui::StyleColorsDark();
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

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	// - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;

	ImVec4 clear_color = ImVec4(0.037f, 0.039f, 0.039f, 1.000f); // Background Window Color
	static float padding = 16.0f;
	bool showModalPopup = false;

	// Main loop
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

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("3D Viewport");
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImGui::End();
		// Create and show main menu bar
	   	if (ImGui::BeginMainMenuBar())
	   	{
	   	    if (ImGui::BeginMenu("File"))
	   	    {
				if (ImGui::MenuItem("New", "Ctrl+N")) { /* Do something here */ }
				ImGui::Separator();
	   	        if (ImGui::MenuItem("Open", "Ctrl+O"))
				{

				}
	   	        if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
	   	        ImGui::Separator();
	   	        if (ImGui::MenuItem("Exit", "Alt+F4")) 
				{ 
					showModalPopup = true; 
				}
	   	        ImGui::EndMenu();
	   	    }
	
	   	    if (ImGui::BeginMenu("Edit"))
	   	    {
	   	        if (ImGui::MenuItem("Undo", "Ctrl+Z")) { /* Do something here */ }
	   	        if (ImGui::MenuItem("Redo", "Ctrl+Y")) { /* Do something here */ }
	   	        ImGui::Separator();
	   	        if (ImGui::MenuItem("Cut", "Ctrl+X")) { /* Do something here */ }
	   	        if (ImGui::MenuItem("Copy", "Ctrl+C")) { /* Do something here */ }
	   	        if (ImGui::MenuItem("Paste", "Ctrl+V")) { /* Do something here */ }
	   	        ImGui::EndMenu();
	   	    }
	
	   	    if (ImGui::BeginMenu("View"))
	   	    {
	   	        static bool showHelp = true;
	   	        ImGui::MenuItem("Show Help", nullptr, &showHelp);
	   	        ImGui::Separator();
	
	   	        // Add more menu items as needed
	   	        ImGui::EndMenu();
	   	    }

			if (ImGui::BeginMenu("Options"))
			{
				if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Do something here */ }
				if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
				ImGui::Separator();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Settings"))
			{
				if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Do something here */ }
				if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
				ImGui::Separator();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("About"))
			{
				if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Do something here */ }
				if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
				ImGui::Separator();
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
	   	}

		// Modal popup
    	if (showModalPopup)
		{
			ImGui::OpenPopup("ExitConfirmation");

			ImVec2 modalSize(460, 210);
			ImGui::SetNextWindowSize(modalSize);

			if (ImGui::BeginPopupModal("ExitConfirmation", &showModalPopup, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
			{
				ImGui::Text("Are you sure you want to exit?");
				ImGui::Checkbox("Don't show this message again", &showModalPopup);
				ImGui::Separator();

        	    // Buttons
        	    if (ImGui::Button("Yes", ImVec2(100.0f, 0.0f)))
        	    {
        	        glfwSetWindowShouldClose(window, true);
        	        showModalPopup = false; // Close the modal
        	    }
        	    ImGui::SameLine();
        	    if (ImGui::Button("No", ImVec2(100.0f, 0.0f)))
        	    {
        	        showModalPopup = false; // Close the modal without exiting
        	    }

        	    ImGui::EndPopup();
        	}
		}

		ImGui::Begin("Layer Stack");

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

		// Rendering
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

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
