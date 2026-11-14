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
 * message.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

// -------------------------------------------------------

/**
 * Translatable messages. A Msg is an English string literal marked for translation: it can only be made from a string
 * literal (its constructor is consteval), and the key the catalogs are searched by is hashed while compiling, so a
 * lookup at run time is one hash-table probe.
 *
 * The key is the text before the first "##" (Dear ImGui's ID marker): "Open##recent" and "PROPERTIES###Properties"
 * are translated as "Open" and "PROPERTIES", and TrL() (i18n_imgui.h) keeps the part after it as the widget's ID.
 * tools/i18n/sedx_i18n.py extracts the same text into resources/locale/sedx.pot.
 *
 * This header and the rest of source/SceneryEditorX/i18n use only the standard library.
 */
namespace SceneryEditorX
{
	namespace I18n
	{
		/* FNV-1a, 64 bits: the hash of every message key, at compile time (Msg) and when a catalog loads. */
		inline constexpr std::uint64_t HASH_OFFSET = 14695981039346656037ull;
		inline constexpr std::uint64_t HASH_PRIME = 1099511628211ull;

		/* Between a message's context and its text in the key, as gettext joins msgctxt and msgid. */
		inline constexpr char CONTEXT_SEPARATOR = '\x04';

		constexpr std::uint64_t HashAppend(std::uint64_t hash, const std::string_view bytes) noexcept
		{
			for (const char c : bytes)
			{
				hash ^= static_cast<unsigned char>(c);
				hash *= HASH_PRIME;
			}
			return hash;
		}

		/* @brief The key of a message: msgid alone, or msgctxt + '\x04' + msgid when it has a context. */
		constexpr std::uint64_t HashKey(const std::string_view context, const bool hasContext, const std::string_view msgid) noexcept
		{
			std::uint64_t hash = HASH_OFFSET;
			if (hasContext)
			{
				hash = HashAppend(hash, context);
				hash = HashAppend(hash, std::string_view(&CONTEXT_SEPARATOR, 1));
			}
			return HashAppend(hash, msgid);
		}

		/* @brief Length of the visible part of a label: the text before its first "##" (all of it when there is none). */
		constexpr std::size_t VisibleLength(const std::string_view text) noexcept
		{
			const std::size_t marker = text.find("##");
			return marker == std::string_view::npos ? text.size() : marker;
		}
	}

	/**
	 * @struct Msg
	 * @brief An English string literal marked for translation, with its catalog key hashed at compile time.
	 *
	 * Functions that translate take a Msg, so a literal converts on its own: Tr("Save"), TrF("{} items", n).
	 * Tables of labels that are translated later are written with TrNoop() so the extractor finds them:
	 *   static constexpr Msg NAMES[] = { TrNoop("Runway"), TrNoop("Taxiway") };  ...  Tr(NAMES[i])
	 */
	struct Msg
	{
		const char *text = "";			// The literal as written (NUL-terminated)
		const char *context = nullptr;	// Ctx()'s context, or nullptr
		std::uint32_t size = 0;			// Bytes of text
		std::uint32_t idSize = 0;		// Bytes of the translated part (before the first "##")
		std::uint32_t contextSize = 0;
		std::uint64_t key = I18n::HASH_OFFSET;	// I18n::HashKey of [context,] the translated part

		constexpr Msg() = default;

		template <std::size_t N>
		consteval Msg(const char (&literal)[N]) : text(literal), size(static_cast<std::uint32_t>(N - 1))
		{
			const std::string_view whole(literal, N - 1);
			idSize = static_cast<std::uint32_t>(I18n::VisibleLength(whole));
			key = I18n::HashKey({}, false, whole.substr(0, idSize));
		}

		/* @brief The literal as written, "##" suffix included. */
		[[nodiscard]] constexpr std::string_view Text() const { return { text, size }; }

		/* @brief The part that is translated (the catalog's msgid): the text before its first "##". */
		[[nodiscard]] constexpr std::string_view Id() const { return { text, idSize }; }

		[[nodiscard]] constexpr bool HasContext() const { return context != nullptr; }
		[[nodiscard]] constexpr std::string_view Context() const { return context ? std::string_view(context, contextSize) : std::string_view(); }

		/* @brief True when nothing is translated: an empty literal, or a pure ImGui ID ("##zoom"). */
		[[nodiscard]] constexpr bool IsEmpty() const { return idSize == 0; }
	};

	/**
	 * @brief A message with a context, for short English words that translate differently depending on use:
	 * Tr(Ctx("verb", "Open")) and Tr(Ctx("state", "Open")) are separate catalog entries (gettext's msgctxt).
	 */
	consteval Msg Ctx(const Msg context, const Msg text)
	{
		Msg message = text;
		message.context = context.text;
		message.contextSize = context.size;
		message.key = I18n::HashKey(context.Text(), true, text.Id());
		return message;
	}

	/* @brief Marks a literal for extraction without translating it here (tables translated later with Tr()). */
	consteval Msg TrNoop(const Msg text)
	{
		return text;
	}
}

// -------------------------------------------------------
