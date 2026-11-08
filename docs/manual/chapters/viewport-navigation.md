# Navigating the viewport

Scenery is kilometers wide and its details are centimeters across, so moving around quickly matters. The viewport camera follows the conventions of Blender, 3ds Max or Maya, whichever you chose, and adds numpad views, orthographic projection, a fly mode and a second viewport.

## Orbit and fly

The camera has two modes. Switch between them with the buttons in the tool strip or the viewport toolbar, or in the **Viewport** menu.

- **Orbit** (the default) turns the camera around a **pivot**: a point in the scene, usually what you're working on. Framing a selection moves the pivot to it. This is the mode for editing.
- **Fly** turns the camera in place and moves it like a drone. The wheel moves the camera forward and back. Use it to look at the scenery from the ground, the way a pilot will see it.

In either mode, holding the right mouse button looks around and lets the movement keys fly the camera.

## Mouse controls by keymap

The orbit, pan and zoom controls depend on the keymap you chose on the first launch (change it in **Edit › Preferences…**, **Navigation** tab).

<!-- class: shortcuts -->
| Action | Blender | 3ds Max | Maya |
|---|---|---|---|
| **Orbit** | Middle drag, or <kbd>Alt</kbd> + left drag | <kbd>Alt</kbd> + middle drag | <kbd>Alt</kbd> + left drag (tumble) |
| **Pan** | <kbd>Shift</kbd> + middle drag, or <kbd>Shift+Alt</kbd> + left drag | Middle drag | <kbd>Alt</kbd> + middle drag, or middle drag (track) |
| **Zoom** | Wheel, <kbd>Ctrl</kbd> + middle drag, or <kbd>Ctrl+Alt</kbd> + left drag | Wheel, or <kbd>Ctrl+Alt</kbd> + middle drag | Wheel, or <kbd>Alt</kbd> + right drag (dolly) |
| **Frame selection** | <kbd>F</kbd> or <kbd>Numpad .</kbd> | <kbd>Z</kbd> or <kbd>Numpad .</kbd> (everything when nothing is selected) | <kbd>F</kbd> or <kbd>Numpad .</kbd> |
| **Frame everything** | <kbd>Home</kbd> | <kbd>Home</kbd> | <kbd>A</kbd> or <kbd>Home</kbd> |
| **Views** | Numpad (below) | Numpad, or <kbd>T</kbd> top, <kbd>F</kbd> front, <kbd>L</kbd> left | Numpad (below) |
| **Perspective / orthographic** | <kbd>Numpad 5</kbd> | <kbd>Numpad 5</kbd>, or <kbd>P</kbd> / <kbd>U</kbd> | <kbd>Numpad 5</kbd> |

Orbiting follows the 3D-package convention: the scene turns with the mouse. If you prefer the camera to move with the mouse instead, tick **Invert orbit horizontally** and **Invert orbit vertically** in the preferences.

> [!NOTE]
> Holding <kbd>Alt</kbd>, or the right or middle mouse button, always gives the mouse to the camera. The editing tools only react to the left button without <kbd>Alt</kbd>, so you can navigate in the middle of an edit without losing your selection.

## Controls in every keymap

<!-- class: shortcuts -->
| Action | Control |
|---|---|
| Look around | Right mouse drag |
| Fly | Hold the right mouse button + <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>; <kbd>Q</kbd> / <kbd>E</kbd> down / up |
| Fly faster / slower | Add <kbd>Shift</kbd> / <kbd>Ctrl</kbd> |
| Move without the mouse | Arrow keys, <kbd>Page Up</kbd> / <kbd>Page Down</kbd> |
| Top / bottom view | <kbd>Numpad 7</kbd> / <kbd>Ctrl+Numpad 7</kbd> |
| Front / back view | <kbd>Numpad 1</kbd> / <kbd>Ctrl+Numpad 1</kbd> |
| Right / left view | <kbd>Numpad 3</kbd> / <kbd>Ctrl+Numpad 3</kbd> |
| Look from the opposite side | <kbd>Numpad 9</kbd> |
| Orbit in 15° steps | <kbd>Numpad 4</kbd> / <kbd>Numpad 6</kbd> around, <kbd>Numpad 8</kbd> / <kbd>Numpad 2</kbd> over |
| Perspective / orthographic | <kbd>Numpad 5</kbd> |
| Frame the selection | <kbd>Numpad .</kbd> |
| Frame everything | <kbd>Home</kbd> |

