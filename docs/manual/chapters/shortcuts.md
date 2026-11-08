# Keyboard and mouse reference

Every shortcut in Scenery Editor X, in one place. **Edit › Preferences…** shows the same lists for your keymap (the **Navigation** and **Tools** tabs), and **Help › Controls & Shortcuts…** opens them.

Shortcuts never fire while you're typing in a field. Tool and camera keys only work while the mouse is over the viewport.

## Files and history

<!-- class: shortcuts -->
| Shortcut | Action |
|---|---|
| <kbd>Ctrl+N</kbd> | New project |
| <kbd>Ctrl+O</kbd> | Open project |
| <kbd>Ctrl+S</kbd> | Save the project (or the open airport, when no project is open) |
| <kbd>Ctrl+Shift+S</kbd> | Save as |
| <kbd>Ctrl+E</kbd> | Export the scenery pack |
| <kbd>Ctrl+Shift+E</kbd> | Export again with the same settings |
| <kbd>Ctrl+Z</kbd> | Undo |
| <kbd>Ctrl+Y</kbd>, <kbd>Ctrl+Shift+Z</kbd> | Redo |
| <kbd>Alt+F4</kbd> | Quit |

## Tools

<!-- class: shortcuts -->
| Shortcut | Tool |
|---|---|
| <kbd>Q</kbd> | Select |
| <kbd>W</kbd> | Move |
| <kbd>E</kbd> | Rotate |
| <kbd>R</kbd> | Scale |
| <kbd>V</kbd> | Nodes |
| <kbd>B</kbd> | Facade |

## Selection

