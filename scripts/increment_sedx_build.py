# -------------------------------------------------------
# Copyright (c) 2025 Thomas Ray
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is furnished
# to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# -------------------------------------------------------
# Created: 10/9/2025
# -------------------------------------------------------
# Increment the build number in resource.h
# Fits in a single unsigned 64‑bit integer (no truncation)
# Bit Layout:
#  - Major:   8 bits
#  - Minor:   8 bits
#  - Patch:   8 bits
#  - Build:  16 bits
# -------------------------------------------------------

#!/usr/bin/env python3
import re
from pathlib import Path

# Path to your resource.h
resource_file = Path(__file__).parent.parent / "source" / "resource.h"

# Read file
lines = resource_file.read_text(encoding="utf-8").splitlines()

# Regex patterns
patterns = {
    "major": re.compile(r'#define\s+SEDX_MAJOR\s+(\d+)'),
    "minor": re.compile(r'#define\s+SEDX_MINOR\s+(\d+)'),
    "patch": re.compile(r'#define\s+SEDX_PATCH\s+(\d+)'),
    "build": re.compile(r'(#define\s+SEDX_BUILD\s+)(\d+)'),
    "string": re.compile(r'(#define\s+SEDX_VERSION_STRING\s+)".*"'),
    "hex": re.compile(r'(#define\s+SEDX_VERSION\s+)(0x[0-9A-Fa-f]+)')
}

# Extract current values
major = minor = patch = build = None
for line in lines:
    if m := patterns["major"].match(line): major = int(m.group(1))
    elif m := patterns["minor"].match(line): minor = int(m.group(1))
    elif m := patterns["patch"].match(line): patch = int(m.group(1))
    elif m := patterns["build"].match(line): build = int(m.group(2))

if None in (major, minor, patch, build):
    raise RuntimeError("Could not find all version components in resource.h")

# Increment build
build += 1
if build > 65535:  # wrap if exceeding 16-bit limit
    build = 0
    patch += 1  # optional rollover behavior

# Build the version string
version_string = f"{major}.{minor}.{patch}.{build}"

# Calculate packed hex: 8 bits each for major/minor/patch, 16 bits for build
hex_val = (major << 32) | (minor << 24) | (patch << 16) | build

# Update lines
new_lines = []
for line in lines:
    if m := patterns["build"].match(line):
        line = f"{m.group(1)}{build}"
    elif m := patterns["string"].match(line):
        line = f'{m.group(1)}"{version_string}"'
    elif m := patterns["hex"].match(line):
        line = f"{m.group(1)}0x{hex_val:010X}"
    new_lines.append(line)

# Write back
resource_file.write_text("\n".join(new_lines) + "\n", encoding="utf-8")

print(f"Updated version to {version_string}")
