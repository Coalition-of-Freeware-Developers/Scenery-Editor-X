/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_compiler.cpp
* -------------------------------------------------------
* Created: 8/4/2025
* -------------------------------------------------------
*/

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/shaders/shader_compiler.h>
#include <string>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Compile the shader using glslangValidator
	 * @param path The path to the shader file
	 * @return A vector of characters containing the compiled shader code
	 */
	static std::vector<char> CompileShader(const std::filesystem::path &path)
	{
	    char compile_string[1024];
	    char inpath[256];
	    char outpath[256];
	    std::string cwd = std::filesystem::current_path().string();
	    sprintf(inpath, "%s/source/Shaders/%s", cwd.c_str(), path.string().c_str());
	    sprintf(outpath, "%s/bin/%s.spv", cwd.c_str(), path.filename().string().c_str());
	    sprintf(compile_string, "%s -V %s -o %s --target-env spirv1.4", GLSL_VALIDATOR, inpath, outpath);
	    EDITOR_LOG_TRACE("[ShaderCompiler] Command: {}", compile_string);
	    EDITOR_LOG_TRACE("[ShaderCompiler] Output:");
	    while (system(compile_string))
	    {
	        EDITOR_LOG_WARN("[ShaderCompiler] Error! Press something to Compile Again");
	        std::cin.get();
	    }
	
	    // 'ate' specify to start reading at the end of the file
	    // then we can use the read position to determine the size of the file
	    std::ifstream file(outpath, std::ios::ate | std::ios::binary);
	    if (!file.is_open())
	    {
            EDITOR_LOG_ERROR("Failed to open file: '{}'", outpath);
	    }
	    size_t fileSize = (size_t)file.tellg();
	    std::vector<char> buffer(fileSize);
	    file.seekg(0);
	    file.read(buffer.data(), fileSize);
	    file.close();
	
	    return buffer;
	}

} // namespace SceneryEditorX

// ---------------------------------------------------------

