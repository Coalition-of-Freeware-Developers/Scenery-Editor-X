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
 * sky_conditions.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "sky_conditions.h"
#include "environment_state.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	namespace
	{
		/* @brief 0 at a, 1 at b, linear in between (either order). */
		float Ramp(const float x, const float a, const float b)
		{
			if (a == b)
				return x >= b ? 1.0f : 0.0f;
			return std::clamp((x - a) / (b - a), 0.0f, 1.0f);
		}

		float SmoothRamp(const float x, const float a, const float b)
		{
			const float t = Ramp(x, a, b);
			return t * t * (3.0f - 2.0f * t);
		}

		size_t Index(const SkyCondition condition) { return static_cast<size_t>(condition); }
	}

	const char *GetSkyConditionCode(const SkyCondition condition)
	{
		static constexpr const char *codes[SKY_CONDITION_COUNT] = { "clean", "hazy", "foggy", "sockd", "ocast", "strat", "snowy", "mount", "hialt", "orbit", "final" };
		return condition < SkyCondition::Count ? codes[Index(condition)] : "";
	}

	SkyWeights ComputeSkyWeights(const EnvironmentState &state, const float observerAltitudeMslM, const float groundElevationM)
	{
		SkyWeights weights{};

		// Visibility: piecewise linear in log2(V) between the classes' anchor visibilities
		struct Anchor { SkyCondition condition; float visibility; };
		static constexpr Anchor anchors[] = { { SkyCondition::Socked, 250.0f }, { SkyCondition::Foggy, 1000.0f }, { SkyCondition::Hazy, 10000.0f }, { SkyCondition::Clean, 40000.0f } };
		const float v = std::log2(std::max(state.visibilityM, 1.0f));
		SkyWeights air{};
		if (v <= std::log2(anchors[0].visibility))
			air[Index(anchors[0].condition)] = 1.0f;
		else if (v >= std::log2(anchors[3].visibility))
			air[Index(anchors[3].condition)] = 1.0f;
		else
		{
			for (size_t i = 0; i + 1 < std::size(anchors); ++i)
			{
				const float a = std::log2(anchors[i].visibility);
				const float b = std::log2(anchors[i + 1].visibility);
				if (v >= a && v <= b)
				{
					const float t = (v - a) / (b - a);
					air[Index(anchors[i].condition)] = 1.0f - t;
					air[Index(anchors[i + 1].condition)] = t;
					break;
				}
			}
		}

		// A full, low cloud deck: overcast, or stratus when the deck is stratus
		float deck = 0.0f;
		float stratus = 0.0f;
		for (const CloudLayer &layer : state.clouds)
		{
			if (layer.coverage <= 0.0f || layer.baseMslM - groundElevationM >= 1000.0f)
				continue;
			const float full = SmoothRamp(layer.coverage, 0.75f, 0.875f);
			if (full > deck)
			{
				deck = full;
				stratus = 1.0f - std::clamp(std::abs(layer.type - 1.0f), 0.0f, 1.0f);	// Type 1 is stratus
			}
		}

		// Falling snow: precipitation below freezing
		const float snowfall = std::clamp(state.rainPercent, 0.0f, 1.0f) * Ramp(state.temperatureC, 1.0f, -1.0f);
		const float snowy = SmoothRamp(snowfall, 0.0f, 0.3f);

		// Observer altitude
		const float orbit = SmoothRamp(observerAltitudeMslM, 20000.0f, 30000.0f);
		const float high = SmoothRamp(observerAltitudeMslM, 6000.0f, 9000.0f) * (1.0f - orbit);
		const float low = 1.0f - orbit - high;

		weights[Index(SkyCondition::Orbit)] = orbit;
		weights[Index(SkyCondition::HighAltitude)] = high;
		weights[Index(SkyCondition::Snowy)] = low * snowy;
		const float ground = low * (1.0f - snowy);
		weights[Index(SkyCondition::Stratus)] = ground * deck * stratus;
		weights[Index(SkyCondition::Overcast)] = ground * deck * (1.0f - stratus);
		for (size_t i = 0; i < SKY_CONDITION_COUNT; ++i)
			weights[i] += ground * (1.0f - deck) * air[i];
		return weights;
	}

	SkyConditionTable::SkyConditionTable() = default;

	bool SkyConditionTable::Parse(const std::string_view json, std::string *error)
	{
		try
		{
			const nlohmann::json root = nlohmann::json::parse(json.begin(), json.end());
			if (const auto it = root.find("xp_build"); it != root.end() && it->is_string())
				m_XPlaneBuild = it->get<std::string>();
			const auto classes = root.find("classes");
			if (classes == root.end() || !classes->is_object())
			{
				if (error)
					*error = "no \"classes\" object";
				return false;
			}
			for (size_t i = 0; i < SKY_CONDITION_COUNT; ++i)
			{
				const auto entry = classes->find(GetSkyConditionCode(static_cast<SkyCondition>(i)));
				if (entry == classes->end() || !entry->is_object())
					continue;
				SkyMultipliers &m = m_Classes[i];
				const auto read = [&](const char *key, float &value)
				{
					if (const auto field = entry->find(key); field != entry->end() && field->is_number())
						value = field->get<float>();
				};
				read("ambient_ratio", m.ambient);
				read("direct_ratio", m.direct);
				read("tone_ratio", m.tone);
				read("rayleigh_ratio", m.rayleigh);
				read("mie_ratio", m.mie);
				read("shadow_level", m.shadowLevel);
				read("shadow_offset", m.shadowOffset);
			}
			return true;
		}
		catch (const std::exception &e)
		{
			if (error)
				*error = e.what();
			return false;
		}
	}

	bool SkyConditionTable::Load(const std::filesystem::path &path, std::string *error)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			if (error)
				*error = "can't open " + path.string();
			return false;
		}
		std::ostringstream text;
		text << file.rdbuf();
		return Parse(text.str(), error);
	}

	SkyMultipliers SkyConditionTable::Blend(const SkyWeights &weights) const
	{
		SkyMultipliers out{ 0, 0, 0, 0, 0, 0, 0 };
		float total = 0.0f;
		for (size_t i = 0; i < SKY_CONDITION_COUNT; ++i)
		{
			const float w = weights[i];
			if (w <= 0.0f)
				continue;
			const SkyMultipliers &m = m_Classes[i];
			out.ambient += w * m.ambient;
			out.direct += w * m.direct;
			out.tone += w * m.tone;
			out.rayleigh += w * m.rayleigh;
			out.mie += w * m.mie;
			out.shadowLevel += w * m.shadowLevel;
			out.shadowOffset += w * m.shadowOffset;
			total += w;
		}
		if (total <= 0.0f)
			return {};
		const float inv = 1.0f / total;
		out.ambient *= inv;
		out.direct *= inv;
		out.tone *= inv;
		out.rayleigh *= inv;
		out.mie *= inv;
		out.shadowLevel *= inv;
		out.shadowOffset *= inv;
		return out;
	}
}

// -------------------------------------------------------
