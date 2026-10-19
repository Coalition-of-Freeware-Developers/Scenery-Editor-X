# Scenery Editor X - Project File Formats

A Scenery Editor X project is an X-Plane 12 scenery pack in the making (airports, objects, polygons, lines, forests,
facades, exclusions...) or a custom X-Plane asset library (the `library.txt` it authors). The same project can be
stored three ways:

| Format | Extension | What it is for |
|---|---|---|
| **.edX** | `MyProject.edX` | JSON text. The project file you commit to git: deterministic, one record per line, easy to read and correct by hand. |
| **.sex** | `MyProject.sex` | Binary serialization. Loads and saves about 10x faster and is about half the size; checksummed so damage is detected. Use it for big projects or shipping, not for diffs. |
| **WorldEditor** | `earth.wed.xml` | WED 2.x projects, imported and exported both ways. |

All positions are WGS84 decimal degrees (latitude, longitude; north and east positive), elevations and lengths are
meters, headings are true degrees clockwise from north.

The code lives in `source/SceneryEditorX/project/` (standard library + the header-only nlohmann/json, so it is unit
tested without the engine: `source/Tests/project_tests/ProjectFormatTests.cpp`).

---

## Working with git

The formats are built for version control:

* **Deterministic output.** The same project always produces the same bytes: keys in a fixed order, numbers in their
  shortest exact form (`163.38`, never `163.3800048828125`), LF line endings, a final newline. Opening and saving a
  project without editing it leaves the file byte-for-byte unchanged.
* **One record per line.** A placed object, a polygon node, a runway end, a taxi node is one line, so moving an object is
  a one-line diff and merges of independent edits rarely conflict.
* **Stable ids.** Every item has an id (`"3f9a0c41d2b7"`) it keeps for life; random 48-bit ids mean two branches
  adding items never pick the same one. If a merge does produce a duplicate, the editor gives the second item a new id
  when the file is opened (and says so in the console).
* **`editdate` moves only when content changes.** A save without edits doesn't touch it. If two branches both edited
  the project, the `editdate` line will conflict: take either side.
* **Relative paths.** Resources are library virtual paths (`lib/airport/...`) or paths relative to the project folder.

New projects get a `.gitattributes` (`*.edX text eol=lf`, `*.sex binary`, ...) and a `.gitignore` (the editor's
temporary files). Keep `*.sex binary` in any repository that holds `.sex` files: a text conversion of a `.sex`
corrupts it (the editor detects that and says so).

---

## .edX (JSON)

```json
{
	"Project": {
		"name": "MySceneryProject",
		"uuid": "3f9a0c41-d2b7-4c55-9e1a-0b6f2a7d8e11",
		"kind": "scenery",
		"format-version": 1,
		"editor-version": "1.0.8",
		"XPlaneVersion": "12.1.4-r3",
		"created": "2023-10-01T12:00:00Z",
		"editdate": "2023-10-01T12:00:00Z",
		"author": "John Doe",
		"description": "",
		"package": ""
	},
	"Libraries": [
		{"Library": "Laminar Research Airport Scenery", "local-path": "./Resources/default scenery/airport scenery/library.txt", "entry-count": 5000, "uuid": "15da3863-07d1-462a-be65-3873058675f3", "short-id": "a1b2c3d4"},
		{"Library": "ZDP Library", "local-path": "./Custom Scenery/ZDP_Library/library.txt", "entry-count": 627, "uuid": "5ec54bdf-7be0-4b54-ac02-5d33472cd216", "short-id": "6587fddb"}
	],
	"Airports": [
		{
			"ident": "KSFO",
			"name": "San Francisco",
			"kind": "airport",
			"elevation-ft": 13,
			"metadata": {"icao_code": "KSFO", "iata_code": "SFO", "faa_code": "KSFO", "city": "San Francisco", "state": "California", "country": "USA", "region_code": "US-W", "datum_lat": "37.618999", "datum_lon": "-122.375", "transition_alt": "18000", "transition_level": "FL180"},
			"other-properties": {"MagVar": -14.0}
		}
	],
	"Assets": [
		{"type": "object", "id": "Asset001", "name": "Terminal", "resource": "lib/airport/Common_Elements/Terminals/Terminal_1.obj", "associated-library": "6587fddb", "latitude": 37.618999, "longitude": -122.375, "altitude": 0.0, "heading": 0.0, "elevation-mode": "ground", "show-level": 1, "other-properties": {"building-type": "Terminal"}},
		{"type": "airport", "id": "Airport001", "name": "San Francisco", "airport": "KSFO"}
	]
}
```

