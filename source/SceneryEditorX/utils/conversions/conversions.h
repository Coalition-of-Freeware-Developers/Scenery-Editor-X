/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* conversions.h
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#pragma once

/// --------------------------------------------

#ifdef __cplusplus

#include "distance.h"
#include "area.h"
#include "volume.h"
#include "pressure.h"
#include "speed.h"
#include "temperature.h"
#include "weight.h"
#include "energy.h"
#include "power.h"
#include "force.h"
#include "consumption.h"

namespace SceneryEditorX { namespace Convert {
  using PressureUnits = PressureUnit; // Back-compat alias
}} // namespace SceneryEditorX::Convert

#endif // __cplusplus
