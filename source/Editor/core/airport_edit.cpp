/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * airport_edit.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "airport_edit.h"
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::AirportEdit
{
	using namespace XPlane;

	namespace
	{
		template<typename T>
		Snapshot CaptureAt(const std::vector<T> &list, const uint32_t index)
		{
			return index < list.size() ? Snapshot(list[index]) : Snapshot();
		}

		template<typename T>
		bool RestoreAt(std::vector<T> &list, const uint32_t index, const Snapshot &snapshot)
		{
			const T *value = std::get_if<T>(&snapshot);
			if (!value || index >= list.size())
				return false;
			list[index] = *value;
			return true;
		}

		template<typename T>
		bool RestoreOptional(std::optional<T> &slot, const uint32_t index, const Snapshot &snapshot)
		{
			const T *value = std::get_if<T>(&snapshot);
			if (!value || index != 0)
				return false;
			slot = *value;
			return true;
		}

		size_t ChainBytes(const std::vector<AptNodeChain> &chains)
		{
			size_t bytes = 0;
			for (const AptNodeChain &chain : chains)
				bytes += sizeof(AptNodeChain) + chain.nodes.size() * sizeof(AptNode);
			return bytes;
		}

		void MovePoint(double &latitude, double &longitude, const Splines::Affine2 &transform, const GeoProjection &projection)
		{
			const Point2 moved = transform.Apply(projection.ToLocal(latitude, longitude));
			projection.ToGeo(moved, latitude, longitude);
		}

		void TransformChains(std::vector<AptNodeChain> &chains, const Splines::Affine2 &transform, const GeoProjection &projection)
		{
			Splines::Shape shape = Splines::FromAptChains(chains, projection);
			Splines::TransformShape(shape, transform);
			chains = Splines::ToAptChains(shape, projection);
		}
	}

	Snapshot Capture(const AptAirport &airport, const Kind kind, const uint32_t index)
	{
		switch (kind)
		{
			case Kind::Runway:				return CaptureAt(airport.runways, index);
			case Kind::WaterRunway:			return CaptureAt(airport.waterRunways, index);
			case Kind::Helipad:				return CaptureAt(airport.helipads, index);
			case Kind::Pavement:			return CaptureAt(airport.pavements, index);
			case Kind::LinearFeature:		return CaptureAt(airport.linearFeatures, index);
			case Kind::Boundary:			return CaptureAt(airport.boundaries, index);
			case Kind::Viewpoint:			return airport.viewpoint && index == 0 ? Snapshot(*airport.viewpoint) : Snapshot();
			case Kind::Beacon:				return airport.beacon && index == 0 ? Snapshot(*airport.beacon) : Snapshot();
			case Kind::Windsock:			return CaptureAt(airport.windsocks, index);
			case Kind::Sign:				return CaptureAt(airport.signs, index);
			case Kind::LightFixture:		return CaptureAt(airport.lightFixtures, index);
			case Kind::Startup:				return CaptureAt(airport.startups, index);
			case Kind::LegacyStartup:		return CaptureAt(airport.legacyStartups, index);
			case Kind::TruckParking:		return CaptureAt(airport.truckParkings, index);
			case Kind::TruckDestination:	return CaptureAt(airport.truckDestinations, index);
			case Kind::Jetway:				return CaptureAt(airport.jetways, index);
			default:						return Snapshot();
		}
	}

	bool Restore(AptAirport &airport, const Kind kind, const uint32_t index, const Snapshot &snapshot)
	{
		switch (kind)
		{
			case Kind::Runway:				return RestoreAt(airport.runways, index, snapshot);
			case Kind::WaterRunway:			return RestoreAt(airport.waterRunways, index, snapshot);
			case Kind::Helipad:				return RestoreAt(airport.helipads, index, snapshot);
			case Kind::Pavement:			return RestoreAt(airport.pavements, index, snapshot);
			case Kind::LinearFeature:		return RestoreAt(airport.linearFeatures, index, snapshot);
			case Kind::Boundary:			return RestoreAt(airport.boundaries, index, snapshot);
			case Kind::Viewpoint:			return RestoreOptional(airport.viewpoint, index, snapshot);
			case Kind::Beacon:				return RestoreOptional(airport.beacon, index, snapshot);
			case Kind::Windsock:			return RestoreAt(airport.windsocks, index, snapshot);
			case Kind::Sign:				return RestoreAt(airport.signs, index, snapshot);
			case Kind::LightFixture:		return RestoreAt(airport.lightFixtures, index, snapshot);
			case Kind::Startup:				return RestoreAt(airport.startups, index, snapshot);
			case Kind::LegacyStartup:		return RestoreAt(airport.legacyStartups, index, snapshot);
			case Kind::TruckParking:		return RestoreAt(airport.truckParkings, index, snapshot);
			case Kind::TruckDestination:	return RestoreAt(airport.truckDestinations, index, snapshot);
			case Kind::Jetway:				return RestoreAt(airport.jetways, index, snapshot);
			default:						return false;
		}
	}

	namespace
	{
		template<typename T>
		bool EraseAt(std::vector<T> &list, const uint32_t index)
		{
			if (index >= list.size())
				return false;
			list.erase(list.begin() + index);
			return true;
		}

		template<typename T>
		bool InsertAt(std::vector<T> &list, const uint32_t index, const Snapshot &snapshot)
		{
			const T *value = std::get_if<T>(&snapshot);
			if (!value || index > list.size())
				return false;
			list.insert(list.begin() + index, *value);
			return true;
		}

		template<typename T>
		bool InsertOptional(std::optional<T> &slot, const uint32_t index, const Snapshot &snapshot)
		{
			const T *value = std::get_if<T>(&snapshot);
			if (!value || index != 0 || slot)
				return false;
			slot = *value;
			return true;
		}
	}

	bool EraseRow(AptAirport &airport, const Kind kind, const uint32_t index)
	{
		switch (kind)
		{
			case Kind::Runway:				return EraseAt(airport.runways, index);
			case Kind::WaterRunway:			return EraseAt(airport.waterRunways, index);
			case Kind::Helipad:				return EraseAt(airport.helipads, index);
			case Kind::Pavement:			return EraseAt(airport.pavements, index);
			case Kind::LinearFeature:		return EraseAt(airport.linearFeatures, index);
			case Kind::Boundary:			return EraseAt(airport.boundaries, index);
			case Kind::Viewpoint:			if (!airport.viewpoint || index != 0) return false; airport.viewpoint.reset(); return true;
			case Kind::Beacon:				if (!airport.beacon || index != 0) return false; airport.beacon.reset(); return true;
			case Kind::Windsock:			return EraseAt(airport.windsocks, index);
			case Kind::Sign:				return EraseAt(airport.signs, index);
			case Kind::LightFixture:		return EraseAt(airport.lightFixtures, index);
			case Kind::Startup:				return EraseAt(airport.startups, index);
			case Kind::LegacyStartup:		return EraseAt(airport.legacyStartups, index);
			case Kind::TruckParking:		return EraseAt(airport.truckParkings, index);
			case Kind::TruckDestination:	return EraseAt(airport.truckDestinations, index);
			case Kind::Jetway:				return EraseAt(airport.jetways, index);
			default:						return false;
		}
	}

	bool InsertRow(AptAirport &airport, const Kind kind, const uint32_t index, const Snapshot &snapshot)
	{
		switch (kind)
		{
			case Kind::Runway:				return InsertAt(airport.runways, index, snapshot);
			case Kind::WaterRunway:			return InsertAt(airport.waterRunways, index, snapshot);
			case Kind::Helipad:				return InsertAt(airport.helipads, index, snapshot);
			case Kind::Pavement:			return InsertAt(airport.pavements, index, snapshot);
			case Kind::LinearFeature:		return InsertAt(airport.linearFeatures, index, snapshot);
			case Kind::Boundary:			return InsertAt(airport.boundaries, index, snapshot);
			case Kind::Viewpoint:			return InsertOptional(airport.viewpoint, index, snapshot);
			case Kind::Beacon:				return InsertOptional(airport.beacon, index, snapshot);
			case Kind::Windsock:			return InsertAt(airport.windsocks, index, snapshot);
			case Kind::Sign:				return InsertAt(airport.signs, index, snapshot);
			case Kind::LightFixture:		return InsertAt(airport.lightFixtures, index, snapshot);
			case Kind::Startup:				return InsertAt(airport.startups, index, snapshot);
			case Kind::LegacyStartup:		return InsertAt(airport.legacyStartups, index, snapshot);
			case Kind::TruckParking:		return InsertAt(airport.truckParkings, index, snapshot);
			case Kind::TruckDestination:	return InsertAt(airport.truckDestinations, index, snapshot);
			case Kind::Jetway:				return InsertAt(airport.jetways, index, snapshot);
			default:						return false;
		}
	}

	size_t SizeOf(const Snapshot &snapshot)
	{
		if (const AptPavement *pavement = std::get_if<AptPavement>(&snapshot))
			return sizeof(AptPavement) + ChainBytes(pavement->rings);
		if (const AptLinearFeature *feature = std::get_if<AptLinearFeature>(&snapshot))
			return sizeof(AptLinearFeature) + ChainBytes(feature->chains);
		if (const AptBoundary *boundary = std::get_if<AptBoundary>(&snapshot))
			return sizeof(AptBoundary) + ChainBytes(boundary->rings);
		return sizeof(Snapshot);
	}

	bool IsPointKind(const Kind kind)
	{
		return SceneAirports::IsPlacedKind(kind);
	}

	bool HasHeading(const Kind kind)
	{
		switch (kind)
		{
			case Kind::Sign:
			case Kind::LightFixture:
			case Kind::Startup:
			case Kind::LegacyStartup:
			case Kind::TruckParking:
			case Kind::TruckDestination:
			case Kind::Jetway:
				return true;
			default:
				return false;
		}
	}

	bool IsShapeKind(const Kind kind)
	{
		return kind == Kind::Pavement || kind == Kind::LinearFeature || kind == Kind::Boundary;
	}

	bool IsMovableKind(const Kind kind)
	{
		return IsPointKind(kind) || IsShapeKind(kind) || kind == Kind::Runway || kind == Kind::WaterRunway || kind == Kind::Helipad;
	}

	bool GetPlacement(const AptAirport &airport, const Kind kind, const uint32_t index, double &latitude, double &longitude, float *heading)
	{
		const auto at = [&](const auto &row, const float rowHeading)
		{
			latitude = row.latitude;
			longitude = row.longitude;
			if (heading)
				*heading = rowHeading;
			return true;
		};
		switch (kind)
		{
			case Kind::Viewpoint:			return airport.viewpoint && index == 0 && at(*airport.viewpoint, 0.0f);
			case Kind::Beacon:				return airport.beacon && index == 0 && at(*airport.beacon, 0.0f);
			case Kind::Windsock:			return index < airport.windsocks.size() && at(airport.windsocks[index], 0.0f);
			case Kind::Sign:				return index < airport.signs.size() && at(airport.signs[index], airport.signs[index].heading);
			case Kind::LightFixture:		return index < airport.lightFixtures.size() && at(airport.lightFixtures[index], airport.lightFixtures[index].heading);
			case Kind::Startup:				return index < airport.startups.size() && at(airport.startups[index], airport.startups[index].heading);
			case Kind::LegacyStartup:		return index < airport.legacyStartups.size() && at(airport.legacyStartups[index], airport.legacyStartups[index].heading);
			case Kind::TruckParking:		return index < airport.truckParkings.size() && at(airport.truckParkings[index], airport.truckParkings[index].heading);
			case Kind::TruckDestination:	return index < airport.truckDestinations.size() && at(airport.truckDestinations[index], airport.truckDestinations[index].heading);
			case Kind::Jetway:				return index < airport.jetways.size() && at(airport.jetways[index], airport.jetways[index].heading);
			case Kind::Helipad:				return index < airport.helipads.size() && at(airport.helipads[index], airport.helipads[index].heading);
			default:						return false;
		}
	}

	bool SetPlacement(AptAirport &airport, const Kind kind, const uint32_t index, const double latitude, const double longitude, const std::optional<float> heading)
	{
		const auto place = [&](auto &row)
		{
			row.latitude = latitude;
			row.longitude = longitude;
			return true;
		};
		const auto placeTurned = [&](auto &row)
		{
			row.latitude = latitude;
			row.longitude = longitude;
			if (heading)
				row.heading = *heading;
			return true;
		};
		switch (kind)
		{
			case Kind::Viewpoint:			return airport.viewpoint && index == 0 && place(*airport.viewpoint);
			case Kind::Beacon:				return airport.beacon && index == 0 && place(*airport.beacon);
			case Kind::Windsock:			return index < airport.windsocks.size() && place(airport.windsocks[index]);
			case Kind::Sign:				return index < airport.signs.size() && placeTurned(airport.signs[index]);
			case Kind::LightFixture:		return index < airport.lightFixtures.size() && placeTurned(airport.lightFixtures[index]);
			case Kind::Startup:				return index < airport.startups.size() && placeTurned(airport.startups[index]);
			case Kind::LegacyStartup:		return index < airport.legacyStartups.size() && placeTurned(airport.legacyStartups[index]);
			case Kind::TruckParking:		return index < airport.truckParkings.size() && placeTurned(airport.truckParkings[index]);
			case Kind::TruckDestination:	return index < airport.truckDestinations.size() && placeTurned(airport.truckDestinations[index]);
			case Kind::Jetway:				return index < airport.jetways.size() && placeTurned(airport.jetways[index]);
			case Kind::Helipad:				return index < airport.helipads.size() && placeTurned(airport.helipads[index]);
			default:						return false;
		}
	}

	std::vector<AptNodeChain> *GetChains(AptAirport &airport, const Kind kind, const uint32_t index)
	{
		switch (kind)
		{
			case Kind::Pavement:		return index < airport.pavements.size() ? &airport.pavements[index].rings : nullptr;
			case Kind::LinearFeature:	return index < airport.linearFeatures.size() ? &airport.linearFeatures[index].chains : nullptr;
			case Kind::Boundary:		return index < airport.boundaries.size() ? &airport.boundaries[index].rings : nullptr;
			default:					return nullptr;
		}
	}

	const std::vector<AptNodeChain> *GetChains(const AptAirport &airport, const Kind kind, const uint32_t index)
	{
		return GetChains(const_cast<AptAirport &>(airport), kind, index);
	}

	float TurnHeading(const float heading, const double turnDegrees)
	{
		// Headings run clockwise, the turn counter-clockwise
		double turned = std::fmod(static_cast<double>(heading) - turnDegrees, 360.0);
		if (turned < 0.0)
			turned += 360.0;
		return static_cast<float>(turned >= 360.0 ? 0.0 : turned);
	}

	bool TransformRow(AptAirport &airport, const Kind kind, const uint32_t index, const Splines::Affine2 &transform, const double turnDegrees, const GeoProjection &projection)
	{
		const bool turned = std::abs(turnDegrees) > 1e-9;
		switch (kind)
		{
			case Kind::Pavement:
			{
				if (index >= airport.pavements.size())
					return false;
				AptPavement &pavement = airport.pavements[index];
				TransformChains(pavement.rings, transform, projection);
				if (turned)
					pavement.textureHeading = TurnHeading(pavement.textureHeading, turnDegrees);
				return true;
			}
			case Kind::LinearFeature:
				if (index >= airport.linearFeatures.size())
					return false;
				TransformChains(airport.linearFeatures[index].chains, transform, projection);
				return true;
			case Kind::Boundary:
				if (index >= airport.boundaries.size())
					return false;
				TransformChains(airport.boundaries[index].rings, transform, projection);
				return true;
			case Kind::Runway:
				if (index >= airport.runways.size())
					return false;
				for (AptRunwayEnd &end : airport.runways[index].ends)
					MovePoint(end.latitude, end.longitude, transform, projection);
				return true;
			case Kind::WaterRunway:
				if (index >= airport.waterRunways.size())
					return false;
				for (AptWaterRunway::End &end : airport.waterRunways[index].ends)
					MovePoint(end.latitude, end.longitude, transform, projection);
				return true;
			default:
			{
				double latitude = 0.0, longitude = 0.0;
				float heading = 0.0f;
				if (!GetPlacement(airport, kind, index, latitude, longitude, &heading))
					return false;
				MovePoint(latitude, longitude, transform, projection);
				const bool hasHeading = HasHeading(kind) || kind == Kind::Helipad;
				return SetPlacement(airport, kind, index, latitude, longitude, hasHeading && turned ? std::optional<float>(TurnHeading(heading, turnDegrees)) : std::nullopt);
			}
		}
	}
}

// -------------------------------------------------------
