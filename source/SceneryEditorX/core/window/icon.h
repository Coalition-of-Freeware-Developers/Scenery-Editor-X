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
 * window_icon.h
 * -------------------------------------------------------
 * Created: 29/5/2025
 * -------------------------------------------------------
 */
#pragma once
#include <stb_image.h>

// -------------------------------------------------------

struct IconData
{
    std::string path; // Path to the icon file
    int width;
    int height;
    int channels;
    std::vector<unsigned char> buffer; // Buffer for the icon data
    mutable unsigned char *pixels;     // Pixel data after loading

    IconData() : path(R"(..\..\resources\icon.png)"), width(0), height(0), channels(0), pixels(nullptr)
    {
        stbi_load(path.data(), &width, &height, &channels, 4);
    }

    ~IconData()
    {
        if (pixels)
        {
            stbi_image_free(pixels);
            pixels = nullptr;
        }
    }
};

// -------------------------------------------------------
