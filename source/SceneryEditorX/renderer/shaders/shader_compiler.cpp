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
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/shaders/shader_compiler.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/input_layouts.h>
#include <SceneryEditorX/platform/filesystem/file_manager.hpp>
#include <algorithm>
#include "shader_compiler.h"
#include "SceneryEditorX/core/threading/thread_pool.h"
#include "SceneryEditorX/core/time/timer.h"

// ---------------------------------------------------------

namespace
{
	std::string to_lower(const std::string &input)
	{
	    std::string result = input;
        std::ranges::transform(result, result.begin(), [](unsigned char c) { return tolower(c); });
	    return result;
	}
}

// ---------------------------------------------------------

namespace SceneryEditorX
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // TODO: Replace and refactor the FileSystem string usage and swap with the StringUtils functions ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    const char *ToString(ShaderType e)
	{
	    switch (e)
	    {
	    case ShaderType::Vertex:				return "Vertex";
	    case ShaderType::TesselationControl:	return "TesselationControl";
	    case ShaderType::TesselationEval:		return "TesselationEval";
	    case ShaderType::Geometry:				return "Geometry";
	    case ShaderType::Fragment:				return "Fragment";
	    case ShaderType::Compute:				return "Compute";
	    case ShaderType::AllGraphics:			return "AllGraphics";
	    case ShaderType::Raygen:				return "Raygen";
	    case ShaderType::MaxEnum:				return "MaxEnum";
	    default: return "Unknown";
	    }
	}

    ShaderCompiler::ShaderCompiler() : Resource() { }

    ShaderCompiler::~ShaderCompiler()
	{
	    if (m_Resource)
	    {
			auto device = RenderContext::GetCurrentDevice();
            device->DeletionQueueAdd(ResourceType::Shader, m_Resource);
            m_Resource = nullptr;
	    }
	}

    /**
	 * @brief Compile the shader using glslangValidator
	 * @param path The path to the shader file
	 * @return A vector of characters containing the compiled shader code
	 */
	std::vector<char> ShaderCompiler::CompileShader(const std::filesystem::path &path)
	{
	    char compileString[1024];
	    char inpath[256];
	    char outpath[256];
	    const std::string cwd = std::filesystem::current_path().string();
	    sprintf(inpath, "%s/source/Shaders/%s", cwd.c_str(), path.string().c_str());
	    sprintf(outpath, "%s/bin/%s.spv", cwd.c_str(), path.filename().string().c_str());
	    sprintf(compileString, "%s -V %s -o %s --target-env spirv1.4", GLSL_VALIDATOR, inpath, outpath);
        SEDX_CORE_TRACE("[ShaderCompiler] Command: {}", compileString);
        SEDX_CORE_TRACE("[ShaderCompiler] Output:");
        while (system(compileString))
	    {
	        SEDX_CORE_WARN("[ShaderCompiler] Error! Press something to Compile Again");
	        std::cin.get();
	    }

	    // 'ate' specify to start reading at the end of the file then we can use the read position to determine the size of the file
	    std::ifstream file(outpath, std::ios::ate | std::ios::binary);
	    if (!file.is_open())
	    {
            SEDX_CORE_WARN("Failed to open file: '{}'", outpath);
        }
	    const size_t fileSize = file.tellg();
	    std::vector<char> buffer(fileSize);
	    file.seekg(0);
	    file.read(buffer.data(), fileSize);
	    file.close();

	    return buffer;
	}

    /*
    void ShaderCompiler::Compile(const ShaderType shaderType, const std::string &filePath, bool async, const VertexFormat vertexType)
    {
        if (!IO::FileSystem::IsFile(filePath))
        {
            SEDX_CORE_ERROR("\"%s\" doesn't exist.", filePath.c_str());
            return;
        }

        // Clear
        m_InputLayout = nullptr;
        m_Descriptors.clear();

        m_ShaderType = shaderType;
        m_VertexType = vertexType;
        if (m_ShaderType == ShaderType::Vertex)
        {
            m_InputLayout = CreateRef<InputLayout>();
        }

        // Load
        LoadFromDrive(filePath);

        // Compile
        {
            m_CompilationState = ShaderCompileState::Idle;

            auto compile = [this, shaderType, async]() {
                // Time compilation
                const Timer timer;

                // Compile
				auto device = RenderContext::GetCurrentDevice();
                m_CompilationState = ShaderCompileState::Compiling;

                void *resource = Compile();
                device->DeletionQueueAdd(ResourceType::Shader, m_Resource);
                m_Resource = resource;
                m_CompilationState = m_Resource ? ShaderCompileState::Succeeded : ShaderCompileState::Failed;

                // log failure
                if (m_CompilationState != ShaderCompileState::Succeeded)
                {
                    std::string defines_str;
                    for (const auto &define : m_Defines)
                    {
                        if (!defines_str.empty())
                            defines_str += ", ";

                        defines_str += define.first + " = " + define.second;
                    }

                    if (defines_str.empty())
                    {
                        SEDX_CORE_ERROR("Failed to compile shader \"%s\".", m_DebugName.c_str());
                    }
                    else
                    {
                        SEDX_CORE_ERROR("Failed to compile shader \"%s\" with definitions \"%s\".", m_DebugName.c_str(), defines_str.c_str());
                    }
                }
            };

            if (async)
            {
                ThreadPool::Submit(compile);
            }
            else
            {
                compile();
            }
        }
    }
    */

    /*
    void ShaderCompiler::PreprocessIncludeDirectives(const std::string &filePath, std::set<std::string> &processedFiles)
    {
        const std::string includePrefix = "#include \"";

        // Normalize file path to lowercase for case-insensitive tracking
        std::string filePathLower = to_lower(filePath);

        // Skip if already processed (prevents circular includes)
        if (processedFiles.contains(filePathLower))
        {
            //SEDX_CORE_WARN("Circular or duplicate include detected: %s", file_path.c_str());
            return;
        }
        processedFiles.insert(filePathLower);

        // try opening the file with the exact path
        std::ifstream fileStream(filePath);
        std::string resolvedPath = filePath;

        // if exact path fails, attempt case-insensitive lookup
        if (!fileStream.is_open())
        {
            std::string directory         = IO::FileSystem::GetDirectoryFromFilePath(filePath);
            std::string filename          = IO::FileSystem::GetFileNameFromFilePath(filePath);
            std::string filenameLower     = to_lower(filename);

            for (std::vector<std::string> dirFiles = IO::FileSystem::GetFilesInDirectory(directory); const std::string& candidate : dirFiles)
            {
                if (to_lower(candidate) == filenameLower)
                {
                    resolvedPath = directory + candidate;
                    fileStream.open(resolvedPath);
                    if (fileStream.is_open())
                        break;
                }
            }

            if (!fileStream.is_open())
            {
                SEDX_CORE_ERROR("Failed to locate include file: %s", filePath.c_str());
                return;
            }
        }

        // read the entire file into a string
        std::stringstream fileContent;
        fileContent << fileStream.rdbuf();
        std::string source = fileContent.str();
        fileStream.close();

        // process the file line by line
        std::istringstream lineStream(source);
        std::string line;
        while (getline(lineStream, line))
        {
            // check for #include directive
            if (size_t includeStart = line.find(includePrefix); includeStart != std::string::npos)
            {
                size_t quoteStart = includeStart + includePrefix.length();
                if (size_t quoteEnd = line.find('\"', quoteStart); quoteEnd != std::string::npos)
                {
                    std::string includeName = line.substr(quoteStart, quoteEnd - quoteStart);
                    std::string includePath = IO::FileSystem::GetDirectoryFromFilePath(resolvedPath) + includeName;

                    // recursively process the included file
                    PreprocessIncludeDirectives(includePath, processedFiles);
                }
                else
                {
                    SEDX_CORE_ERROR("Malformed #include in %s: %s", resolvedPath.c_str(), line.c_str());
                }
            }
            else
            {
                // append non-include lines to the preprocessed source
                m_PreprocessedSource += line + "\n";
            }
        }

        // store metadata for debugging or inspection
        m_Names.push_back(IO::FileSystem::GetFileNameFromFilePath(resolvedPath));
        m_FilePaths.push_back(resolvedPath);
        m_Sources.push_back(source);
    }
    */

    /*
    void ShaderCompiler::LoadFromDrive(const std::string &filePath)
    {
        // Initialize a couple of things
        m_DebugName = IO::FileSystem::GetFileNameWithoutExtensionFromFilePath(filePath);
        m_FilePath   = filePath;
        m_PreprocessedSource.clear();
        m_Names.clear();
        m_FilePaths.clear();
        m_Sources.clear();
        m_FilePathsMultiple.clear();

        // Construct the source by recursively processing all include directives, starting from the actual file path.
        std::set<std::string> processedFiles;
        PreprocessIncludeDirectives(filePath, processedFiles);

        // Update hash
        {
            std::hash<std::string> hasher;
            m_Hash = 0;
            m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_PreprocessedSource)));
            for (const auto& it : m_Defines)
            {
                m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(it.first)));
                m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(it.second)));
            }
        }

        // reverse the vectors so they have the main shader before the subsequent include directives.
        // this also helps with the editor's shader editor where you are interested more in the first source.
        std::ranges::reverse(m_Names);
        std::ranges::reverse(m_FilePaths);
        std::ranges::reverse(m_Sources);
    }
    */

    void ShaderCompiler::SetSource(const uint32_t index, const std::string &source)
    {
        if (index >= m_Sources.size())
        {
            SEDX_CORE_ERROR("No source with index %d exists.", index);
            return;
        }

        m_Sources[index] = source;
    }

    uint32_t ShaderCompiler::GetVertexSize() const { return m_InputLayout->GetVertexSize(); }

    const char *ShaderCompiler::GetEntryPoint() const
    {
        switch (m_ShaderType)
        {
            case ShaderType::Vertex:				return "main_vs";
            case ShaderType::TesselationControl:	return "main_hs";
            case ShaderType::TesselationEval:		return "main_ds";
            case ShaderType::Fragment:				return "main_ps";
            case ShaderType::Compute:				return "main_cs";
            case ShaderType::Raygen:				return "ray_gen";
            /*
            case ShaderType::RayMiss:				return "miss";
            case ShaderType::RayHit:				return "closest_hit";
            */
            default: return nullptr;
        }
    }

    const char *ShaderCompiler::GetTargetProfile() const
    {
        switch (m_ShaderType)
        {
            case ShaderType::Vertex:				return "vs_6_8";
            case ShaderType::TesselationControl:	return "hs_6_8";
            case ShaderType::TesselationEval:		return "ds_6_8";
            case ShaderType::Fragment:				return "ps_6_8";
            case ShaderType::Compute:				return "cs_6_8";
            case ShaderType::Raygen:
            /*
            case ShaderType::RayMiss:
            case ShaderType::RayHit:        return "lib_6_8";
            */
            default: return nullptr;
        }
    }
}

// ---------------------------------------------------------

