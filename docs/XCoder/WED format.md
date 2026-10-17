# Structure
WED.xml is made entirely of "object" elements. They are stored in doc.objects


# Referencing
object elements are identified by ids. ids are not required to be contigous, just unique.

# object properties
- class: the type of object this is
- id: the id
- parent_id: the id of the parent

# object children
- children tag: This contains a list of children objects, in the form <children><child id="123"/></children>
- hierarchy: This contains the information to show in the list panel on the right, i.e. <hierarchy name="Node 1" locked="0" hidden="0"/>

# Polygonal Data
When a given WED type stores polygon data, it does so with "ring". Each ring is simply a child of the polygon. The first ring is the outer boundary, rest are holes. Holes are only permitted on draped polygons and forests

# Overlay object types
- WED_Root:
	- Top level container. Never seen by the user. Children are WED_Select, WED_KeyObjects, and WED_Group world (the top level folder for things placed in WED)
- WED_Group:
	- Provides a group in the hierarchy. Referenced children are various objects
## Node objects
- WED_SimpleBezierBoundaryNode:
	- Bezeir node. Contains child <point latitude="28.1" longitude="-177.3" split="0" ctrl_latitude_lo="0.0" ctrl_longitude_lo="0.0" ctrl_latitude_hi="0.0" ctrl_longitude_hi="0.0"/>
- WED_SimpleBoundaryNode:
	- Same as WED_SimpleBezierBoundaryNode, without the control points
- WED_FacadeNode:
	- Same as WED_SimpleBezierBoundaryNode but specifies param with <facade_node wall_type="Wall 1"/>
		- wall_type is really just a string that is referenced against the a wall name specified in a .fac, despite DSFs using an index system
- WED_TextureBezierNode
	- Bezeir node with UVs. Same as SimpleBezeirBoundaryNode but also has <texture_node s="0.1836" t="0.5010" sc_lo="0.0" tc_lo="0.0" sc_hi="0.0" tc_hi="0.0"/>
	- NOTE: While WED will allow it, curved polygons and polygons with UVs are mutually exclusive in DSFs
- WED_TextureNode
	- Same as the bezeir node, without the control points
- WED_AirportNode
	- Specifies a node, but also airport metadata
	- Location and control points specified with <point latitude="28.197976734" longitude="-177.388809586" split="0" ctrl_latitude_lo="0.0" ctrl_longitude_lo="0.0" ctrl_latitude_hi="0.0" ctrl_longitude_hi="0.0"/>
	- Marking metadata specified with markings tag
		- markings tag contains marking children. These children can specify a marking AND/OR a lighting value. Only *zero-one* of each may be provided, however the format does not indicate this, it is up to the reader/writer to interpret the provided tags appropriately. See line types and light types for valid types.
## Ring objects
- WED_Ring:
	- A ring for polygonal data. Simply contains node children. Child nodes must be all the same type.
- WED_ForestRing:
	- Same as WED_Ring but used for forests
- WED_FacadeRing:
	- Same as WED_Ring but for facades
## Placement objects
- WED_PolygonPlacement
	- Polygon. Referenced are WED_Ring.
	- Params specified with <polygon_placement heading="0.0" resource="MyAsset.pol"/>
		- Heading 0-360 degrees CW
- WED_DrapedOrthophoto:
	- Polygon with explicit UVs. Referenced children are WED_Ring.
	- Params specified with <draped_orthophoto heading="0.0" resource="MyAsset.pol" decal="" tex_top="1.0000" tex_bottom="0.0" tex_left="0.0" tex_right="1.0000"/>
		- decal is always blank, tex_ are for WED internal UV mapping purposes but are super buggy anyway - leave them default
- WED_ObjPlacement:
	- Object. Can be .obj or .agp.
	- Locations specified with <point latitude="28.198227065" longitude="-177.391740736" heading="0.0"/>
	- Resource and msl specified with <obj_placement custom_msl="0" msl="0.0" resource="Both/PositiveObj2.obj" show_level="1 Default"/>
		- custom_msl is 0 for on ground, 1 for custom msl, OR "set_AGL" when in AGL mode. When in AGL mode, msl is actually agl (both are meters)
		- show_level can be "1 Default" "2 A lot" "3 Tons" "4 Mega tons" "5 Too many" or "6 Totally insane"
- WED_ForestPlacement
	- Object. Children references one or more WED_ForestRings
	- Params specified with <forest_placement density="1.0" resource="myResource.for" closed="Linear"/>
		- Closed is either "Area" for a zone forest, "Linear" for a line of trees, or "Points" for one tree per node
- WED_StringPlacement:
	- Object String (a line of repeating objects). References children are a list of nodes, like a WED_Ring, but skipping the intermediate data structure.
	- Params specified with <string_placement resource="MyString.str" spacing="30.00" closed="0"/>
		- Spacing is meters, close is 0 for open, 1 for closed
- WED_LinePlacement:
	- Places a line. References children are a list of nodes, like a WED_Ring, but skipping the intermediate data structure.
	- Params specified with <line_placement closed="0" resource="AEP/PrivateAssets/Taxi_Skids_E.lin"/>
		- close is 0 for open, 1 for closed