<!-- class: shortcuts -->
| Shortcut | Action |
|---|---|
| Click | Select what's under the cursor |
| <kbd>Shift</kbd> + click | Add to the selection |
| <kbd>Ctrl</kbd> + click | Toggle in or out of the selection |
| Drag on empty space | Box select |
| <kbd>Esc</kbd> | Deselect (or cancel what's in progress) |
| <kbd>Del</kbd> | Delete the selection (selected nodes, with the Nodes tool) |
| <kbd>Ctrl+D</kbd> | Duplicate the selected project items |
| <kbd>X</kbd> | Gizmo: local or world axes |
| <kbd>Ctrl</kbd> while dragging | Snap (or don't, when snapping is on) |
| Right-click (quick) | Quad menu |
| Hold <kbd>Space</kbd> | Hotbox |

## Nodes tool

<!-- class: shortcuts -->
| Shortcut | Action |
|---|---|
| Click a node, <kbd>Shift</kbd> / <kbd>Ctrl</kbd> + click | Select, add, toggle nodes |
| Click a curve | Select the two nodes of the segment |
| Drag a node | Move the selected nodes (snaps onto nearby nodes) |
| Drag a curve | Bend it through the cursor |
| Drag a handle grip | Change the handle; <kbd>Shift</kbd>: move one handle on its own |
| Double-click a curve | Insert a node (a control point on a B-spline) |
| <kbd>Ctrl</kbd> + click past a selected line end | Extend the line |
| <kbd>Ctrl</kbd> + click the other end | Close the line |
| <kbd>Ctrl+A</kbd> | Select every node |
| <kbd>Shift+C</kbd> | Corner node |
| <kbd>Shift+S</kbd> | Smooth node |
| <kbd>Shift+Y</kbd> | Symmetric node |
| <kbd>Shift+F</kbd> | Free node |
| <kbd>Shift+L</kbd> | Straighten the selected segments |
| <kbd>Shift+B</kbd> | Edit as a B-spline / back to bezier |
| <kbd>Del</kbd> | Delete the selected nodes |

## Facade tool

<!-- class: shortcuts -->
| Shortcut | Action |
|---|---|
| <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>4</kbd> | Polygon, Rectangle, Circle, Line mode |
| Click | Add a corner (or the next point of the mode) |
| Double-click, <kbd>Enter</kbd> | Finish (close a polygon, end a line) |
| Right-click | Finish a line |
| <kbd>Shift</kbd> while drawing | 45° steps; a square in Rectangle mode |
| <kbd>Backspace</kbd> | Take back the last corner |
| <kbd>Esc</kbd> | Cancel the facade being drawn |
| Click a wall of a selected facade | Pick the wall (<kbd>Shift</kbd> adds) |

## Camera: every keymap

<!-- class: shortcuts -->
| Shortcut | Action |
|---|---|
| Right mouse drag | Look around |
| Hold right mouse + <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> | Fly |
| Hold right mouse + <kbd>Q</kbd> / <kbd>E</kbd> | Down / up |
| Add <kbd>Shift</kbd> / <kbd>Ctrl</kbd> | Faster / slower |
| Arrow keys, <kbd>Page Up</kbd> / <kbd>Page Down</kbd> | Move the camera |
| <kbd>Numpad 7</kbd> / <kbd>Ctrl+Numpad 7</kbd> | Top / bottom view |
| <kbd>Numpad 1</kbd> / <kbd>Ctrl+Numpad 1</kbd> | Front / back view |
| <kbd>Numpad 3</kbd> / <kbd>Ctrl+Numpad 3</kbd> | Right / left view |
| <kbd>Numpad 9</kbd> | Opposite side |
| <kbd>Numpad 4</kbd> / <kbd>Numpad 6</kbd> | Orbit around in 15° steps |
| <kbd>Numpad 8</kbd> / <kbd>Numpad 2</kbd> | Orbit over in 15° steps |
| <kbd>Numpad 5</kbd> | Perspective / orthographic |
| <kbd>Numpad .</kbd> | Frame the selection |
| <kbd>Home</kbd> | Frame everything |

## Camera: by keymap

<!-- class: shortcuts -->
| Action | Blender | 3ds Max | Maya |
|---|---|---|---|
| Orbit | Middle drag; <kbd>Alt</kbd> + left drag | <kbd>Alt</kbd> + middle drag | <kbd>Alt</kbd> + left drag |
| Pan | <kbd>Shift</kbd> + middle drag; <kbd>Shift+Alt</kbd> + left drag | Middle drag | <kbd>Alt</kbd> + middle drag; middle drag |
| Zoom | Wheel; <kbd>Ctrl</kbd> + middle drag; <kbd>Ctrl+Alt</kbd> + left drag | Wheel; <kbd>Ctrl+Alt</kbd> + middle drag | Wheel; <kbd>Alt</kbd> + right drag |
| Frame selection | <kbd>F</kbd> | <kbd>Z</kbd> (everything when nothing is selected) | <kbd>F</kbd> |
| Frame everything | <kbd>Home</kbd> | <kbd>Home</kbd> | <kbd>A</kbd>, <kbd>Home</kbd> |
| Views | Numpad | Numpad; <kbd>T</kbd> top, <kbd>F</kbd> front, <kbd>L</kbd> left | Numpad |
| Perspective / orthographic | <kbd>Numpad 5</kbd> | <kbd>P</kbd> / <kbd>U</kbd>, <kbd>Numpad 5</kbd> | <kbd>Numpad 5</kbd> |

## Panels

<!-- class: shortcuts -->
| Shortcut | Where | Action |
|---|---|---|
| <kbd>/</kbd> | This manual's website | Search the manual |
| <kbd>Ctrl</kbd> + wheel | X-PLANE LIBRARY grid | Thumbnail size |
| <kbd>Ctrl</kbd> + wheel | FACADE EDITOR texture | Zoom the texture |
| <kbd>Alt</kbd> + drag an edge | FACADE EDITOR texture | Place the edge off the pixel grid |
| <kbd>Ctrl+Z</kbd> / <kbd>Ctrl+Y</kbd> | FACADE EDITOR (focused) | Undo / redo facade edits |
| <kbd>Enter</kbd> | Text fields | Apply the value |
| <kbd>Esc</kbd> | Dialogs | Cancel or close |
