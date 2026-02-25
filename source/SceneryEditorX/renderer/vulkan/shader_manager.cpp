/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * shader_manager.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "shader_manager.h"
#include <utility>
#include <tracy/Tracy.hpp>
#include <shaderc/shaderc.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	ShaderManager::ShaderManager(const void* spirvCode, size_t codeSize)
	{
	    // Use the same module for vertex and fragment stages by default.
	    m_Stages.reserve(2);
	    m_Modules.reserve(2);
	    m_Stages.push_back(VK_SHADER_STAGE_VERTEX_BIT);
	    m_Modules.emplace_back(spirvCode, codeSize);
	    m_Stages.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);
	    m_Modules.emplace_back(spirvCode, codeSize);
	}
	
	ShaderManager::ShaderManager(const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void*, size_t>>>& stages)
	{
	    m_Stages.reserve(stages.size());
	    m_Modules.reserve(stages.size());
	    for (const auto& s : stages)
		{
	        m_Stages.push_back(s.first);
	        m_Modules.emplace_back(s.second.first, s.second.second);
	    }
	}
	
	Ref<Shader>& ShaderManager::CreateShader(const std::string& name)
	{
		if (m_Shaders.find(name) != m_Shaders.end())
			return m_Shaders[name];
	
		m_Shaders[name] = std::make_shared<VulkanShader>();
		return m_Shaders[name];
	}
	
	void ShaderManager::Clear()
	{
		m_Shaders.clear();
	}
	
	Ref<Shader>& ShaderManager::GetShader(const std::string& name)
	{
		SEDX_CORE_ASSERT(m_Shaders.contains(name) == false, "Shader %s is not present", name.c_str());
		return m_Shaders[name];
	}

    static shaderc_shader_kind GetShadercKind(Stage stage)
	{
		switch (stage)
		{
			case Stage::Vertex:					return shaderc_glsl_vertex_shader;
			case Stage::TessellationControl:	return shaderc_glsl_tess_control_shader;
			case Stage::TessellationEvaluation: return shaderc_glsl_tess_evaluation_shader;
			case Stage::Fragment:				return shaderc_glsl_fragment_shader;
			case Stage::Compute:				return shaderc_glsl_compute_shader;
			case Stage::Geometry:				return shaderc_glsl_geometry_shader;
			case Stage::None:					SEDX_CORE_ASSERT(false, "Invalid shader stage");
		}
	
		return shaderc_vertex_shader;
	}
	
	static std::string ReadFile(const std::string& filepath)
	{
		std::ifstream file(filepath);
		SEDX_CORE_ASSERT(!file.good(), "File %s doesn't exist.", filepath.c_str());
	
		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		std::string buffer(size, ' ');
		file.seekg(0);
		file.read(buffer.data(), size);
	
		return buffer;
	}

    static std::vector<uint32_t> ShaderCompiler::CompileVulkanShader(Stage stage, const std::string& filepath, bool optimize)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
	
		shaderc_util::FileFinder fileFinder;
		options.SetIncluder(std::make_unique<glslc::FileIncluder>(&fileFinder));
	
		std::string sourceCode = ReadFile(filepath);
	
		if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_performance); 
		options.SetOptimizationLevel(shaderc_optimization_level_zero);
		options.SetGenerateDebugInfo();
	
		size_t begString = filepath.find_last_of('/');
		std::string shaderName = filepath.substr(begString + 1);
	
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(sourceCode, GetShadercKind(stage), shaderName.c_str(), options);
	
		if (module.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << module.GetErrorMessage();
			return std::vector<uint32_t>();
		}
	
		return { module.cbegin(), module.cend() };
	}
	
	std::vector<ShaderInput> ShaderCompiler::Reflect(ShaderStage stage, const std::vector<uint32_t>& shaderBytecode)
	{
		std::vector<ShaderInput> shaderInput;
	
		spirv_cross::Compiler compiler(shaderBytecode);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();
	
		// Uniform buffers
		for (const spirv_cross::Resource& uniformBuffer : resources.uniform_buffers)
		{
			ShaderInput uniformBufferInput = {};
			uniformBufferInput.Stage = stage;
			uniformBufferInput.DebugName = uniformBuffer.name;
			uniformBufferInput.Set = compiler.get_decoration(uniformBuffer.id, spv::DecorationDescriptorSet);
			uniformBufferInput.Binding = compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding);
			uniformBufferInput.Count = compiler.get_type(uniformBuffer.type_id).array[0] == 0 ? 1 : compiler.get_type(uniformBuffer.type_id).array[0];
			uniformBufferInput.Type = ShaderInputType::UNIFORM_BUFFER;
	
			shaderInput.push_back(uniformBufferInput);
		}
	
		// Samplers
		for (const spirv_cross::Resource& sampler : resources.sampled_images)
		{
			ShaderInput sampleImageInput = {};
			sampleImageInput.Stage = stage;
			sampleImageInput.DebugName = sampler.name;
			sampleImageInput.Set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.Binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.Count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
			sampleImageInput.Type = ShaderInputType::COMBINED_IMAGE_SAMPLER;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& texture : resources.separate_images)
		{
			const spirv_cross::SPIRType& spirType = compiler.get_type(texture.type_id);
	
			ShaderInput sampleImageInput = {};
			sampleImageInput.Stage = stage;
			sampleImageInput.DebugName = texture.name;
			sampleImageInput.Set = compiler.get_decoration(texture.id, spv::DecorationDescriptorSet);
			sampleImageInput.Binding = compiler.get_decoration(texture.id, spv::DecorationBinding);
			sampleImageInput.Count = spirType.array.empty() ? 1 : spirType.array[0];
			sampleImageInput.Type = ShaderInputType::TEXTURE;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& sampler : resources.separate_samplers)
		{
			ShaderInput sampleImageInput = {};
			sampleImageInput.Stage = stage;
			sampleImageInput.DebugName = sampler.name;
			sampleImageInput.Set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.Binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.Count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
			sampleImageInput.Type = ShaderInputType::SAMPLER;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& sampler : resources.storage_images)
		{
			ShaderInput sampleImageInput = {};
			sampleImageInput.Stage = stage;
			sampleImageInput.DebugName = sampler.name;
			sampleImageInput.Set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.Binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.Count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
			sampleImageInput.Type = ShaderInputType::STORAGE_IMAGE;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& storageBuffer : resources.storage_buffers)
		{
			ShaderInput storageBufferInput = {};
			storageBufferInput.Stage = stage;
			storageBufferInput.DebugName = storageBuffer.name;
			storageBufferInput.Set = compiler.get_decoration(storageBuffer.id, spv::DecorationDescriptorSet);
			storageBufferInput.Binding = compiler.get_decoration(storageBuffer.id, spv::DecorationBinding);
			storageBufferInput.Count = compiler.get_type(storageBuffer.type_id).array[0] == 0 ? 1 : compiler.get_type(storageBuffer.type_id).array[0];
			storageBufferInput.Type = ShaderInputType::STORAGE_BUFFER;
	
			shaderInput.push_back(storageBufferInput);
		}
	
		return shaderInput;
	}
	*/

} // namespace SceneryEditorX

// -------------------------------------------------------
