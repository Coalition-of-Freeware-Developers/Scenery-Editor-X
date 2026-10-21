# Menu reference

Every item in the menu bar, in order. Items that need something (an open project, a selection) are greyed out until it's there. The same menus open at the cursor when you hold <kbd>Space</kbd> over the viewport.

## File

| Item | Shortcut | What it does |
|---|---|---|
| New Project… | <kbd>Ctrl+N</kbd> | A new scenery, airport or library project, created in Custom Scenery or a folder you choose. [Projects](projects.md#kinds-of-project) |
| New Airport in Project… | | Adds a new airport (ICAO code, name, reference point) to the open project. |
| Open Project… | <kbd>Ctrl+O</kbd> | Opens a `.edX`, `.sex` or `earth.wed.xml` project. |
| Open Recent › | | The projects you opened last. |
| Save Project | <kbd>Ctrl+S</kbd> | Saves the project, its airport edits included. |
| Save Project As… | <kbd>Ctrl+Shift+S</kbd> | Saves a copy as `.edX`, `.sex` or `earth.wed.xml` and continues with it. |
| Close Project | | Closes the project, asking about unsaved changes. |
| Export Scenery Pack… | <kbd>Ctrl+E</kbd> | Writes `Earth nav data/apt.dat`, the DSF tiles and the pack's art. [Exporting](import-export.md#exporting-a-scenery-pack) |
| Export Again | <kbd>Ctrl+Shift+E</kbd> | Exports with the last settings, without the dialog. |
| Import › WorldEditor Project (earth.wed.xml)… | | Imports a WorldEditor project as a new project. |
| Import › Airports (apt.dat)… | | Adds an `apt.dat` file's airports to the project. |
| Import › Library (library.txt)… | | Reads a `library.txt` into the project's library. |
| Export › X-Plane Scenery Pack… | <kbd>Ctrl+E</kbd> | Same as Export Scenery Pack…. |
| Export › WorldEditor Project (earth.wed.xml)… | | Writes the project for WorldEditor. |
| Export › Airports (apt.dat)… | | Writes the project's airports to an `apt.dat`. |
| Export › Library (library.txt)… | | Writes the project's `library.txt`. |
| Export › Binary Copy (.sex)… | | Writes a binary copy of the project. |
| Show Project Folder | | Opens the project's folder in Explorer. |
| Exit | <kbd>Alt+F4</kbd> | Quits, asking about unsaved changes. |

## Edit

| Item | Shortcut | What it does |
|---|---|---|
| Undo *step* | <kbd>Ctrl+Z</kbd> | Undoes the named step. |
| Redo *step* | <kbd>Ctrl+Y</kbd> | Redoes the named step. |
| Delete | <kbd>Del</kbd> | Removes the selection. |
| Duplicate | <kbd>Ctrl+D</kbd> | Copies the selected project items 3 m south-east. |
| Deselect | <kbd>Esc</kbd> | Clears the selection. |
| Frame Selection | <kbd>F</kbd> (<kbd>Z</kbd> in 3ds Max) | Turns the camera to the selection. |
| Hide Selection | | Hidden items aren't drawn or exported. |
| Show All Hidden | | Shows every hidden project item. |
| Lock / Unlock | | Locked items can't be moved or edited. |
| Select, Move, Rotate, Scale, Nodes, Facade | <kbd>Q</kbd> <kbd>W</kbd> <kbd>E</kbd> <kbd>R</kbd> <kbd>V</kbd> <kbd>B</kbd> | Picks the tool. |
| Preferences… | | Navigation keymap and camera options; the tool shortcuts. |

## Scene

| Item | What it does |
|---|---|
| Add Empty Entity | Adds an empty entity to the scene (shown in the OUTLINER). |

## X-Plane

| Item | Shortcut | What it does |
|---|---|---|
| Import Object (.obj)… | | Places X-Plane object files from disk in the scene. |
| Airports | | Shows the AIRPORTS panel. |
| Open Airport (apt.dat)… | | Opens an airport file from anywhere. |
| Save Airport | <kbd>Ctrl+S</kbd> | Saves the open airport's `apt.dat` (when no project is open). |
| Save Airport As… | <kbd>Ctrl+Shift+S</kbd> | Saves the airport into another scenery pack. |
| Frame Airport | | Frames the open airport. |
| Close Airport | | Unloads the open airport. |
| Rescan Airports | | Indexes the install's `apt.dat` files again. |
| Library Browser | | Shows the X-PLANE LIBRARY panel. |
| Set X-Plane 12 Folder… | | Chooses the X-Plane folder. |
| Track Library Folder… | | Adds a library folder outside X-Plane to the scan. |
| Rescan Libraries | | Reads every library again. |

The menu ends with the X-Plane folder in use, or what's wrong with it.

## Facade

| Item | Shortcut | What it does |
|---|---|---|
| New Building Facade… | | A type 1 facade from a texture, in the FACADE EDITOR. |
| New Sidewalk / Curb Facade | | A type 2 ground facade from a cross-section. |
| Edit Facade | | The selected facade in the FACADE EDITOR (a copy, for library facades). |
| Facade Editor | | Shows the FACADE EDITOR. |
| Facade Tool | <kbd>B</kbd> | Draw facades and edit their walls. |
| Draw Polygon / Rectangle / Circle / Line | <kbd>1</kbd> – <kbd>4</kbd> | Draw a new facade in that mode with the armed facade. |
| Library › | | Arm a facade from the library, grouped by folder (`airport`, `g10`…). |
| Recent › | | Arm a facade you used recently. |
| Wall Type › | | Set the wall type of the picked walls (or every wall) of the selected facades. |
| Height › | | Set the selected facades' height. |
| Replace With › | | Another facade from the same library folder; walls keep their types by name. |
| Pick Walls | | On: walls are built with the types set here. Off: X-Plane picks them by its rules. |
| Make Picked Wall the Front | | The picked wall becomes wall 1. |
| Subdivide Curved Walls | | Cuts curved walls into straight 0.5–0.8 m pieces (the 3D sidewalk recipe). |
| Square Corners | | Turns nearly square corners into right angles. |
| Reverse Direction | | Walls face the other way. |
| Draw With This Facade | | Arms the selected facade in the facade tool. |
| Inspect Facade | | Opens it in the FACADES inspector. |
| Wall Labels | | Numbers, types and lengths of the selected facades' walls, in the viewport. |
| Reload Facades | | Reads the project's facades and other art again after editing them on disk. |

## Window

| Item | What it does |
|---|---|
| New Viewport | Opens VIEWPORT 2: its own camera, a top view to start with. |
| *each panel* | Shows or hides it. |
| Reset Layout | Puts every panel back in the default layout. |
| Dear ImGui › Metrics / Debugger, Style Editor, Demo Window | The interface library's own tools (for developers). |

## Viewport

| Item | Shortcut | What it does |
|---|---|---|
| Orbit Navigation | | The camera turns around a pivot. |
| Fly Navigation | | The camera turns in place and flies. |
| Perspective / Orthographic | <kbd>Numpad 5</kbd> | Projection. |
| Top, Bottom | <kbd>Numpad 7</kbd>, <kbd>Ctrl+Numpad 7</kbd> | View presets. |
| Front, Back | <kbd>Numpad 1</kbd>, <kbd>Ctrl+Numpad 1</kbd> | |
| Right, Left | <kbd>Numpad 3</kbd>, <kbd>Ctrl+Numpad 3</kbd> | |
| Frame Selection | <kbd>Numpad .</kbd> | |
| Frame All | <kbd>Home</kbd> | |
| Reset Camera | | Back to the startup view. |
| Navigation Keymap… | | Opens the preferences' Navigation tab. |

## Help

| Item | What it does |
|---|---|
| About Scenery Editor X | The version and build configuration, the license, and the Dear ImGui version. Quote the version when you report a problem. |
| Choose a Navigation Keymap… | The first-launch keymap window. |
| Controls & Shortcuts… | The preferences, with the bindings for your keymap and the tool shortcuts. |
