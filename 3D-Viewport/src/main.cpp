#include <filesystem>
namespace fs = std::filesystem;

//#define GLM_ENABLE_EXPERIMENTAL

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

// Get the current project's name
std::string projectName = GetCurrentProjectName();
// Construct the window title
std::string windowTitle = "Scenery Editor X | " + projectName;
// Use `windowTitle` for setting the window title in your 3D editor
*/

const unsigned int width = 800;
const unsigned int height = 800;




int main()
{
	// Initialize GLFW
	glfwInit();
    

	// Tell GLFW the version of OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a windowed mode window and its OpenGL context"
	GLFWwindow* window = glfwCreateWindow(width, height, "Scenery Editor X | ", NULL, NULL);
	if (window == NULL) // Error check if the window fails to create
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	//Load GLAD so it configures OpenGL
	gladLoadGL();

	// Viewport of OpenGL in the Window
	// x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, width, height);

	// Generates Shader object using shaders defualt.vert and default.frag
	Shader shaderProgram("shaders/default.vert", "shaders/default.frag");
	Shader grassProgram("shaders/default.vert", "shaders/grass.frag");

	// Take care of all the light related things
	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, lightPos);

	shaderProgram.Activate();
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	grassProgram.Activate();
	glUniform4f(glGetUniformLocation(grassProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(grassProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

	
    // Enables the Depth Buffer
    glEnable(GL_DEPTH_TEST);
    //Choses which depth function to use
    glDepthFunc(GL_LESS);
    // Enables Backface Culling
    glEnable(GL_CULL_FACE);
    // Specifys which face to keep in this case front faces
    glCullFace(GL_FRONT);
    // Uses counter clock-wise standard
    glFrontFace(GL_CCW);
	// Configures the blending function
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Scene Camera creation
	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));

	//ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);


	/*
	* This is all subject to change from reletive paths to call from the main scene 
	* project instead so it can dynamically load multiple assets and textures at a single time.
	* This is just for testing uses.
	*/
	std::string parentDir = (fs::current_path().fs::path::parent_path()).string();
	std::string modelPath	= "/assets/models/statue/scene.gltf";
	std::string groundPath	= "/assets/models/ground/scene.gltf";
	std::string grassPath	= "/assets/models/grass/scene.gltf";


	// Load in models
	Model ground((parentDir + groundPath).c_str());
	Model grass((parentDir + grassPath).c_str());

	/*
	##########################################################
				IMGUI INITIALIZATION
	##########################################################
	*/

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
		
	/*
	##########################################################
	##########################################################
	*/

    // Variables to create periodic event for FPS displaying
    double prevTime = 0.0;
    double crntTime = 0.0;
    double timeDiff;
    // Keeps track of the amount of frames in timeDiff
    unsigned int counter = 0;
	// Use this to disable VSync (not advized)
	//glfwSwapInterval(0);


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
		/*
		##########################################################
					IMGUI PASS INCLUSION
		##########################################################
		*/

		/*
		##########################################################
		##########################################################
		*/
		
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		
        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

    	// Set the position and size of the FPS counter to the top-left corner
    	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
    	ImGui::SetNextWindowSize(ImVec2(100.0f, 50.0f)); // Set a specific size

        // Draw the FPS in an ImGui panel
        {
            static float fps = 0.0f;
            static float lastTime = glfwGetTime();
            static int frames = 0;

            double currentTime = glfwGetTime();
            float dt = currentTime - lastTime;
            frames++;

        	if (dt >= 1.0f)
        	{
        	    fps = frames / dt;
        	    lastTime = currentTime;
        	    frames = 0;
        	}

        	ImGui::Begin("FPS Counter", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove);
        	ImGui::Text("FPS: %.2f", fps);
        	ImGui::End();
        }

		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		// Handles camera inputs
		camera.Inputs(window);
		// Updates and exports the camera matrix to the Vertex Shader
		camera.updateMatrix(45.0f, 0.1f, 100.0f);

		// Draw the normal model
		ground.Draw(shaderProgram, camera);

		// Disable cull face so that grass and windows have both faces
		glDisable(GL_CULL_FACE);
		grass.Draw(grassProgram, camera);
		// Enable blending for windows
		glEnable(GL_BLEND);

		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);

        // Render ImGui
        ImGui::Render();
        //int display_w, display_h;
        //glfwGetFramebufferSize(window, &display_w, &display_h);
        //glViewport(0, 0, display_w, display_h);
        
		// Ensure that OpenGL state is reset to default before rendering ImGui
    	glDisable(GL_DEPTH_TEST);
    	glDisable(GL_CULL_FACE);

		// Render ImGui draw data
    	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		
    	// Ensure that OpenGL state is reset to default after rendering ImGui
    	glEnable(GL_DEPTH_TEST);
    	glEnable(GL_CULL_FACE);

		// Take care of all GLFW events
		glfwPollEvents();

		// Resize window if necessary
        // Get the current framebuffer size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Check if the new size is different from the previous one
        static int display_w = 0, display_h = 0;
        if (width != display_w || height != display_h) {
            // Set the viewport to the new size
            glViewport(0, 0, width, height);
            
            // Update the variables with the new values
			display_w = width;
			display_h = height;
        }
	}

	// Deletes all ImGUI instances
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Delete all the objects we've created
	shaderProgram.Delete();
	grassProgram.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	
	return 0;
}
