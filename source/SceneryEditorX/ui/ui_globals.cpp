/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_globals.cpp
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/
#include "ui_manager.h"

/// -------------------------------------------------------
 
namespace SceneryEditorX::UI
{
	bool showViewport = true;
	bool showCreateProjectModal = false;
	bool showExitModal = false;
	bool showAboutModal = false;
	bool showSettingsPanel = false;

    /// -------------------------------------------------------

    /*
    bool PropertyScriptReference(const char* label, UUID& outScriptID, const PropertyAssetReferenceSettings& settings, bool doPushUndo)
	{
		bool modified = false;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		auto hold = outScriptID;

		//const auto& scriptEngine = ScriptEngine::GetInstance();

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		{
			ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
			float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
			float itemHeight = 28.0f;

			std::string buttonText = "None";
			bool valid = true;
			/*
			if (scriptEngine.IsValidScript(outScriptID))
			{
				const auto& metadata = scriptEngine.GetScriptMetadata(outScriptID);
				buttonText = metadata.FullName;
			}
			#1#

			if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
				buttonText = "---";

			/**
			 * PropertyAssetReference could be called multiple times in same "context"
			 * and so we need a unique id for the asset search popup each time.
			 * notes
			 * - don't use GenerateID(), that's inviting id clashes, which would be super confusing.
			 * - don't store return from GenerateLabelId in a const char* here. Because it's pointing to an internal
			 *   buffer which may get overwritten by the time you want to use it later on.
             #1#
			std::string assetSearchPopupID = GenerateLabelID("ARSP");
			{
				ScopedColour buttonLabelColor(ImGuiCol_Text, valid ? settings.ButtonLabelColor : settings.ButtonLabelColorError);
				ImGui::Button(GenerateLabelID(buttonText), { width, itemHeight });

                if (const bool isHovered = ImGui::IsItemHovered())
				{
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						/// TODO: Open script in default editor
					}
					else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						ImGui::OpenPopup(assetSearchPopupID.c_str());
					}
				}
			}

			ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

			bool clear = false;
			if (Widgets::ScriptSearchPopup(assetSearchPopupID.c_str(), scriptEngine, outScriptID, &clear))
			{
				if (clear)
					outScriptID = 0;
				modified = true;
				//s_PropertyAssetReferenceAssetHandle = outScriptID;
			}
		}

		/*if (!IsItemDisabled())
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("asset_payload");

				if (data)
				{
					AssetHandle assetHandle = *(AssetHandle*)data->Data;
					s_PropertyAssetReferenceAssetHandle = assetHandle;
					Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
					if (asset && asset->GetAssetType() == T::GetStaticType())
					{
						outHandle = assetHandle;
						modified = true;
					}
				}

				ImGui::EndDragDropTarget();
			}
		}#1#

		ImGui::PopItemWidth();
		if (settings.AdvanceToNextColumn)
		{
			ImGui::NextColumn();
			Draw::Underline();
		}

		if (modified && doPushUndo)
			EditorStack::Get().PushCopy<decltype(hold)>(&outScriptID, hold);

		return modified;
	}
	*/