### Top-level sections

| Key | Contents |
|---|---|
| `Project` | The header (below). Required: a file without it isn't a project. |
| `Datum` | Optional reference point: `latitude`, `longitude`, `elevation` (m). Without it the first airport's datum is used. |
| `Libraries` | Libraries the scenery uses. Items point at them with `associated-library` = `short-id`. |
| `Airports` | Airports, as apt.dat 12.00 data (below). |
| `Assets` | The scene hierarchy: groups, airports, placements. The top level is WorldEditor's "world". |
| `Library` | For asset libraries: the `library.txt` being authored (below). |
| `WED` | `{"prefs": {...}}`: WorldEditor's document preferences, kept so a WED round trip doesn't lose them. |
| `other-properties` | Free-form data. Any JSON value; the editor keeps it as written. |

### `Project`

| Key | Meaning |
|---|---|
| `name` | Project name. |
| `uuid` | Identity, set once when the project is created. |
| `kind` | `scenery` (a scenery pack) or `library` (an asset library). |
| `format-version` | Data model version (1). A newer number means a newer editor wrote it. |
| `editor-version` | Editor version that last changed the project. |
| `XPlaneVersion` | X-Plane version the scenery targets. |
| `created`, `editdate` | ISO 8601 UTC. `editdate` changes only when a save changes the content. |
| `author`, `description` | Free text. |
| `package` | The scenery pack folder name the project exports to. |

### `Libraries`

`Library` (name), `local-path` (its library.txt, relative to the X-Plane folder), `entry-count`, `uuid`, `short-id`
(8 hex digits). The PROJECT panel's *Find Used Libraries* fills this from the X-Plane library index: the short id is
derived from the library name, so every machine computes the same one.

### `Assets`: the hierarchy

Every entry has `type`, `id` and `name`; `locked` / `hidden` appear when true. Hidden items stay in the project but
aren't exported to X-Plane or drawn. Groups and airports carry `children`. An entry without `type` is an object (the
first version of this document wrote them that way).

| `type` | Keys besides type / id / name |
|---|---|
| `group` | `children` |
| `airport` | `airport` (the `ident` of an entry in `Airports`), `children` (placements WorldEditor keeps inside the airport) |
| `object` | `resource`, `associated-library`, `latitude`, `longitude`, `altitude`, `heading`, `elevation-mode` (`ground` / `msl` / `agl`; `altitude` is ignored on the ground), `show-level` (1-6) |
| `polygon` | `resource` (.pol), `texture-heading`, `rings` |
| `orthophoto` | `resource` (.pol), `texture-heading`, `decal`, `tex-top` / `tex-bottom` / `tex-left` / `tex-right`, `rings` (nodes carry `uv`) |
| `line` | `resource` (.lin), `closed`, `nodes` |
| `string` | `resource` (.str), `closed`, `spacing` (m), `nodes` |
| `forest` | `resource` (.for), `density` (0-1), `fill` (`area` / `line` / `points`), `rings` |
| `facade` | `resource` (.fac), `height` (m), `pick-walls`, `show-level`, `rings` (nodes carry `wall`) |
| `autogen` | `resource` (.ags / .agb), `height`, `spelling`, `rings` |
| `exclusion-zone` | `exclude`, `nodes` (two corners) |
| `exclusion-polygon` | `exclude`, `rings` |
| `wed-raw` | `xml`: WorldEditor objects this format doesn't model (roads, terrain, ...), kept verbatim for the round trip |

`exclude` lists `objects`, `facades`, `forests`, `beaches`, `roads`, `lines`, `polygons`, `strings`.
`rings` is a list of node lists: the first is the outline, the rest are holes. `nodes` is a single node list.

**Nodes** are `[latitude, longitude]`, or an object when they carry more:

```json
{"latitude": 28.19837281, "longitude": -177.392411683, "lo": [0.000014289, 0.000148383], "hi": [-0.000014289, -0.000148383]}
```

| Key | Meaning |
|---|---|
| `lo`, `hi` | Bezier handles as offsets in degrees from the node (as WorldEditor stores them): `lo` toward the previous node, `hi` toward the next |
| `split` | `true` when the two handles are independent (otherwise `lo` mirrors `hi`) |
| `uv`, `uv-lo`, `uv-hi` | Orthophoto texture coordinates `[s, t]` |
| `wall` | Facade wall index (0-based) of the segment starting here |
| `spawn-tiles` | `false` stops autogen tiles along the segment starting here |

