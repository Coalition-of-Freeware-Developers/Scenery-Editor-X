#pragma once

#define SEDX_VER_MAJOR 0
#define SEDX_VER_MINOR 2
#define SEDX_VER_PATCH 26

#define SEDX_TO_VERSION(major, minor, patch) (major * 10000 + minor * 100 + patch)
#define SEDX_VERSION SEDX_TO_VERSION(SEDX_VER_MAJOR, SEDX_VER_MINOR, SEDX_VER_PATCH)

#define SEDX_GET_VERSION() SEDX_VERSION
