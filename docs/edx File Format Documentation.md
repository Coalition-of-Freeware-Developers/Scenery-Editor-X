# Scenery Editor X - Project File Format

Scenery Editor X doesn't use a customized json formatting for storing data for a users scenery project. This format contains all important information regarding the scenery geo location and all assets and positions in the scene in WGS84 geocentric datum. The WGS84 coordinate system has three values, latitude, longitude, and height.

---

## Usability

The `.edX` file format is a format to contain data for the active project with out massive overhead computation and easily readable and correctable to the user. It serves as an intermediate format between X-Planes WED editor format which uses xml in utf-8 format.

### Advantages

* Easy human and computer readable layout
* Contains an expandable set of data for a project
* Can be tracked properly within git repositorys
* Uses the JSON file format, a widely utilized and globally adopted file format

### Example of data stored:

```json
{
	"Project":
	{
			"name": "MySceneryProject",
			"editor-version": "1.0.8",
			"XPlaneVersion": "12.1.4-r3",
			"editdate": "2023-10-01T12:00:00Z",
			"author": "John Doe"
	},
	"Airport": {
			"ICAO": "KSFO",
			"IATA": "SFO",
			"FAA": "KSFO",
			"City": "San Francisco",
			"State": "California",
			"Country": "USA",
			"RegionCode": "US-W",
			"DatumLat": 37.618999,
			"DatumLon": -122.375,
			"TransitionAlt": 18000,
			"TransitionLevel": "FL180",
			"Elevation": 13,
			"MagVar": -14.0,
			"Libraries": [
			{
				"Library": "Laminar Research Airport Scenery",
				"local-path": "./Resources/default scenery/airport scenery/library.txt",
				"entry-count": 5000,
				"uuid": "15da3863-07d1-462a-be65-3873058675f3",
				"short-id": "a1b2c3d4"
			},
			{
				"Library": "ZDP Library",
				"local-path": "./Custom Scenery/ZDP_Library/library.txt",
				"entry-count": 627,
				"uuid": "5ec54bdf-7be0-4b54-ac02-5d33472cd216",
				"short-id": "6587fddb"
			}
		]
	},
	"Assets": [
		{
			"id": "Asset001",
			"latitude": 37.618999,
			"longitude": -122.375,
			"altitude": 642,
			"heading": 0.0,
			"associated-library": "6587fddb",
			"other-properties": {
				"building-type": "Terminal"
			}
		},
		{
			"id": "Asset002",
			"latitude": 37.621,
			"longitude": -122.379,
			"altitude": 641,
			"heading": 90.0,
			"associated-library": "a1b2c3d4",
			"other-properties": {
				"object-type": "Hangar"
			}
		},
		{
			"id": "Asset003",
			"latitude": 37.6185,
			"longitude": -122.380,
			"altitude": 641,
			"heading": 45.0,
			"associated-library": "6587fddb",
			"other-properties": {
				"object-type": "ControlTower"
			}
		}
	]
}
```

---

## Implementation
