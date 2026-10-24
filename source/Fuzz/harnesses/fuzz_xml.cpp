/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_xml.cpp
 * -------------------------------------------------------
 * The XML DOM under the WorldEditor import (xml_document): parsing (entities, character references, CDATA, comments,
 * nesting), writing the tree back, which must parse into a tree that writes the same text.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/project/xml_document.h>

using namespace SceneryEditorX::ProjectFile;

namespace
{
	/* @brief Walks the tree with an explicit stack, so only the code under test recurses. */
	void Visit(const XmlElement &root)
	{
		std::vector<const XmlElement *> pending = { &root };
		while (!pending.empty())
		{
			const XmlElement *element = pending.back();
			pending.pop_back();
			for (const auto &[key, value] : element->attributes)
				(void)EscapeXml(value);
			(void)element->FindChild("object");
			for (const XmlElement &child : element->children)
				pending.push_back(&child);
		}
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	XmlElement root;
	std::string error;
	if (!ParseXml(SceneryEditorX::Fuzz::AsText(data, size), root, &error))
		return 0;
	Visit(root);

	std::string written;
	WriteXml(root, written);
	XmlElement reread;
	SEDX_FUZZ_CHECK(ParseXml(written, reread, &error), "written XML doesn't parse: " + error);
	std::string again;
	WriteXml(reread, again);
	SEDX_FUZZ_CHECK(again == written, "XML text changes on a second round trip");
	return 0;
}
