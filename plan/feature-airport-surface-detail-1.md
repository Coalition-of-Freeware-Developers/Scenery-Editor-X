---
goal: Draw the surface detail X-Plane 12 shows at airports and SEDX was missing - tire rubber, grunge and dirt, cracks and seams, soft pavement edges, lawn tracks, inset and elevated light fixtures, runway lights, blast pads, shoulders, unpaved runways and the rest of the runway markings
version: 1.0
date_created: 2026-10-26
last_updated: 2026-10-26
owner: Scenery Editor X renderer (Coalition of Freeware Developers)
status: 'In progress'
tags: [feature, bug, renderer, x-plane, airport, overlay, lights]
---

# Introduction

![Status: In progress](https://img.shields.io/badge/status-In%20progress-yellow)

Side by side with X-Plane 12, SEDX's airports were missing a lot of the small surface detail that makes an airport read
as real: tire skid marks, grunge, embedded taxiway and runway lights, lawn mowing tracks, taxiway and runway cracks,
the soft blend where taxiway pavement meets grass. This plan records why each was missing (found on 2026-10-26 by
comparing the reference probe's X-Plane 12.4.4 shots with SEDX captures of the same scenarios at KGON, KORD, EIDW and
KBTV) and the work to bring each back.

X-Plane gets this detail from three places, and SEDX had a gap in each:

1. **Art the scenery places** (Gateway / custom DSFs): `lib/airport/ground/pavement_FX/...` lines (soft and cracked
   pavement edges, taxiway dirt, crack shoulders, seams, drains), `lib/airport/ground/terrain_FX/...` lines, polygons and
   draped OBJs (lawn tracks, grass / soil / sand patches, sign bases). KORD's Gateway DSF alone places 320 soft edge
   lines, 74 dirt lines, 14 crack-shoulder lines, 54 seams and 33 lawn-track areas.
2. **What the simulator builds from apt.dat itself**: runway lights (edge, centerline, touchdown zone, threshold / end,
   REIL, approach), tire rubber (`lib/airport/default_runways/rubber.pol`), blast pads
   (`default_runways/<family>/blastpad*.pol`), unpaved runways (`default_runways/<family>/runway.lin`) and taxiways.
3. **The library art's own shading**: every `.lin` / `.pol` carries the XP12 standard-shader material (normal map,
   gloss, decals, weather, `DITHER_ALPHA`).

## 1. Requirements & Constraints

### Findings (baseline `c0edc480`, branch `xp12-environment-rendering`)

- **FND-001 (grunge, dirt, cracks, soft edges, lawn tracks)**: every `.lin` was alpha-tested at 0.5
  (`OverlayBuilder::BuildLine`, apt.dat painted lines). X-Plane 12 blends line alpha. The pavement FX atlas
  (`sim objects/apt_pavement/textures/apt_sfx1_ALB.dds`) has a mean alpha of 0.14: its grunge, dirt and soft-edge strips
  sit at 10-40% alpha, so the test deleted everything but the crisp cracks.
- **FND-002**: `.lin` files' material was ignored: `TEXTURE_NORMAL` ratio, `NORMAL_METALNESS`, `SPECULAR`,
  `SUPER_ROUGHNESS`, `WEATHER[_TRANSPARENT]`, `SNOW_ALBEDO_LUMA`, `DECAL_LIB` and decal rows were not parsed or not
  applied, so lines drew flat albedo with no normal map, gloss or detail decals.
- **FND-003 (embedded taxiway lights)**: apt.dat light strings used `lib/airport/lights/fast/<code>_*.str`. The "fast"
  objects are lights only (`POINT_COUNTS 0 0 0 0`): nothing to see by day. The "slow" set
  (`lib/airport/lights/slow/...`) carries the inset / elevated fixture meshes (146-537 vertices) plus the same lights.
  Inset fixtures were also placed at ground level, under the pavement's lifted drape.
- **FND-004 (embedded runway lights)**: SEDX placed no runway lights at all. X-Plane builds them from row 100 (edge
  light code, centerline flag, per end: TDZ flag, REIL code, approach light code) with the slow fixtures
  (`HIRL_* / MIRL_* / LIRL_*`, `RCLS_*`, `TDZL_left/right`, `thr_*_inset_*`, `REIL_strobe_*`, `FAA_* / calvert_*`).
  Two-color fixtures are named for what they show along their heading first (`edge_y` faces +Z, `edge_w_rev` -Z in
  `lights.txt`), e.g. `HIRL_yw` = yellow to traffic along its heading, white behind.
- **FND-005 (tire skid marks)**: X-Plane draws `lib/airport/default_runways/rubber.pol` (`RWY_skids_1.dds`, 2048 px
  = 30 m, four 7.5 m streak columns: a fade, two dense, one light; `DECAL_LIB apt_skids_1.dcl`) in each runway's
  touchdown zone. SEDX did not. X-Plane's layout is in the simulator, not the library: it is fitted by eye.
- **FND-006**: Blast pads are drawn with a flat color; X-Plane uses `default_runways/<family>/blastpad*.pol` (its
  `RUNWAY_MARKINGS 0.95 0.68 0 0` = yellow chevrons).