	/*
	bool DrawFieldValue(Ref<Scene> sceneContext, std::string_view fieldName, FieldStorage& storage)
	{
		ImGui::PushID(fieldName.data());

		bool result = false;

		auto hold = storage.Copy();
		switch (storage.GetType())
		{
			case DataType::Bool:
			{
                if (bool value = storage.GetValue<bool>(); Property(fieldName.data(), value, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::SByte:
			{
                if (int8_t value = storage.GetValue<int8_t>(); Property(fieldName.data(), value, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Short:
			{
                if (int16_t value = storage.GetValue<int16_t>(); Property(fieldName.data(), value, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Int:
			{
                if (int32_t value = storage.GetValue<int32_t>(); Property(fieldName.data(), value, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Long:
			{
                if (int64_t value = storage.GetValue<int64_t>(); Property(fieldName.data(), value, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Byte:
			{
                if (uint8_t value = storage.GetValue<uint8_t>(); Property(fieldName.data(), value, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::UShort:
			{
                if (uint16_t value = storage.GetValue<uint16_t>(); Property(fieldName.data(), value, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::UInt:
			{
                if (uint32_t value = storage.GetValue<uint32_t>(); Property(fieldName.data(), value, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::ULong:
			{
                if (uint64_t value = storage.GetValue<uint64_t>(); Property(fieldName.data(), value, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Float:
			{
                if (float value = storage.GetValue<float>(); Property(fieldName.data(), value, 0, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Double:
			{
                if (double value = storage.GetValue<double>(); Property(fieldName.data(), value, 0, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::String:
			{
                // TODO: `Property` looks like it expects a C string, but we're passing string_view data which may
				//				Not be null-terminated or may not be intended to read to terminator.
				if (std::string value = storage.GetValue<std::string>(); Property(fieldName.data(), value, "", false))
				{
					storage.SetValue<std::string>(value);
					result = true;
				}
				break;
			}
			case DataType::Vector2:
			{
                if (Vec2 value = storage.GetValue<Vec2>(); Property(fieldName.data(), value, 0, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Vector3:
			{
                if (Vec3 value = storage.GetValue<Vec3>(); Property(fieldName.data(), value, 0, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Vector4:
			{
                if (Vec4 value = storage.GetValue<Vec4>(); Property(fieldName.data(), value, 0, 0, 0, "", false))
				{
					storage.SetValue(value);
					result = true;
				}
				break;
			}
			case DataType::Prefab:
			{
				/*if (sceneContext->IsPlaying())
				{
					Coral::ManagedObject prefabObj = storage.GetValue<Coral::ManagedObject>();
					AssetHandle handle = prefabObj.GetFieldValue<uint64_t>("Handle");
					if (PropertyAssetReference<Prefab>(fieldName.data(), handle))
					{
						prefabObj.SetFieldValue("Handle", (uint64_t)handle);
						result = true;
					}
				}
				else#1#
				if (!sceneContext->IsPlaying())
				{
                    if (AssetHandle handle = storage.GetValue<uint64_t>(); PropertyAssetReference<Prefab>(fieldName.data(), handle, "", 0, {}, false))
					{
						storage.SetValue(handle);
						result = true;
					}
				}
				
				break;
			}
			case DataType::Entity:
			{
				/*if (sceneContext->IsPlaying())
				{
					Coral::ManagedObject entityObj = storage.GetValue<Coral::ManagedObject>();
					UUID uuid = entityObj.GetFieldValue<uint64_t>("ID");
					if (PropertyEntityReference(fieldName.data(), uuid, sceneContext))
					{
						entityObj.SetFieldValue("ID", (uint64_t)uuid);
						result = true;
					}
				}
				else#1#
				if (!sceneContext->IsPlaying())
				{
                    if (UUID uuid = storage.GetValue<UUID>(); PropertyEntityReference(fieldName.data(), uuid, sceneContext, "", false))
					{
						storage.SetValue(uuid);
						result = true;
					}
				}

				break;
			}
			case DataType::Mesh:
			{
				if (!sceneContext->IsPlaying())
				{
                    if (AssetHandle handle = storage.GetValue<AssetHandle>(); PropertyAssetReference<Mesh>(fieldName.data(), handle, "", 0, {}, false))
					{
						storage.SetValue(handle);
						result = true;
					}
				}
				break;
			}
			case DataType::StaticMesh:
			{
				if (!sceneContext->IsPlaying())
				{
                    if (AssetHandle handle = storage.GetValue<AssetHandle>(); PropertyAssetReference<StaticMesh>(fieldName.data(), handle, "", 0, {}, false))
					{
						storage.SetValue(handle);
						result = true;
					}
				}
				break;
			}
			case DataType::Material:
			{
				if (!sceneContext->IsPlaying())
				{
                    if (AssetHandle handle = storage.GetValue<AssetHandle>(); PropertyAssetReference<MaterialAsset>(fieldName.data(), handle, "", 0, {}, false))
					{
						storage.SetValue(handle);
						result = true;
					}
				}
				break;
			}
			case DataType::Texture2D:
			{
                if (AssetHandle handle = storage.GetValue<AssetHandle>(); PropertyAssetReference<Texture2D>(fieldName.data(), handle, "", 0, {}, false))
					storage.SetValue(handle);
				break;
			}
			case DataType::Scene:
			{
				if (!sceneContext->IsPlaying())
				{
                    if (AssetHandle handle = storage.GetValue<AssetHandle>(); PropertyAssetReference<Scene>(fieldName.data(), handle, "", 0, {}, false))
						storage.SetValue(handle);
				}
				break;
			}
		}

		ImGui::PopID();

		if (result)
			EditorStack::Get().PushCopy<FieldStorage>(&storage, hold);

		return result;
	}
	*/

