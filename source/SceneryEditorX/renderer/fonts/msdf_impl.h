/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* msdf_impl.h
* -------------------------------------------------------
* Created: 3/8/2025
* -------------------------------------------------------
*/
#pragma once
#undef INFINITE
#include <msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	struct MSDFData
	{
	    msdf_atlas::FontGeometry FontGeometry;
	    std::vector<msdf_atlas::GlyphGeometry> Glyphs;
	};

} 

/// -------------------------------------------------------
