// DO NOT USE: This project previously had a custom header named 'math.h' which
// shadowed the standard C <math.h>/<cmath> headers on MSVC, hiding declarations
// for sin/cos/tan/sqrt/etc. This stub intentionally hard-errors to eliminate
// all lingering includes and restore correct access to the CRT math functions.
// Replace any include of <Math/includes/math.h> with <Math/includes/xmath.hpp>.
// If you intended the system header, include <cmath> instead.
//
// Any build reaching this file must fix its includes.
//
#pragma once
#error "Include <Math/includes/xmath.hpp> (project math) or <cmath> (standard). <Math/includes/math.h> is removed."