- WED_ExclusionZone:
	- Rectangular exclusion zone. Two referenced children, a start simple boundary node and an end simple boundary node
	- What is excluded is specified with <excluzions> tag with <exclusion value="Type"/> child nodes.
		Valid types are "Object", "Facades", "Forests", "Beaches", "Road Networks", "Lines", "Draped Polygons", and "String"
- WED_ExclusionPoly:
	- Same as WED_ExclusionZone but uses a ring for a child, allowing for polygonal data vs a square
## Airport objects
- WED_Airport:
	- Also functions as a WED_Group
	- Params controlled with <airport kind="Airport" elevation="0.0" has_atc="1" icao="ICAO" always_flatten="0" drive_on_left="0" scenery_id="-1"/>
		- Kind can be Airport Heliport or Seaport. elevation is meters, has_atc always_flatten and drive_on_left are 0 false 1 true. scenery_id is the gateway ID I believe
	- Metadata controllwed with <meta_data> tag with <meta_data_entry pair="icao_code,MAPT"/> children (pair being the "metadata key,value")
- WED_Runway
	- Runway. Referenced children are WED_RunwayNode
	- <line width="50.0"> specifies the width of the runway
	- Other params are specified with <runway surface="Asphalt Worn" shoulder="None" shoulder_width="0.0" roughness="0.0" center_lites="0" edge_lites="None" distance_signs="0" displaced1="0.0" blastpad1="0.0" markings1="None" app_lites1="SALS" TDZL1="0" REIL1="None" displaced2="0.0" blastpad2="0.0" markings2="None" app_lites2="SALS" TDZL2="0" REIL2="None"/>
		- Distances are in meters, light types come from a list (coming soon), surface is a valid surface string name
- WED_Sealane:
	- Creates a sealane. Referenced children are WED_RunwayNode
	- <sealane has_buoys="1"/> Specifies whether there is a buoys
	- <line width="50.0"> specifies the width of the sealane
- WED_Helipad:
	- Creates a helipad, no referenced children
	- Location heading width and length specified with <point latitude="28.196992879" longitude="-177.389098790" heading="0.0" width="50.0" length="50.0"/>
	- Params specified  with <helipad surface="Asphalt L" markings="Default" shoulder="None" roughness="0.0" lights="None"/>
		- Shoulder and surface are a valid surface string name
- WED_AirportSign
	- Creates a taxisign. No referenced children
	- Lat/lon/heading specified with <point latitude="28.197087514" longitude="-177.388658566" heading="163.38"/>
	- NOTE: hierarchy name attribute specifies the text. Uses taxisign text format (see taxisign text section)
- WED_Windsock
	- Creates a windsock. No referenced children
	- Specify lat/lon with <point latitude="28.197098984" longitude="-177.388496719"/>
	- Specify lit with <windsock lit="0"/>
- WED_AirportBeacon
	- Creates a beacon. No referenced children
- WED_RampPosition
	- Creates a ramp start. No referenced children.
	- Specify lat/lon/heading with <point latitude="28.197345606" longitude="-177.387965858" heading="339.87"/>
	- Specify properties with <ramp_start type="Tie-Down" traffic="8" width="A" ramp_op_type="None" airlines=""/>
		- width actually specifies size Size is A-F. A smallest, F largest, rough size estimates are:
			- A=172
			- B=CRJ700
			- C=737
			- D=MD11
			- E=747
			- F=A380
		- Types are:
			- Misc (default if not specified)
			- Tie-Down
			- Hangar
			- Gate
		- ramp_op_type is the operation type:
			- None (default)
			- General Aviation
			- Airline
			- Cargo
			- Military
		- airlines:
			- space seperated list of airline ICAO for valid operators
- WED_Taxiway
	- Creates a taxiway, referenced child is a WED_AirportChain
		- A note on WED_AirportChain: Even though it is a taxiway, it *can* have markings and lights along the edge
	- Specify params with <taxiway surface="Gravel" roughness="0.0" heading="0.0"/>
		- Roughness is unused, heading is 0-360 CW, surface is a valid Surface String Name (see Surface String Names section)
- WED_AirportBoundary
	- Creates an airport boundary. Referenced child is WED_AirportChain
- WED_AirportChain
	- Creates an polygonal airport element. Can be used in WED_Taxiway or WED_AirportBoundary. When used standalone it is simply a polygonal data element with lights, lines, or nothing placed along the nodes, as defined by the properties in the WED_AirportNode(s).
	- Referenced children are WED_AirportNode
	- 
- WED_TowerViewpoint
	- Creates a tower viewpoint.
	- Specify lat/lon with <point latitude="28.197179279" longitude="-177.388166550"/>
	- Specify height with <tower_viewpoint height="25.0"/>
- WED_TruckDestination
	- Specifes a destination that a service vehicle can go to. No referenced children.
	- Specify lat/lon/heading with <point latitude="28.196814277" longitude="-177.390125145" heading="0.0"/>
	- Vehicles that can do there are specified with elements in the truck_destination element
		- Contains types tags, with value attribute. Value can be valid service vehicle type (see Service Vehicle Type section)
