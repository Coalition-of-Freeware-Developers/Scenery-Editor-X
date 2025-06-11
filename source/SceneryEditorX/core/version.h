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

/// -------------------------------------------------------

#ifndef SEDX_VERSION_MAJOR
#define SEDX_VERSION_MAJOR(version) (((version) >> 24) & 0xFF)
#endif

#ifndef SEDX_VERSION_MINOR
#define SEDX_VERSION_MINOR(version) (((version) >> 16) & 0xFF)
#endif

#ifndef SEDX_VERSION_PATCH
#define SEDX_VERSION_PATCH(version) (((version) >> 8) & 0xFF)
#endif

#ifndef SEDX_VERSION_BUILD
#define SEDX_VERSION_BUILD(version) ((version) & 0xFF)
#endif

#define SEDX_VERSION(major, minor, patch, build) \
    ((((uint32_t)(major)) << 24U) | (((uint32_t)(minor)) << 16U) | (((uint32_t)(patch)) << 8U) | ((uint32_t)(build)))


//#define SEDX_VERSION(SEDX_VERSION_MAJOR, SEDX_VERSION_MINOR, SEDX_VERSION_PATCH, SEDX_VERSION_BUILD)

#define SEDX_GET_VERSION_MAJOR() SEDX_VERSION_MAJOR


#endif

/// -------------------------------------------------------
