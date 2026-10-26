# Project file formats

A project can be stored in three formats. They hold the same information, and you can switch between them at any time with **File › Save Project As…**. This chapter is the user's view of them; the field-by-field specification is in the developer documentation, [Project File Formats](../../edx%20File%20Format%20Documentation.md).

| Format | Extension | Use it for |
|---|---|---|
| **Scenery Editor X project** | `.edX` | Everyday work, and anything in git or other version control. JSON text you can read and, carefully, edit by hand. |
| **Binary project** | `.sex` | Very large projects, and handing a compact copy to someone. Loads and saves several times faster, about half the size, checksummed. |
| **WorldEditor project** | `earth.wed.xml` | Exchanging a pack with WorldEditor (WED 2.x), both ways. |

All three store positions as WGS84 latitude and longitude in decimal degrees (north and east positive), lengths and elevations in meters, and headings in true degrees clockwise from north.

## .edX: made for version control

An `.edX` file is a JSON document with sections for the project's header (`Project`), its reference point (`Datum`), the libraries it uses (`Libraries`), its airports (`Airports`, field for field from `apt.dat` 12.00), its placements (`Assets`, the hierarchy of groups and items), and, for libraries, the `library.txt` it authors (`Library`).

It's designed so git works well with it:

- **The same project always produces the same file.** Keys come in a fixed order, numbers are written in their shortest exact form, lines end in LF. Opening and saving a project without editing it leaves the file byte for byte unchanged.
- **One record per line.** Moving an object changes one line, so diffs are readable and independent edits on two branches rarely conflict.
- **Stable ids.** Every item keeps a random id for life, so two branches adding items never pick the same one. If a merge does produce a duplicate, the editor gives the second item a new id when it opens the file, and says so in the console.
- **The edit date only moves when the content changes.** If two branches both edited the project, the `editdate` line conflicts: take either side.
- **Relative paths.** Art is referred to by library virtual path (`lib/airport/…`) or by a path relative to the project folder.

New projects come with a `.gitattributes` (`*.edX text eol=lf`, `*.sex binary`) and a `.gitignore` for the editor's temporary files.

> [!TIP]
> Reading is lenient, so hand edits and merges stay openable: `//` and `/* */` comments are allowed, missing keys take their defaults, and a value of the wrong type or an unknown key is a console warning, not a failure. Only broken JSON, or a file without a `Project` section, fails to open.

## .sex: compact and checked

A `.sex` file is a binary encoding of exactly the same data, in chunks (the header, libraries, one per airport, the hierarchy, the library), each with its own checksum.

- **Damage is detected, not loaded.** A truncated file, a damaged chunk, or a file mangled by a text-mode transfer or a git line-ending conversion is refused with the reason, for example *Chunk APRT 0 is damaged: checksum mismatch*.
- **It can grow.** Newer editors can add fields and chunks; older ones skip what they don't know.

> [!WARNING]
> Never let git (or an FTP client) treat a `.sex` file as text. Keep `*.sex binary` in `.gitattributes`; the editor adds it to new projects.

## earth.wed.xml: WorldEditor

The editor reads and writes WorldEditor 2.x projects, following WorldEditor's own conventions: its names for surfaces, lights, lines and signs; split bezier nodes; meters in WorldEditor and feet in `apt.dat`. Saving over an `earth.wed.xml` keeps the previous file as `earth.wed.bak.xml`, as WorldEditor does.

Airports, groups and every kind of placement cross both ways with their names, order, lock and hide flags. WorldEditor objects the editor doesn't model are kept verbatim and written back unchanged.

What doesn't cross, each with a console warning:

- `apt.dat` jetways (row 1500) and rows the editor doesn't know have no WorldEditor class: they stay in the project, not in `earth.wed.xml`.
- Legacy row-15 startup locations become ramp starts.
- Curved taxi routes become straight.
- Free-form *other properties* have no place in `earth.wed.xml`.

## Files X-Plane reads

The project formats are for editing. Exporting writes the files X-Plane itself reads:

| File | Contents |
|---|---|
| `Earth nav data/apt.dat` | The pack's airports, in the `apt.dat` 12.00 format |
| `Earth nav data/+NN-EEE/+nn-eee.dsf` | One DSF tile per 1°×1° the project places anything in: objects, facades, forests, lines, polygons, strings, roads |
| `objects/`, `textures/`, `facades/` … | The pack's own art, copied when exporting to another folder |
| `library.txt` | For library projects: the exports |
| `earth.wed.xml` | Optional: the pack as a WorldEditor project |

See [Importing and exporting](import-export.md).
