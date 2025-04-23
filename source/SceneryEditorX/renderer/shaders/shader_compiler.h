/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_compiler.h
* -------------------------------------------------------
* Created: 8/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class ShaderCompiler
    {
    public:
        /// -------------------------------------------------------
        /// Path to the glslangValidator executable
        /// -------------------------------------------------------

        //static const std::string_view GLSL_VALIDATOR;


        /// -------------------------------------------------------

        /**
         * @brief Compile the shader using glslangValidator
         * @param path The path to the shader file
         * @return A vector of characters containing the compiled shader code
         */

        static std::vector<char> CompileShader(const std::filesystem::path &path);


    };

} // namespace SceneryEditorX

// -------------------------------------------------------