- WED_TruckParkingLocation
	- Specifies a spawn location for a service vehicle
	- Specify lat/lon/heading with <point latitude="28.196821160" longitude="-177.390358204" heading="0.0"/>
	- Specify params with <truck_parking_spot type="Fuel Truck (Jets)" number_of_cars="3" custom_veh=""/>
		- type is valid service vehicle type (see Service Vehicle Type section)
		- number_of_cars is the number of baggage carts if this is a baggage cart vehicle. Otherwise 0
		- custom_veh is a resource string for a custom .obj to use for the service vehicle. If the vehicle has multiple assets (i.e. a baggage tug and a baggage cart), this is a space seperated list.

# Surface String Names
- Asphalt L
- Asphalt L Patched
- Asphalt L Worn
- Asphalt L Plain
- Asphalt
- Asphalt Patched
- Asphalt Worn
- Asphalt Plain
- Asphalt D
- Asphalt D Patched
- Asphalt D Worn
- Asphalt D Plain
- Asphalt D2
- Asphalt D2 Patched
- Asphalt D2 Worn
- Asphalt D2 Plain
- Asphalt D3
- Asphalt D3 Patched
- Asphalt D3 Worn
- Asphalt D3 Plain
- Concrete L
- Concrete L Dirty
- Concrete L Worn
- Concrete
- Concrete Dirty
- Concrete Worn
- Concrete D
- Concrete D Dirty
- Concrete D Worn
- Grass
- Dirt
- Gravel
- Dry Lakebed
- Snow
- Transparent

# Runway Marking Types
- Visual
- Non-Precision (US)
- Precision (US)
- Non-Precision (UK)
- Precision (UK)
- Non-Precision (EASA)
- Precision (EASA)

# Approach Light Types
- ALSF-I
- ALSF-II
- Calvert
- Calvert-II
- SSALR
- SSALF
- SALS
- MALSR
- MALSF
- ODALS
- RAIL

# RAIL Strobe Types
- None
- Omnidirectional REIL
- Unidirectional REIL
- None, but threshold lights

# Light Types
- None
- Taxiway Centerline Lights (Green)
- Taxiway Edge Lights (Blue)
- Hold Short Lights (Amber)
- Hold Short Lights (Pulsing Amber)
- Hold Short Centerline Lights (Amber/Green)
- Boundary Edge Lights (Red)
- Centerline Twy (Unidirectional Green)
- Centerline Twy (Unidirectional Amber/Green)

# Line Types
- None
- Solid Yellow
- Broken Yellow
- Double Solid Yellow
- Runway Hold
- Other Hold
- ILS Hold
- ILS Critical Centerline
- Wide Broken Single Yellow
- Wide Broken Double Yellow
- Solid White (Roadway)
- Chequered White (Roadway)
- Broken White (Roadway)
- Single Taxi Wide
- Taxi Hold Wide
- Double Hold Wide
- Other Hold Wide
- Ils Hold Wide
- Taxi Hatch
- Road Dash
- Road Edge Wide
- Road Dash Wide
- Red Line
- Red Line Dash
- Red Line Wide
- Orange Line
- Blue Line
- Green Line
- Solid Yellow (Black)
- Broken Yellow (Black)
- Double Solid Yellow (Black)
- Runway Hold (Black)
- Other Hold (Black)
- ILS Hold (Black)
- ILS Critical Centerline (Black)
- Wide Broken Single Yellow (Black)
- Wide Broken Double Yellow (Black)
- Road Edge (Black)
- Road Edge Zipper (Black)
- Road Edge Dash (Black)
- Single Taxi Wide (Black)
- Taxi Hold Wide (Black)
- Double Hold Wide (Black)
- Other Hold Wide (Black)
- Ils Hold Wide (Black)
- Road Dash (Black)
- Road Edge Wide (Black)
- Road Dash Wide (Black)
- Red Line (Black)
- Red Line Dash (Black)
- Red Line Wide (Black)
- Orange Line (Black)
- Blue Line (Black)
- Green Line (Black)

# Service Vehicle Section
- Baggage Loader
- Baggage Train
- Crew Car
- Crew Ferrari
- Crew Limo
- Fuel Truck (Jets)
- Fuel Truck (Liners)
- Fuel Truck (Props)
- Catering Truck
- Ground Power Unit
- Pushback

# Taxisign Text Format
Text is fairly complex due to allowing formatting in straight ASCII text.
Some notes:
	- Special characters/format changes are in {} brackets
	- Arrows are represented by ^ then letters for the direction. Ie ^ul for upper left. ^u for up. etc.
	- Roman numerals are represented by r then 1 2 or 3.
	- Formatting is done via the following:
		- @Y - Yellow
		- @R - Red
		- @L - Black
		- @@ - Everything from here is on the back of the sign
	- Other characters are pretty normal, but there are some nodes:
		- _ for space
		- | for seperators