- **FND-007**: Grass, dirt, gravel, snow and lakebed runways / taxiways / helipads are drawn with a flat color;
  X-Plane 12 uses `default_runways/<family>/runway.lin` (unpaved runways are a line) and `taxiway.pol` / `helipad.pol`.
- **FND-008**: Runway shoulders (row 100's shoulder code and 12.00 width) are not drawn.
- **FND-009**: Displaced-threshold arrows and arrowheads (EIDW 28R, KBTV 33) are missing; the `RWY_markings_*.dds`
  atlas has both sprites.
- **FND-010**: `.lin TEXTURE_TILE <x> <y> <map w> <map h> <map>` (lawn-track strips `sfx_strip_lawntrack_*.lin`:
  `TEXTURE_TILE 1 8 1 64 swizzle8_64.png`) is not parsed: the whole 8-tile atlas is squeezed into one repeat.
- **FND-011 (open)**: KGON runway 5/23 sits in a wide light-grey band where X-Plane shows green airport grass. Ruled
  out: apt.dat pavement (none contains points 30-60 m off the edge), DSF polygons (the Gateway's `lib/airport/pavement/*`
  and `safety_area_*` polygons are elsewhere), blast pads, the land class (`lib/g10/terrain10/apt_tmp_wet.ter`, the same
  as the green infield). EIDW 28R shows the same grey beside its runway (`apt_tmp_sdry.ter`). Strong lead: a KORD
  close-up (`kord_fx_edge`) shows the airport-grass terrain as flat grey with only its decal's brown streaks on top, so
  the `.ter`'s BASE_TEX / COMPOSITE_TEX grass isn't reaching SEDX's terrain shader at close and grazing range (from
  600 m the infield is green). This grey-plus-streaks surface is also where X-Plane shows its mowed-grass look.
- **FND-012**: X-Plane's runway lights throw spill onto the pavement at night (the red glow on KGON's threshold bars);
  SEDX's light objects have no spill there.

### Requirements

- **REQ-001**: Library art is resolved through the library like X-Plane does (scenery packs that replace
  `default_runways` or the light fixtures, such as AEP, are honored).
- **REQ-002**: Optional art (rubber, fixtures) degrades silently on X-Plane 11-style libraries: the light-only set when
  there are no fixtures, no rubber and no warning when there is no `rubber.pol`.
- **REQ-003**: Every new layout constant that is not read from an X-Plane file is marked as an ASSUMPTION with the
  reference it was fitted to.
- **REQ-004**: Edits to a runway rebuild only that runway's lights and rubber (`AIRPORT_LIGHTS_RUNWAY` source tag).
- **CON-001**: Clean-room: X-Plane's shaders and binaries are behavioral reference only. Layout rules come from the
  library files, FAA AC 150/5340-30 dimensions and the reference shots.
- **CON-002**: `xp_overlay.*` stays standard-library only (unit tested by `XPlaneFormatTests` on all platforms).

## 2. Implementation Steps

### Phase 1 - Line material and light fixtures (FND-001, 002, 003)

| Task | Description | Completed | Date |
| -------- | --------------------- | --------- | ---------- |
| TASK-001 | `LineAsset` parses the standard shader's material (`TEXTURE_NORMAL` ratio, `NORMAL_METALNESS`, `SPECULAR`, `SUPER_ROUGHNESS`), alpha (`NO_ALPHA`, `NO_BLEND`, `DITHER_ALPHA`), weather and decal rows | ✅ | 2026-10-26 |
| TASK-002 | `OverlayBuilder::ApplyLineShading`: lines blend their alpha (alpha test only with `NO_BLEND`), get their normal map, gloss, weather and decals (shared `AddDecals` with `ApplyPolygonShading`) | ✅ | 2026-10-26 |
| TASK-003 | Per-vertex ground tangents (`OverlayVertex::tx, tz`) so a curving line's normal map and normal decals follow it | ✅ | 2026-10-26 |
| TASK-004 | Light strings prefer `lib/airport/lights/slow/` (fixtures), falling back to `fast/`; inset strings (no `OFFSET`) stand at the lines' height over the pavement drape | ✅ | 2026-10-26 |

### Phase 2 - What X-Plane builds from the runway row (FND-004, 005)

| Task | Description | Completed | Date |
| -------- | --------------------- | --------- | ---------- |
| TASK-005 | `OverlayBuilder::PlaceRunwayLights`: edge (LIRL / MIRL / HIRL, 200 ft, yellow caution zone, red before a displaced threshold), threshold / end bars (inset, 10 ft), centerline (50 ft, red / white / red), touchdown zone barrettes, REILs; transparent runways get lights too | ✅ | 2026-10-26 |
| TASK-006 | `OverlayBuilder::AppendRunwayRubber` from `rubber.pol`; runway markings and rubber become parts of their own in the airport scene | ✅ | 2026-10-26 |
| TASK-007 | Approach lighting systems (row 100 approach codes 1-12: ALSF-I/II, Calvert, SSALx, MALSx, ODALS, RAIL) from `FAA_*` / `calvert_*` / `ODAL_*` / `RAIL_*` fixtures | | |
| TASK-008 | Calibrate against the probe's night shots (`*_rwy_night`): edge-light spacing and offset, threshold light count, centerline side (fitted at KORD / EIDW 28R), TDZ geometry | | |
| TASK-009 | Calibrate the rubber zone against top-down X-Plane shots of a short and a long runway (needs new probe scenarios) | | |
| TASK-010 | Light spill on the pavement at night (FND-012) | | |

