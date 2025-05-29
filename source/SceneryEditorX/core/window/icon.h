/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* window_icon.h
* -------------------------------------------------------
* Created: 29/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <stb_image.h>

/// -------------------------------------------------------

struct IconData
{
    std::string path; /// Path to the icon file
    int width;
    int height;
    int channels;
    std::vector<unsigned char> buffer; /// Buffer for the icon data
    mutable unsigned char *pixels;     /// Pixel data after loading

    IconData() : path(R"(..\..\assets\icon.png)"), width(0), height(0), channels(0), pixels(nullptr) { }

    ~IconData()
    {
        if (pixels)
        {
            stbi_image_free(pixels);
            pixels = nullptr;
        }
    }
};

/// -------------------------------------------------------
