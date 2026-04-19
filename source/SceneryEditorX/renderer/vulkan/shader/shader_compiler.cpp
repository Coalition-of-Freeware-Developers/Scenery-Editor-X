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
 * shader_compiler.h
 * -------------------------------------------------------
 * Created: 11/04/2026
 * -------------------------------------------------------
 */
// ReSharper disable CppReinterpretCastFromVoidPtr
#include "shader_compiler.h"
#include <array>
#include <unordered_set>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <spirv_cross/spirv_cross.hpp>

// --------------------------------------------------------------

namespace ShaderCompiler
{
	/**
	 * @brief Checks if a module is a primary shared library module.
	 * @param moduleName The name of the module to check.
	 * @return True if the module is a primary shared library module, false otherwise.
	 */
	static bool IsPrimarySharedLibraryModule(const std::string& moduleName)
	{
		static const std::unordered_set<std::string> SHARED_LIBRARY_PRIMARY_MODULES = {
			"constants",
			"resources",
			"color",
			"math",
			"noise",
			"depth",
			"position",
			"common",
			"core",
			"surface",
			"lighting_lib",
			"pipeline"
		};

		return SHARED_LIBRARY_PRIMARY_MODULES.contains(moduleName);
	}

	/**
	 * @brief Checks if a module is an entry shader file.
	 * @param moduleName The name of the module to check.
	 * @return True if the module is an entry shader file, false otherwise.
	 */
	static bool IsEntryShaderFile(const std::string& moduleName)
	{
		static const std::unordered_set<std::string> ENTRY_SHADER_MODULES = {
			"line",
			"grid",
			"outline",
			"depth_prepass",
			"depth_light",
			"light",
			"light_integration",
			"light_composition",
			"light_base",
			"blur",
			"bloom",
			"skysphere",
			"blit",
			"gbuffer",
			"font",
			"icon",
			"icons",
			"output",
			"indirect_cull",
			"pre_pass",
			"dof_lighting",
			"lighting",
			"shadow_mapping",
			"ssao",
			"terrain",
			"ui",
			"fog",
			"fxaa",
			"dof",
			"cloud_noise",
			"cloud_shadows",
			"camera",
			"brdf",
			"shader"
		};

		return ENTRY_SHADER_MODULES.contains(moduleName);
	}

	/**
	 * @brief Creates a Slang compilation session configured for Vulkan SPIR-V output.
	 * @param outGlobalSession Receives the global Slang session.
	 * @param outSession Receives the per-compilation Slang session.
	 * @return True when the session is created successfully, false otherwise.
	 */
	static bool CreateSlangSession(Slang::ComPtr<slang::IGlobalSession>& outGlobalSession, Slang::ComPtr<slang::ISession>& outSession)
	{
		if (SLANG_FAILED(slang::createGlobalSession(outGlobalSession.writeRef())))
		{
			SEDX_CORE_FATAL_TAG("ShaderCompiler", "Failed to create Slang compiler global session");
			return false;
		}

		auto targets = std::to_array<slang::TargetDesc>({
			{.format = SLANG_SPIRV, .profile = outGlobalSession->findProfile("spirv_1_4")}
		});

		auto options = std::to_array<slang::CompilerOptionEntry>({
			{.name = slang::CompilerOptionName::EmitSpirvDirectly,
			 .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}}
		});

		slang::SessionDesc sessionDesc{};
		sessionDesc.targets = targets.data();
		sessionDesc.targetCount = static_cast<SlangInt>(targets.size());
		sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

		if (SLANG_FAILED(outGlobalSession->createSession(sessionDesc, outSession.writeRef())))
		{
			SEDX_CORE_FATAL_TAG("ShaderCompiler", "Failed to create Slang session");
			return false;
		}

