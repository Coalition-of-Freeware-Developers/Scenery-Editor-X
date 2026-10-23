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
 * xml_document.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * A small XML DOM for WorldEditor's earth.wed.xml: elements, attributes and text, with the five named entities
	 * and numeric character references. Comments, the XML declaration, processing instructions and DOCTYPE are
	 * skipped; CDATA becomes text. No namespaces or DTD validation (WorldEditor uses neither).
	 */
	struct XmlElement
	{
		std::string name;
		std::vector<std::pair<std::string, std::string>> attributes;
		std::vector<XmlElement> children;
		std::string text;					// Character data directly inside the element, trimmed

		[[nodiscard]] const std::string *FindAttribute(std::string_view key) const;
		[[nodiscard]] std::string_view Attribute(std::string_view key, std::string_view fallback = {}) const;
		[[nodiscard]] bool HasAttribute(std::string_view key) const { return FindAttribute(key) != nullptr; }

		[[nodiscard]] const XmlElement *FindChild(std::string_view childName) const;

		/* @brief Sets an attribute (replacing a value it already has); attributes keep the order they were added in. */
		XmlElement &Set(std::string key, std::string value);
		XmlElement &AddChild(std::string childName);
	};

	/**
	 * @brief Parses an XML document into its root element.
	 * @return False with "line N: reason" in error when the text isn't well-formed.
	 */
	bool ParseXml(std::string_view text, XmlElement &root, std::string *error = nullptr);

	/* @brief Escapes & < > " for attribute values and text; drops control characters XML can't carry. */
	std::string EscapeXml(std::string_view text);

	/* @brief Writes an element and its children, two spaces per level, empty elements self-closed. */
	void WriteXml(const XmlElement &element, std::string &out, int indent = 0);
}

// -------------------------------------------------------
