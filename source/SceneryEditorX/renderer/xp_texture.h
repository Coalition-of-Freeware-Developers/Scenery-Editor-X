/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* xp_texture.h
* -------------------------------------------------------
* Created: 9/5/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
	struct BaseTexture
	{
	    struct Albedo
	    {
	        float r;
	        float g;
	        float b;
            float a; /// Alpha channel
	        std::string name;
	    };
	
	    struct Normal
	    {
            float r; /// OpenGL Red
            float g; /// OpenGL Green
	        float b; /// This channel is sometimes black
	        std::string name;
	    };
	
	    struct RoughMet
	    {
            float r; /// Metallic
            float g; /// Roughness
	        float b; /// This channel is sometimes black
            std::string name;
	    };
	};

    struct Emissive
    {
        float r;
        float g;
        float b;
        std::string name;
    };

	struct LegacyTextures
	{
        struct Albedo
        {
            float r;
            float g;
            float b;
            float a; /// Alpha channel
            std::string name;
        };

		struct Normal
        {
            float r; /// OpenGL Red
            float g; /// OpenGL Green
            float b; /// Fresnel Level (f0)
            float a; /// Glossiness
            std::string name;
        };
	};

}
