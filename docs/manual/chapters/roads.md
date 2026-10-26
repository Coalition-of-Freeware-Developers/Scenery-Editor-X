# Road networks

X-Plane draws roads, railways and power lines from **road networks**: a `.net` file (most scenery uses X-Plane's `lib/g10/roads.net`) that defines the road types, and DSF data that lays roads between **junctions**. Scenery Editor X reads, draws and edits road networks, and writes them back to DSF and WorldEditor projects exactly as X-Plane and WorldEditor expect.

## Roads and junctions

A road network in a project is made of two kinds of item:

- **Road nodes (junctions):** the points where roads start, end and meet. A junction draws nothing itself.
- **Roads:** each runs from one junction to another along a bezier chain, and has a **road type** from its network.

Roads that share a junction are connected: X-Plane builds the intersection there.

Roads come into a project from a WorldEditor project, where they are `WED_RoadNode` and `WED_RoadEdge` objects. In this alpha there is no tool yet to start a new network from scratch: lay out the network in WorldEditor, import it, and refine it here.

## How roads are drawn

The viewport draws each road as its road type's strips from the `.net` file: the lanes, shoulders and markings with their own textures, mitered at the bends. Only these flat strips are drawn so far; the 3D objects a network places along roads (bridges, street lights, signs) aren't drawn yet.

## Editing roads

Select a road, then press <kbd>V</kbd> for the **Nodes** tool:

- drag its shape points and handles, bend its curves, insert and delete shape points, like any other shape (see [Editing shapes and paths](node-editing.md));
- a road's two ends are its junctions: they can't be deleted, and a road can't be extended past a junction other roads share.

Select a **junction** (click it in the PROJECT panel's hierarchy, since it isn't drawn) and move it with the Nodes tool: every road that starts or ends there follows, keeping its handles, in a single undo step.

### Road properties

The **NODES** section of PROPERTIES shows, for a road:

- **Network:** the `.net` file it belongs to;
- **Road type:** the network's road types (its *virtual types*), which set the subtype X-Plane draws the road with. If the network can't be found, type the subtype number (0–255) instead;
- **Levels:** the height level at the start and end: 0 on the ground, 1 and 2 raised (bridges and overpasses).

For a junction it shows **Roads here**: how many roads start or end at it, all of which follow it when it moves.

## Exporting roads

When you export, roads are written to the DSF tiles as X-Plane's road network data: each road becomes a chain of segments between numbered junctions, with its subtype and levels, and curves written the way WorldEditor writes them. Exporting to `earth.wed.xml` writes them back as WorldEditor road nodes and edges.

The project's **VALIDATION** list reports road ends that don't sit on their junctions, so a pack never ships disconnected roads.
