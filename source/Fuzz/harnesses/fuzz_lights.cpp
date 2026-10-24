/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_lights.cpp
 * -------------------------------------------------------
 * X-Plane's lights.txt (xp_lights): the light catalog, then resolving named and parameterized OBJ lights against it
 * the way the object loader does (billboard cells, spill, parameter substitution) and the light evaluation terms.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_lights.h>
#include <SceneryEditorX/xplane/xp_obj8.h>
#include <set>

using namespace SceneryEditorX::XPlane;

namespace
{
	/* @brief Light names the input mentions: the second word of each line, where lights.txt puts them. */
	std::set<std::string> CandidateNames(const std::string_view text)
	{
		std::set<std::string> names = { "airplane_landing_core", "taxi_g", "rwy_edge_w", "obs_red_flash", "VERTICAL" };
		size_t start = 0;
		while (start < text.size() && names.size() < 48)
		{
			size_t end = text.find('\n', start);
			if (end == std::string_view::npos)
				end = text.size();
			const std::string_view line = text.substr(start, end - start);
			const size_t first = line.find_first_of(" \t");
			if (first != std::string_view::npos)
			{
				const size_t begin = line.find_first_not_of(" \t", first);
				if (begin != std::string_view::npos)
				{
					const size_t stop = line.find_first_of(" \t\r", begin);
					names.emplace(line.substr(begin, stop == std::string_view::npos ? std::string_view::npos : stop - begin));
				}
			}
			start = end + 1;
		}
		return names;
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	const std::string_view text = SceneryEditorX::Fuzz::AsText(data, size);
	LightCatalog catalog;
	std::string error;
	if (!catalog.Parse(text, &error))
		return 0;

	for (const std::string &name : CandidateNames(text))
	{
		const LightDefinition *definition = catalog.Find(name);

		Obj8Light light;
		light.kind = Obj8Light::Kind::Named;
		light.name = name;
		ResolvedLight resolved;
		std::string warning;
		(void)catalog.Resolve(light, resolved, &warning);

		// LIGHT_PARAM with as many arguments as the definition names, then too few and too many
		light.kind = Obj8Light::Kind::Param;
		const size_t params = definition ? definition->params.size() : 2;
		for (const size_t count : { params, params / 2, params + 3 })
		{
			light.params.assign(count, 0.5f);
			if (count > 0)
				light.params[0] = 1.0e9f;
			(void)catalog.Resolve(light, resolved, &warning);
			for (const LightSpillSpec &spill : resolved.spills)
				(void)spill.GetRange();
		}
	}
	return 0;
}