	/*
	/**
	 * TODO: This needs a more complex undo.
	 #1#
	template<typename T>
	static bool PropertyAssetReferenceArray(const char* label, AssetHandle& outHandle, FieldStorage& arrayStorage, uint32_t index, intptr_t& elementToRemove, const char* helpText = "", const PropertyAssetReferenceSettings& settings = {})
	{
		bool modified = false;

		ImGuiStyle& style = ImGui::GetStyle();

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		const float buttonSize = ImGui::GetFrameHeight();
		float itemWidth = ImMax(1.0f, ImGui::CalcItemWidth() - (buttonSize + style.ItemInnerSpacing.x));
		ImGui::SetNextItemWidth(itemWidth);

        {
            ImVec2 originalButtonTextAlign = style.ButtonTextAlign;
            ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };

            std::string buttonText = "Null";
			bool valid = AssetManager::IsAssetValid(outHandle) && !AssetManager::IsAssetMissing(outHandle);

			if (valid)
                buttonText = Project::GetEditorAssetManager()->GetMetadata(outHandle).FilePath.stem().string();

            /**
             * PropertyAssetReferenceTarget could be called multiple times in same "context"
			 * and so we need a unique id for the asset search popup each time.
			 * notes
			 * - don't use GenerateID(), that's inviting id clashes, which would be super confusing.
			 * - don't store return from GenerateLabelId in a const char* here. Because its pointing to an internal
			 *   buffer which may get overwritten by the time you want to use it later on.
             #1#
			std::string assetSearchPopupID = GenerateLabelID("ARTSP");
			{
                float itemHeight = 28.0f;
                UI::ScopedColour buttonLabelColor(ImGuiCol_Text, valid ? settings.ButtonLabelColor : settings.ButtonLabelColorError);
				ImGui::Button(GenerateLabelID(buttonText), { itemWidth, itemHeight });

                if (const bool isHovered = ImGui::IsItemHovered())
				{
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						// NOTE(Peter): Ugly workaround since AssetEditorPanel includes ImGui.h (meaning ImGui.h can't include AssetEditorPanel).
						//				Will rework those include at a later date...
						AssetEditorPanelInterface::OpenEditor(AssetManager::GetAsset<Asset>(outHandle));
					}
					else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						ImGui::OpenPopup(assetSearchPopupID.c_str());
					}
				}
			}

			ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

			bool clear = false;
			if (Widgets::AssetSearchPopup(assetSearchPopupID.c_str(), T::GetStaticType(), outHandle, &clear))
			{
				if (clear)
					outHandle = 0;

				modified = true;
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
            if (auto data = ImGui::AcceptDragDropPayload("asset_payload"))
			{
				AssetHandle assetHandle = *(AssetHandle*)data->Data;
				s_PropertyAssetReferenceAssetHandle = assetHandle;

                if (const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(assetHandle); metadata.Type == T::GetStaticType())
				{
					outHandle = assetHandle;
					modified = true;
				}
			}

			ImGui::EndDragDropTarget();
		}

		if (modified)
			arrayStorage.SetValue<uint64_t>(outHandle, static_cast<uint64_t>(index));

		const ImVec2 backupFramePadding = style.FramePadding;
		style.FramePadding.x = style.FramePadding.y;
		ImGui::SameLine(0, style.ItemInnerSpacing.x);

		if (ImGui::Button(GenerateLabelID("x"), ImVec2(buttonSize, buttonSize)))
			elementToRemove = static_cast<intptr_t>(index);

		style.FramePadding = backupFramePadding;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}
	*/

	/*
	static bool PropertyEntityReferenceArray(const char* label, UUID& entityID, Ref<Scene> context, FieldStorage& arrayStorage, uintptr_t index, intptr_t& elementToRemove)
	{
		bool receivedValidEntity = false;

		ImGuiStyle& style = ImGui::GetStyle();

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);
		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		const float buttonSize = ImGui::GetFrameHeight();
		float itemWidth = ImMax(1.0f, ImGui::CalcItemWidth() - (buttonSize + style.ItemInnerSpacing.x));
		ImGui::SetNextItemWidth(itemWidth);

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		{
			ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
			float width = ImGui::GetContentRegionAvail().x;
			float itemHeight = 28.0f;

			std::string buttonText = "Null";

            if (Entity entity = context->TryGetEntityWithUUID(entityID))
				buttonText = entity.GetComponent<TagComponent>().Tag;

			ImGui::Button(GenerateLabelID(buttonText), { width - buttonSize, itemHeight });
		}
		ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

		if (ImGui::BeginDragDropTarget())
		{
            if (auto data = ImGui::AcceptDragDropPayload("scene_entity_hierarchy"))
			{
				entityID = *(UUID*)data->Data;
				receivedValidEntity = true;
			}

			ImGui::EndDragDropTarget();
		}

		if (receivedValidEntity)
			arrayStorage.SetValue<uint64_t>(entityID, (uint64_t)index);

		const ImVec2 backupFramePadding = style.FramePadding;
		style.FramePadding.x = style.FramePadding.y;
		ImGui::SameLine(0, style.ItemInnerSpacing.x);

		if (ImGui::Button(GenerateLabelID("x"), ImVec2(buttonSize, buttonSize)))
			elementToRemove = index;

		style.FramePadding = backupFramePadding;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return receivedValidEntity;
	}
	*/

