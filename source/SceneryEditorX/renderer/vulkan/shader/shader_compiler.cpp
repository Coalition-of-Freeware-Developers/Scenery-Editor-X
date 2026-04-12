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
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <spirv_cross/spirv_cross.hpp>

// --------------------------------------------------------------

namespace ShaderCompiler
{

	std::vector<uint32_t> CompileShader(SceneryEditorX::StageType stage, const std::string& filepath, bool optimize)
	{
	    (void)stage;
		(void)optimize;

		Slang::ComPtr<slang::IGlobalSession> globalSession;
		if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef())))
		{
		    SEDX_CORE_ERROR_TAG("Shader", "Failed to create Slang global session");
			return {};
		}

		auto targets = std::to_array<slang::TargetDesc>({
			{.format = SLANG_SPIRV, .profile = globalSession->findProfile("spirv_1_4")}
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

		Slang::ComPtr<slang::ISession> session;
		if (SLANG_FAILED(globalSession->createSession(sessionDesc, session.writeRef())))
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to create Slang session");
			return {};
		}

		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		const std::filesystem::path shaderPath(filepath);
		const std::string moduleName = shaderPath.stem().string();

		Slang::ComPtr<slang::IModule> module{session->loadModuleFromSource(moduleName.c_str(), filepath.c_str(), nullptr, diagnosticsBlob.writeRef())};
		if (!module)
		{
			if (diagnosticsBlob)
			{
			   SEDX_CORE_ERROR_TAG("Shader", "Slang compilation failed: {}", static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
			}
			return {};
		}

		Slang::ComPtr<ISlangBlob> spirv;
		if (SLANG_FAILED(module->getTargetCode(0, spirv.writeRef())) || !spirv)
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to retrieve SPIR-V from Slang module: {}", filepath);
			return {};
		}

		const size_t byteSize = spirv->getBufferSize();
		if (byteSize == 0 || (byteSize % sizeof(uint32_t)) != 0)
		{
			SEDX_CORE_ERROR_TAG("Shader", "Invalid SPIR-V blob size from Slang for {}", filepath);
			return {};
		}

		std::vector<uint32_t> byteCode(byteSize / sizeof(uint32_t));
		std::memcpy(byteCode.data(), spirv->getBufferPointer(), byteSize);
		return byteCode;
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