Movement speed scales with the scene: the camera moves faster the higher it is and the farther away the pivot, so the same keys cross an airport or nudge along a taxiway line.

The camera keys only work while the mouse is over the viewport, and never while you're typing in a field. While the right mouse button is held, <kbd>Q</kbd> <kbd>W</kbd> <kbd>E</kbd> fly the camera instead of picking tools.

## Views and orthographic projection

The **view presets** look along the compass directions:

| View | Looks | Key |
|---|---|---|
| **Top** | Straight down, north up | <kbd>Numpad 7</kbd> |
| **Bottom** | Straight up | <kbd>Ctrl+Numpad 7</kbd> |
| **Front** | North | <kbd>Numpad 1</kbd> |
| **Back** | South | <kbd>Ctrl+Numpad 1</kbd> |
| **Right** | West, from the east side | <kbd>Numpad 3</kbd> |
| **Left** | East, from the west side | <kbd>Ctrl+Numpad 3</kbd> |

They are also in the **Viewport** menu and in the **View** menu on the viewport toolbar, for keyboards without a keypad.

**Orthographic** projection removes perspective: parallel lines stay parallel and distances read the same across the view. It's the projection for lining up pavement, measuring and drawing footprints, and with the Top view it gives you a map. Switching projection keeps what's at the pivot the same size, so you don't lose your place.

By default the camera glides to a new view instead of jumping. Turn off **Smooth view changes** in the preferences if you'd rather it jumped.

## The CAMERA panel

The **CAMERA** panel shows the camera's state and lets you type exact values:

- **Navigation:** the mode, projection and view; the orbit center, distance, position, and heading / pitch; a **Reset** back to the startup view.
- **Lens:** the field of view.
- **Exposure:** aperture, shutter and ISO, used when the exposure is set to manual (see [Exposure](environment.md#exposure)).

The **Reset camera** button at the right of the viewport toolbar also returns to the startup view.

## A second viewport

![VIEWPORT 2 floating over the left of the workspace with a top orthographic view of Portland's runways, while the main viewport looks across the airfield in perspective.](../images/viewport2.png "Window › New Viewport: VIEWPORT 2 (left) opens as a top orthographic view. Drag its tab to dock it, or onto another monitor.")

**Window › New Viewport** opens **VIEWPORT 2**, a second view of the same scene with its own camera. It starts as a top-down orthographic view of the airport: a live map while you work in the main view.

- It has its own camera, projection and view presets, and uses your keymap, the numpad views and <kbd>Home</kbd> while the mouse is over it.
- Its toolbar has **Top**, **Front** and **Right**, the orthographic toggle, **Frame the airport**, and **Match the main viewport's camera**.
- Dock it anywhere like a panel, or drag its tab out of the window and onto another monitor.
- It doesn't come back by itself: every launch starts with the main viewport only, and **Window › Reset Layout** closes it. Open it again with **Window › New Viewport** (or its button on the panel strip). It reopens where you last put it, on the same monitor, with its camera as you left it.
- Until you open it, and once you close it, the editor doesn't render it at all, so the main viewport is as fast as it would be without it.

VIEWPORT 2 is for looking: selecting and editing happen in the main viewport, and the second view shows the results as you work.

## Comparing with X-Plane

The **Reference image** menu at the right of the viewport toolbar loads a PNG screenshot from X-Plane 12 and lays it over the view, to compare the editor's rendering with the sim's: **Split** (half and half), **Difference** (with an adjustable gain), or **Flicker** (once a second). **Clear** removes it. [Time, weather and the X-Plane 12 look](environment.md#comparing-with-an-x-plane-screenshot) explains how to set up a matching view.