### Phase 3 - Runway surfaces (FND-006, 007, 008, 009)

| Task | Description | Completed | Date |
| -------- | --------------------- | --------- | ---------- |
| TASK-011 | Blast pads / overruns from `default_runways/<family>/blastpad[_variant].pol` with its yellow chevron markings | | |
| TASK-012 | Unpaved runways as `default_runways/<family>/runway.lin`; grass / dirt / gravel / snow / lakebed taxiways and helipads from their `.pol` | | |
| TASK-013 | Runway shoulders (surface code and 12.00 width) | | |
| TASK-014 | Displaced-threshold arrows, arrowheads and threshold bar from the marking atlas | | |

### Phase 4 - Scenery-placed art (FND-010, 011)

| Task | Description | Completed | Date |
| -------- | --------------------- | --------- | ---------- |
| TASK-015 | `.lin TEXTURE_TILE` atlases with their mapping texture (lawn-track and patch strips) | | |
| TASK-016 | Find the grey band at KGON (FND-011): terrain COMPOSITE / BORDER_TEX shading of `apt_*.ter` | | |
| TASK-017 | Close-up references in X-Plane for the scenery-placed art (probe scenarios over KORD's `pavement_FX` edges, dirt, cracks and lawn tracks; SEDX side in `fx_scenarios.json` style) | | |

## 3. Alternatives

- **ALT-001**: Alpha-to-coverage or dithered alpha (X-Plane's `DITHER_ALPHA` path) for lines instead of blending. Not
  chosen: SEDX's blended drapes already split at alpha 0.9 between the deferred and transparent passes, which keeps the
  opaque paint lit by the deferred lighting and the faint grunge blended.
- **ALT-002**: Keep runway lights as apt.dat-style ribbons. Not chosen: the user wants the fixtures X-Plane shows.

## 4. Dependencies

- **DEP-001**: An X-Plane 12 install with `Resources/default scenery/sim objects` (light fixtures, default runways,
  pavement FX) and `1000 world terrain` (terrain FX).
- **DEP-002**: Reference probe shots (`X-Plane 12/Output/sedx_probe`) for calibration.

## 5. Files

- **FILE-001**: `source/SceneryEditorX/xplane/xp_assets.h/.cpp` - `.lin` material, alpha and decals.
- **FILE-002**: `source/SceneryEditorX/xplane/xp_overlay.h/.cpp` - line shading, light fixtures, runway lights and rubber.
- **FILE-003**: `source/SceneryEditorX/xplane/xp_airport_scene.cpp` - per-vertex tangents, runway layers as parts, runway light sources.
- **FILE-004**: `source/Tests/xplane_tests/OverlayTests.cpp` - parser, shading, fixture and runway layout tests.

## 6. Testing

- **TEST-001**: `XPlaneFormatTests "[overlay],[assets]"`: `.lin` material parsing, blended lines with tangents, slow / fast
  light paths, and "Overlay builder adds X-Plane's runway lights and tire rubber" (fixture names, colors per direction,
  positions, headings, inset heights, rubber tiles and columns, unpaved runways).
- **TEST-002**: Scenario captures beside the probe's references: `kgon_rwy_sunp40`, `kord_rwy_sunp40`, `eidw_rwy_sunp40`
  (rubber, centerline fixtures), `kgon_rwy_night` (edge light positions).

## 7. Risks & Assumptions

- **ASSUMPTION-001**: Runway light layout follows FAA AC 150/5340-30 (spacing, colors, TDZ geometry); X-Plane's own
  rules are in the simulator. Centerline lights sit 2 ft right of the centerline seen from end 0 (fitted: KORD and
  EIDW 28R show it south of the centerline).
- **ASSUMPTION-002**: Rubber covers 60% of the width, starts 5% of the landing length past the threshold (40-300 m),
  runs 40% of it (at most 600 m), only on paved runways of 800 m or more; fitted by eye to KGON 5 and KORD 28R.
- **RISK-001**: Fixture counts: KORD gains ~10k runway light objects (28k light fixtures in all). They are instanced
  library objects with a 500 m LOD; watch KORD / KDEN frame times.
- **RISK-002**: Blending every line moves its faint texels into the forward transparent pass (no AO / SSR there).

## 8. Related Specifications / Further Reading

- `plan/feature-xp12-environment-rendering-1.md` (decals, drape shading, materials)
- `research_notes/X Plane 12 graphics pipeline/authoring_material_specs.md` (`DITHER_ALPHA`, `.lin` directives)
- FAA AC 150/5340-30, Design and Installation Details for Airport Visual Aids
