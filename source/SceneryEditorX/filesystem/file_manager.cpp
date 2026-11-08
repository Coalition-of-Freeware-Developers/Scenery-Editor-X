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
 * FileManager.cpp
 * -------------------------------------------------------
 * Created: 17/3/2025
 * -------------------------------------------------------
 */
#include "file_manager.hpp"
#include "file_system.h"
#include "vfs.h"
#include <codecvt>
#include <commdlg.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_process.h>
#include <SceneryEditorX/asset/asset_extensions.h>
#include <SceneryEditorX/core/time/time.h>
#include <SceneryEditorX/utils/string_utils.h>

// -------------------------------------------------------

namespace SceneryEditorX::IO
{

	// create a silent process (no visible console window) without waiting
	// caller is responsible for calling SDL_WaitProcess and SDL_DestroyProcess
	// note: using STDIO_APP for stdout/stderr helps ensure no console window appears
	static SDL_Process* CreateSilentProcess(const std::vector<std::string>& args)
	{
		std::vector<const char*> c_args;
		for (const auto& arg : args)
		{
			c_args.push_back(arg.c_str());
		}

		c_args.push_back(nullptr);

		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, const_cast<char**>(c_args.data()));
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true);

		SDL_Process* process = SDL_CreateProcessWithProperties(props);
		SDL_DestroyProperties(props);
		return process;
	}

	// run a process silently (no visible console window) and wait for completion
	// note: always using STDIO_APP ensures no console window appears on any platform
	static void RunSilentProcess(const std::vector<std::string>& args, std::string * output = nullptr)
	{
		std::vector<const char*> c_args;
		for (const auto& arg : args)
		{
			c_args.push_back(arg.c_str());
		}

		c_args.push_back(nullptr);
	
		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, const_cast<char**>(c_args.data()));
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true);
	
		SDL_Process* process = SDL_CreateProcessWithProperties(props);
		SDL_DestroyProperties(props);
	
		if (process)
		{
			// read and wait - this drains stdout/stderr and waits for completion
			size_t data_size = 0;
			int exit_code = 0;
			char* data = static_cast<char*>(SDL_ReadProcess(process, &data_size, &exit_code));
			if (output && data && data_size > 0)
			{
				*output = std::string(data, data_size);
			}
			if (data)
				SDL_free(data);
	
			SDL_DestroyProcess(process);
		}
	}

	// -------------------------------------------------------

	// FileSystem implementations live in filesystem/file_system.cpp.
	// file_manager.cpp should not define FileSystem methods to avoid duplicate symbols.

	bool FileManager::IsSceneFile(const std::string &path)
	{
		return FileSystem::IsValidExtension(path, SceneryEditorX::AssetType::Scene);
	}

	bool FileManager::IsTexture(const std::filesystem::path &path)
	{
		const std::string ext = Utils::String::ToLowerCopy(path.extension().string());
		return ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp";
	}

	bool FileManager::IsModel(const std::filesystem::path &path)
	{
		const std::string ext = Utils::String::ToLowerCopy(path.extension().string());
		return ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx" || ext == ".3ds";
	}


	/**
	 * @brief Reads the raw bytes from a file.
	 *
	 * This function opens a file in binary mode, reads its contents into a vector of bytes,
	 * and returns the vector. It uses the 'ate' flag to start reading at the end of the file
	 * to determine the file size, then reads the file from the beginning.
	 *
	 * @param path The path of the file to read.
	 * @param filename The name of the file to read.
	 * @return std::vector<char> A vector containing the raw bytes of the file.
	*/
	std::vector<uint8_t> FileManager::ReadRawBytes(const std::filesystem::path &path)
	{
		std::ifstream input(path, std::ios::binary);
		std::vector<uint8_t> bytes((std::istreambuf_iterator(input)), (std::istreambuf_iterator<char>()));
		input.close();
		return bytes;
	}

	/**
	 * @brief Reads the shader code from a file.
	 *
	 * This function opens a file in binary mode, reads its contents into a vector of bytes,
	 * and returns the vector. It uses the 'ate' flag to start reading at the end of the file
	 * to determine the file size, then reads the file from the beginning.
	 *
	 * @param filename The name of the shader file to read.
	 * @return std::vector<char> A vector containing the raw bytes of the shader file.
	*/
	std::vector<char> FileManager::ReadShaders(const std::string &filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			SEDX_CORE_ERROR("Failed to open file: {}", ToString(filename));
			ErrMsg(std::string("Failed to open file: ") + ToString(filename));
			return {}; // Return empty vector on failure
		}

		size_t fileSize = file.tellg();
		if (fileSize == 0)
		{
			SEDX_CORE_ERROR("File is empty: {}", ToString(filename));
			return {};
		}

		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		if (!file)
		{
			SEDX_CORE_ERROR("Failed to read entire file: {}", ToString(filename));
			ErrMsg(std::string("Failed to read entire file: ") + ToString(filename));
			return {};
		}

		file.close();

		SEDX_CORE_TRACE("Successfully read file: {} ({} bytes)", filename, fileSize);
		return buffer;
	}

	/**
	 * @brief Reads the raw bytes from a file.
	 *
	 * This function opens a file in binary mode, reads its contents into a vector of bytes,
	 * and returns the vector. It uses the 'ate' flag to start reading at the end of the file
	 * to determine the file size, then reads the file from the beginning.
	 *
	 * @param filename The name of the file to read.
	 * @return std::vector<char> A vector containing the raw bytes of the file.
	*/
	std::vector<char> FileManager::ReadFile(const std::string &filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			SEDX_CORE_ERROR("Failed to open file: {}", ToString(filename));
			ErrMsg(std::string("Failed to open file: ") + ToString(filename));
			return {}; // Return empty vector on failure
		}

		size_t fileSize = file.tellg();
		if (fileSize == 0)
		{
			SEDX_CORE_ERROR("File is empty: {}", ToString(filename));
			return {};
		}

		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		if (!file)
		{
			SEDX_CORE_ERROR("Failed to read entire file: {}", ToString(filename));
			ErrMsg(std::string("Failed to read entire file: ") + ToString(filename));
			return {};
		}

		file.close();

		SEDX_CORE_TRACE("Successfully read file: {} ({} bytes)", filename, fileSize);
		return buffer;
	}


}

// -------------------------------------------------------