### `Airports`

An airport is apt.dat 12.00 data in JSON form, field for field (so a round trip through apt.dat loses nothing):
`ident`, `name`, `kind` (`airport` / `seaport` / `heliport`), `elevation-ft`, `metadata` (the 1302 rows, in file order),
then `runways`, `water-runways`, `helipads`, `pavements`, `linear-features`, `boundaries`, `viewpoint`,
`legacy-startups`, `beacon`, `windsocks`, `signs`, `light-fixtures`, `frequencies`, `flows`, `taxi-network`,
`startups`, `truck-parkings`, `truck-destinations`, `jetways`, `unknown-rows` (rows the editor doesn't know, kept
verbatim). Empty lists are left out. Codes are apt.dat's (surface `26`, approach lights `7`, ...). Airport nodes are
`[latitude, longitude]` or `{"latitude", "longitude", "control": [lat, lon], "line": 51, "light": 102}` (`control` is
the apt.dat bezier control point; `line` / `light` the painted line / light string starting at the node).
WorldEditor's airport flags are `has-atc` and `scenery-id` (the Gateway submission).

The first version of this document's friendly keys are still read: an `Airport` object with `ICAO`, `IATA`, `FAA`,
`City`, `State`, `Country`, `RegionCode`, `DatumLat`, `DatumLon`, `TransitionAlt`, `TransitionLevel` (they become 1302
metadata), `Elevation` (feet), `MagVar` (kept in `other-properties`) and `Libraries`. Saving writes the current layout.

### `Library` (asset libraries)

```json
"Library": {
	"version": 800,
	"regions": [{"name": "oregon", "rects": [[-125, 42, -116, 46]]}],
	"exports": [
		{"virtual": "lib/toledo/hangar.obj", "file": "objects/hangar.obj"},
		{"virtual": "lib/toledo/tree.for", "file": "forests/tree winter.for", "seasons": "fal,win", "region": "oregon", "deprecated": true},
		{"virtual": "lib/toledo/hangar.obj", "file": "objects/hangar_b.obj", "ratio": 0.25, "private": true, "public-date": 20261018}
	]
}
```

An export has `virtual` and `file` (relative to the project folder); `kind` (`export` / `extend` / `exclude` /
`backup`), `ratio` (EXPORT_RATIO), `seasons`, `region`, `private`, `deprecated`, `semi-deprecated` and `public-date`
appear when they differ from the defaults. *File > Export > Library* writes the `library.txt` (A / 800 / LIBRARY,
region definitions, then the exports with their PUBLIC / PRIVATE / DEPRECATED / REGION switches); *Import* reads one.

### Reading rules

Reading is lenient so hand edits and merges stay openable:

