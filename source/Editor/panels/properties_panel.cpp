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
 * properties_panel.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "properties_panel.h"
#include <Editor/core/edit_changes.h>
#include <Editor/core/editor_scene.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <Editor/core/editor_xplane.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/renderer/gpu_scene.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/xplane/xp_scene.h>
#include <Editor/core/editor_airports.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/* Edits a Vec3 through a float[3] copy, so nothing depends on the vector's memory layout. */
		bool DragVec3(const char *id, Vec3 &value, const float speed, const char *format = "%.3f")
		{
			float components[3] = { value.x, value.y, value.z };
			if (!ImGui::DragFloat3(id, components, speed, 0.0f, 0.0f, format))
				return false;

			value = Vec3(components[0], components[1], components[2]);
			return true;
		}

		/* One texture row: file name with a loaded / missing marker and the format in the tooltip. */
		void TextureRow(const char *label, const std::string &reference, const XPlane::TextureInfo &info, const bool loaded)
		{
			if (reference.empty())
				return;

			EditorUI::PropertyLabel(label);
			ImGui::PushStyleColor(ImGuiCol_Text, EditorTheme::ToStyle(loaded ? EditorTheme::Palette::Text : EditorTheme::Palette::Error));
			ImGui::TextWrapped("%s %s", loaded ? SEDX_ICON_FILE_IMAGE_O : SEDX_ICON_EXCLAMATION_TRIANGLE, info.path.empty() ? reference.c_str() : info.path.filename().string().c_str());
			ImGui::PopStyleColor();

			char tooltip[512];
			if (loaded)
				snprintf(tooltip, sizeof(tooltip), "%s\n%ux%u, %u mips, %s%s", info.path.string().c_str(), info.width, info.height, info.mipCount,
					info.fromDds ? "DDS" : "PNG/RGBA8", info.hasAlpha ? ", alpha" : "");
			else
				snprintf(tooltip, sizeof(tooltip), "%s\n%s", reference.c_str(), info.error.empty() ? "Not loaded" : info.error.c_str());
			EditorUI::ItemTooltip(tooltip);
		}

		// -------------------------------------------------------
		// Airport elements (apt.dat rows): edits go straight into the open document
		// -------------------------------------------------------

		bool PropertyDouble(const char *label, double &value, const char *format = "%.8f")
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			ImGui::InputDouble("##Value", &value, 0.0, 0.0, format);
			const bool edited = ImGui::IsItemDeactivatedAfterEdit();
			ImGui::PopID();
			return edited;
		}

		bool PropertyFloat(const char *label, float &value, const char *format = "%.2f")
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			ImGui::InputFloat("##Value", &value, 0.0f, 0.0f, format);
			const bool edited = ImGui::IsItemDeactivatedAfterEdit();
			ImGui::PopID();
			return edited;
		}

		bool PropertyInt(const char *label, int &value)
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			ImGui::InputInt("##Value", &value, 0, 0);
			const bool edited = ImGui::IsItemDeactivatedAfterEdit();
			ImGui::PopID();
			return edited;
		}

		bool PropertyText(const char *label, std::string &value)
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			ImGui::InputText("##Value", &value);
			const bool edited = ImGui::IsItemDeactivatedAfterEdit();
			ImGui::PopID();
			return edited;
		}

		bool PropertyBool(const char *label, bool &value)
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			const bool edited = ImGui::Checkbox("##Value", &value);
			ImGui::PopID();
			return edited;
		}

		/* Combo over apt.dat numeric codes with display names. */
		bool PropertyCodeCombo(const char *label, int &value, const std::vector<int> &codes, const char *(*name)(int))
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			bool edited = false;
			const std::string current = std::string(name(value)) + " (" + std::to_string(value) + ")";
			if (ImGui::BeginCombo("##Value", current.c_str()))
			{
				for (const int code : codes)
				{
					const std::string option = std::string(name(code)) + " (" + std::to_string(code) + ")";
					if (ImGui::Selectable(option.c_str(), code == value) && code != value)
					{
						value = code;
						edited = true;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();
			return edited;
		}

		/* Combo over 0..count-1 with display names. */
		bool PropertyIndexCombo(const char *label, int &value, const char *(*name)(int), const int first, const int last)
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			bool edited = false;
			if (ImGui::BeginCombo("##Value", name(value)))
			{
				for (int code = first; code <= last; ++code)
				{
					if (ImGui::Selectable(name(code), code == value) && code != value)
					{
						value = code;
						edited = true;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();
			return edited;
		}

		bool PropertyStringCombo(const char *label, std::string &value, const std::vector<const char *> &options)
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			bool edited = false;
			if (ImGui::BeginCombo("##Value", value.c_str()))
			{
				for (const char *option : options)
				{
					if (ImGui::Selectable(option, value == option) && value != option)
					{
						value = option;
						edited = true;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();
			return edited;
		}

		bool PropertyPosition(double &latitude, double &longitude)
		{
			bool edited = PropertyDouble("Latitude", latitude);
			edited |= PropertyDouble("Longitude", longitude);
			return edited;
		}

		/* The shared "Aircraft types" pipe list edited as checkboxes. */
		bool PropertyAircraftTypes(const char *label, std::string &value)
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			bool edited = false;
			for (const char *type : XPlane::GetAircraftTypes())
			{
				const std::string needle = type;
				const bool has = value.find(needle) != std::string::npos;
				bool flag = has;
				if (ImGui::Checkbox(type, &flag))
				{
					std::vector<std::string> parts;
					for (const char *candidate : XPlane::GetAircraftTypes())
					{
						const bool keep = candidate == needle ? flag : value.find(candidate) != std::string::npos;
						if (keep)
							parts.emplace_back(candidate);
					}
					value.clear();
					for (const std::string &part : parts)
						value += (value.empty() ? "" : "|") + part;
					edited = true;
				}
				ImGui::SameLine();
			}
			ImGui::NewLine();
			ImGui::PopID();
			return edited;
		}

		const char *SurfaceNameOf(const int code) { return XPlane::GetSurfaceName(code); }

		/* "AIRPORT" section for an entity that draws an apt.dat row of the open airport. */
		void DrawAirportElement(EditorContext *context, const uint64_t selected)
		{
			using namespace XPlane;
			const AirportElementComponent *element = EditorAirports::GetSelectedElement(context);
			AirportDocument *document = EditorAirports::GetDocument();
			if (!element || !document || !EditorUI::SectionHeader("AIRPORT"))
				return;

			AptAirport &airport = document->airport;
			const AirportElementKind kind = element->kind;
			const uint32_t index = element->index;
			const uint64_t rootId = SceneAirports::GetRootId();
			bool geometry = false;	// Needs the meshes rebuilt
			bool moved = false;		// Only a placement changed
			bool removed = false;

			if (kind != AirportElementKind::Airport)
			{
				ImGui::TextDisabled("%s of", GetAirportElementKindName(kind));
				ImGui::SameLine();
				if (ImGui::SmallButton(document->GetTitle().c_str()) && context)
					context->selectedEntity = rootId;
			}
			if (document->IsReadOnly())
			{
				ImGui::TextColored(EditorTheme::ToStyle(EditorTheme::Palette::Warning), "%s read-only source: Save As writes a copy", SEDX_ICON_EXCLAMATION_TRIANGLE);
			}

			const auto stale = [](const char *what)
			{
				ImGui::TextDisabled("This %s no longer exists in the document", what);
			};

			if (EditorUI::BeginPropertyGrid("##AirportElement"))
			{
				switch (kind)
				{
					case AirportElementKind::Airport:
					{
						EditorUI::PropertyLabel("ID");
						ImGui::InputText("##Id", &airport.id, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);
						moved |= ImGui::IsItemDeactivatedAfterEdit();
						moved |= PropertyText("Name", airport.name);
						EditorUI::PropertyLabel("Type");
						ImGui::TextUnformatted(GetAirportTypeName(airport.type));
						EditorUI::PropertyLabel("Scenery pack");
						ImGui::TextWrapped("%s", document->GetPackageName().c_str());
						if (document->HasFile())
							EditorUI::ItemTooltip(document->GetAptDatPath().string().c_str());
						EditorUI::PropertyLabel("Elements");
						ImGui::Text("%zu", airport.GetElementCount());
						EditorUI::PropertyLabel("Scene");
						const SceneAirports::Stats stats = SceneAirports::GetStats();
						ImGui::Text("%u entities, %u triangles, %u objects (%.0f ms)", stats.entities, stats.triangles, stats.objects, stats.buildMilliseconds);
						if (stats.warnings > 0)
						{
							ImGui::SameLine();
							ImGui::TextDisabled("%u asset warnings", stats.warnings);
							EditorUI::ItemTooltip("Library or scenery pack art that couldn't be found or read; the log names each one");
						}
						break;
					}
					case AirportElementKind::Group:
					{
						const AirportElementKind groupKind = static_cast<AirportElementKind>(index);
						EditorUI::PropertyLabel("Shows");
						ImGui::TextUnformatted(GetAirportElementKindName(groupKind));
						bool visible = SceneAirports::IsKindVisible(groupKind);
						if (PropertyBool("Visible", visible))
							SceneAirports::SetKindVisible(groupKind, visible);
						break;
					}
					case AirportElementKind::Runway:
					{
						if (index >= airport.runways.size()) { stale("runway"); break; }
						AptRunway &runway = airport.runways[index];
						EditorUI::PropertyLabel("Length");
						ImGui::Text("%.0f m, heading %.1f", runway.GetLengthMeters(), runway.GetHeading());
						geometry |= PropertyFloat("Width (m)", runway.width);
						geometry |= PropertyCodeCombo("Surface", runway.surface, GetSurfaceCodes(), SurfaceNameOf);
						geometry |= PropertyInt("Shoulder code", runway.shoulder);
						EditorUI::ItemTooltip("Surface code, or surface + 100 x shoulder width in meters");
						geometry |= PropertyFloat("Smoothness", runway.smoothness);
						geometry |= PropertyBool("Centerline lights", runway.centerlineLights);
						int edge = runway.edgeLights;
						if (PropertyIndexCombo("Edge lights", edge, GetEdgeLightsName, 0, 3)) { runway.edgeLights = edge; geometry = true; }
						geometry |= PropertyBool("Distance signs", runway.distanceSigns);
						for (int e = 0; e < 2; ++e)
						{
							AptRunwayEnd &end = runway.ends[e];
							ImGui::PushID(e);
							EditorUI::PropertyLabel(e == 0 ? "End 1" : "End 2");
							ImGui::InputText("##Designator", &end.designator, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);
							geometry |= ImGui::IsItemDeactivatedAfterEdit();
							geometry |= PropertyPosition(end.latitude, end.longitude);
							geometry |= PropertyFloat("Displaced (m)", end.displacedThreshold);
							geometry |= PropertyFloat("Blast pad (m)", end.blastPad);
							geometry |= PropertyIndexCombo("Markings", end.marking, GetRunwayMarkingName, 0, 7);
							geometry |= PropertyIndexCombo("Approach lights", end.approachLights, GetApproachLightsName, 0, 12);
							geometry |= PropertyBool("TDZ lights", end.touchdownZoneLights);
							geometry |= PropertyIndexCombo("REIL", end.reil, GetReilName, 0, 2);
							ImGui::PopID();
						}
						break;
					}
					case AirportElementKind::WaterRunway:
					{
						if (index >= airport.waterRunways.size()) { stale("water runway"); break; }
						AptWaterRunway &runway = airport.waterRunways[index];
						geometry |= PropertyFloat("Width (m)", runway.width);
						geometry |= PropertyBool("Buoys", runway.buoys);
						for (int e = 0; e < 2; ++e)
						{
							ImGui::PushID(e);
							geometry |= PropertyText(e == 0 ? "End 1" : "End 2", runway.ends[e].designator);
							geometry |= PropertyPosition(runway.ends[e].latitude, runway.ends[e].longitude);
							ImGui::PopID();
						}
						break;
					}
					case AirportElementKind::Helipad:
					{
						if (index >= airport.helipads.size()) { stale("helipad"); break; }
						AptHelipad &helipad = airport.helipads[index];
						geometry |= PropertyText("Designator", helipad.designator);
						geometry |= PropertyPosition(helipad.latitude, helipad.longitude);
						geometry |= PropertyFloat("Heading", helipad.heading);
						geometry |= PropertyFloat("Length (m)", helipad.length);
						geometry |= PropertyFloat("Width (m)", helipad.width);
						geometry |= PropertyCodeCombo("Surface", helipad.surface, GetSurfaceCodes(), SurfaceNameOf);
						geometry |= PropertyInt("Shoulder (0-2)", helipad.shoulder);
						geometry |= PropertyFloat("Smoothness", helipad.smoothness);
						int edge = helipad.edgeLights;
						if (PropertyIndexCombo("Edge lights", edge, GetHelipadEdgeLightsName, 0, 1)) { helipad.edgeLights = edge; geometry = true; }
						break;
					}
					case AirportElementKind::Pavement:
					{
						if (index >= airport.pavements.size()) { stale("pavement"); break; }
						AptPavement &pavement = airport.pavements[index];
						geometry |= PropertyText("Description", pavement.description);
						geometry |= PropertyCodeCombo("Surface", pavement.surface, GetSurfaceCodes(), SurfaceNameOf);
						geometry |= PropertyFloat("Smoothness", pavement.smoothness);
						geometry |= PropertyFloat("Texture heading", pavement.textureHeading);
						EditorUI::PropertyLabel("Outline");
						size_t nodes = 0;
						for (const AptNodeChain &ring : pavement.rings)
							nodes += ring.nodes.size();
						ImGui::Text("%zu rings, %zu nodes", pavement.rings.size(), nodes);
						EditorUI::PropertyLabel("Layer");
						ImGui::Text("%u of %zu (earlier draws on top)", index + 1, airport.pavements.size());
						break;
					}
					case AirportElementKind::LinearFeature:
					{
						if (index >= airport.linearFeatures.size()) { stale("line"); break; }
						AptLinearFeature &feature = airport.linearFeatures[index];
						geometry |= PropertyText("Description", feature.description);
						EditorUI::PropertyLabel("Nodes");
						size_t nodes = 0;
						for (const AptNodeChain &chain : feature.chains)
							nodes += chain.nodes.size();
						ImGui::Text("%zu in %zu chain%s", nodes, feature.chains.size(), feature.chains.size() == 1 ? "" : "s");
						// Restyle every node at once
						int line = 0, lights = 0;
						for (const AptNodeChain &chain : feature.chains)
						{
							for (const AptNode &node : chain.nodes)
							{
								if (line == 0) line = node.lineType;
								if (lights == 0) lights = node.lightType;
							}
						}
						std::vector<int> lineCodes = GetPaintedLineCodes();
						if (PropertyCodeCombo("Painted line", line, lineCodes, GetLineTypeName))
						{
							for (AptNodeChain &chain : feature.chains)
								for (AptNode &node : chain.nodes)
									node.lineType = line;
							geometry = true;
						}
						std::vector<int> lightCodes = { 0 };
						lightCodes.insert(lightCodes.end(), GetLightLineCodes().begin(), GetLightLineCodes().end());
						if (PropertyCodeCombo("Light string", lights, lightCodes, GetLineTypeName))
						{
							for (AptNodeChain &chain : feature.chains)
								for (AptNode &node : chain.nodes)
									node.lightType = lights;
							geometry = true;
						}
						break;
					}
					case AirportElementKind::Boundary:
					{
						if (index >= airport.boundaries.size()) { stale("boundary"); break; }
						geometry |= PropertyText("Description", airport.boundaries[index].description);
						EditorUI::PropertyLabel("Rings");
						ImGui::Text("%zu", airport.boundaries[index].rings.size());
						break;
					}
					case AirportElementKind::Viewpoint:
					{
						if (!airport.viewpoint) { stale("viewpoint"); break; }
						moved |= PropertyPosition(airport.viewpoint->latitude, airport.viewpoint->longitude);
						moved |= PropertyFloat("Height (ft)", airport.viewpoint->heightFt);
						moved |= PropertyText("Name", airport.viewpoint->name);
						break;
					}
					case AirportElementKind::Beacon:
					{
						if (!airport.beacon) { stale("beacon"); break; }
						moved |= PropertyPosition(airport.beacon->latitude, airport.beacon->longitude);
						geometry |= PropertyIndexCombo("Type", airport.beacon->type, GetBeaconTypeName, 0, 4);
						moved |= PropertyText("Name", airport.beacon->name);
						break;
					}
					case AirportElementKind::Windsock:
					{
						if (index >= airport.windsocks.size()) { stale("windsock"); break; }
						AptWindsock &windsock = airport.windsocks[index];
						moved |= PropertyPosition(windsock.latitude, windsock.longitude);
						moved |= PropertyBool("Lit", windsock.lit);
						moved |= PropertyText("Name", windsock.name);
						break;
					}
					case AirportElementKind::Sign:
					{
						if (index >= airport.signs.size()) { stale("sign"); break; }
						AptSign &sign = airport.signs[index];
						moved |= PropertyPosition(sign.latitude, sign.longitude);
						moved |= PropertyFloat("Heading", sign.heading);
						geometry |= PropertyIndexCombo("Size", sign.size, GetSignSizeName, 1, 5);
						geometry |= PropertyText("Text", sign.text);
						EditorUI::ItemTooltip("X-Plane sign syntax, e.g. {@Y}A{@L}22{^r}");
						break;
					}
					case AirportElementKind::LightFixture:
					{
						if (index >= airport.lightFixtures.size()) { stale("light fixture"); break; }
						AptLightFixture &fixture = airport.lightFixtures[index];
						moved |= PropertyPosition(fixture.latitude, fixture.longitude);
						geometry |= PropertyIndexCombo("Type", fixture.type, GetLightFixtureName, 1, 8);
						moved |= PropertyFloat("Heading", fixture.heading);
						moved |= PropertyFloat("Glideslope", fixture.glideslope);
						moved |= PropertyText("Name", fixture.name);
						break;
					}
					case AirportElementKind::Startup:
					{
						if (index >= airport.startups.size()) { stale("startup location"); break; }
						AptStartup &startup = airport.startups[index];
						geometry |= PropertyText("Name", startup.name);
						moved |= PropertyPosition(startup.latitude, startup.longitude);
						moved |= PropertyFloat("Heading", startup.heading);
						geometry |= PropertyStringCombo("Type", startup.type, GetStartupTypes());
						geometry |= PropertyAircraftTypes("Aircraft", startup.aircraftTypes);
						geometry |= PropertyBool("Ramp metadata", startup.hasMetadata);
						if (startup.hasMetadata)
						{
							static const std::vector<const char *> widths = { "A", "B", "C", "D", "E", "F" };
							geometry |= PropertyStringCombo("Width code", startup.widthCode, widths);
							geometry |= PropertyStringCombo("Operation", startup.operationType, GetOperationTypes());
							geometry |= PropertyText("Airlines", startup.airlines);
							EditorUI::ItemTooltip("Space separated ICAO airline codes (asa dlh ...)");
						}
						break;
					}
					case AirportElementKind::LegacyStartup:
					{
						if (index >= airport.legacyStartups.size()) { stale("startup location"); break; }
						AptLegacyStartup &startup = airport.legacyStartups[index];
						geometry |= PropertyText("Name", startup.name);
						moved |= PropertyPosition(startup.latitude, startup.longitude);
						moved |= PropertyFloat("Heading", startup.heading);
						EditorUI::PropertyLabel("Note");
						ImGui::TextWrapped("Row 15 is deprecated; convert to a 1300 startup location.");
						if (ImGui::SmallButton("Convert"))
						{
							AptStartup converted;
							converted.latitude = startup.latitude;
							converted.longitude = startup.longitude;
							converted.heading = startup.heading;
							converted.type = "misc";
							converted.name = startup.name;
							airport.startups.push_back(converted);
							airport.legacyStartups.erase(airport.legacyStartups.begin() + index);
							removed = true;
						}
						break;
					}
					case AirportElementKind::TaxiRoutes:
					case AirportElementKind::TaxiNodes:
					{
						if (!airport.taxiNetwork) { stale("taxi network"); break; }
						EditorUI::PropertyLabel("Nodes");
						ImGui::Text("%zu", airport.taxiNetwork->nodes.size());
						EditorUI::PropertyLabel("Edges");
						size_t ground = 0;
						for (const AptTaxiEdge &edge : airport.taxiNetwork->edges)
							ground += edge.groundVehicles ? 1 : 0;
						ImGui::Text("%zu (%zu ground vehicle)", airport.taxiNetwork->edges.size(), ground);
						break;
					}
					case AirportElementKind::TruckParking:
					{
						if (index >= airport.truckParkings.size()) { stale("truck parking"); break; }
						AptTruckParking &truck = airport.truckParkings[index];
						geometry |= PropertyText("Name", truck.name);
						moved |= PropertyPosition(truck.latitude, truck.longitude);
						moved |= PropertyFloat("Heading", truck.heading);
						geometry |= PropertyStringCombo("Type", truck.type, GetTruckTypes());
						geometry |= PropertyInt("Cars (baggage train)", truck.count);
						geometry |= PropertyText("Custom object", truck.customObject);
						EditorUI::ItemTooltip("Library path of a replacement vehicle (row 1402), empty for the default");
						break;
					}
					case AirportElementKind::TruckDestination:
					{
						if (index >= airport.truckDestinations.size()) { stale("truck destination"); break; }
						AptTruckDestination &destination = airport.truckDestinations[index];
						geometry |= PropertyText("Name", destination.name);
						moved |= PropertyPosition(destination.latitude, destination.longitude);
						moved |= PropertyFloat("Heading", destination.heading);
						geometry |= PropertyText("Truck types", destination.truckTypes);
						EditorUI::ItemTooltip("Pipe separated: baggage_loader|fuel_jets|food|pushback ...");
						break;
					}
					case AirportElementKind::Jetway:
					{
						if (index >= airport.jetways.size()) { stale("jetway"); break; }
						AptJetway &jetway = airport.jetways[index];
						moved |= PropertyPosition(jetway.latitude, jetway.longitude);
						moved |= PropertyFloat("Tunnel heading", jetway.heading);
						geometry |= PropertyIndexCombo("Style", jetway.style, GetJetwayStyleName, 0, 3);
						geometry |= PropertyIndexCombo("Size", jetway.size, GetJetwaySizeName, 0, 3);
						geometry |= PropertyFloat("Tunnel length (m)", jetway.tunnelLength);
						geometry |= PropertyFloat("Cabin heading", jetway.cabinHeading);
						EditorUI::PropertyLabel("Custom objects");
						ImGui::Text("%zu", jetway.customObjects.size());
						break;
					}
					default:
						break;
				}
				EditorUI::EndPropertyGrid();
			}

			if (removed)
			{
				EditorAirports::MarkEdited(true);
				if (context)
					context->selectedEntity = rootId;
				return;
			}

			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CROSSHAIRS, "Frame").c_str()))
				SceneAirports::FrameEntity(selected);
			EditorUI::ItemTooltip("Move the camera to look at it");

			// Rows that can go
			const bool deletable = kind != AirportElementKind::Airport && kind != AirportElementKind::Group && kind != AirportElementKind::TaxiRoutes && kind != AirportElementKind::TaxiNodes && !SceneAirports::IsOverlayKind(kind);
			if (deletable)
			{
				ImGui::SameLine();
				if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_TRASH, "Delete").c_str()))
				{
					const auto erase = [index](auto &list) { if (index < list.size()) list.erase(list.begin() + index); };
					switch (kind)
					{
						case AirportElementKind::Runway:			erase(airport.runways); break;
						case AirportElementKind::WaterRunway:		erase(airport.waterRunways); break;
						case AirportElementKind::Helipad:			erase(airport.helipads); break;
						case AirportElementKind::Pavement:			erase(airport.pavements); break;
						case AirportElementKind::LinearFeature:		erase(airport.linearFeatures); break;
						case AirportElementKind::Boundary:			erase(airport.boundaries); break;
						case AirportElementKind::Viewpoint:			airport.viewpoint.reset(); break;
						case AirportElementKind::Beacon:			airport.beacon.reset(); break;
						case AirportElementKind::Windsock:			erase(airport.windsocks); break;
						case AirportElementKind::Sign:				erase(airport.signs); break;
						case AirportElementKind::LightFixture:		erase(airport.lightFixtures); break;
						case AirportElementKind::Startup:			erase(airport.startups); break;
						case AirportElementKind::LegacyStartup:		erase(airport.legacyStartups); break;
						case AirportElementKind::TruckParking:		erase(airport.truckParkings); break;
						case AirportElementKind::TruckDestination:	erase(airport.truckDestinations); break;
						case AirportElementKind::Jetway:			erase(airport.jetways); break;
						default: break;
					}
					EditorAirports::MarkEdited(true);
					if (context)
						context->selectedEntity = rootId;
					return;
				}
				EditorUI::ItemTooltip("Remove this row from the airport");
			}

			if (geometry)
				EditorAirports::MarkEdited(true);
			else if (moved)
				EditorAirports::MarkEdited(false);
		}

		/* "X-PLANE OBJECT" section for an entity that belongs to a placed OBJ. */
		void DrawXPlaneObject(EditorContext *context, const uint64_t selected)
		{
			uint64_t rootId = 0;
			XPlane::XPlaneObjectComponent *object = XPlane::SceneObjects::FindObject(selected, &rootId);
			if (!object || !object->asset || !EditorUI::SectionHeader("X-PLANE OBJECT"))
				return;

			if (!object->loaded)
			{
				// Loading in the background: meshes, animation and lights appear below the root when it is ready.
				ImGui::PushStyleColor(ImGuiCol_Text, EditorTheme::ToStyle(EditorTheme::Palette::Warning));
				ImGui::TextWrapped("%s %s %s...", SEDX_ICON_HOURGLASS_HALF, XPlane::GetObjectLoadStateName(object->asset->GetState()),
					object->asset->GetPath().filename().string().c_str());
				ImGui::PopStyleColor();
				EditorUI::ItemTooltip(object->asset->GetPath().string().c_str());
				if (!object->virtualPath.empty())
					ImGui::TextDisabled("%s", object->virtualPath.c_str());
				return;
			}

			const XPlane::ObjectAsset &asset = *object->asset;
			const XPlane::Obj8Data &data = asset.GetData();
			const XPlane::ObjectTextures &textures = asset.GetTextures();

			if (rootId != selected)
			{
				ImGui::TextDisabled("Part of");
				ImGui::SameLine();
				if (ImGui::SmallButton(EditorScene::GetName(EditorScene::FindHandle(rootId))) && context)
					context->selectedEntity = rootId;
			}

			if (EditorUI::BeginPropertyGrid("##XPlaneObject"))
			{
				EditorUI::PropertyLabel("File");
				ImGui::TextWrapped("%s", asset.GetPath().filename().string().c_str());
				EditorUI::ItemTooltip(asset.GetPath().string().c_str());

				if (!object->virtualPath.empty())
				{
					EditorUI::PropertyLabel("Library path");
					ImGui::TextWrapped("%s", object->virtualPath.c_str());
				}

				EditorUI::PropertyLabel("Geometry");
				ImGui::Text("%u triangles, %zu batches, %u materials", data.GetTriangleCount(), asset.GetBatches().size(), asset.GetMaterialCount());

				if (!data.lights.empty())
				{
					EditorUI::PropertyLabel("Lights");
					ImGui::Text("%zu of %zu resolved: %u billboards, %u spill", asset.GetLights().size(), data.lights.size(), asset.GetBillboardCount(),
						asset.GetSpillCount());
					EditorUI::ItemTooltip("Named lights come from X-Plane's lights.txt; they show at night or with \"Force lights on\"");
				}

				EditorUI::PropertyLabel("Visible");
				ImGui::Checkbox("##Visible", &object->visible);

				EditorUI::PropertyLabel("Animate");
				ImGui::BeginDisabled(!data.IsAnimated());
				ImGui::Checkbox("##Animate", &object->animate);
				ImGui::EndDisabled();
				EditorUI::ItemTooltip("Off freezes the animation groups in their current pose");

				if (!data.lods.empty())
				{
					EditorUI::PropertyLabel("LOD");
					const auto lod_label = [&data](const int value) -> std::string
					{
						if (value == -1)
							return "Auto (camera distance)";
						if (value == -2)
							return "All LODs";
						const XPlane::Obj8Lod &lod = data.lods[static_cast<size_t>(value)];
						return "LOD " + std::to_string(value) + ": " + std::to_string(static_cast<int>(lod.nearDistance)) + " - " + std::to_string(static_cast<int>(lod.farDistance)) + " m";
					};

					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::BeginCombo("##Lod", lod_label(object->lodOverride).c_str()))
					{
						for (int value = -1; value < static_cast<int>(data.lods.size()); ++value)
						{
							if (ImGui::Selectable(lod_label(value).c_str(), object->lodOverride == value))
								object->lodOverride = value;
							if (value == -1 && ImGui::Selectable(lod_label(-2).c_str(), object->lodOverride == -2))
								object->lodOverride = -2;
						}
						ImGui::EndCombo();
					}
				}

				EditorUI::PropertyLabel("In view");
				ImGui::Text("%.0f m away, %u / %zu batches", object->cameraDistance, object->visibleBatches, asset.GetBatches().size());
				if (!asset.GetLights().empty())
					ImGui::Text("%u billboards, %u spill lit", object->visibleBillboards, object->litSpills);

				TextureRow("Texture", data.texture, textures.albedoInfo, textures.albedo.Get() != nullptr);
				TextureRow("Normal map", data.textureNormal, textures.normalInfo, textures.normal.Get() != nullptr);
				TextureRow("Lit texture", data.textureLit, textures.litInfo, textures.lit.Get() != nullptr);
				TextureRow("Draped", data.textureDraped, textures.drapedInfo, textures.drapedAlbedo.Get() != nullptr);

				EditorUI::EndPropertyGrid();
			}

			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CROSSHAIRS, "Frame").c_str()))
				XPlane::SceneObjects::FrameObject(rootId);
			EditorUI::ItemTooltip("Move the camera to look at the object");

			if (data.IsAnimated())
			{
				ImGui::SeparatorText("Animation");
				EditorXPlane::DrawDatarefTable("##ObjectDatarefs", &data.datarefs);
				if (ImGui::TreeNode("Simulation"))
				{
					EditorXPlane::DrawSimulationControls();
					ImGui::TreePop();
				}
			}
		}
	}

	PropertiesPanel::PropertiesPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "PROPERTIES";
	}

	void PropertiesPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("PROPERTIES###Properties", &isOpen))
		{
			const entt::entity handle = EditorScene::FindHandle(m_Context ? m_Context->selectedEntity : 0);

			if (handle == entt::null)
			{
				EditorUI::EmptyState("No properties found");
			}
			else
			{
				if (EditorUI::SectionHeader("GENERAL PROPERTIES") && EditorUI::BeginPropertyGrid("##General"))
				{
					if (TagComponent *tag = EditorScene::TryGet<TagComponent>(handle))
					{
						EditorUI::PropertyLabel("Name");
						ImGui::InputText("##Name", &tag->pTag);
					}

					EditorUI::PropertyLabel("ID");
					ImGui::TextDisabled("%llu", static_cast<unsigned long long>(EditorScene::GetID(handle)));

					EditorUI::EndPropertyGrid();
				}

				if (TransformComponent *transform = EditorScene::TryGet<TransformComponent>(handle))
				{
					if (EditorUI::SectionHeader("TRANSFORM") && EditorUI::BeginPropertyGrid("##Transform"))
					{
						// Each field's edit (a drag or typed value) is one undo step: the transform as it was when the field
						// became active, against how it is when it lets go
						static uint64_t s_EditedEntity = 0;
						static TransformComponent s_EditedBefore;
						const uint64_t entityId = EditorScene::GetID(handle);
						const TransformComponent previous = *transform;
						const auto track = [&](const char *what)
						{
							if (ImGui::IsItemActivated())
							{
								s_EditedEntity = entityId;
								s_EditedBefore = previous;
							}
							if (ImGui::IsItemDeactivatedAfterEdit() && s_EditedEntity == entityId)
							{
								EditChanges::ChangeSet changes = EditChanges::Begin();
								changes.entities.push_back({ entityId, s_EditedBefore, *transform });
								changes.syncProject = true;
								EditChanges::Publish(changes);
								EditChanges::Record(std::string("Edit ") + what + " of " + EditorScene::GetName(handle), std::move(changes));
								s_EditedEntity = 0;
							}
						};

						EditorUI::PropertyLabel("Position");
						bool moved = DragVec3("##Position", transform->translation, 0.05f);
						track("position");

						// Stored in radians, edited in degrees.
						EditorUI::PropertyLabel("Rotation");
						Vec3 degrees = transform->GetRotationEuler() * xMath::RAD_TO_DEG;
						if (DragVec3("##Rotation", degrees, 0.5f, "%.1f"))
						{
							transform->SetRotationEuler(degrees * xMath::DEG_TO_RAD);
							moved = true;
						}
						track("rotation");

						EditorUI::PropertyLabel("Scale");
						moved |= DragVec3("##Scale", transform->scale, 0.01f);
						track("scale");

						// the GPU scene re-reads the subtree's world matrices this frame instead of within its round robin
						if (moved)
							GpuScene::MarkTransformDirty(handle);


						EditorUI::EndPropertyGrid();
					}
				}

				DrawXPlaneObject(m_Context, m_Context ? m_Context->selectedEntity : 0);
				DrawAirportElement(m_Context, m_Context ? m_Context->selectedEntity : 0);

				if (EditorUI::SectionHeader("COMPONENTS"))
				{
					auto listComponent = [](const char *name, const bool present)
					{
						if (present)
							ImGui::BulletText("%s", name);
					};

					Entity *entity = EditorScene::FindEntity(handle);
					listComponent("Transform", EditorScene::Has<TransformComponent>(handle));
					listComponent("Relationship", EditorScene::Has<RelationshipComponent>(handle));
					listComponent("Camera", EditorScene::Has<CameraComponent>(handle));
					listComponent("Renderable", entity && entity->HasComponent<Renderable>());
					listComponent("Directional Light", EditorScene::Has<DirectionalLightComponent>(handle));
					listComponent("Point Light", EditorScene::Has<PointLightComponent>(handle));
					listComponent("Spot Light", EditorScene::Has<SpotLightComponent>(handle));
					listComponent("X-Plane Object", EditorScene::Has<XPlane::XPlaneObjectComponent>(handle));
					listComponent("Airport Element", EditorScene::Has<XPlane::AirportElementComponent>(handle));
				}
			}
		}
		EditorUI::EndPanel();
	}

}

// -------------------------------------------------------
