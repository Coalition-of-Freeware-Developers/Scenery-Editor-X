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
 * bitmap.cpp
 * -------------------------------------------------------
 * Created: 12/03/2026
 * -------------------------------------------------------
 */
#include "bitmap.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	Bitmap::Bitmap() : InheritanceBundle<RefCounted, IResource>(ResourceType::Image2D)
	{
	}

	Bitmap::Bitmap(std::filesystem::path filename) : InheritanceBundle<RefCounted, IResource>(ResourceType::Image2D), m_Filename(std::move(filename))
	{
		Bitmap::LoadFromFile(m_Filename.string());
	}

	Bitmap::Bitmap(std::unique_ptr<uint8_t[]> &&data, const UVec2 &size, uint32_t bytesPerPixel) : 
		InheritanceBundle<RefCounted, IResource>(ResourceType::Image2D), m_Data(std::move(data)), m_Size(size), m_BytesPerPixel(bytesPerPixel)
	{

	}

	Bitmap::~Bitmap()
	{
	}

	uint32_t Bitmap::GetWidth() const
	{
		return m_Size.x;
	}

	uint32_t Bitmap::GetHeight() const
	{
		return m_Size.y;
	}

	void Bitmap::SaveToFile(const std::string &filePath)
	{
		InheritanceBundle<RefCounted, IResource>::SaveToFile(filePath);
	}

	void Bitmap::LoadFromFile(const std::string &filePath)
	{
		InheritanceBundle<RefCounted, IResource>::LoadFromFile(filePath);
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
