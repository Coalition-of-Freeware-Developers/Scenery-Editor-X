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
 * i18n_imgui.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "i18n_imgui.h"
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>
#include <unordered_map>

// "a###b" must hash like "b" (Dear ImGui 1.92.6): TrL relies on it to keep every ID
static_assert(IMGUI_VERSION_NUM >= 19206, "TrL needs Dear ImGui 1.92.6 or newer");

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		struct LabelKey
		{
			const char *text;
			const char *context;
			bool operator==(const LabelKey &) const = default;
		};

		struct LabelKeyHash
		{
			std::size_t operator()(const LabelKey &key) const noexcept
			{
				const auto a = reinterpret_cast<std::uintptr_t>(key.text);
				const auto b = reinterpret_cast<std::uintptr_t>(key.context);
				return static_cast<std::size_t>(a * 0x9E3779B97F4A7C15ull ^ (b + 0x632BE59BD9B4E019ull));
			}
		};

		struct LabelCache
		{
			std::uint64_t generation = 0;
			std::unordered_map<LabelKey, std::string, LabelKeyHash> labels;	// Literal (pointer identity) -> label
		};

		LabelCache &Cache()
		{
			static LabelCache cache;
			return cache;
		}
	}

	namespace I18n
	{
		std::string MakeLabel(const std::string_view shown, const std::string_view original)
		{
			std::string label;
			const std::size_t ids = original.find("###");
			label.reserve(shown.size() + original.size() + 3);
			label.append(shown);
			if (ids != std::string_view::npos)
				label.append(original.substr(ids));
			else
			{
				label += "###";
				label.append(original);
			}
			return label;
		}

		void ApplyImGuiLocalization()
		{
			if (!ImGui::GetCurrentContext())
				return;

			// Dear ImGui's own strings (imgui.cpp, GLocalizationEntriesEnUS). Those with "###" are menu labels; the
			// others are shown as they are. The strings are copied: ImGui keeps the pointers until the next call.
			struct Entry
			{
				ImGuiLocKey key;
				Msg text;
			};
			static constexpr Entry ENTRIES[] =
			{
				// TRANSLATORS: Dear ImGui's table header context menu
				{ ImGuiLocKey_TableSizeOne, Ctx("imgui", "Size column to fit###SizeOne") },
				{ ImGuiLocKey_TableSizeAllFit, Ctx("imgui", "Size all columns to fit###SizeAll") },
				{ ImGuiLocKey_TableSizeAllDefault, Ctx("imgui", "Size all columns to default###SizeAll") },
				{ ImGuiLocKey_TableReset, Ctx("imgui", "Reset") },
				{ ImGuiLocKey_TableResetOrder, Ctx("imgui", "Reset order###ResetOrder") },
				{ ImGuiLocKey_TableResetVisibility, Ctx("imgui", "Reset visibility###ResetVisibility") },
				// TRANSLATORS: names in Dear ImGui's window list (Ctrl+Tab)
				{ ImGuiLocKey_WindowingMainMenuBar, Ctx("imgui", "(Main menu bar)") },
				{ ImGuiLocKey_WindowingPopup, Ctx("imgui", "(Popup)") },
				{ ImGuiLocKey_WindowingUntitled, Ctx("imgui", "(Untitled)") },
				// TRANSLATORS: %s is a web address
				{ ImGuiLocKey_OpenLink_s, Ctx("imgui", "Open '%s'") },
				{ ImGuiLocKey_CopyLink, Ctx("imgui", "Copy Link###CopyLink") },
				// TRANSLATORS: Dear ImGui's docking tab menu and hints
				{ ImGuiLocKey_DockingHideTabBar, Ctx("imgui", "Hide tab bar###HideTabBar") },
				{ ImGuiLocKey_DockingHoldShiftToDock, Ctx("imgui", "Hold SHIFT to enable Docking window.") },
				{ ImGuiLocKey_DockingDragToUndockOrMoveNode, Ctx("imgui", "Click and drag to move or undock whole node.") },
			};

			static std::vector<std::string> storage;
			std::vector<std::string> strings;
			strings.reserve(std::size(ENTRIES));
			for (const Entry &entry : ENTRIES)
			{
				const bool isLabel = entry.text.Text().find("###") != std::string_view::npos;
				strings.emplace_back(isLabel ? TrL(entry.text) : Tr(entry.text));
			}

			ImGuiLocEntry localized[std::size(ENTRIES)];
			for (std::size_t i = 0; i < std::size(ENTRIES); ++i)
				localized[i] = { ENTRIES[i].key, strings[i].c_str() };
			ImGui::LocalizeRegisterEntries(localized, static_cast<int>(std::size(ENTRIES)));
			storage = std::move(strings);	// The new pointers are registered: the old strings can go
		}
	}

	const char *TrL(const Text &text)
	{
		if (!text.IsTranslated())
			return text.Source();

		const Msg &label = text.GetMsg();
		const char *translated = I18n::Translate(label);
		if (translated == label.text)
			return label.text;	// English or untranslated: the label as it is, so its ID by definition

		LabelCache &cache = Cache();
		const std::uint64_t generation = I18n::GetGeneration();
		if (cache.generation != generation)
		{
			cache.labels.clear();
			cache.generation = generation;
		}

		const LabelKey key { label.text, label.context };
		const auto it = cache.labels.find(key);
		if (it != cache.labels.end())
			return it->second.c_str();

		std::string made = I18n::MakeLabel(translated, label.Text());
		if (ImHashStr(made.c_str()) != ImHashStr(label.text))
			SEDX_CORE_ERROR_TAG("I18n", "Translated label '{}' doesn't keep the ID of '{}'", made, label.Text());
		return cache.labels.emplace(key, std::move(made)).first->second.c_str();
	}
}

// -------------------------------------------------------