	/*
	bool DrawFieldArray(Ref<Scene> sceneContext, std::string_view fieldName, FieldStorage& storage)
	{
		bool modified = false;
		intptr_t elementToRemove = -1;

		std::string arrayID = std::format("{0}FieldArray", fieldName);
		ImGui::PushID(arrayID.c_str());

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(fieldName.data());
		ImGui::NextColumn();
		ImGui::NextColumn();

		int32_t length = storage.GetLength();

		if (UI::PropertyInput("Length", length, 1, 1, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			storage.Resize(length);
			modified = true;

			length = storage.GetLength();
		}

		ShiftCursorY(5.0f);

		auto drawScalarElement = [&storage, &elementToRemove, &arrayID]<typename T0>(std::string_view indexString, uintptr_t index, ImGuiDataType dataType, T0& data, int32_t components, const char* format)
		{
			ImGuiStyle& style = ImGui::GetStyle();

			ShiftCursor(10.0f, 9.0f);
			ImGui::Text(indexString.data());
			ImGui::NextColumn();
			ShiftCursorY(4.0f);
			ImGui::PushItemWidth(-1);

			const float buttonSize = ImGui::GetFrameHeight();
			ImGui::SetNextItemWidth(ImMax(1.0f, ImGui::CalcItemWidth() - (buttonSize + style.ItemInnerSpacing.x)));

			size_t dataSize = ImGui::DataTypeGetInfo(dataType)->Size;
			if (components > 1)
			{
				if (UI::DragScalarN(GenerateID(), dataType, &data, components, 1.0f, (const void*)0, (const void*)0, format, 0))
					storage.SetValue<std::remove_reference_t<T0>>(data, index);
			}
			else
			{
				if (UI::DragScalar(GenerateID(), dataType, &data, 1.0f, (const void*)0, (const void*)0, format, (ImGuiSliderFlags)0))
					storage.SetValue<std::remove_reference_t<T0>>(data, index);
			}

			const ImVec2 backupFramePadding = style.FramePadding;
			style.FramePadding.x = style.FramePadding.y;
			ImGui::SameLine(0, style.ItemInnerSpacing.x);

			if (ImGui::Button(GenerateLabelID("x"), ImVec2(buttonSize, buttonSize)))
				elementToRemove = index;

			style.FramePadding = backupFramePadding;

			ImGui::PopItemWidth();
			ImGui::NextColumn();
			Draw::Underline();
		};

		auto drawStringElement = [&storage, &elementToRemove](std::string_view indexString, uintptr_t index, std::string& data)
		{
			ImGuiStyle& style = ImGui::GetStyle();

			ShiftCursor(10.0f, 9.0f);
			ImGui::Text(indexString.data());
			ImGui::NextColumn();
			ShiftCursorY(4.0f);
			ImGui::PushItemWidth(-1);

			const float buttonSize = ImGui::GetFrameHeight();
			ImGui::SetNextItemWidth(ImMax(1.0f, ImGui::CalcItemWidth() - (buttonSize + style.ItemInnerSpacing.x)));

			if (UI::InputText(GenerateID(), &data))
				storage.SetValue<std::string>(data, index);

			const ImVec2 backupFramePadding = style.FramePadding;
			style.FramePadding.x = style.FramePadding.y;
			ImGui::SameLine(0, style.ItemInnerSpacing.x);

			if (ImGui::Button(GenerateLabelID("x"), ImVec2(buttonSize, buttonSize)))
				elementToRemove = index;

			style.FramePadding = backupFramePadding;

			ImGui::PopItemWidth();
			ImGui::NextColumn();
			Draw::Underline();
		};

		for (int32_t i = 0; i < length; i++)
		{
			std::string idString = std::format("[{0}]{1}-{0}", i, arrayID);
			std::string indexString = std::format("[{0}]", i);

			ImGui::PushID(idString.c_str());

			switch (storage.GetType())
			{
				case DataType::Bool:
				{
                    if (bool value = storage.GetValue<bool>(i); UI::Property(indexString.c_str(), value))
					{
						storage.SetValue(value, i);
						modified = true;
					}
					break;
				}
				case DataType::SByte:
				{
					int8_t value = storage.GetValue<int8_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_S8, value, 1, "%d");
					break;
				}
				case DataType::Short:
				{
					int16_t value = storage.GetValue<int16_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_S16, value, 1, "%d");
					break;
				}
				case DataType::Int:
				{
					int32_t value = storage.GetValue<int32_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_S32, value, 1, "%d");
					break;
				}
				case DataType::Long:
				{
					int64_t value = storage.GetValue<int64_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_S64, value, 1, "%d");
					break;
				}
				case DataType::Byte:
				{
					uint8_t value = storage.GetValue<uint8_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_U8, value, 1, "%d");
					break;
				}
				case DataType::UShort:
				{
					uint16_t value = storage.GetValue<uint16_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_U16, value, 1, "%d");
					break;
				}
				case DataType::UInt:
				{
					uint32_t value = storage.GetValue<uint32_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_U32, value, 1, "%d");
					break;
				}
				case DataType::ULong:
				{
					uint64_t value = storage.GetValue<uint64_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_U64, value, 1, "%d");
					break;
				}
				case DataType::Float:
				{
					float value = storage.GetValue<float>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Float, value, 1, "%.3f");
					break;
				}
				case DataType::Double:
				{
					double value = storage.GetValue<double>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Double, value, 1, "%.6f");
					break;
				}
				case DataType::String:
				{
					std::string value = storage.GetValue<std::string>(i);
					drawStringElement(indexString, i, value);
					break;
				}
				case DataType::Vector2:
				{
					Vec2 value = storage.GetValue<Vec2>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Float, value, 2, "%.3f");
					break;
				}
				case DataType::Vector3:
				{
					Vec3 value = storage.GetValue<Vec3>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Float, value, 3, "%.3f");
					break;
				}
				case DataType::Vector4:
				{
					Vec4 value = storage.GetValue<Vec4>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Float, value, 4, "%.3f");
					break;
				}
				case DataType::Prefab:
				{
					AssetHandle handle = storage.GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<Prefab>(indexString.c_str(), handle, storage, i, elementToRemove);
					break;
				}
				case DataType::Entity:
				{
					UUID uuid = storage.GetValue<UUID>(i);
					PropertyEntityReferenceArray(indexString.c_str(), uuid, sceneContext, storage, i, elementToRemove);
					break;
				}
				case DataType::Mesh:
				{
					AssetHandle handle = storage.GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<Mesh>(indexString.c_str(), handle, storage, i, elementToRemove);
					break;
				}
				case DataType::StaticMesh:
				{
					AssetHandle handle = storage.GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<StaticMesh>(indexString.c_str(), handle, storage, i, elementToRemove);
					break;
				}
				case DataType::Material:
				{
					AssetHandle handle = storage.GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<MaterialAsset>(indexString.c_str(), handle, storage, i, elementToRemove);
					break;
				}
				/*case UnmanagedType::PhysicsMaterial:
				{
					ColliderMaterial material = storage->GetValue<ColliderMaterial>(managedInstance);

					if (PropertyGridHeader(fmt::format("{} (ColliderMaterial)", fieldName)))
					{
						if (Property("Friction", material.Friction))
						{
							storage->SetValue(managedInstance, material);
							result = true;
						}

						if (Property("Restitution", material.Restitution))
						{
							storage->SetValue(managedInstance, material);
							result = true;
						}

						EndTreeNode();
					}

					AssetHandle handle = valueWrapper.GetOrDefault<AssetHandle>(0);
					PropertyAssetReferenceArray<PhysicsMaterial>(indexString.c_str(), handle, managedInstance, arrayStorage, i, elementToRemove);
					break;
				}#1#
				case DataType::Texture2D:
				{
					AssetHandle handle = storage.GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<Texture2D>(indexString.c_str(), handle, storage, i, elementToRemove);
					break;
				}
				case DataType::Scene:
				{
					AssetHandle handle = storage.GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<Scene>(indexString.c_str(), handle, storage, i, elementToRemove);
					break;
				}
			}

			ImGui::PopID();
		}

		ImGui::PopID();

		if (elementToRemove != -1)
		{
			storage.RemoveAt(elementToRemove);
			modified = true;
		}

		return modified;
	}
	*/

}

/// -------------------------------------------------------
