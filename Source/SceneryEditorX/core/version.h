/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* version.h
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#ifndef VERSION_H
#define VERSION_H

#define SEDX_VER_MAJOR 0
#define SEDX_VER_MINOR 2
#define SEDX_VER_PATCH 27
#define SEDX_VER_BUILD 197

#define SEDX_TO_VERSION(major, minor, patch, build) (major * 10000 + minor * 100 + patch + build)
#define SEDX_VERSION SEDX_TO_VERSION(SEDX_VER_MAJOR, SEDX_VER_MINOR, SEDX_VER_PATCH, SEDX_VER_BUILD)

#define SEDX_GET_VERSION() SEDX_VERSION

#define SEDX_GET_VERSION_MAJOR() SEDX_VER_MAJOR
#endif
