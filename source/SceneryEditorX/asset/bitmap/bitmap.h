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
 * bitmap.h
 * -------------------------------------------------------
 * Created: 12/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class Bitmap
	 * @brief A class representing a bitmap image resource, inheriting from SharedResource.
	 */
	class Bitmap : public SharedResource
	{
	public:
		Bitmap();
		explicit Bitmap(std::filesystem::path filename);
		Bitmap(std::unique_ptr<uint8_t[]> &&data, const UVec2 &size, uint32_t bytesPerPixel = 4);
		virtual ~Bitmap();

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		void SaveToFile(const std::string &filePath) override;
		void LoadFromFile(const std::string &filePath) override;

		uint32_t GetBytesPerPixel() const { return m_BytesPerPixel; }
		void SetBytesPerPixel(uint32_t bytesPerPixel) { this->m_BytesPerPixel = bytesPerPixel; }

	private:
		std::filesystem::path m_Filename;
		Scope<uint8_t[]> m_Data;
		UVec2 m_Size;
		uint32_t m_BytesPerPixel = 0;
	};

}

// -------------------------------------------------------