* `//` and `/* */` comments are allowed (they aren't kept when the editor saves).
* Missing keys take their defaults. A value of the wrong type (`"latitude": "abc"`) or an unknown key is a warning
  in the console, not a failure; the key is left out when the project is saved again.
* Text that isn't UTF-8 (old Latin-1 apt.dat names) is converted.
* Only broken JSON, or a file without a `Project` object, fails to open.

---

## .sex (binary)

Little-endian throughout.

| Offset | Size | Contents |
|---|---|---|
| 0 | 8 | Magic `89 53 45 58 0D 0A 1A 0A` (`\x89SEX\r\n\x1A\n`, like PNG: a text-mode copy or a git line-ending conversion changes it, and the editor says so) |
| 8 | 2 | Format major (1). Readers refuse a newer major. |
| 10 | 2 | Format minor (0). |
| 12 | 4 | Flags (0) |
| 16 | 4 | Chunk count |
| 20 | 4 | CRC-32 of the chunk table |
| 24 | 8 | Total file size (truncation check) |
| 32 | 32 × count | Chunk table: tag (4 chars), version (u32), offset (u64), size (u64), CRC-32 (u32), reserved (u32) |

Chunks: `INFO` (project header, datum, WED preferences, properties), `LIBS` (library references), `APRT` (one per
airport, in order), `ITEM` (the hierarchy), `LIBR` (asset libraries' library.txt). Other tags are skipped, so newer
editors can add chunks; the editor's cache adds `STMP` (source file stamp) and `RCVR` (recovery record).

Every chunk is checked against its CRC before it is decoded; a damaged or truncated file is refused with the reason
("Chunk APRT 0 is damaged: checksum mismatch") instead of loading garbage. (The cache's recovery files load what
survived.)

Inside a chunk, values are encoded field by field from the same field lists the .edX uses
(`project_archive.h`), so the two formats always carry the same data: integers at their size, floats / doubles as
IEEE bits, enums as int32, booleans as one byte, strings and lists as a u32 count then the items, optionals as a
presence byte, and every struct as a u32 byte size followed by its fields. That size makes the format evolvable: a
reader skips fields a newer writer appended, and keeps defaults for fields an older file lacks (new fields go at the
end of a struct). Counts are checked against the bytes left, so a corrupt count can't trigger a huge allocation.

---

## WorldEditor (earth.wed.xml)

*File > Import > WorldEditor Project* (or opening an earth.wed.xml directly) converts a WED 2.x project;
*File > Export > WorldEditor Project* writes one WED opens (`doc/xml_compatibility` 205, the previous file kept as
`earth.wed.bak.xml`). The conversion follows xptools' WED sources:

* Enums travel as WED's display names (`Asphalt Worn` = surface 26, `SALS` = approach lights 7, `Tie-Down`,
  `Fuel Truck (Jets)`, `1 Default`...). Painted lines and lights include X-Plane 12's numbered library lines
  (`Red Line` = 30, `Double Hold Wide (Black)` = 62, ...) named the way WED names them from
  `lib/airport/lines/<code>_<name>.lin`; the editor adds every such line its library index knows.
* Airports convert like WED's own apt.dat export / import: split bezier nodes become co-located apt.dat nodes and
  merge back, `always_flatten` / `drive_on_left` are `flatten` / `drive_on_left` metadata, elevations are meters in WED
  and feet in apt.dat, frequencies MHz in WED, runway taxi routes get both runway ends as hot zones.
* Overlay placements (objects, polygons, orthophotos, lines, strings, forests, facades, autogen, exclusion zones and
  polygons) keep their names, groups, hierarchy order, lock and hide flags and every attribute.
* WED classes the project doesn't model (roads, terrain patches, shapes, overlay images) and hidden airport elements
  become `wed-raw` items and are written back verbatim with their references renumbered.

What doesn't cross, with the console warning it produces:

* apt.dat jetways (1500) and unknown rows have no WED class: they stay in the project but aren't in earth.wed.xml.
* Row-15 legacy startups become ramp starts; WED keeps one ceiling, visibility and pattern rule per ATC flow.
* Curved taxi routes become straight (apt.dat 1203 shape points aren't modeled yet).
* Items' `other-properties` have no place in earth.wed.xml.

---

## The edit cache

The editor keeps a cache per project file in `%APPDATA%/SceneryEditorX/project-cache/<hash of the path>/` (outside
the project folder, so a git working tree stays clean):

* **snapshot.sex**: the project as last opened or saved. While the project file's size and time are unchanged,
  opening it reads the snapshot instead of parsing the .edX or earth.wed.xml again. A changed file (a hand edit, a git
  checkout) is parsed again.
* **recovery.sex**: autosave (every minute while there are unsaved changes, and when the editor quits) writes
  unsaved work here, never into the project file. Opening the project again offers to restore it. A save or a
  discard removes it. Autosaves only encode the parts of the project that changed since the last one.
* **backups/**: the previous version of the project file before each save (the newest five).

---

## In the editor

* **File** menu: New / Open / Open Recent / Save (Ctrl+S) / Save As (Ctrl+Shift+S) / Close Project; Import a
  WorldEditor project, airports (apt.dat) or a library.txt; Export to WorldEditor, apt.dat, library.txt or a binary
  copy; Show Project Folder. Unsaved changes are asked about before anything replaces them.
* **PROJECT** panel: the header, datum, airports (pick the one the airport editor shows), the hierarchy (hide, lock,
  remove; selecting an object selects it in the scene), libraries, the library.txt exports of asset libraries, and
  validation.
* The project's first airport opens in the airport editor; its edits are saved with the project. Project objects are
  placed in the scene; moving or deleting them, or placing new objects from the X-PLANE LIBRARY panel, updates the
  project. Polygons, lines, forests, facades and other placements are kept and exported but not drawn yet.
* Command line: `--project <file>` opens a project at startup (once the X-Plane library index is ready);
  `--project-autosave <seconds>` changes the autosave interval.
