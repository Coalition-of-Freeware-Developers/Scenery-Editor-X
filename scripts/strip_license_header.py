#!/usr/bin/env python3
"""
strip_license_header.py
-----------------------
Simplified Doxygen input filter.

New behavior (per updated requirements):
1. Inspect only the first 12 lines of the incoming C/C++ source file.
2. If within those 12 lines there exists a block comment that:
     - Starts with '/**' (after optional leading whitespace on line 1)
     - Contains the text 'Scenery Editor X'
     - Contains a closing '*/' somewhere in those 12 lines
   THEN the script removes (skips) the first exactly 12 lines (the banner is
   always exactly 12 lines long) and outputs the remainder of the file.
3. Otherwise the file is passed through unchanged.

Notes:
* Unlike the previous version, we intentionally REMOVE the 12 banner lines;
  line numbering in Doxygen output will shift (this is desired now).
* No partial / fuzzy detection beyond the explicit checks above.
"""
from __future__ import annotations
import sys
from typing import List
from pathlib import Path


def _read_input() -> str:
  """Return the file contents for Doxygen filtering.

  Doxygen invokes filters as:  <filter> <absolute-path-to-file>
  It does NOT pipe the file to stdin by default. Our previous implementation
  blocked forever on sys.stdin.read() because stdin was never written/closed.
  To remain flexible we:
    1. If an argument is provided, read that file directly (preferred path).
    2. Otherwise fall back to reading stdin (supports manual piping / tests).
  """
  if len(sys.argv) > 1:
    file_path = Path(sys.argv[1])
    try:
      # Use utf-8 with fallback error replacement to avoid crashes on
      # the rare chance of an encoding anomaly.
      return file_path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:  # pragma: no cover - defensive
      # Emit nothing but ensure the filter does not hang Doxygen.
      # We cannot print diagnostics to stdout (would pollute output),
      # but we can fall back to an empty string safely.
      return ""
  # Fallback: read any piped data (this call will return immediately if no data)
  return sys.stdin.read()


def process(data: str) -> str:
  lines: List[str] = data.splitlines(keepends=True)

  # Need at least 12 lines to consider stripping.
  if len(lines) >= 12:
    first12 = ''.join(lines[:12])
    # Conditions:
    #  - First non-whitespace characters of line 1 start with '/**'
    #  - 'Scenery Editor X' appears somewhere in first 12 lines
    #  - '*/' appears somewhere in first 12 lines (ensures closed comment)
    line1_stripped = lines[0].lstrip()
    if (
      line1_stripped.startswith('/**')
      and 'Scenery Editor X' in first12
      and '*/' in first12
    ):
      return ''.join(lines[12:])  # Strip (skip) the first 12 lines entirely.
  return data


def main() -> None:
  data = _read_input()
  sys.stdout.write(process(data))


if __name__ == '__main__':  # pragma: no cover
  main()

