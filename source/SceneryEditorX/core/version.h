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
 * version.h
 * -------------------------------------------------------
 * Created: 16/3/2025
 * -------------------------------------------------------
 */

#ifndef VERSION_H
#define VERSION_H

// -------------------------------------------------------

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

// -------------------------------------------------------
