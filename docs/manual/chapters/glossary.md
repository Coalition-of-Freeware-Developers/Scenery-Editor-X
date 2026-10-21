# Glossary

The X-Plane and editor terms used in this manual.

| Term | Meaning |
|---|---|
| **Airport ID** | An airport's identifier in `apt.dat`, up to seven characters: its ICAO code when it has one (`KSEA`), otherwise a code the Scenery Gateway assigns. Unique across X-Plane. |
| **apt.dat** | The text file that describes airports: runways, pavement, lines, lights, signs, startup locations, taxi routes, frequencies. Scenery Editor X reads and writes version 12.00. A pack's is at `Earth nav data/apt.dat`. |
| **B-spline** | A way of editing a curve through a *control polygon*: moving a control point changes the curve smoothly and only nearby. Stored as ordinary bezier nodes. |
| **Bezier node, handle** | A point on a shape, and the two lines from it that set how the curve leaves and arrives. |
| **Corner, smooth, symmetric, free** | The four node types: no handles; opposite handles of their own lengths; mirror-image handles; independent handles. |
| **Custom Scenery** | The folder in X-Plane that holds add-on scenery packs. |
| **Datum, reference point** | The point a project or airport is built around: its latitude, longitude and elevation. |
| **Dataref** | A named value in X-Plane (`sim/time/local_time_sec`) that animated objects follow. The editor emulates the ones scenery uses. |
| **Draped** | Laid on the ground surface, like a decal: polygons, lines, sidewalks, pavement. |
| **DSF** | Distribution Scenery Format: X-Plane's binary files for scenery placed in a 1°×1° tile (objects, facades, forests, lines, polygons, roads). |
| **Exclusion zone** | An area where X-Plane leaves out its default scenery (objects, facades, forests…) so a pack's own can go there. |
| **Export (library)** | A line in `library.txt` that publishes a file under a virtual path. |
| **Facade** | A building, fence, wall, sidewalk or curb X-Plane generates along a footprint, from a `.fac` file. *Type 1* facades are built from texture rectangles; *type 2* from 3D segments. |
| **Floor** | In a type 2 facade, one of the heights it can be built at, each with its own roof. |
| **Forest** | Trees scattered over an area, along a line or at points, from a `.for` file. |
| **Front wall** | Wall 1 of a facade, the first side drawn. Wall headings and the roof are measured from it. |
| **Gizmo** | The arrows, planes and rings drawn on a selection by the Move, Rotate and Scale tools. |
| **Global Airports** | The airports X-Plane ships (from the Scenery Gateway), in `Global Scenery`. Read-only. |
| **Hotbox** | The menus that open around the cursor while you hold <kbd>Space</kbd> over the viewport. |
| **Junction, road node** | Where roads start, end and meet. |
| **Keymap** | The set of mouse and key controls for the viewport camera: Blender, 3ds Max or Maya. |
| **Level of detail (LOD)** | Simpler versions of an object drawn when it's far away. |
| **Library** | A scenery pack that publishes art through `library.txt` for other packs to use. |
| **library.txt** | The file in a library that lists its exports. |
| **Line (.lin)** | Paint or a strip of texture drawn along a path: road markings, taxi lines, curbs. |
| **Object (.obj)** | A 3D model in X-Plane's OBJ8 format, placed at a point with a heading. |
| **Object string (.str)** | Objects repeated along a path: lights, posts, cones. |
| **Orthographic** | A projection without perspective: parallel lines stay parallel, good for lining things up from above. |
| **Orthophoto** | A draped polygon with its own texture coordinates: an aerial photo laid on the ground. |
| **Overlay** | Scenery a DSF adds on top of the terrain: objects, facades, forests, lines, polygons, roads. |
| **Pivot** | The point the camera orbits around, or the point a selection turns and scales about. |
| **Polygon (.pol)** | A texture draped over an area: grass, concrete, markings. |
| **Project** | The file you work in (`.edX`, `.sex` or `earth.wed.xml`), which exports to a scenery pack. |
| **Project item** | Anything a project places in DSF scenery: an object, facade, forest, line, polygon, string, road. |
| **Quad menu** | The four-part menu that opens where you right-click in the viewport. |
| **Road network (.net)** | The file that defines road, railway and power-line types and how they're drawn. |
| **Scenery Gateway** | X-Plane's shared online database of airports, at gateway.x-plane.com. |
| **Scenery pack** | A folder X-Plane loads scenery from: `Earth nav data` (apt.dat, DSF tiles), art, and optionally `library.txt`. |
| **scenery_packs.ini** | The file in `Custom Scenery` that lists packs in priority order, first highest. |
| **Segment, spelling** | In a type 2 facade, a 3D piece of wall, and a sequence of segments that fills a wall of a given length. |
| **Snapping** | Moving, turning or scaling in fixed steps. |
| **Tile** | A 1°×1° area of the world; each has its own DSF file. |
| **Virtual path** | The name an asset is published under (`lib/airport/…`), independent of its real file. |
| **Wall type, wall rule** | One of a facade's looks for a wall, and the rule (width, heading) that decides which walls get it. |
| **WED, WorldEditor** | Laminar Research's scenery editor. Its projects are `earth.wed.xml` files. |
