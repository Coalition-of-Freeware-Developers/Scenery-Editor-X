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

// Get the current project's name
std::string projectName = GetCurrentProjectName();
// Construct the window title
std::string windowTitle = "Scenery Editor X | " + projectName;
// Use `windowTitle` for setting the window title in your 3D editor
*/

const unsigned int width = 1400;
const unsigned int height = 810;

struct Color {
	float r, g, b;
};

float gridThickness = 1.0f;
Color gridColor = { 0.5f, 0.5f, 0.5f };

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


	bool showGrid = true; 

	// Viewport of OpenGL in the Window
	// x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, width, height);

	// Vertex data for the axes indicator
    // float axisVertices[] = {
    //      0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    //      1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

    //      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    //      0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

    //      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    //      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
    // };

	// Generates Shader object
	Shader shaderProgram("shaders/default.vert", "shaders/default.frag");
	Shader gridShader("shaders/grid.vert", "shaders/grid.frag");
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

	// Create a VAO for the grid
	VAO gridVAO;

	// Create a VBO for the grid vertices
	VBO gridVBO(gVP);

	// Bind the VAO and set up the VBO linking
	gridVAO.Bind();
	gridVBO.Bind();

	// Link the VBO with the VAO (using layout location 0, for position)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind the VBO and VAO after linking
	gridVBO.Unbind();
	gridVAO.Unbind();

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
	##########################################################
	*/

	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);

		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		/*
		##########################################################
					IMGUI PASS INCLUSION
		##########################################################
		*/

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		// Render ImGui Grid Settings
		ImGui::Begin("Grid Settings");
		ImGui::Checkbox("Show Grid", &showGrid);
		ImGui::SliderFloat("Grid Thickness", &gridThickness, 0.1f, 5.0f, "%.1f");
		ImGui::ColorEdit3("Grid Color", (float*)&gridColor);
		ImGui::End();

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

			// Begin the ImGui window
        	ImGui::Begin("FPS Counter", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove);
			// Display the FPS value
        	ImGui::Text("FPS: %.2f", fps);
			// End the ImGui window
        	ImGui::End();
        }


		/*
		##########################################################
					Viewport Camera 
		##########################################################
		*/

		// Handles camera inputs

		if (!ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard) {
			camera.Inputs(window);
		}
		// Updates and exports the camera matrix to the Vertex Shader
		camera.updateMatrix(45.0f, 0.1f, 100.0f);

		
		/*
		##########################################################
					Viewport Orientation Axis
		##########################################################
		*/

		//RenderViewportAxes();
		//drawAxes();

		/*
		##########################################################
					Viewport Grid
		##########################################################
		*/
		// Draw ground plane grid
		if (showGrid){
    		// Activate grid shader and draw the grid
    		gridShader.Activate();
    		gridVAO.Bind();
			camera.Matrix(gridShader, "gVP");

			// Pass camera position in the xz-plane
			glm::vec3 camPos = camera.Position;
			glUniform2f(glGetUniformLocation(gridShader.ID, "camPos"), camPos.x, camPos.z);

			// Set grid color
			glUniform3f(glGetUniformLocation(gridShader.ID, "gridColor"), gridColor.r, gridColor.g, gridColor.b);

			// Set grid color
			//glLineWidth(gridThickness);

			// Draw the grid
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


		}

		/*
		##########################################################
		##########################################################
		*/

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
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
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
        //static int display_w = 0, display_h = 0;
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
	gridShader.Delete();
	grassProgram.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	
	return 0;
}
