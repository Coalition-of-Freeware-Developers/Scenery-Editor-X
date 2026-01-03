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
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include "shader.h"
#include "SceneryEditorX/renderer/descriptors.h"
#include <SceneryEditorX/renderer/input_layouts.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    enum class ShaderCompileState : uint8_t
	{
	    Idle,
	    Compiling,
	    Succeeded,
	    Failed
	};

    class ShaderCompiler : public Resource
    {
    public:
        ShaderCompiler();
        virtual ~ShaderCompiler() override;

        ShaderCompiler(ShaderCompiler &&) = delete;
        ShaderCompiler &operator=(ShaderCompiler &&) = delete;

        void CompileShader(const ShaderDataType type, const std::string &filePath, bool async, const VertexFormat vertexType = VertexFormat::MaxEnum);
        ShaderCompileState GetCompilationState() const { return m_CompilationState; }
        bool IsCompiled() const                        { return m_CompilationState == ShaderCompileState::Succeeded; }

		// Source
        void LoadFromDrive(const std::string& filePath);
        void SetSource(const uint32_t index, const std::string &source);
        const std::vector<std::string>& GetNames()     const { return m_Names; }
        const std::vector<std::string>& GetFilePaths() const { return m_FilePaths; }
        const std::vector<std::string>& GetSources()   const { return m_Sources; }

        // Defines
        void AddDefine(const std::string& define, const std::string& value = "1") { m_Defines[define] = value; }
        auto& GetDefines() const { return m_Defines; }

        // Misc
        uint32_t GetVertexSize() const;
        const std::vector<Descriptors>& GetDescriptors()      	const { return m_Descriptors; }
        const Ref<InputLayout>& GetInputLayout() 	 			const { return m_InputLayout; } // only valid for a vertex shader
        const auto& GetFilePath()                               const { return m_FilePath; }
        ShaderType GetShaderStage()                         	const { return m_ShaderType; }
        uint64_t GetHash()                                      const { return m_Hash; }
        const char* GetEntryPoint()                             const;
        const char* GetTargetProfile()                          const;
        void* GetResource()                                  	const { return m_Resource; }

        /**
         * @brief Compile the shader using glslangValidator
         * @param path The path to the shader file
         * @return A vector of characters containing the compiled shader code
         */
        static std::vector<char> CompileShader(const std::filesystem::path &path);
        void Compile(ShaderType shaderType, const std::string &filePath, bool async, VertexFormat vertexType);

    private:
        //void Reflect(const ShaderType shader_type, const uint32_t *ptr, uint32_t size);
        std::string m_FilePath;
        std::string m_PreprocessedSource;
        std::vector<std::string> m_Names;      			// The names of the files from the include directives in the shader
        std::vector<std::string> m_FilePaths; 			// The file paths of the files from the include directives in the shader
        std::vector<std::string> m_Sources;    			// The source of the files from the include directives in the shader
        std::vector<std::string> m_FilePathsMultiple; 	// The file paths of include directives which are defined multiple times in the shader
        std::unordered_map<std::string, std::string> m_Defines;
        std::vector<Descriptors> m_Descriptors;
        Ref<InputLayout> m_InputLayout;
        std::atomic<ShaderCompileState> m_CompilationState = ShaderCompileState::Idle;
        ShaderType m_ShaderType = ShaderType::MaxEnum;
        VertexFormat m_VertexType = VertexFormat::MaxEnum;
        uint64_t m_Hash = 0;

        void *m_Resource = nullptr;
    };


}

// -------------------------------------------------------
