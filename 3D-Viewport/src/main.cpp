#include <filesystem>
namespace fs = std::filesystem;

#define GLM_ENABLE_EXPERIMENTAL

#include "model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

//#include "ImGuizmo/ImGuizmo.h"

/*
std::string GetCurrentProjectName() {
    // This should be replaced with actual logic to get the project name.
    return "New Project"; // Example for demonstration
}
*/
const unsigned int width = 800;
const unsigned int height = 800;

/*
// Get the current project's name
std::string projectName = GetCurrentProjectName();
// Construct the window title
std::string windowTitle = "Scenery Editor X | " + projectName;
// Use `windowTitle` for setting the window title in your 3D editor
*/


int main()
{
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(width, height, "Scenery Editor X | ", NULL, NULL);

	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	//Load GLAD so it configures OpenGL
	gladLoadGL();

	// Specify the viewport of OpenGL in the Window
	// viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, width, height);

	// Generates Shader object using shaders defualt.vert and default.frag
	Shader shaderProgram("shaders/default.vert", "shaders/default.frag");

	// Take care of all the light related things
	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, lightPos);

	shaderProgram.Activate();
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);


	// Enables the Depth Buffer
	glEnable(GL_DEPTH_TEST);


	//Scene Camera creation
	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));

	//ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);
	/*
	* I'm doing this relative path thing in order to centralize all the resources into one folder and not
	* duplicate them between tutorial folders. You can just copy paste the resources from the 'Resources'
	* folder and then give a relative path from this folder to whatever resource you want to get to.
	* Also note that this requires C++17, so go to Project Properties, C/C++, Language, and select C++17
	*/
		std::string parentDir = (fs::current_path().fs::path::parent_path()).string();
		std::string modelPath = "/assets/models/bunny/scene.gltf";

	// Load in a model
	Model model((parentDir + modelPath).c_str());

	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	/*
	##########################################################
			MAIN WHILE LOOP RUNNING SHADER ENGINE
	##########################################################
	*/
	/*
	while (!glfwWindowShouldClose(window))
	{
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Tell OpenGL a new frame is about to begin
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGUI window creation
		ImGui::Begin("My name is window, ImGUI window");
		// Text that appears in the window
		ImGui::Text("Hello there adventurer!");
		// Ends the window
		ImGui::End();

		// Handles camera inputs
		camera.Inputs(window);
		// Updates and exports the camera matrix to the Vertex Shader
		camera.updateMatrix(45.0f, 0.1f, 100.0f);

		// Draws different meshes
		floor.Draw(shaderProgram, camera);
		light.Draw(lightShader, camera);

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();


	}	*/
	/*
	void DrawGrid(const float* view, const float* projection, const float* matrix, const float gridSize)
	{
		matrix_t viewProjection = *(matrix_t*)view * *(matrix_t*)projection;
		vec_t frustum[6];
		ComputeFrustumPlanes(frustum, viewProjection.m16);
		matrix_t res = *(matrix_t*)matrix * viewProjection;

		for (float f = -gridSize; f <= gridSize; f += 1.f)
		{
			for (int dir = 0; dir < 2; dir++)
			{
				vec_t ptA = makeVect(dir ? -gridSize : f, 0.f, dir ? f : -gridSize);
				vec_t ptB = makeVect(dir ? gridSize : f, 0.f, dir ? f : gridSize);
				bool visible = true;
				for (int i = 0; i < 6; i++)
				{
					float dA = DistanceToPlane(ptA, frustum[i]);
					float dB = DistanceToPlane(ptB, frustum[i]);
					if (dA < 0.f && dB < 0.f)
					{
						visible = false;
						break;
					}
					if (dA > 0.f && dB > 0.f)
					{
						continue;
					}
					if (dA < 0.f)
					{
						float len = fabsf(dA - dB);
						float t = fabsf(dA) / len;
						ptA.Lerp(ptB, t);
					}
					if (dB < 0.f)
					{
						float len = fabsf(dB - dA);
						float t = fabsf(dB) / len;
						ptB.Lerp(ptA, t);
					}
				}
				if (visible)
				{
					ImU32 col = 0xFF808080;
					col = (fmodf(fabsf(f), 10.f) < FLT_EPSILON) ? 0xFF909090 : col;
					col = (fabsf(f) < FLT_EPSILON) ? 0xFF404040 : col;

					float thickness = 1.f;
					thickness = (fmodf(fabsf(f), 10.f) < FLT_EPSILON) ? 1.5f : thickness;
					thickness = (fabsf(f) < FLT_EPSILON) ? 2.3f : thickness;

					gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
				}
			}
		}
	}
	*/
	/*
	##########################################################
	##########################################################
	*/

	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		// Start the ImGui frame
    	ImGui_ImplOpenGL3_NewFrame();
    	ImGui_ImplGlfw_NewFrame();
    	ImGui::NewFrame();

    	// Your code to create ImGui widgets goes here

    	// Rendering
    	ImGui::Render();
    	int display_w, display_h;

		
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Handles camera inputs
		camera.Inputs(window);
		// Updates and exports the camera matrix to the Vertex Shader
		camera.updateMatrix(45.0f, 0.1f, 100.0f);

		// Draw a model
		model.Draw(shaderProgram, camera);

		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}

	// Deletes all ImGUI instances
	//ImGui_ImplOpenGL3_Shutdown();
	//ImGui_ImplGlfw_Shutdown();
	//ImGui::DestroyContext();


	// Delete all the objects we've created
	shaderProgram.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}