		return true;
	}

	/**
	 * @brief Loads a Slang module from source and reports diagnostics on failure.
	 * @param session The active Slang session.
	 * @param filepath Source file path.
	 * @param outDiagnostics Receives diagnostic output blob.
	 * @return Loaded module pointer or nullptr on failure.
	 */
	static Slang::ComPtr<slang::IModule> LoadSlangModule(const Slang::ComPtr<slang::ISession>& session, const std::string& filepath, Slang::ComPtr<slang::IBlob>& outDiagnostics)
	{
		const std::filesystem::path shaderPath(filepath);
		const std::string moduleName = shaderPath.stem().string();

		if (!IsPrimarySharedLibraryModule(moduleName) && !IsEntryShaderFile(moduleName))
		{
			SEDX_CORE_ERROR_TAG("ShaderCompiler", "Refusing to compile non-primary/non-entry Slang file '{}'. Compile module primaries or entry shaders only.", filepath);
			return nullptr;
		}

		Slang::ComPtr<slang::IModule> module{session->loadModuleFromSource(moduleName.c_str(), filepath.c_str(), nullptr, outDiagnostics.writeRef())};
		if (!module)
		{
			if (outDiagnostics)
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Slang compilation failed: {}", static_cast<const char*>(outDiagnostics->getBufferPointer()));
			}
			else
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Slang compilation failed for '{}'", filepath);
			}
		}

		return module;
	}

	std::vector<uint32_t> CompileShader(SceneryEditorX::StageType stage, const std::string& filepath, bool optimize)
	{
		(void)stage;
		(void)optimize;

		Slang::ComPtr<slang::IGlobalSession> globalSession;
		Slang::ComPtr<slang::ISession> session;

		if (!CreateSlangSession(globalSession, session))
			return {};

		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		Slang::ComPtr<slang::IModule> module = LoadSlangModule(session, filepath, diagnosticsBlob);
		if (!module)
			return {};

		Slang::ComPtr<ISlangBlob> spirv;
		if (SLANG_FAILED(module->getTargetCode(0, spirv.writeRef())) || !spirv)
		{
			SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to retrieve SPIR-V from Slang module: {}", filepath);
			return {};
		}

		const size_t byteSize = spirv->getBufferSize();
		if (byteSize == 0 || (byteSize % sizeof(uint32_t)) != 0)
		{
			SEDX_CORE_ERROR_TAG("ShaderCompiler", "Invalid SPIR-V blob size from Slang for {}", filepath);
			return {};
		}

		std::vector<uint32_t> byteCode(byteSize / sizeof(uint32_t));
		std::memcpy(byteCode.data(), spirv->getBufferPointer(), byteSize);
		return byteCode;
	}

	bool CompileSlangModule(const std::string& filepath)
	{
		Slang::ComPtr<slang::IGlobalSession> globalSession;
		Slang::ComPtr<slang::ISession> session;

		if (!CreateSlangSession(globalSession, session))
			return false;

		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		Slang::ComPtr<slang::IModule> module = LoadSlangModule(session, filepath, diagnosticsBlob);

		if (!module)
			return false;

		if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
		{
			SEDX_CORE_INFO_TAG("ShaderCompiler", "Slang module compile diagnostics for '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
		}

		SEDX_CORE_INFO_TAG("ShaderCompiler", "Slang module compiled successfully: {}", filepath);
		return true;
	}
	
	std::vector<SceneryEditorX::ShaderInput> Reflect(SceneryEditorX::StageType stage, const std::vector<uint32_t>& shaderBytecode)
	{

		/*
		std::vector<SceneryEditorX::ShaderInput> shaderInput;
	
		spirv_cross::Compiler compiler(shaderBytecode);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();
	
		// Uniform buffers
		for (const spirv_cross::Resource& uniformBuffer : resources.uniform_buffers)
		{
			SceneryEditorX::ShaderInput uniformBufferInput = {};
			uniformBufferInput.stage = stage;
			uniformBufferInput.debugName = uniformBuffer.name;
			uniformBufferInput.set = compiler.get_decoration(uniformBuffer.id, spv::DecorationDescriptorSet);
			uniformBufferInput.binding = compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding);
			uniformBufferInput.count = compiler.get_type(uniformBuffer.type_id).array[0] == 0 ? 1 : compiler.get_type(uniformBuffer.type_id).array[0];
			uniformBufferInput.type = SceneryEditorX::ShaderInputType::UniformBuffer;
	
			shaderInput.push_back(uniformBufferInput);
		}
	
		// Samplers
		for (const spirv_cross::Resource& sampler : resources.sampled_images)
		{
			SceneryEditorX::ShaderInput sampleImageInput = {};
			sampleImageInput.stage = stage;
			sampleImageInput.debugName = sampler.name;
			sampleImageInput.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
			sampleImageInput.type = SceneryEditorX::ShaderInputType::CombinedImageSampler;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& texture : resources.separate_images)
		{
			const spirv_cross::SPIRType& spirType = compiler.get_type(texture.type_id);
	
			SceneryEditorX::ShaderInput sampleImageInput = {};
			sampleImageInput.stage = stage;
			sampleImageInput.debugName = texture.name;
			sampleImageInput.set = compiler.get_decoration(texture.id, spv::DecorationDescriptorSet);
			sampleImageInput.binding = compiler.get_decoration(texture.id, spv::DecorationBinding);
			sampleImageInput.count = spirType.array.empty() ? 1 : spirType.array[0];
			sampleImageInput.type = SceneryEditorX::ShaderInputType::Texture;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& sampler : resources.separate_samplers)
		{
			SceneryEditorX::ShaderInput sampleImageInput = {};
			sampleImageInput.stage = stage;
			sampleImageInput.debugName = sampler.name;
			sampleImageInput.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
			sampleImageInput.type = SceneryEditorX::ShaderInputType::Sampler;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& sampler : resources.storage_images)
		{
			SceneryEditorX::ShaderInput sampleImageInput = {};
			sampleImageInput.stage = stage;
			sampleImageInput.debugName = sampler.name;
			sampleImageInput.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			sampleImageInput.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			sampleImageInput.count = compiler.get_type(sampler.type_id).array[0] == 0 ? 1 : compiler.get_type(sampler.type_id).array[0];
			sampleImageInput.type = SceneryEditorX::ShaderInputType::StorageImage;
	
			shaderInput.push_back(sampleImageInput);
		}
	
		for (const spirv_cross::Resource& storageBuffer : resources.storage_buffers)
		{
			SceneryEditorX::ShaderInput storageBufferInput = {};
			storageBufferInput.stage = stage;
			storageBufferInput.debugName = storageBuffer.name;
			storageBufferInput.set = compiler.get_decoration(storageBuffer.id, spv::DecorationDescriptorSet);
			storageBufferInput.binding = compiler.get_decoration(storageBuffer.id, spv::DecorationBinding);
			storageBufferInput.count = compiler.get_type(storageBuffer.type_id).array[0] == 0 ? 1 : compiler.get_type(storageBuffer.type_id).array[0];
			storageBufferInput.type = SceneryEditorX::ShaderInputType::StorageBuffer;
	
			shaderInput.push_back(storageBufferInput);
		}
	
		return shaderInput;
		*/

		(void)stage;
		(void)shaderBytecode;
		SEDX_CORE_WARN_TAG("Shader", "SPIR-V reflection disabled: SPIRV-Cross backend not linked.");
		return {};
	}

}

// --------------------------------------------------------------
