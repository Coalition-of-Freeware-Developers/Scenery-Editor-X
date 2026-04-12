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
 * FileManager.hpp
 * -------------------------------------------------------
 * Created: 17/3/2025
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <SceneryEditorX/core/memory/buffer.h>

// -------------------------------------------------------

namespace SceneryEditorX::IO
{

	/**
	 * @class FileManager
	 * @brief Manages file operations such as reading, writing, and querying file status.
	 */
	class FileManager
	{
	public:
		/**
		 * @brief Opens the specified file and reads its contents into a vector of bytes.
		 * @param path The path to the file to read.
		 * @return A vector containing the raw bytes read from the file.
		 */
		static std::vector<uint8_t> ReadRawBytes(const std::filesystem::path &path);

		/**
		 * @brief Opens the specified shader file and reads its contents into a vector of bytes.
		 * @param filename The name of the shader file to read.
		 * @return A vector containing the raw bytes read from the shader file.
		 */
		static std::vector<char> ReadShaders(const std::string &filename);

		/**
		 * @brief Reads the raw bytes from a file.
		 * @param filename The name of the file to read.
		 * @return std::vector<char> A vector containing the raw bytes of the file.
		 */
		static std::vector<char> ReadFile(const std::string &filename);

		/**
		 * @brief 
		 * @param path 
		 * @return 
		 */
		static bool IsTexture(const std::filesystem::path &path);

		/**
		 * @brief 
		 * @param path 
		 * @return 
		 */
		static bool IsModel(const std::filesystem::path &path);

		/**
		 * @brief  
		 * @param path 
		 * @return 
		 */
		static bool IsSceneFile(const std::string &path);

	};

}

// -------------------------------------------------------
