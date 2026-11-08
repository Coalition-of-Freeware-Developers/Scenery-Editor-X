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
 * shader_cache.cpp
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#include "shader_cache.h"
#include "shader.h"
#include "shader_reflection.h"
#include <array>
#include <cstring>
#include <fstream>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/utils/filestreaming/file_streaming.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr std::array<char, 4> kProgramCacheMagic = {'S', 'X', 'S', 'P'};
		constexpr uint32_t kProgramCacheVersion = 1;

		// Upper bounds used to reject corrupt cache files before allocating.
		constexpr uint32_t kMaxCachedStages = 16;
		constexpr uint32_t kMaxCachedDependencies = 512;
		constexpr uint32_t kMaxCachedSpirvWords = 64u * 1024u * 1024u;
	}

	std::filesystem::path ShaderCache::ResolveSpirvPath(std::string_view sourcePath)
	{
		std::filesystem::path path = VFS::FromUtf8(sourcePath);	// UTF-8, like every path string
		if (path.extension() == ".slang" || path.extension() == ".hlsl" || path.extension() == ".glsl")
			path.replace_extension(".spv");

		if (path.extension() != ".spv")
			path += ".spv";

		if (path.is_relative())
		{
			const std::filesystem::path spirvRoot = std::filesystem::path(Shader::GetShaderDirectoryPath()) / "spirv";
			return (spirvRoot / path).lexically_normal();
		}

		return path.lexically_normal();
	}

	bool ShaderCache::TryLoadSpirv(const std::filesystem::path& spirvPath, std::vector<uint32_t>& outSpirv)
	{
		const std::array<std::filesystem::path, 2> candidates = {
			spirvPath,
			std::filesystem::path("..") / spirvPath,
		};

		for (const std::filesystem::path& candidate : candidates)
		{
			std::ifstream file(candidate, std::ios::binary | std::ios::ate);
			if (!file)
				continue;

			const std::streamsize size = file.tellg();
			if (size <= 0 || (size % 4) != 0)
				continue;

			outSpirv.resize(static_cast<size_t>(size / 4));
			file.seekg(0, std::ios::beg);
			file.read(reinterpret_cast<char*>(outSpirv.data()), size);
			if (!file.good())
			{
				outSpirv.clear();
				continue;
			}

			return outSpirv.size() >= 5u && outSpirv.front() == 0x07230203u;
		}

		outSpirv.clear();
		return false;
	}

	std::filesystem::path ShaderCache::GetProgramCacheDirectory()
	{
		// The cache folder, not temp: Windows' Storage Sense empties temp folders and every shader would compile again
		return VFS::GetAppLocation(VFS::AppLocation::Cache) / "shaders";
	}

	bool ShaderCache::TryLoadProgram(const std::filesystem::path& cacheFile, CachedShaderProgram& outProgram)
	{
		std::error_code ec;
		const auto cacheWriteTime = std::filesystem::last_write_time(cacheFile, ec);
		if (ec)
			return false;

		FileStreamReader reader(cacheFile);
		if (!reader)
			return false;

		std::array<char, 4> magic{};
		uint32_t version = 0;
		reader.ReadData(magic.data(), magic.size());
		reader.ReadRaw(version);
		if (!reader || magic != kProgramCacheMagic || version != kProgramCacheVersion)
			return false;

		CachedShaderProgram program;
		reader.ReadRaw(program.vertexType);
		reader.ReadRaw(program.pushConstantSize);

		uint32_t dependencyCount = 0;
		reader.ReadRaw(dependencyCount);
		if (!reader || dependencyCount > kMaxCachedDependencies)
			return false;

		program.dependencies.reserve(dependencyCount);
		for (uint32_t i = 0; i < dependencyCount; ++i)
		{
			std::string dependency;
			reader.ReadString(dependency);
			program.dependencies.emplace_back(std::filesystem::path(std::u8string(dependency.begin(), dependency.end())));
		}

		// Stale when any source file (entry module or an import) changed after the cache was written.
		for (const std::filesystem::path& dependency : program.dependencies)
		{
			const auto dependencyWriteTime = std::filesystem::last_write_time(dependency, ec);
			if (ec || dependencyWriteTime > cacheWriteTime)
				return false;
		}

		uint32_t stageCount = 0;
		reader.ReadRaw(stageCount);
		if (!reader || stageCount == 0 || stageCount > kMaxCachedStages)
			return false;

		for (uint32_t i = 0; i < stageCount; ++i)
		{
			ShaderType stage = ShaderType::None;
			uint32_t wordCount = 0;
			reader.ReadRaw(stage);
			reader.ReadRaw(wordCount);
			if (!reader || wordCount == 0 || wordCount > kMaxCachedSpirvWords)
				return false;

			std::vector<uint32_t>& spirv = program.stages[stage];
			spirv.resize(wordCount);
			reader.ReadData(reinterpret_cast<char*>(spirv.data()), static_cast<size_t>(wordCount) * sizeof(uint32_t));
		}

		reader.ReadMap(program.resources);
		if (!reader)
			return false;

		outProgram = std::move(program);
		return true;
	}

	bool ShaderCache::StoreProgram(const std::filesystem::path& cacheFile, const CachedShaderProgram& program)
	{
		std::error_code ec;
		std::filesystem::create_directories(cacheFile.parent_path(), ec);
		if (ec)
			return false;

		// Write to a sibling temp file first so a crash mid-write never leaves a truncated cache entry.
		std::filesystem::path tempFile = cacheFile;
		tempFile += ".tmp";

		{
			FileStreamWriter writer(tempFile);
			if (!writer)
				return false;

			writer.WriteData(kProgramCacheMagic.data(), kProgramCacheMagic.size());
			writer.WriteRaw(kProgramCacheVersion);
			writer.WriteRaw(program.vertexType);
			writer.WriteRaw(program.pushConstantSize);

			writer.WriteRaw(static_cast<uint32_t>(program.dependencies.size()));
			for (const std::filesystem::path& dependency : program.dependencies)
			{
				const std::u8string utf8 = dependency.u8string();
				writer.WriteString(std::string(utf8.begin(), utf8.end()));
			}

			writer.WriteRaw(static_cast<uint32_t>(program.stages.size()));
			for (const auto& [stage, spirv] : program.stages)
			{
				writer.WriteRaw(stage);
				writer.WriteRaw(static_cast<uint32_t>(spirv.size()));
				writer.WriteData(reinterpret_cast<const char*>(spirv.data()), spirv.size() * sizeof(uint32_t));
			}

			writer.WriteMap(program.resources);
			if (!writer)
			{
				std::filesystem::remove(tempFile, ec);
				return false;
			}
		}

		std::filesystem::rename(tempFile, cacheFile, ec);
		if (ec)
		{
			std::filesystem::remove(tempFile, ec);
			return false;
		}

		return true;
	}

}  // namespace SceneryEditorX

// -------------------------------------------------------
