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
 * properties.cpp
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#include "properties.h"
#include "SceneryEditorX/renderer/ui/actions/color_picker.h"
#include <colors.h>
#include <Editor/core/editor_layer.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/threading/thread_pool.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/ui/ui.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/components/component.h>
#include <SceneryEditorX/scene/components/spline.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	WeakRef<MaterialAsset> Properties::m_inspected_material;
	
	namespace
	{
		// color pickers
		Scope<ButtonColorPicker> m_material_color_picker;
		Scope<ButtonColorPicker> m_colorPicker_light;
		Scope<ButtonColorPicker> m_colorPicker_camera;
		Scope<ButtonColorPicker> m_colorPicker_particle_start;
		Scope<ButtonColorPicker> m_colorPicker_particle_end;
	
		// context menu state
		std::string context_menu_id;
		Component* copied_component = nullptr;
	
		// deferred component removal - storing the id prevents a use-after-free
		// crash when the component is destroyed while its Show* function is still on the stack
		ComponentType pending_removal_type = ComponentType::MaxEnum;
		Entity*       pending_removal_owner = nullptr;
	
		// component content tracking
		bool component_content_active = false;
	
		//----------------------------------------------------------
		// design system - consistent spacing, colors, and dimensions
		//----------------------------------------------------------
	
		namespace design
		{
			// spacing
			constexpr float SPACING_XS     = 2.0f;
			constexpr float SPACING_SM     = 4.0f;
			constexpr float SPACING_MD     = 8.0f;
			constexpr float SPACING_LG     = 12.0f;
			constexpr float SPACING_XL     = 16.0f;
			constexpr float SPACING_XXL    = 24.0f;
	
			// layout
			constexpr float LABEL_WIDTH    = 0.38f;  // percentage of available width
			constexpr float ROW_HEIGHT     = 26.0f;
			constexpr float SECTION_GAP    = 6.0f;
	
			// component accent colors (subtle, professional)
			inline ImVec4 AccentEntity()			{ return ImVec4(0.45f, 0.55f, 0.70f, 1.0f); }
			inline ImVec4 AccentLight()			{ return ImVec4(0.85f, 0.75f, 0.35f, 1.0f); }
			inline ImVec4 AccentCamera()			{ return ImVec4(0.50f, 0.70f, 0.55f, 1.0f); }
			inline ImVec4 AccentRenderable()		{ return ImVec4(0.60f, 0.50f, 0.70f, 1.0f); }
			inline ImVec4 AccentMaterial()			{ return ImVec4(0.70f, 0.55f, 0.50f, 1.0f); }
			inline ImVec4 AccentPhysics()			{ return ImVec4(0.55f, 0.65f, 0.80f, 1.0f); }
			inline ImVec4 AccentAudio()			{ return ImVec4(0.70f, 0.45f, 0.55f, 1.0f); }
			inline ImVec4 AccentTerrain()			{ return ImVec4(0.50f, 0.70f, 0.45f, 1.0f); }
			inline ImVec4 AccentVolume()			{ return ImVec4(0.55f, 0.55f, 0.75f, 1.0f); }
			inline ImVec4 AccentSpline()			{ return ImVec4(0.30f, 0.75f, 0.70f, 1.0f); }
			inline ImVec4 AccentSplineFollower()	{ return ImVec4(0.35f, 0.80f, 0.65f, 1.0f); }
			inline ImVec4 AccentScript()			{ return ImVec4(0.60f, 0.70f, 0.50f, 1.0f); }
			inline ImVec4 AccentParticles()		{ return ImVec4(0.90f, 0.55f, 0.30f, 1.0f); }
	
			// helper to get dimmed version for backgrounds
			inline ImVec4 Dimmed(const ImVec4& color, float factor = 0.15f)
			{
				return ImVec4(color.x * factor, color.y * factor, color.z * factor, 0.4f);
			}
		}
	
		//----------------------------------------------------------
		// layout helpers - consistent property row rendering
		//----------------------------------------------------------
	
		namespace layout
		{
			// get label column width
			inline float LabelWidth()
			{
				return ImGui::GetContentRegionAvail().x * design::LABEL_WIDTH;
			}
	
			// get value column width
			inline float ValueWidth()
			{
				return ImGui::GetContentRegionAvail().x * (1.0f - design::LABEL_WIDTH) - design::SPACING_SM;
			}
	
			// start a property row with label
			inline void begin_property(const char* label, const char* tooltip = nullptr)
			{
				ImGui::AlignTextToFramePadding();
	
				// subtle text color for labels
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
				ImGui::TextUnformatted(label);
				ImGui::PopStyleColor();
	
				if (tooltip && ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(300.0f);
					ImGui::TextUnformatted(tooltip);
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
	
				ImGui::SameLine(LabelWidth());
				ImGui::SetNextItemWidth(ValueWidth());
			}
	
			// property row without label (for multi-value rows)
			inline void begin_value()
			{
				ImGui::SameLine(LabelWidth());
				ImGui::SetNextItemWidth(ValueWidth());
			}
	
			// position cursor at value column (alias for begin_value)
			inline void move_to_value_column()
			{
				ImGui::SameLine(LabelWidth());
				ImGui::SetNextItemWidth(ValueWidth());
			}
	
			// add vertical spacing between groups
			inline void group_spacing()
			{
				ImGui::Dummy(ImVec2(0, design::SECTION_GAP));
			}
	
			// draw a subtle horizontal separator
			inline void separator()
			{
				ImGui::Dummy(ImVec2(0, design::SPACING_SM));
				ImVec2 p = ImGui::GetCursorScreenPos();
				ImGui::GetWindowDrawList()->AddLine(
					ImVec2(p.x, p.y),
					ImVec2(p.x + ImGui::GetContentRegionAvail().x, p.y),
					IM_COL32(255, 255, 255, 20), 1.0f
				);
				ImGui::Dummy(ImVec2(0, design::SPACING_MD));
			}
	
			// section header within a component
			inline void section_header(const char* title)
			{
				ImGui::Dummy(ImVec2(0, design::SPACING_SM));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
				ImGui::PushFont(EditorLayer::fontBold);
				ImGui::TextUnformatted(title);
				ImGui::PopFont();
				ImGui::PopStyleColor();
				ImGui::Dummy(ImVec2(0, design::SPACING_XS));
			}
		}
	
		//----------------------------------------------------------
		// selection helpers
		//----------------------------------------------------------
	
		Entity* get_selected_entity()
		{
			if (Camera* camera = Scene::GetCamera())
			{
				return camera->GetSelectedEntity();
			}
			return nullptr;
		}
	
		uint32_t get_selected_entity_count()
		{
			if (Camera* camera = Scene::GetCamera())
			{
				return camera->GetSelectedEntities().size();
			}
			return 0;
		}
	
		const std::vector<Entity*>& get_selected_entities()
		{
			static std::vector<Entity*> empty;
			if (Camera* camera = Scene::GetCamera())
			{
				return camera->GetSelectedEntities();
			}
			return empty;
		}
	
		//----------------------------------------------------------
		// component context menu
		//----------------------------------------------------------
	
		void component_context_menu_options(const std::string & id, Component* component, const bool removable)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(design::SPACING_MD, design::SPACING_MD));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(design::SPACING_MD, design::SPACING_SM));
	
			if (ImGui::BeginPopup(id.c_str()))
			{
				if (removable)
				{
					if (ImGui::MenuItem("Remove Component"))
					{
						if (Entity* entity = get_selected_entity())
						{
							if (component)
							{
								// defer the removal so we don't destroy a component
								// while its Show* function is still on the call stack
							   pending_removal_type  = component->GetType();
								pending_removal_owner = entity;
							}
						}
					}
				}
	
				if (ImGui::MenuItem("Copy Attributes"))
				{
					copied_component = component;
				}
	
				ImGui::BeginDisabled(!copied_component || (copied_component && copied_component->GetType() != component->GetType()));
				if (ImGui::MenuItem("Paste Attributes"))
				{
					if (copied_component && copied_component->GetType() == component->GetType())
					{
						component->SetAttributes(copied_component->GetAttributes());
					}
				}
				ImGui::EndDisabled();
	
				ImGui::EndPopup();
			}
	
			ImGui::PopStyleVar(2);
		}
	
		//----------------------------------------------------------
		// component begin/end - styled component headers and content
		//----------------------------------------------------------
	
		bool ComponentBegin(const char* name, const ImVec4& accent_color, Component* component_instance, bool options = true, const bool removable = true)
		{
			ImGui::PushID(name);
	
			// header styling
			ImVec4 header_bg       = design::Dimmed(accent_color, 0.25f);
			ImVec4 header_hovered  = design::Dimmed(accent_color, 0.35f);
			ImVec4 header_active   = design::Dimmed(accent_color, 0.30f);
	
			ImGui::PushStyleColor(ImGuiCol_Header, header_bg);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, header_hovered);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, header_active);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(design::SPACING_MD, design::SPACING_MD));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	
			// draw collapsing header
			ImGui::PushFont(EditorLayer::fontBold);
			const bool is_expanded = UI::CollapsingHeader(name, ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);
			ImGui::PopFont();
	
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(3);
	
			// accent bar on the left of header
			ImVec2 header_min = ImGui::GetItemRectMin();
			ImVec2 header_max = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddRectFilled(
				ImVec2(header_min.x, header_min.y + 2.0f),
				ImVec2(header_min.x + 3.0f, header_max.y - 2.0f),
				ImGui::ColorConvertFloat4ToU32(accent_color),
				2.0f
			);
	
			// gear icon for context menu
			if (options)
			{
				// size based on header height
				const float header_height = header_max.y - header_min.y;
				const float v_padding     = 5.0f;
				const float r_padding     = 8.0f;  // small padding from right edge
				const float icon_size     = header_height - v_padding * 2.0f;
	
				// position: near right edge with small padding
				float icon_x = header_max.x - icon_size - r_padding;
				float icon_y = header_min.y + v_padding;
	
				ImGui::SetCursorScreenPos(ImVec2(icon_x, icon_y));
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				if (UI::ImageButton(ResourceCache::GetIcon(IconType::Gear), {icon_x, icon_y}, false))
				{
					context_menu_id = name;
					ImGui::OpenPopup(context_menu_id.c_str());
				}
				ImGui::PopStyleVar();
				ImGui::PopStyleColor(2);
	
				if (component_instance && context_menu_id == name)
				{
					component_context_menu_options(context_menu_id, component_instance, removable);
				}
			}
	
			// wrap expanded content in styled child region
			if (is_expanded)
			{
				component_content_active = true;
	
				// content background
				const ImVec4& bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
				ImVec4 content_bg = ImVec4(bg.x + 0.02f, bg.y + 0.02f, bg.z + 0.02f, 1.0f);
	
				ImGui::PushStyleColor(ImGuiCol_ChildBg, content_bg);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(design::SPACING_LG, design::SPACING_MD));
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(design::SPACING_SM, design::SPACING_SM));
				ImGui::BeginChild(("##content_" + std::string(name)).c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
			}
	
			return is_expanded;
		}
	
		void component_end()
		{
			if (component_content_active)
			{
				ImGui::EndChild();
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor();
				component_content_active = false;
			}
			ImGui::PopID();
			ImGui::Dummy(ImVec2(0, design::SPACING_SM));
		}
	
		//----------------------------------------------------------
		// custom property widgets
		//----------------------------------------------------------
	
		// styled combo box
		bool PropertyCombo(const char* label, const std::vector<std::string>& options, uint32_t* index, const char* tooltip = nullptr)
		{
			layout::begin_property(label, tooltip);
			return UI::ComboBox(("##" + std::string(label)).c_str(), options, index);
		}
	
		// styled float input with drag
		bool PropertyFloat(const char* label, float* value, float speed = 0.1f, float min = 0.0f, float max = 0.0f, const char* tooltip = nullptr, const char* format = "%.3f")
		{
			layout::begin_property(label, tooltip);
			return UI::DrawFloatWrap(("##" + std::string(label)).c_str(), value, speed, min, max, format);
		}
	
		// styled toggle switch
		bool PropertyToggle(const char* label, bool* value, const char* tooltip = nullptr)
		{
			layout::begin_property(label, tooltip);
			return UI::CheckBox(("##" + std::string(label)).c_str(), value);
		}
	
		// styled text input (read-only display)
		void PropertyText(const char* label, const std::string& text, const char* tooltip = nullptr)
		{
			layout::begin_property(label, tooltip);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
			ImGui::TextUnformatted(text.c_str());
			ImGui::PopStyleColor();
		}
	
		// styled text input field
		void PropertyInputText(const char* label, std::string* text, bool readonly = false, const char* tooltip = nullptr)
		{
			layout::begin_property(label, tooltip);
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll;
			if (readonly) flags |= ImGuiInputTextFlags_ReadOnly;
			ImGui::InputText(("##" + std::string(label)).c_str(), reinterpret_cast<char *>(text), flags);
		}
	
		// color picker property
		void PropertyColor(const char* label, ButtonColorPicker* picker, const char* tooltip = nullptr)
		{
			layout::begin_property(label, tooltip);
			picker->Update();
		}
	
		// vector3 property with colored axis indicators - respects label/value columns
		void PropertyVec3(const char* label, xMath::Vec3& vec, const char* tooltip = nullptr)
		{
			ImGui::PushID(label);
	
			// label in left column
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(label);
			ImGui::PopStyleColor();
	
			if (tooltip && ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(tooltip);
				ImGui::EndTooltip();
			}
	
			// move to value column
			ImGui::SameLine(layout::LabelWidth());
	
			// use full remaining width for the 3 inputs
			float total_avail     = ImGui::GetContentRegionAvail().x;
			float axis_label_w    = 10.0f;
			float label_to_input  = 10.0f;  // space between X/Y/Z label and input box
			float between_groups  = 8.0f;   // space between groups
			float input_width     = (total_avail - axis_label_w * 3 - label_to_input * 3 - between_groups * 2) / 3.0f;
	
			const ImU32 colors[3] = {
				IM_COL32(200, 60, 60, 255),   // x - red
				IM_COL32(90, 160, 40, 255),   // y - green
				IM_COL32(50, 120, 200, 255)   // z - blue
			};
			const char* axis[3] = { "X", "Y", "Z" };
			float* values[3] = { &vec.x, &vec.y, &vec.z };
	
			for (int i = 0; i < 3; ++i)
			{
				if (i > 0) ImGui::SameLine(0, between_groups);
	
				// axis label with color
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(colors[i]));
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(axis[i]);
				ImGui::PopStyleColor();
	
				// SPACE between label and input
				ImGui::SameLine(0, label_to_input);
	
				// input field - wide
				ImGui::PushItemWidth(input_width);
				ImGui::PushID(i);
				UI::DrawFloatWrap("##v", values[i], 0.01f);
				ImGui::PopID();
				ImGui::PopItemWidth();
			}
	
			ImGui::PopID();
		}
	
		// transform widget with position, rotation, scale
		void property_transform(Entity* entity)
		{
			xMath::Vec3 position    = entity->GetPositionLocal();
		   xMath::Vec3 rotation	= entity->GetRotationLocal();
			xMath::Vec3 scale       = entity->GetScaleLocal();
	
			// position
			PropertyVec3("Position", position, "local position in meters");
	
			// rotation
		  PropertyVec3("Rotation", rotation, "local rotation in radians");
	
			// scale
			PropertyVec3("Scale", scale, "local scale multiplier");
	
			entity->SetPositionLocal(position);
		   entity->SetRotationLocal(rotation);
			entity->SetScaleLocal(scale);
		}
	
		// file/resource selector with browse button
		bool property_resource(const char* label, std::string* name, const char* tooltip, const std::function<void(const std::string&)>& on_browse)
		{
			layout::begin_property(label, tooltip);
	
			float browse_width = 28.0f;
			float input_width  = layout::ValueWidth() - browse_width - design::SPACING_SM;
	
			ImGui::PushItemWidth(input_width);
			ImGui::InputText(("##" + std::string(label)).c_str(), reinterpret_cast<char *>(name), ImGuiInputTextFlags_ReadOnly);
			ImGui::PopItemWidth();
	
			ImGui::SameLine(0, design::SPACING_SM);
	
			/*
			if (file_selection::BrowseButton(("browse_" + std::string(label)).c_str()))
			{
				file_selection::open(on_browse);
				return true;
			}
			*/
	
			return false;
		}
	}
	
	Properties::Properties(EditorLayer* editor) : Widget(editor)
	{
		m_Title         = "Properties";
		m_InitialSize.x = 500;
		m_Editor = editor;
	
		m_colorPicker_light          = CreateScope<ButtonColorPicker>("Light Color Picker");
		m_material_color_picker      = CreateScope<ButtonColorPicker>("Material Color Picker");
		m_colorPicker_camera         = CreateScope<ButtonColorPicker>("Camera Color Picker");
		m_colorPicker_particle_start = CreateScope<ButtonColorPicker>("Particle Start Color");
		m_colorPicker_particle_end   = CreateScope<ButtonColorPicker>("Particle End Color");
	
		//file_selection::initialize(editor);
	}
	
	void Properties::OnTickVisible()
	{
		ImGui::BeginDisabled();
		{
			uint32_t selected_count = get_selected_entity_count();
	
			if (selected_count > 1)
			{
				// multiple entities selected - show summary
				ImGui::Dummy(ImVec2(0, design::SPACING_MD));
	
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.4f, 1.0f));
				ImGui::PushFont(EditorLayer::fontBold);
				char buf[64];
				std::snprintf(buf, sizeof(buf), "%d entities selected", selected_count);
				ImGui::TextUnformatted(buf);
				ImGui::PopFont();
				ImGui::PopStyleColor();
	
				layout::separator();
	
				// list selected entities
				const auto& selected = get_selected_entities();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
				for (Entity* entity : selected)
				{
					if (entity)
					{
						ImGui::BulletText("%s", entity->GetObjectName().c_str());
					}
				}
				ImGui::PopStyleColor();
			}
			else if (Entity* entity = get_selected_entity())
			{
				ShowEntity(entity);
				ShowScript(entity->GetComponent<Script>());
				ShowLight(entity->GetComponent<Light>());
				ShowCamera(entity->GetComponent<Camera>());
				ShowTerrain(entity->GetComponent<Terrain>());
				ShowSpline(entity->GetComponent<Spline>());
	
	
				// re-fetch after ShowSpline since clearing a road mesh removes the render
				Renderable* render = entity->GetComponent<Renderable>();
				MaterialAsset* material = render ? render->GetMaterialAsset() : nullptr;
				ShowRender(render);
				ShowMaterial(material);
				ShowVolume(entity->GetComponent<Volume>());
				ShowParticleSystem(entity->GetComponent<ParticleSystem>());
	
				ShowAddComponentButton();
	
				// process deferred component removal now that all Show* calls are done
			   if (pending_removal_owner && pending_removal_type != ComponentType::MaxEnum)
				{
				   pending_removal_owner->RemoveComponentByType(pending_removal_type);
					pending_removal_owner = nullptr;
				  pending_removal_type  = ComponentType::MaxEnum;
				}
			}
			else if (!m_inspected_material.Expired())
			{
				ShowMaterial(m_inspected_material.Lock().Get());
			}
			else
			{
				// empty state
				ImGui::Dummy(ImVec2(0, design::SPACING_XXL));
				ImVec2 text_size = ImGui::CalcTextSize("Select an entity to view properties");
				ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - text_size.x) * 0.5f);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				ImGui::TextUnformatted("Select an entity to view properties");
				ImGui::PopStyleColor();
			}
		}
		ImGui::EndDisabled();
	
		// handle file browser dialog
		//file_selection::tick();
	}
	
	void Properties::Inspect(Entity* entity)
	{
		// if we were previously inspecting a material, save changes
		if (!m_inspected_material.Expired())
		{
			m_inspected_material.Lock()->SaveToFile(m_inspected_material.Lock()->GetResourceFilePath());
		}
		m_inspected_material.Reset();
	}
	
	void Properties::Inspect(const Ref<MaterialAsset> &material)
	{
		// clear entity selection so the material is shown instead
		if (Camera* camera = Scene::GetCamera())
		{
			camera->ClearSelection();
		}
	
		m_inspected_material = material;
	}
	
	void Properties::ShowEntity(Entity* entity) const
	{
		if (ComponentBegin("Entity", design::AccentEntity(), nullptr, true, false))
		{
			// entity name display
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::PushFont(EditorLayer::fontBold);
			ImGui::TextUnformatted(entity->GetObjectName().c_str());
			ImGui::PopFont();
			ImGui::PopStyleColor();
	
			/*
			// prefab indicator
			if (entity->HasPrefabData())
			{
				ImGui::SameLine();
	
				// badge styling
				bool is_code = entity->IsCodePrefab();
				bool is_file = entity->IsFilePrefab();
				ImVec4 badge_color = is_code ? ImVec4(0.55f, 0.35f, 0.70f, 1.0f) : ImVec4(0.30f, 0.60f, 0.45f, 1.0f);
	
				ImGui::PushStyleColor(ImGuiCol_Text, badge_color);
				ImGui::PushFont(Editor::fontBold);
				ImGui::TextUnformatted(is_code ? "[prefab:code]" : "[prefab:file]");
				ImGui::PopFont();
				ImGui::PopStyleColor();
	
				layout::group_spacing();
	
				// prefab type (for code prefabs)
				if (is_code)
				{
					PropertyText("Prefab Type", entity->GetPrefabType(), "registered code prefab type");
				}
	
				// prefab file path (for file prefabs)
				if (is_file)
				{
					PropertyText("Prefab File", entity->GetPrefabFilePath(), "path to the .prefab file");
				}
	
				// code prefab attributes (read-only)
				if (is_code && !entity->GetPrefabAttributes().empty())
				{
					layout::separator();
					layout::section_header("Prefab Attributes");
	
					for (const auto& [key, value] : entity->GetPrefabAttributes())
					{
						if (key == "type")
							continue; // already shown above
						PropertyText(key.c_str(), value, "prefab attribute (read-only)");
					}
				}
	
				// prefab action buttons
				layout::separator();
	
				// save prefab (for file prefabs only)
				if (is_file)
				{
					float button_width = ImGui::GetContentRegionAvail().x;
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.50f, 0.35f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.60f, 0.40f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.45f, 0.30f, 1.0f));
					if (UI::Button("Save Prefab", ImVec2(button_width, 0)))
					{
						Prefab::SaveToFile(entity, entity->GetPrefabFilePath());
					}
					ImGui::PopStyleColor(3);
				}
	
				// detach from prefab
				{
					float button_width = ImGui::GetContentRegionAvail().x;
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.35f, 0.30f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.40f, 0.35f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.30f, 0.25f, 1.0f));
					if (UI::Button("Detach from Prefab", ImVec2(button_width, 0)))
					{
						entity->ClearPrefabData();
					}
					ImGui::PopStyleColor(3);
				}
			}
			*/
	
			layout::group_spacing();
	
			// active toggle
			bool is_active = entity->GetActive();
			if (PropertyToggle("Active", &is_active, "enable or disable this entity"))
			{
				entity->SetActive(is_active);
			}
	
			layout::separator();
			layout::section_header("Transform");
	
			// transform properties
			property_transform(entity);
		}
		component_end();
	}
	
	void Properties::ShowScript(Script* script) const
	{
		if (!script)
			return;
	
		if (ComponentBegin("Script", design::AccentScript(), nullptr))
		{
			PropertyText("Status", "Script inspector is temporarily disabled");
		}
		component_end();
	}
	
	void Properties::ShowLight(Light* light) const
	{
		if (!light)
			return;
	
		if (ComponentBegin("Light", design::AccentLight(), light))
		{
			//= REFLECT ==========================================================================
			static std::vector<std::string> types = { "Directional", "Point", "Spot", "Area" };
			float intensity             = light->GetIntensityLumens();
			float temperature_kelvin    = light->GetTemperature();
			float angle                 = light->GetAngle() * xMath::RAD_TO_DEG * 2.0f;
			bool shadows                = light->GetFlag(LightFlags::Shadows);
			bool shadows_screen_space   = light->GetFlag(LightFlags::ShadowsScreenSpace);
			bool volumetric             = light->GetFlag(LightFlags::Volumetric);
			float range                 = light->GetRange();
			float area_width            = light->GetAreaWidth();
			float area_height           = light->GetAreaHeight();
			m_colorPicker_light->SetColor(light->GetLightColor());
			//====================================================================================
	
			// type
			uint32_t selection_index = static_cast<uint32_t>(light->GetLightType());
			if (PropertyCombo("Type", types, &selection_index))
			{
				light->SetLightType(static_cast<LightType>(selection_index));
			}
	
			layout::separator();
			layout::section_header("Appearance");
	
			// color
			PropertyColor("Color", m_colorPicker_light.get(), "light color");
	
			// temperature
			PropertyFloat("Temperature", &temperature_kelvin, 10.0f, 1000.0f, 40000.0f, "color temperature in kelvin", "%.0f K");
	
			// intensity
			{
				static std::vector<std::string> intensity_types = {
					"Stadium",
					"500W Bulb",
					"150W Bulb",
					"100W Bulb",
					"60W Bulb",
					"25W Bulb",
					"Flashlight",
					"Black Hole",
					"Custom"
				};
	
				bool is_directional = light->GetLightType() == LightType::Directional;
	
				if (!is_directional)
				{
					uint32_t intensity_type_index = static_cast<uint32_t>(light->GetIntensity());
					if (PropertyCombo("Preset", intensity_types, &intensity_type_index, "common light intensity presets"))
					{
						light->SetIntensity(static_cast<LightIntensity>(intensity_type_index));
						intensity = light->GetIntensityLumens();
					}
				}
	
				const char* unit_tooltip = is_directional ? "intensity in lux" : "intensity in lumens";
				PropertyFloat("Intensity", &intensity, 10.0f, 0.0f, 120000.0f, unit_tooltip, is_directional ? "%.0f lux" : "%.0f lm");
			}
	
			layout::separator();
			layout::section_header("Shadows");
	
			PropertyToggle("Enabled", &shadows, "cast shadows from this light");
	
			if (shadows)
			{
				PropertyToggle("Screen Space", &shadows_screen_space, "screen space contact shadows");
				PropertyToggle("Volumetric", &volumetric, "volumetric light scattering");
			}
	
			// directional-specific options
			if (light->GetLightType() == LightType::Directional)
			{
				layout::separator();
				layout::section_header("Day/Night");
	
				bool day_night_cycle = light->GetFlag(LightFlags::DayNightCycle);
				if (PropertyToggle("Day/Night Cycle", &day_night_cycle, "automatic sun movement"))
				{
					light->SetFlag(LightFlags::DayNightCycle, day_night_cycle);
				}
			}
	
			// range (point/spot/area)
			if (light->GetLightType() != LightType::Directional)
			{
				layout::separator();
				layout::section_header("Attenuation");
				PropertyFloat("Range", &range, 0.1f, 0.0f, 1000.0f, "light falloff distance in meters", "%.1f m");
			}
	
			// spot angle
			if (light->GetLightType() == LightType::Spot)
			{
				PropertyFloat("Angle", &angle, 0.5f, 1.0f, 179.0f, "cone angle in degrees", "%.1f°");
			}
	
			// area dimensions
			if (light->GetLightType() == LightType::Area)
			{
				layout::separator();
				layout::section_header("Dimensions");
				PropertyFloat("Width", &area_width, 0.01f, 0.01f, 100.0f, "area light width", "%.2f m");
				PropertyFloat("Height", &area_height, 0.01f, 0.01f, 100.0f, "area light height", "%.2f m");
			}
	
			//= MAP ===================================================================================================
			if (intensity != light->GetIntensityLumens())             light->SetIntensity(intensity);
			if (angle != light->GetAngle() * xMath::RAD_TO_DEG * 0.5f) light->SetAngle(angle * xMath::DEG_TO_RAD * 0.5f);
			if (range != light->GetRange())                           light->SetRange(range);
			if (area_width != light->GetAreaWidth())                  light->SetAreaWidth(area_width);
			if (area_height != light->GetAreaHeight())                light->SetAreaHeight(area_height);
			if (m_colorPicker_light->GetStandardColor() != light->GetLightColor()) light->SetLightColor(m_colorPicker_light->GetStandardColor());
			if (temperature_kelvin != light->GetTemperature())        light->SetTemperature(temperature_kelvin);
			light->SetFlag(LightFlags::ShadowsScreenSpace, shadows_screen_space);
			light->SetFlag(LightFlags::Volumetric, volumetric);
			light->SetFlag(LightFlags::Shadows, shadows);
			//=========================================================================================================
		}
		component_end();
	}
	
	void Properties::ShowRender(Renderable* renderable) const
	{
		if (!renderable)
			return;
	
		if (ComponentBegin("Render", design::AccentRenderable(), renderable))
		{
			//= REFLECT ========================================================================================================
			std::string& name_mesh                 = const_cast<std::string&>(renderable->GetMeshName());
			MaterialAsset* material                = renderable->GetMaterialAsset();
			uint32_t instance_count           = renderable->GetInstanceCount();
			static std::string name_material_empty = "N/A";
			std::string& name_material             = material ? const_cast<std::string&>(material->GetObjectName()) : name_material_empty;
			bool cast_shadows                 = renderable->HasFlag(RenderableFlags::CastsShadows);
			bool is_visible                   = renderable->IsVisible();
			//==================================================================================================================
	
			// mesh info
			PropertyInputText("Mesh", &name_mesh, true);
	
			// lod information
			int lod_count = renderable->GetLodCount();
			if (lod_count > 0)
			{
				layout::separator();
				layout::section_header("Level of Detail");
	
				// styled lod table with visible cells
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(design::SPACING_MD, design::SPACING_SM));
				ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.16f, 0.16f, 0.18f, 1.0f));
	
				if (ImGui::BeginTable("##lod_table", lod_count + 1,
					ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame))
				{
					// header row
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 70.0f);
					for (int i = 0; i < lod_count; ++i)
					{
						char col_name[16];
						std::snprintf(col_name, sizeof(col_name), "LOD %d", i);
						ImGui::TableSetupColumn(col_name);
					}
	
					ImGui::TableHeadersRow();
	
					// vertices row
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted("Vertices");
					for (int i = 0; i < lod_count; ++i)
					{
						ImGui::TableSetColumnIndex(i + 1);
						ImGui::Text("%d", renderable->GetVertexCount(i));
					}
	
					// indices row
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted("Indices");
					for (int i = 0; i < lod_count; ++i)
					{
						ImGui::TableSetColumnIndex(i + 1);
						ImGui::Text("%d", renderable->GetIndexCount(i));
					}
	
					ImGui::EndTable();
				}
				ImGui::PopStyleColor(5);
				ImGui::PopStyleVar();
	
				if (!renderable->HasInstancing())
				{
					char lod_buf[32];
					std::snprintf(lod_buf, sizeof(lod_buf), "%u", renderable->GetLodIndex());
					PropertyText("Current LOD", lod_buf);
				}
			}
	
			// instancing
			if (instance_count > 1 || renderable->HasInstancing())
			{
				layout::separator();
				layout::section_header("Instancing");
	
				char buf[32];
				std::snprintf(buf, sizeof(buf), "%u", instance_count);
				PropertyText("Instances", buf);
	
				if (renderable->HasInstancing() && ImGui::TreeNode("Instance Transforms"))
				{
					for (uint32_t i = 0; i < renderable->GetInstanceCount(); ++i)
					{
						Matrix instance = renderable->GetInstance(i, true);
	
						ImGui::PushID(static_cast<int>(i));
	
					 xMath::Vec3 pos, scale;
						Quat rot;
						instance.Decompose(scale, rot, pos);
						xMath::Vec3 euler = rot.ToEulerDegrees();
	
						auto compose_matrix = [](const xMath::Vec3& in_scale, const Quat& in_rot, const xMath::Vec3& in_pos)
						{
							xMath::Matrix matrix;
							return matrix.CreateScale(in_scale) * matrix.CreateRotation(in_rot) * matrix.CreateTranslation(in_pos);
						};
	
						char instance_name[32];
						std::snprintf(instance_name, sizeof(instance_name), "Instance %u", i);
	
						if (ImGui::TreeNode(instance_name))
						{
							if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
							{
							   instance = compose_matrix(scale, rot, pos);
							}
							if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f))
							{
							 rot = Quat::EulerDegrees(euler.x, euler.y, euler.z);
								instance = compose_matrix(scale, rot, pos);
							}
							if (ImGui::DragFloat3("Scale", &scale.x, 0.1f))
							{
							   instance = compose_matrix(scale, rot, pos);
							}
							ImGui::TreePop();
						}
	
						ImGui::PopID();
					}
					ImGui::TreePop();
				}
			}
	
			layout::separator();
			layout::section_header("Rendering");
	
			// draw distance
			float draw_distance = renderable->GetMaxRenderDistance();
			if (PropertyFloat("Draw Distance", &draw_distance, 1.0f, 0.0f, 10000.0f, "maximum render distance", "%.0f m"))
			{
				renderable->SetMaxRenderDistance(draw_distance);
			}
	
			// material
			{
				layout::begin_property("Material", "assigned material");
	
				float button_width = 28.0f;
				float input_width  = layout::ValueWidth() - button_width * 2 - design::SPACING_SM * 2;
	
				ImGui::PushItemWidth(input_width);
				ImGui::InputText("##Material", reinterpret_cast<char *>(&name_material), ImGuiInputTextFlags_ReadOnly);
				ImGui::PopItemWidth();
	
				/*
				// drag drop for material
				if (auto payload = UI::ReceiveDragDropPayload(UI::DragPayloadType::Material))
				{
					renderable->SetMaterial(std::get<const char*>(payload->data));
				}
				*/
	
				/*
				// browse
				ImGui::SameLine(0, design::SPACING_SM);
				if (file_selection::browse_button("browse_Material"))
				{
					file_selection::open([renderable](const std::string& path) {
						if (IO::FileSystem::IsEngineMaterialFile(path))
						{
							renderable->SetMaterial(path);
						}
					});
				}
				*/
	
				// clear - reset to default material
				ImGui::SameLine(0, design::SPACING_SM);
				ImGui::PushID("clear_material");
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
				if (UI::Button("x"))
				{
					renderable->SetDefaultMaterial();
				}
				ImGui::PopStyleVar();
				ImGui::PopID();
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("reset to default material");
					ImGui::EndTooltip();
				}
			}
	
			layout::group_spacing();
	
			PropertyToggle("Cast Shadows", &cast_shadows, "whether this object casts shadows");
			PropertyText("Visible", is_visible ? "Yes" : "No", "current visibility state");
	
			//= MAP =========================================================
			renderable->SetFlag(RenderableFlags::CastsShadows, cast_shadows);
			//===============================================================
		}
		component_end();
	}
	
	/*
	void Properties::ShowPhysics(Physics* body) const
	{
		if (!body)
			return;
	
		if (component_begin("Physics", design::accent_physics(), body))
		{
			// reflect
			float mass             = body->GetMass();
			float friction         = body->GetFriction();
			float friction_rolling = body->GetFrictionRolling();
			float restitution      = body->GetRestitution();
			bool freeze_pos_x      = static_cast<bool>(body->GetPositionLock().x);
			bool freeze_pos_y      = static_cast<bool>(body->GetPositionLock().y);
			bool freeze_pos_z      = static_cast<bool>(body->GetPositionLock().z);
			bool freeze_rot_x      = static_cast<bool>(body->GetRotationLock().x);
			bool freeze_rot_y      = static_cast<bool>(body->GetRotationLock().y);
			bool freeze_rot_z      = static_cast<bool>(body->GetRotationLock().z);
			xMath::Vec3 center_of_mass = body->GetCenterOfMass();
			bool is_static         = body->IsStatic();
			bool is_kinematic      = body->IsKinematic();
	
			// body type
			static vector<std::string> body_types = {
				"Box", "Sphere", "Plane", "Capsule",
				"Mesh", "Mesh (Convex)", "Controller", "Vehicle", "Cloth"
			};
	
			uint32_t body_type_index = static_cast<uint32_t>(body->GetBodyType());
			if (property_combo("Type", body_types, &body_type_index, "body type"))
			{
				body->SetBodyType(static_cast<BodyType>(body_type_index));
			}
	
			layout::separator();
			layout::section_header("Physical Properties");
	
			property_float("Mass", &mass, 0.1f, 0.0f, 10000.0f, "mass in kilograms", "%.2f kg");
			property_float("Friction", &friction, 0.01f, 0.0f, 1.0f, "surface friction coefficient", "%.3f");
			property_float("Rolling Friction", &friction_rolling, 0.01f, 0.0f, 1.0f, "rolling friction coefficient", "%.3f");
			property_float("Restitution", &restitution, 0.01f, 0.0f, 1.0f, "bounciness", "%.3f");
	
			layout::separator();
			layout::section_header("Constraints");
	
			// freeze position with axis toggles
			{
				layout::begin_property("Freeze Position", "lock position on specific axes");
	
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
				ImGui::TextUnformatted("X");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				UI::toggle_switch("##freeze_pos_x", &freeze_pos_x);
	
				ImGui::SameLine(0, design::spacing_md);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
				ImGui::TextUnformatted("Y");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				UI::toggle_switch("##freeze_pos_y", &freeze_pos_y);
	
				ImGui::SameLine(0, design::spacing_md);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.6f, 0.9f, 1.0f));
				ImGui::TextUnformatted("Z");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				UI::toggle_switch("##freeze_pos_z", &freeze_pos_z);
			}
	
			// freeze rotation with axis toggles
			{
				layout::begin_property("Freeze Rotation", "lock rotation on specific axes");
	
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
				ImGui::TextUnformatted("X");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				UI::toggle_switch("##freeze_rot_x", &freeze_rot_x);
	
				ImGui::SameLine(0, design::spacing_md);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
				ImGui::TextUnformatted("Y");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				UI::toggle_switch("##freeze_rot_y", &freeze_rot_y);
	
				ImGui::SameLine(0, design::spacing_md);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.6f, 0.9f, 1.0f));
				ImGui::TextUnformatted("Z");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				UI::toggle_switch("##freeze_rot_z", &freeze_rot_z);
			}
	
			layout::separator();
			layout::section_header("Body Type");
	
			property_toggle("Static", &is_static, "immovable object");
			property_toggle("Kinematic", &is_kinematic, "script-controlled movement");
	
			layout::separator();
			layout::section_header("Center of Mass");
	
			property_vector3("Offset", center_of_mass, "center of mass offset");
	
			// cloth-specific properties
			if (body->GetBodyType() == BodyType::Cloth)
			{
				layout::separator();
				layout::section_header("Cloth Simulation");
	
				float cloth_stiffness  = body->GetClothStiffness();
				float cloth_damping    = body->GetClothDamping();
				float cloth_iterations = static_cast<float>(body->GetClothIterations());
	
				property_float("Stiffness", &cloth_stiffness, 0.01f, 0.0f, 1.0f, "constraint stiffness per iteration", "%.2f");
				property_float("Damping", &cloth_damping, 0.001f, 0.0f, 1.0f, "velocity damping factor", "%.3f");
				property_float("Iterations", &cloth_iterations, 1.0f, 1.0f, 32.0f, "constraint solver iterations per step", "%.0f");
	
				bool cloth_wind = body->GetClothWindEnabled();
				if (property_toggle("Wind", &cloth_wind, "allow wind to affect cloth simulation"))
				{
					body->SetClothWindEnabled(cloth_wind);
				}
	
				if (cloth_stiffness != body->GetClothStiffness())                                      body->SetClothStiffness(cloth_stiffness);
				if (cloth_damping != body->GetClothDamping())                                          body->SetClothDamping(cloth_damping);
				if (static_cast<uint32_t>(cloth_iterations) != body->GetClothIterations()) body->SetClothIterations(static_cast<uint32_t>(cloth_iterations));
			}
	
			// map values back
			if (mass != body->GetMass())                        body->SetMass(mass);
			if (friction != body->GetFriction())                body->SetFriction(friction);
			if (friction_rolling != body->GetFrictionRolling()) body->SetFrictionRolling(friction_rolling);
			if (restitution != body->GetRestitution())          body->SetRestitution(restitution);
	
			if (freeze_pos_x != static_cast<bool>(body->GetPositionLock().x) ||
				freeze_pos_y != static_cast<bool>(body->GetPositionLock().y) ||
				freeze_pos_z != static_cast<bool>(body->GetPositionLock().z))
			{
				body->SetPositionLock(xMath::Vec3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
			}
	
			if (freeze_rot_x != static_cast<bool>(body->GetRotationLock().x) ||
				freeze_rot_y != static_cast<bool>(body->GetRotationLock().y) ||
				freeze_rot_z != static_cast<bool>(body->GetRotationLock().z))
			{
				body->SetRotationLock(xMath::Vec3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
			}
	
			if (center_of_mass != body->GetCenterOfMass()) body->SetCenterOfMass(center_of_mass);
			if (is_static != body->IsStatic())             body->SetStatic(is_static);
			if (is_kinematic != body->IsKinematic())       body->SetKinematic(is_kinematic);
		}
		component_end();
	}
	*/
	
	void Properties::ShowMaterial(MaterialAsset* material) const
	{
		if (!material)
			return;
	
		/*
		if (component_begin("Material", design::accent_material(), nullptr, false))
		{
			//= REFLECT ================================================
	
			xMath::Vec2 tiling = xMath::Vec2(
				material->GetProperty(MaterialProperty::TextureTilingX),
				material->GetProperty(MaterialProperty::TextureTilingY)
			);
	
			xMath::Vec2 offset = xMath::Vec2(
				material->GetProperty(MaterialProperty::TextureOffsetX),
				material->GetProperty(MaterialProperty::TextureOffsetY)
			);
	
			m_material_color_picker->SetColor(Color(
				material->GetProperty(MaterialProperty::ColorR),
				material->GetProperty(MaterialProperty::ColorG),
				material->GetProperty(MaterialProperty::ColorB),
				material->GetProperty(MaterialProperty::ColorA)
			));
	
			//==========================================================
	
			// material name
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::PushFont(Editor::fontBold);
			ImGui::TextUnformatted(material->GetObjectName().c_str());
			ImGui::PopFont();
			ImGui::PopStyleColor();
	
			layout::separator();
			layout::section_header("Surface");
	
			// texture slot helper lambda
			const auto show_property = [this, &material](const char* name, const char* tooltip, const MaterialTextureType mat_tex, const MapBits mat_type)
			{
				bool show_texture  = mat_tex      != MaterialTextureType::MaxEnum;
				bool show_type = mat_type		  != MapBits::MaxEnum;
				bool show_modifier = mat_property != MaterialProperty::MaxEnum;
	
				ImGui::PushID(name);
	
				// property label
				if (name)
				{
					layout::begin_property(name, tooltip);
				}
	
				// texture slot
				if (show_texture)
				{
					for (uint32_t slot = 0; slot < material->GetUsedSlotCount(); ++slot)
					{
						MaterialTextureType texture_type = static_cast<MaterialTextureType>(mat_tex);
	
						auto setter = [material, texture_type, slot](ImageResource* texture) 
						{
							material->SetTexture(texture_type, texture, slot);
						};
	
						if (slot > 0) ImGui::SameLine();
	
						// push unique id for each slot to avoid id collisions in image_slot
						ImGui::PushID(static_cast<int>(slot));
						ImageResource* texture = material->GetTexture(texture_type, slot);
						if (UI::ImageSlot(texture, setter))
						{
							file_selection::open([setter](const std::string& path)
							{
								if (IO::FileSystem::IsSupportedImageFile(path))
								{
									if (const auto tex = ResourceCache::Load<ImageResource>(path).Get())
									{
										setter(tex);
									}
								}
							});
						}
						ImGui::PopID();
					}
	
					if (show_modifier) ImGui::SameLine();
				}
	
				// modifier/multiplier
				if (show_modifier)
				{
					// constrain width to available space
					float available_width = ImGui::GetContentRegionAvail().x;
					float slider_width    = ImMin(available_width, 120.0f);
	
					if (mat_property == MaterialProperty::ColorA)
					{
						m_material_color_picker->Update();
					}
					else if (mat_property == MaterialProperty::Metalness)
					{
						bool is_metallic = material->GetProperty(mat_property) != 0.0f;
						if (UI::CheckBox("##metalness", &is_metallic))
						{
							material->SetProperty(mat_property, is_metallic ? 1.0f : 0.0f);
						}
					}
					else
					{
						ImGui::PushItemWidth(slider_width);
						float value = material->GetProperty(mat_property);
						if (UI::DrawFloatWrap("##val", &value, 0.004f, 0.0f, 1.0f))
						{
							material->SetProperty(mat_property, value);
						}
						ImGui::PopItemWidth();
					}
				}
	
				ImGui::PopID();
			};
	
	
			// properties with textures
			show_property("Color",                "Surface color",                                                                     MaterialTextureType::Color,		MapBits::Albedo);
			show_property("Roughness",            "Specifies microfacet roughness of the surface for diffuse and specular reflection", MaterialTextureType::Greyscale,	MapBits::Roughness);
			show_property("Metalness",            "Blends between a non-metallic and metallic material model",                         MaterialTextureType::Greyscale,	MapBits::Metalness);
			show_property("Normal",               "Controls the normals of the base layers",                                           MaterialTextureType::Normal,		MapBits::Normal);
			show_property("Height",               "Perceived depth for parallax mapping",                                              MaterialTextureType::Greyscale,	MapBits::Height);
			show_property("Occlusion",            "Amount of light loss, can be complementary to SSAO",                                MaterialTextureType::Greyscale,	MapBits::AmbientOcclusion);
			show_property("Emission",             "Light emission from the surface, works nice with bloom",                            MaterialTextureType::Color,		MapBits::Emission);
			show_property("Alpha mask",           "Discards pixels",                                                                   MaterialTextureType::Greyscale,	MapBits::AlphaMask);
			show_property("Clearcoat",            "Extra white specular layer on top of others",														  MaterialTextureType::MaxEnum, MapBits:: ,		MaterialProperty::Clearcoat);
			show_property("Clearcoat roughness",  "Roughness of clearcoat specular",																	  MaterialTextureType::MaxEnum, MapBits:: ,		MaterialProperty::Clearcoat_Roughness);
			show_property("Anisotropic",          "Amount of anisotropy for specular reflection",														  MaterialTextureType::MaxEnum, MapBits:: ,		MaterialProperty::Anisotropic);
			show_property("Anisotropic rotation", "Rotates the direction of anisotropy, with 1.0 going full circle",									  MaterialTextureType::MaxEnum, MapBits:: ,		MaterialProperty::AnisotropicRotation);
			show_property("Sheen",                "Amount of soft velvet like reflection near edges",													  MaterialTextureType::MaxEnum, MapBits:: ,		MaterialProperty::Sheen);
			show_property("Subsurface scattering","Amount of translucency",																				  MaterialTextureType::MaxEnum, MapBits:: ,		MaterialProperty::SubsurfaceScattering);
	
			layout::separator();
			layout::section_header("UV Mapping");
	
	
			// tiling
			{
				layout::begin_property("Tiling", "texture repeat");
	
				float w = (layout::value_width() - design::spacing_md - 24.0f) * 0.5f;
	
				ImGui::PushItemWidth(w);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.5f, 0.5f, 1.0f));
				ImGui::TextUnformatted("X");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::InputFloat("##tileX", &tiling.x, 0.0f, 0.0f, "%.2f");
				ImGui::PopItemWidth();
	
				ImGui::SameLine();
				ImGui::PushItemWidth(w);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.9f, 0.5f, 1.0f));
				ImGui::TextUnformatted("Y");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::InputFloat("##tileY", &tiling.y, 0.0f, 0.0f, "%.2f");
				ImGui::PopItemWidth();
			}
	
			// offset
			{
				layout::begin_property("Offset", "texture offset");
	
				float w = (layout::value_width() - design::spacing_md - 24.0f) * 0.5f;
	
				ImGui::PushItemWidth(w);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.5f, 0.5f, 1.0f));
				ImGui::TextUnformatted("X");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::InputFloat("##offsetX", &offset.x, 0.0f, 0.0f, "%.2f");
				ImGui::PopItemWidth();
	
				ImGui::SameLine();
				ImGui::PushItemWidth(w);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.9f, 0.5f, 1.0f));
				ImGui::TextUnformatted("Y");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::InputFloat("##offsetY", &offset.y, 0.0f, 0.0f, "%.2f");
				ImGui::PopItemWidth();
			}
	
			// inversion
			bool invert_x = material->GetProperty(MaterialProperty::TextureInvertX) > 0.5f;
			bool invert_y = material->GetProperty(MaterialProperty::TextureInvertY) > 0.5f;
			{
				layout::begin_property("Invert", "flip texture axes");
	
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.5f, 0.5f, 1.0f));
				ImGui::TextUnformatted("X");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				UI::CheckBox("##invertX", &invert_x);
	
				ImGui::SameLine(0, design::spacing_md);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.9f, 0.5f, 1.0f));
				ImGui::TextUnformatted("Y");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				UI::CheckBox("##invertY", &invert_y);
			}
	
			layout::separator();
			layout::section_header("Rendering Options");
	
			// cull mode
			static std::vector<std::string> cull_modes = { "Back", "Front", "None" };
			uint32_t cull_mode_index = static_cast<uint32_t>(material->GetProperty(MaterialProperty::CullMode));
			if (PropertyCombo("Culling", cull_modes, &cull_mode_index, "face culling mode"))
			{
				material->SetProperty(MaterialProperty::CullMode, static_cast<float>(cull_mode_index));
			}
	
			// feature toggles
			bool tessellation = material->GetProperty(MaterialProperty::Tessellation) != 0.0f;
			if (PropertyToggle("Tessellation", &tessellation, "hardware tessellation"))
			{
				material->SetProperty(MaterialProperty::Tessellation, tessellation ? 1.0f : 0.0f);
			}
	
			bool wind_animation = material->GetProperty(MaterialProperty::WindAnimation) != 0.0f;
			if (PropertyToggle("Wind Animation", &wind_animation, "vertex animation from wind"))
			{
				material->SetProperty(MaterialProperty::WindAnimation, wind_animation ? 1.0f : 0.0f);
			}
	
			bool emissive_from_albedo = material->GetProperty(MaterialProperty::EmissiveFromAlbedo) != 0.0f;
			if (PropertyToggle("Emissive from Albedo", &emissive_from_albedo, "use albedo as emission"))
			{
				material->SetProperty(MaterialProperty::EmissiveFromAlbedo, emissive_from_albedo ? 1.0f : 0.0f);
			}
	
			bool world_space_uv = material->GetProperty(MaterialProperty::WorldSpaceUv) != 0.0f;
			if (PropertyToggle("World Space UV", &world_space_uv, "world-space texture coordinates"))
			{
				material->SetProperty(MaterialProperty::WorldSpaceUv, world_space_uv ? 1.0f : 0.0f);
			}
	
			//= MAP ===============================================================================
			material->SetProperty(MaterialProperty::TextureTilingX, tiling.x);
			material->SetProperty(MaterialProperty::TextureTilingY, tiling.y);
			material->SetProperty(MaterialProperty::TextureOffsetX, offset.x);
			material->SetProperty(MaterialProperty::TextureOffsetY, offset.y);
			material->SetProperty(MaterialProperty::TextureInvertX, invert_x ? 1.0f : 0.0f);
			material->SetProperty(MaterialProperty::TextureInvertY, invert_y ? 1.0f : 0.0f);
			material->SetProperty(MaterialProperty::ColorR, m_material_color_picker->GetColor().r);
			material->SetProperty(MaterialProperty::ColorG, m_material_color_picker->GetColor().g);
			material->SetProperty(MaterialProperty::ColorB, m_material_color_picker->GetColor().b);
			material->SetProperty(MaterialProperty::ColorA, m_material_color_picker->GetColor().a);
			//=====================================================================================
		}
	
	
		component_end();
		*/
	}
	
	void Properties::ShowCamera(Camera* camera) const
	{
		if (!camera)
			return;
	
	   if (ComponentBegin("Camera", design::AccentCamera(), nullptr))
		{
			//= REFLECT ======================================================================
			static std::vector<std::string> projection_types = { "Perspective", "Orthographic" };
			float aperture                         = camera->GetAperture();
			float shutter_speed                    = camera->GetShutterSpeed();
			float iso                              = camera->GetIso();
			float fov                              = camera->GetFovHorizontalDeg();
			//================================================================================
	
			// background
			PropertyColor("Background", m_colorPicker_camera.get(), "clear color");
	
			// projection
			uint32_t proj_index = static_cast<uint32_t>(camera->GetProjectionType());
			if (PropertyCombo("Projection", projection_types, &proj_index, "camera projection type"))
			{
				camera->SetProjection(static_cast<CameraType>(proj_index));
			}
	
			PropertyFloat("Field of View", &fov, 0.5f, 1.0f, 179.0f, "horizontal field of view", "%.1f°");
	
			layout::separator();
			layout::section_header("Exposure");
	
			PropertyFloat("Aperture", &aperture, 0.1f, 0.01f, 150.0f, "f-stop (affects DoF and brightness)", "f/%.1f");
			PropertyFloat("Shutter Speed", &shutter_speed, 0.0001f, 0.0f, 1.0f, "exposure time in seconds (affects motion blur)", "%.4f s");
			PropertyFloat("ISO", &iso, 10.0f, 0.0f, 2000.0f, "sensor sensitivity (affects noise)", "%.0f");
	
			layout::separator();
			layout::section_header("Controls");
	
			//= MAP =======================================================================================================================================================
			if (aperture != camera->GetAperture())          camera->SetAperture(aperture);
			if (shutter_speed != camera->GetShutterSpeed()) camera->SetShutterSpeed(shutter_speed);
			if (iso != camera->GetIso())                    camera->SetIso(iso);
			if (fov != camera->GetFovHorizontalDeg())       camera->SetFovHorizontalDeg(fov);
			//=============================================================================================================================================================
		}
		component_end();
	}
	
	void Properties::ShowTerrain(Terrain* terrain) const
	{
		if (!terrain)
			return;
	
		if (ComponentBegin("Terrain", design::AccentTerrain(), nullptr))
		{
			PropertyText("Status", "Terrain inspector is temporarily disabled");
		}
		component_end();
	}
	
	void Properties::ShowSpline(Spline* spline) const
	{
		if (!spline)
			return;
	
		if (ComponentBegin("Spline", design::AccentSpline(), spline))
		{
			//= REFLECT ===============================================
			bool closed_loop            = spline->GetClosedLoop();
			uint32_t resolution         = spline->GetResolution();
			uint32_t point_count        = spline->GetControlPointCount();
			//uint32_t profile            = static_cast<uint32_t>(spline->GetProfile());
			float height                = spline->GetHeight();
			float thickness             = spline->GetThickness();
			uint32_t tube_sides         = spline->GetTubeSides();
			float uv_tiling_u           = spline->GetUvTilingU();
			float uv_tiling_v           = spline->GetUvTilingV();
			bool conform_to_terrain     = spline->GetConformToTerrain();
			float terrain_offset        = spline->GetTerrainOffset();
			bool mesh_enabled           = spline->GetMeshEnabled();
			float inst_spacing          = spline->GetInstanceSpacing();
			bool inst_align             = spline->GetAlignInstancesToSpline();
			float inst_random_offset    = spline->GetInstanceRandomOffset();
			float inst_random_scale_min = spline->GetInstanceRandomScaleMin();
			float inst_random_scale_max = spline->GetInstanceRandomScaleMax();
			float inst_random_yaw       = spline->GetInstanceRandomYaw();
			//=========================================================
	
			layout::section_header("Spline");
	
			if (PropertyToggle("Closed Loop", &closed_loop, "connect the last point back to the first"))
			{
				spline->SetClosedLoop(closed_loop);
			}
	
			float resolution_f = static_cast<float>(resolution);
			if (PropertyFloat("Resolution", &resolution_f, 1.0f, 2.0f, 100.0f, "line segments per span", "%.0f"))
			{
				spline->SetResolution(static_cast<uint32_t>(resolution_f));
			}
	
			layout::separator();
			layout::section_header("Control Points");
	
			char stat_buf[64];
			std::snprintf(stat_buf, sizeof(stat_buf), "%u", point_count);
			PropertyText("Count", stat_buf);
	
			if (point_count >= 2)
			{
				std::snprintf(stat_buf, sizeof(stat_buf), "%.2f m", spline->GetLength());
				PropertyText("Length", stat_buf);
			}
	
			layout::group_spacing();
	
			float button_width = 100.0f * Window::GetDpiScale();
			float total_width  = button_width * 2.0f + design::SPACING_MD;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - total_width) * 0.5f + ImGui::GetCursorPosX());
	
			if (UI::Button("+ Add Point", ImVec2(button_width, 0)))
			{
			   xMath::Vec3 position = xMath::Vec3(0.0f, 0.0f, 0.0f);
				if (point_count > 0)
				{
					Entity* parent = spline->GetEntity();
				   const auto& children = parent->Children();
					for (uint32_t i = static_cast<uint32_t>(children.size()); i > 0; i--)
					{
					 Entity child = Scene::TryGetEntityWithUUID(children[i - 1]);
						if (child)
						{
							if (child.GetObjectName().starts_with("spline_point_"))
							{
							   position = child.GetPositionLocal() + xMath::Vec3(5.0f, 0.0f, 0.0f);
								break;
							}
						}
					}
				}
				spline->AddControlPoint(position);
			}
	
			ImGui::SameLine(0, design::SPACING_MD);
	
			ImGui::BeginDisabled(point_count == 0);
			if (UI::Button("- Remove Last", ImVec2(button_width, 0)))
			{
				spline->RemoveLastControlPoint();
			}
			ImGui::EndDisabled();
	
			layout::separator();
			layout::section_header("Mesh Generation");
	
			if (PropertyToggle("Enabled", &mesh_enabled, "automatically generate a mesh along the spline"))
			{
				spline->SetMeshEnabled(mesh_enabled);
				if (!mesh_enabled)
				{
					spline->ClearRoadMesh();
				}
			}
	
			ImGui::BeginDisabled(!mesh_enabled);
	
			/*
			// profile type
			static std::vector<std::string> profile_names = { "Road", "Wall", "Tube", "Fence", "Channel" };
			if (PropertyCombo("Profile", profile_names, &profile, "cross-section shape extruded along the spline"))
			{
				spline->SetProfile(static_cast<SplineType>(profile));
			}
			*/
	
			/*
			// profile-specific properties
			SplineType current_profile = static_cast<SplineType>(profile);
			bool needs_height = current_profile == SplineType::Wall ||
								current_profile == SplineType::Fence ||
								current_profile == SplineType::Channel;
			if (needs_height)
			{
				if (PropertyFloat("Height", &height, 0.1f, 0.1f, 100.0f, "height in meters", "%.1f m"))
				{
					spline->SetHeight(height);
				}
			}
	
			bool needs_thickness = current_profile == SplineType::Wall ||
								   current_profile == SplineType::Fence;
			if (needs_thickness)
			{
				if (PropertyFloat("Thickness", &thickness, 0.01f, 0.01f, 10.0f, "thickness in meters", "%.2f m"))
				{
					spline->SetThickness(thickness);
				}
			}
			*/
	
			/*
			if (current_profile == SplineType::Tube)
			{
				float tube_sides_f = static_cast<float>(tube_sides);
				if (PropertyFloat("Sides", &tube_sides_f, 1.0f, 3.0f, 64.0f, "tube cross-section subdivisions", "%.0f"))
				{
					spline->SetTubeSides(static_cast<uint32_t>(tube_sides_f));
				}
			}
			*/
	
			// uv tiling
			if (PropertyFloat("UV Tiling U", &uv_tiling_u, 0.01f, 0.01f, 100.0f, "texture tiling across the profile", "%.2f"))
			{
				spline->SetUvTilingU(uv_tiling_u);
			}
			if (PropertyFloat("UV Tiling V", &uv_tiling_v, 0.01f, 0.01f, 100.0f, "texture tiling along the spline", "%.2f"))
			{
				spline->SetUvTilingV(uv_tiling_v);
			}
	
			// terrain conforming
			if (PropertyToggle("Conform to Terrain", &conform_to_terrain, "snap the mesh to the terrain surface"))
			{
				spline->SetConformToTerrain(conform_to_terrain);
			}
			if (conform_to_terrain)
			{
				if (PropertyFloat("Terrain Offset", &terrain_offset, 0.001f, 0.0f, 10.0f, "vertical offset above the terrain", "%.3f m"))
				{
					spline->SetTerrainOffset(terrain_offset);
				}
			}
	
			ImGui::EndDisabled();
	
			layout::separator();
			layout::section_header("Instancing");
	
			if (PropertyFloat("Spacing", &inst_spacing, 0.1f, 0.5f, 100.0f, "distance between instances in meters", "%.1f m"))
			{
				spline->SetInstanceSpacing(inst_spacing);
			}
	
			if (PropertyToggle("Align to Spline", &inst_align, "rotate instances to follow the spline direction"))
			{
				spline->SetAlignInstancesToSpline(inst_align);
			}
	
			// procedural placement randomization
			if (PropertyFloat("Random Offset", &inst_random_offset, 0.1f, 0.0f, 50.0f, "random lateral offset from the spline", "%.1f m"))
			{
				spline->SetInstanceRandomOffset(inst_random_offset);
			}
			if (PropertyFloat("Random Scale Min", &inst_random_scale_min, 0.01f, 0.01f, 10.0f, "minimum random scale", "%.2f"))
			{
				spline->SetInstanceRandomScaleMin(inst_random_scale_min);
			}
			if (PropertyFloat("Random Scale Max", &inst_random_scale_max, 0.01f, 0.01f, 10.0f, "maximum random scale", "%.2f"))
			{
				spline->SetInstanceRandomScaleMax(inst_random_scale_max);
			}
			if (PropertyFloat("Random Yaw", &inst_random_yaw, 1.0f, 0.0f, 360.0f, "random rotation around the up axis in degrees", "%.0f\xc2\xb0"))
			{
				spline->SetInstanceRandomYaw(inst_random_yaw);
			}
	
			layout::group_spacing();
	
			// spawn / clear instance buttons
			float inst_button_width = 120.0f * Window::GetDpiScale();
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - inst_button_width) * 0.5f + ImGui::GetCursorPosX());
	
			ImGui::BeginDisabled(point_count < 2);
			if (UI::Button("Spawn", ImVec2(inst_button_width, 0)))
			{
				spline->SpawnInstances();
			}
			ImGui::EndDisabled();
	
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - inst_button_width) * 0.5f + ImGui::GetCursorPosX());
			if (UI::Button("Clear Instances", ImVec2(inst_button_width, 0)))
			{
				spline->ClearInstances();
			}
		}
		component_end();
	}
	
	/*
	void Properties::ShowSplineFollower(SplineFollower* follower) const
	{
		if (!follower)
			return;
	
		if (component_begin("Spline Follower", design::accent_spline_follower(), follower))
		{
			//= REFLECT ========================================
			float speed         = follower->GetSpeed();
			uint32_t mode       = static_cast<uint32_t>(follower->GetFollowMode());
			bool align          = follower->GetAlignToSpline();
			float progress      = follower->GetProgress();
			uint64_t spline_id  = follower->GetSplineEntityId();
			Entity* spline_ent  = follower->GetSplineEntity();
			//==================================================
	
			layout::section_header("Spline Reference");
	
			// build a list of entities that have a spline component
			const vector<Entity*>& all_entities = World::GetEntities();
			vector<std::string> spline_names;
			vector<uint64_t> spline_ids;
			uint32_t selected_index = 0;
	
			// first entry is "none"
			spline_names.push_back("(none)");
			spline_ids.push_back(0);
	
			for (Entity* entity : all_entities)
			{
				if (entity && entity->GetComponent<Spline>())
				{
					spline_ids.push_back(entity->GetObjectId());
					spline_names.push_back(entity->GetObjectName());
	
					if (entity->GetObjectId() == spline_id)
					{
						selected_index = static_cast<uint32_t>(spline_names.size() - 1);
					}
				}
			}
	
			if (property_combo("Spline", spline_names, &selected_index, "the spline entity to follow"))
			{
				follower->SetSplineEntityId(spline_ids[selected_index]);
			}
	
			layout::separator();
			layout::section_header("Movement");
	
			// speed
			if (property_float("Speed", &speed, 0.1f, 0.0f, 1000.0f, "movement speed in world units per second", "%.1f"))
			{
				follower->SetSpeed(speed);
			}
	
			// follow mode
			static vector<std::string> mode_names = { "Clamp", "Loop", "Ping Pong" };
			if (property_combo("Mode", mode_names, &mode, "behavior when reaching the end of the spline"))
			{
				follower->SetFollowMode(static_cast<spartan::SplineFollowMode>(mode));
			}
	
			// align to spline
			if (property_toggle("Align To Spline", &align, "orient the entity along the spline tangent"))
			{
				follower->SetAlignToSpline(align);
			}
	
			// progress (read-only)
			char progress_buf[32];
			snprintf(progress_buf, sizeof(progress_buf), "%.1f%%", progress * 100.0f);
			property_text("Progress", progress_buf, "current position along the spline");
		}
		component_end();
	}
	*/
	
	/*
	void Properties::ShowAudioSource(spartan::AudioSource* audio_source) const
	{
		if (!audio_source)
			return;
	
		if (component_begin("Audio Source", design::accent_audio(), audio_source))
		{
			//= REFLECT ==============================================
			std::string audio_clip_name  = audio_source->GetAudioClipName();
			bool mute               = audio_source->GetMute();
			bool play_on_start      = audio_source->GetPlayOnStart();
			bool loop               = audio_source->GetLoop();
			bool is_3d              = audio_source->GetIs3d();
			float volume            = audio_source->GetVolume();
			float pitch             = audio_source->GetPitch();
			bool reverb_enabled     = audio_source->GetReverbEnabled();
			float reverb_room_size  = audio_source->GetReverbRoomSize();
			float reverb_decay      = audio_source->GetReverbDecay();
			float reverb_wet        = audio_source->GetReverbWet();
			//========================================================
	
			// audio clip resource
			property_resource("Audio Clip", &audio_clip_name, "audio file", [audio_source](const std::string& path) {
				if (FileSystem::IsSupportedAudioFile(path))
				{
					audio_source->SetAudioClip(path);
				}
			});
	
			if (auto payload = UI::receive_drag_drop_payload(UI::DragPayloadType::Audio))
			{
				audio_source->SetAudioClip(std::get<const char*>(payload->data));
			}
	
			layout::separator();
			layout::section_header("Playback");
	
			property_toggle("Play on Start", &play_on_start, "auto-play when scene starts");
			property_toggle("Loop", &loop, "repeat playback");
			property_toggle("Mute", &mute, "silence output");
	
			layout::group_spacing();
	
			// volume slider
			{
				layout::begin_property("Volume", "output volume");
				ImGui::SliderFloat("##volume", &volume, 0.0f, 1.0f, "%.0f%%");
			}
	
			// pitch slider
			{
				layout::begin_property("Pitch", "playback speed");
				ImGui::SliderFloat("##pitch", &pitch, 0.01f, 5.0f, "%.2fx");
			}
	
			layout::separator();
			layout::section_header("Spatialization");
	
			property_toggle("3D Sound", &is_3d, "position-based audio");
	
			layout::separator();
			layout::section_header("Progress");
	
			// progress bar
			{
				layout::begin_property("", nullptr);
				float progress = audio_source->GetProgress();
				ImGui::ProgressBar(progress, ImVec2(-1, 0), "");
			}
	
			layout::separator();
			layout::section_header("Reverb");
	
			property_toggle("Enabled", &reverb_enabled, "apply reverb effect");
	
			ImGui::BeginDisabled(!reverb_enabled);
			{
				layout::begin_property("Room Size", "reverb room size");
				ImGui::SliderFloat("##room_size", &reverb_room_size, 0.0f, 1.0f);
	
				layout::begin_property("Decay", "reverb decay time");
				ImGui::SliderFloat("##decay", &reverb_decay, 0.0f, 0.99f);
	
				layout::begin_property("Wet Mix", "reverb blend amount");
				ImGui::SliderFloat("##wet", &reverb_wet, 0.0f, 1.0f);
			}
			ImGui::EndDisabled();
	
			//= MAP =========================================================================================
			if (mute != audio_source->GetMute())                       audio_source->SetMute(mute);
			if (play_on_start != audio_source->GetPlayOnStart())       audio_source->SetPlayOnStart(play_on_start);
			if (loop != audio_source->GetLoop())                       audio_source->SetLoop(loop);
			if (is_3d != audio_source->GetIs3d())                      audio_source->SetIs3d(is_3d);
			if (volume != audio_source->GetVolume())                   audio_source->SetVolume(volume);
			if (pitch != audio_source->GetPitch())                     audio_source->SetPitch(pitch);
			if (reverb_enabled != audio_source->GetReverbEnabled())    audio_source->SetReverbEnabled(reverb_enabled);
			if (reverb_room_size != audio_source->GetReverbRoomSize()) audio_source->SetReverbRoomSize(reverb_room_size);
			if (reverb_decay != audio_source->GetReverbDecay())        audio_source->SetReverbDecay(reverb_decay);
			if (reverb_wet != audio_source->GetReverbWet())            audio_source->SetReverbWet(reverb_wet);
			//===============================================================================================
		}
		component_end();
	}
	*/
	
	void Properties::ShowVolume(Volume* volume) const
	{
		if (!volume)
			return;
	
	   if (ComponentBegin("Volume", design::AccentVolume(), nullptr))
		{
			PropertyText("Status", "Volume inspector is temporarily disabled");
		}
		component_end();
	}
	
	void Properties::ShowParticleSystem(ParticleSystem* particle_system) const
	{
		if (!particle_system)
			return;
	
		if (ComponentBegin("Particle System", design::AccentParticles(), nullptr))
		{
			PropertyText("Status", "Particle system inspector is temporarily disabled");
		}
		component_end();
	}
	
	void Properties::ShowAddComponentButton()
	{
		ImGui::Dummy(ImVec2(0, design::SPACING_LG));
	
		// centered add button
		float button_width = 140.0f * Window::GetDpiScale();
		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - button_width) * 0.5f + ImGui::GetCursorPosX());
	
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(design::SPACING_LG, design::SPACING_MD));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.4f, 0.55f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.65f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.35f, 0.5f, 1.0f));
	
		if (UI::Button("+ Add Component", ImVec2(button_width, 0)))
		{
			ImGui::OpenPopup("##ComponentContextMenu_Add");
		}
	
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
	
		ComponentContextMenu_Add();
	
		// save as prefab button (only for non-prefab entities, or for file prefabs that want to save-as)
		if (Entity* entity = get_selected_entity())
		{
			ImGui::Dummy(ImVec2(0, design::SPACING_SM));
	
			float save_button_width = 160.0f * Window::GetDpiScale();
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - save_button_width) * 0.5f + ImGui::GetCursorPosX());
	
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(design::SPACING_LG, design::SPACING_MD));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.50f, 0.35f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.60f, 0.40f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.45f, 0.30f, 1.0f));
	
			if (UI::Button("Save as Prefab...", ImVec2(save_button_width, 0)))
			{
				ImGui::OpenPopup("##SaveAsPrefab");
			}
	
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);
	
			// save-as-prefab popup
			ShowSaveAsPrefabPopup(entity);
		}
	}
	
	void Properties::ShowSaveAsPrefabPopup(Entity* entity)
	{
		static char prefab_name[256]  = "";
		static bool needs_init        = true;
	
		// detect when the popup is about to open (was closed, now opening)
		bool is_open = ImGui::IsPopupOpen("##SaveAsPrefab");
		if (is_open && needs_init)
		{
			strncpy_s(prefab_name, sizeof(prefab_name), entity->GetObjectName().c_str(), _TRUNCATE);
			needs_init = false;
		}
		else if (!is_open)
		{
			needs_init = true;
		}
	
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(design::SPACING_XL, design::SPACING_LG));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(design::SPACING_MD, design::SPACING_MD));
	
		if (ImGui::BeginPopup("##SaveAsPrefab"))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
			ImGui::PushFont(EditorLayer::fontBold);
			ImGui::TextUnformatted("Save as Prefab");
			ImGui::PopFont();
			ImGui::PopStyleColor();
	
			ImGui::Dummy(ImVec2(0, design::SPACING_SM));
	
			ImGui::TextUnformatted("Name:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("##prefab_name_input", prefab_name, sizeof(prefab_name));
	
			// show the path that will be used
			std::string preview_path = std::string("prefabs/") + prefab_name + ".prefab";
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::Text("file: %s", preview_path.c_str());
			ImGui::PopStyleColor();
	
			ImGui::Dummy(ImVec2(0, design::SPACING_SM));
	
			// save button
			bool name_valid = strlen(prefab_name) > 0;
			ImGui::BeginDisabled(!name_valid);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.50f, 0.35f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.60f, 0.40f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.45f, 0.30f, 1.0f));
	
			if (UI::Button("Save", ImVec2(80.0f, 0)))
			{
				std::string file_path = std::string(ResourceCache::GetProjectDirectory()) + "/prefabs/" + prefab_name + ".prefab";
				EDITOR_WARN_TAG("Properties", "Prefab saving is currently disabled");
				/*if (Prefab::SaveToFile(entity, file_path))
				{
					// tag the entity as a file prefab so future world saves reference the file
					entity->SetPrefabFilePath(file_path);
				}*/
				ImGui::CloseCurrentPopup();
			}
	
			ImGui::PopStyleColor(3);
			ImGui::EndDisabled();
	
			ImGui::SameLine();
	
			if (UI::Button("Cancel", ImVec2(80.0f, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
	
			ImGui::EndPopup();
		}
	
		ImGui::PopStyleVar(2);
	}
	
	void Properties::ComponentContextMenu_Add()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(design::SPACING_MD, design::SPACING_MD));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(design::SPACING_MD, design::SPACING_SM));
	
		if (ImGui::BeginPopup("##ComponentContextMenu_Add"))
		{
			if (Entity* entity = get_selected_entity())
			{
				// scripting (Lua support)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
				ImGui::TextUnformatted("SCRIPTING");
				ImGui::PopStyleColor();
				ImGui::Separator();
	
				if (ImGui::MenuItem("Script"))
				{
					entity->AddComponent<Script>();
				}
	
				ImGui::Dummy(ImVec2(0, design::SPACING_SM));
	
				// rendering
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
				ImGui::TextUnformatted("RENDERING");
				ImGui::PopStyleColor();
				ImGui::Separator();
	
				if (ImGui::MenuItem("Camera"))
				{
					entity->AddComponent<Camera>();
				}
	
				if (ImGui::MenuItem("Render"))
				{
					entity->AddComponent<Renderable>();
				}
	
				if (ImGui::MenuItem("Terrain"))
				{
					entity->AddComponent<Terrain>();
				}
	
				if (ImGui::MenuItem("Spline"))
				{
					entity->AddComponent<Spline>();
				}
	
				ImGui::Dummy(ImVec2(0, design::SPACING_SM));
	
				// lighting
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
				ImGui::TextUnformatted("LIGHTING");
				ImGui::PopStyleColor();
				ImGui::Separator();
	
				if (ImGui::BeginMenu("Light"))
				{
					if (ImGui::MenuItem("Directional"))
					{
						entity->AddComponent<Light>()->SetLightType(LightType::Directional);
					}
					if (ImGui::MenuItem("Point"))
					{
						entity->AddComponent<Light>()->SetLightType(LightType::Point);
					}
					if (ImGui::MenuItem("Spot"))
					{
						entity->AddComponent<Light>()->SetLightType(LightType::Spot);
					}
					/*if (ImGui::MenuItem("Area"))
					{
						entity->AddComponent<Light>()->SetLightType(LightType::Area);
					}*/
					ImGui::EndMenu();
				}
	
				if (ImGui::MenuItem("Volume"))
				{
					entity->AddComponent<Volume>();
				}
	
				ImGui::Dummy(ImVec2(0, design::SPACING_SM));
	
				// effects
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
				ImGui::TextUnformatted("EFFECTS");
				ImGui::PopStyleColor();
				ImGui::Separator();
	
				if (ImGui::MenuItem("Particle System"))
				{
					entity->AddComponent<ParticleSystem>();
				}
	
				ImGui::Dummy(ImVec2(0, design::SPACING_SM));
	
				// physics & audio
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
				ImGui::TextUnformatted("PHYSICS & AUDIO");
				ImGui::PopStyleColor();
				ImGui::Separator();
	
				/*
				if (ImGui::MenuItem("Physics"))
				{
					entity->AddComponent<Physics>();
				}
				*/
	
				/*
				if (ImGui::BeginMenu("Audio"))
				{
					if (ImGui::MenuItem("Audio Source"))
					{
						entity->AddComponent<AudioSource>();
					}
					ImGui::EndMenu();
				}
				*/
	
			}
	
			ImGui::EndPopup();
		}
	
		ImGui::PopStyleVar(2);
	}
	
}

// -------------------------------------------------------
