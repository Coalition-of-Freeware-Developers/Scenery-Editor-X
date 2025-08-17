/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_manager.cpp
* -------------------------------------------------------
* Created: 1/8/2025
* -------------------------------------------------------
*/
#include "ui_manager.h"
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include "SceneryEditorX/renderer/vulkan/vk_image.h"

/// -------------------------------------------------------

namespace ImGui
{
	extern bool ImageButtonEx(ImGuiID id, ImTextureID texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec2& padding, const ImVec4& bg_col, const ImVec4& tint_col);
}

/// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	
	static int s_UIContextID = 0;
	static uint32_t s_Counter = 0;
	static char s_IDBuffer[16 + 2 + 1] = "##";
	static char s_LabelIDBuffer[1024 + 1];

    /// -------------------------------------------------------

	const char *UIManager::GenerateID()
	{
		snprintf(s_IDBuffer + 2, 16, "%u", s_Counter++);
		return s_IDBuffer;
	}

	const char *UIManager::GenerateLabelID(std::string_view label)
	{
		*std::format_to_n(s_LabelIDBuffer, std::size(s_LabelIDBuffer), "{}##{}", label, s_Counter++).out = 0;
		return s_LabelIDBuffer;
	}

	void UIManager::PushID()
	{
		ImGui::PushID(s_UIContextID++);
		s_Counter = 0;
	}

	void UIManager::PopID()
	{
		ImGui::PopID();
		s_UIContextID--;
	}

	bool UIManager::IsInputEnabled()
	{
		const auto& io = ImGui::GetIO();
		return (io.ConfigFlags & ImGuiConfigFlags_NoMouse) == 0 && (io.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard) == 0;
	}

	void UIManager::SetInputEnabled(bool enabled)
	{
		auto& io = ImGui::GetIO();

		if (enabled)
		{
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			io.ConfigFlags &= ~ImGuiConfigFlags_NavNoCaptureKeyboard;
		}
		else
		{
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
			io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;
		}
	}

	void UIManager::ShiftCursorX(float distance)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + distance);
	}

	void UIManager::ShiftCursorY(float distance)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + distance);
	}

	void UIManager::ShiftCursor(float x, float y)
	{
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor.x + x, cursor.y + y));
	}

	void UIManager::BeginPropertyGrid(uint32_t columns)
	{
		PushID();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
		ImGui::Columns(columns);
	}

	void UIManager::EndPropertyGrid()
	{
		ImGui::Columns(1);
		Draw::Underline();
		ImGui::PopStyleVar(2); /// ItemSpacing, FramePadding
		ShiftCursorY(18.0f);
		PopID();
	}

	bool UIManager::BeginTreeNode(const char *name, bool defaultOpen)
	{
		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
		if (defaultOpen)
			treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

		return ImGui::TreeNodeEx(name, treeNodeFlags);
	}

	void UIManager::EndTreeNode()
    {
        ImGui::TreePop();
    }

    bool UIManager::ColoredButton(const char *label, const ImVec4 &backgroundColor, ImVec2 buttonSize)
    {
        ScopedColour buttonColor(ImGuiCol_Button, backgroundColor);
        return ImGui::Button(label, buttonSize);
    }

    bool UIManager::ColoredButton(const char *label,
                                const ImVec4 &backgroundColor,
                                const ImVec4 &foregroundColor,
                                ImVec2 buttonSize)
	{
		ScopedColour textColor(ImGuiCol_Text, foregroundColor);
		ScopedColour buttonColor(ImGuiCol_Button, backgroundColor);
		return ImGui::Button(label, buttonSize);
	}

	bool UIManager::TableRowClickable(const char *id, float rowHeight)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DC.CurrLineSize.y = rowHeight;

		ImGui::TableNextRow(0, rowHeight);
		ImGui::TableNextColumn();

		window->DC.CurrLineTextBaseOffset = 3.0f;
		const ImVec2 rowAreaMin = ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), 0).Min;
		const ImVec2 rowAreaMax = {
		    ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(),
		    ImGui::TableGetColumnCount() - 1).Max.x, rowAreaMin.y + rowHeight
		};

		ImGui::PushClipRect(rowAreaMin, rowAreaMax, false);

		bool isRowHovered, held;
		bool isRowClicked = ImGui::ButtonBehavior(ImRect(rowAreaMin, rowAreaMax), ImGui::GetID(id), &isRowHovered, &held, ImGuiButtonFlags_AllowOverlap);

		ImGui::SetItemAllowOverlap();
		ImGui::PopClipRect();

		return isRowClicked;
	}

	void UIManager::Separator(ImVec2 size, ImVec4 color)
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
		ImGui::BeginChild("sep", size);
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	bool UIManager::IsWindowFocused(const char *windowName, const bool checkRootWindow)
	{
		ImGuiWindow* currentNavWindow = GImGui->NavWindow;
		if (checkRootWindow)
		{
			/// Get the actual nav window (not e.g a table)
			ImGuiWindow* lastWindow = nullptr;
			while (lastWindow != currentNavWindow)
			{
				lastWindow = currentNavWindow;
				currentNavWindow = currentNavWindow->RootWindow;
			}
		}

		return currentNavWindow == ImGui::FindWindowByName(windowName);
	}

	void UIManager::HelpMarker(const char *desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	bool UIManager::ImageButton(const Ref<Texture2D> &texture, const ImVec2 &size, const ImVec4 &tint)
	{
		return ImageButton(texture, size, ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), tint);
	}

	void UIManager::ImageToolTip(const Ref<Texture2D> &texture)
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		const std::string filepath = texture->GetPath().string();
		ImGui::TextUnformatted(filepath.c_str());
		ImGui::PopTextWrapPos();
		Image(texture, ImVec2(384, 384));
		ImGui::EndTooltip();
	}
	
	ImTextureID UIManager::GetTextureID(const Ref<Image2D> &image)
	{
	    Ref<Image2D> vulkanImage = image.As<Image2D>();
	    if (const auto&[sampler, imageView, imageLayout] = vulkanImage->GetDescriptorInfoVulkan(); imageView)
            return reinterpret_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler, imageView, imageLayout));

        return 0;
	}

	ImTextureID UIManager::GetTextureID(const Ref<Texture2D> &texture)
	{
	    Ref<Texture2D> vulkanTexture = texture.As<Texture2D>();
	    const auto&[sampler, imageView, imageLayout] = vulkanTexture->GetDescriptorInfoVulkan();
	    if (!imageView)
            return reinterpret_cast<ImTextureID>(nullptr);

        return reinterpret_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler, imageView, imageLayout));
	}

	void UIManager::Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		SEDX_CORE_VERIFY(image, "Image is null");

	    Ref<Image2D> vulkanImage = image.As<Image2D>();
	    const auto& imageInfo = vulkanImage->GetImageInfo();
	    if (!imageInfo.view)
	        return;

	    const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.view, vulkanImage->GetDescriptorInfoVulkan().imageLayout);
	    ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
	}

	void UIManager::Image(const Ref<Image2D>& image, uint32_t imageLayer, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		SEDX_CORE_VERIFY(image, "Image is null");

	    Ref<Image2D> vulkanImage = image.As<Image2D>();
	    auto imageInfo = vulkanImage->GetImageInfo();
	    imageInfo.view = vulkanImage->GetLayerImageView(imageLayer);
	    if (!imageInfo.view)
	        return;

	    const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.view, vulkanImage->GetDescriptorInfoVulkan().imageLayout);
	    ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
	}

	void UIManager::ImageMip(const Ref<Image2D>& image, uint32_t mip, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		SEDX_CORE_VERIFY(image, "Image is null");

		Ref<Image2D> vulkanImage = image.As<Image2D>();
		auto imageInfo = vulkanImage->GetImageInfo();
		imageInfo.view = vulkanImage->GetMipImageView(mip);
		if (!imageInfo.view)
			return;

		const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.view, vulkanImage->GetDescriptorInfoVulkan().imageLayout);
		ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
	}

	void UIManager::Image(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		SEDX_CORE_VERIFY(texture, "Texture is null");

	    Ref<Texture2D> vulkanTexture = texture.As<Texture2D>();
	    const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetDescriptorInfoVulkan();
	    if (!imageInfo.imageView)
	        return;

	    const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
		ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
	}

	bool UIManager::ImageButton(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImageButton(nullptr, image, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool UIManager::ImageButton(const char* stringID, const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		Ref<Image2D> vulkanImage = image.As<Image2D>();
		const auto& imageInfo = vulkanImage->GetImageInfo();
		if (!imageInfo.view)
			return false;

		const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.view, vulkanImage->GetDescriptorInfoVulkan().imageLayout);
		ImGuiID id = static_cast<ImGuiID>(reinterpret_cast<uint64_t>(imageInfo.view) >> 32 ^ static_cast<uint32_t>(reinterpret_cast<uint64_t>(imageInfo.view)));
		if (stringID)
		{
			const ImGuiID strID = ImGui::GetID(stringID);
			id = id ^ strID;
		}

        return ImGui::ImageButtonEx(id, reinterpret_cast<ImTextureID>(textureID), size, uv0, uv1,
            ImVec2{ static_cast<float>(frame_padding), static_cast<float>(frame_padding) },
			ImColor(bg_col).Value, ImColor(tint_col).Value);
	}

	bool UIManager::ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImageButton(nullptr, texture, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool UIManager::ImageButton(const char* stringID, const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		SEDX_CORE_VERIFY(texture);
		if (!texture)
			return false;

		Ref<Texture2D> vulkanTexture = texture.As<Texture2D>();
		
		/// This is technically okay, could mean that GPU just hasn't created the texture yet
		SEDX_CORE_VERIFY(vulkanTexture->GetImage()); 
		if (!vulkanTexture->GetImage())
			return false;

		const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetDescriptorInfoVulkan();
		const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
		ImGuiID id = static_cast<ImGuiID>(reinterpret_cast<uint64_t>(imageInfo.imageView) >> 32 ^ static_cast<uint32_t>(reinterpret_cast<uint64_t>(imageInfo.imageView)));
		if (stringID)
		{
			const ImGuiID strID = ImGui::GetID(stringID);
			id = id ^ strID;
		}

        return ImGui::ImageButtonEx(id, reinterpret_cast<ImTextureID>(textureID), size, uv0, uv1,
        ImVec2{static_cast<float>(frame_padding), static_cast<float>(frame_padding)},ImColor(bg_col).Value,ImColor(tint_col).Value);
    }

}

/// -------------------------------------------------------
