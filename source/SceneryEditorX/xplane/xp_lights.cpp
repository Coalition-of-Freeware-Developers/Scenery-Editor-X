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
 * xp_lights.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_lights.h"
#include "xp_text.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		/* @brief Billboards shrink as distance^(decay - 1) on screen (lights.txt LIGHT_PARAMS suggests 0.56 .. 0.7). */
		constexpr float BILLBOARD_DECAY = 0.69f;
		/* @brief World size of a size-1 billboard at 1 m; tuned so a 15 cd taxiway light reads like X-Plane's. */
		constexpr float BILLBOARD_SCALE = 0.02f;
		/* @brief Illuminance (lux) at which a photometric spill is treated as dark. */
		constexpr float SPILL_CUTOFF_LUX = 2.0f;

		bool ParseRecordType(const std::string_view word, LightRecordType &type)
		{
			if (word == "BILLBOARD_HW")
				type = LightRecordType::BillboardHW;
			else if (word == "BILLBOARD_SW")
				type = LightRecordType::BillboardSW;
			else if (word == "SPILL_HW_DIR")
				type = LightRecordType::SpillHWDir;
			else if (word == "SPILL_HW_FLA")
				type = LightRecordType::SpillHWFlash;
			else if (word == "SPILL_SW")
				type = LightRecordType::SpillSW;
			else if (word == "SPILL_GND")
				type = LightRecordType::SpillGround;
			else if (word == "SPILL_GND_REV")
				type = LightRecordType::SpillGroundReverse;
			else
				return false;
			return true;
		}

		/* @brief "ZERO__" -> "ZERO": hint parameters may repeat with trailing underscores. */
		std::string_view StripUnderscores(std::string_view word)
		{
			while (!word.empty() && word.back() == '_')
				word.remove_suffix(1);
			return word;
		}

		/* @brief The fixed value a hint parameter stands for (ZERO, ONE, NEG_ONE, UNUSED). */
		float HintValue(const std::string_view word)
		{
			const std::string_view base = StripUnderscores(word);
			if (base == "ONE")
				return 1.0f;
			if (base == "NEG_ONE")
				return -1.0f;
			return 0.0f;
		}

		/**
		 * @class ArgumentReader
		 * @brief Evaluates a record's arguments: numbers, "<n>cd" intensities, LIGHT_PARAM_DEF names (replaced by the
		 * OBJ's LIGHT_PARAM values) and hint names.
		 */
		class ArgumentReader
		{
		public:
			ArgumentReader(const LightRecord &record, const LightDefinition &definition, const std::vector<float> &values)
				: m_Record(record), m_Definition(definition), m_Values(values) {}

			[[nodiscard]] size_t Count() const { return m_Record.args.size(); }
			[[nodiscard]] std::string_view Word(const size_t index) const { return index < m_Record.args.size() ? std::string_view(m_Record.args[index]) : std::string_view(); }

			/* @brief The value of an argument; candela receives whether it is a photometric intensity. */
			float Value(const size_t index, const float fallback = 0.0f, bool *candela = nullptr) const
			{
				if (candela)
					*candela = false;
				if (index >= m_Record.args.size())
					return fallback;

				std::string_view word = m_Record.args[index];
				if (word.size() > 2 && word.ends_with("cd"))
				{
					if (candela)
						*candela = true;
					word.remove_suffix(2);
				}

				float value = 0.0f;
				if (TryParseFloat(word, value))
					return value;

				// A LIGHT_PARAM_DEF name: the OBJ supplies it, in definition order
				const auto it = std::ranges::find(m_Definition.params, word);
				if (it != m_Definition.params.end())
				{
					if (candela && word == "INTENSITY")
						*candela = true;
					const size_t slot = static_cast<size_t>(it - m_Definition.params.begin());
					return slot < m_Values.size() ? m_Values[slot] : HintValue(word);
				}

				return HintValue(word);
			}

			/* @brief True when the argument names the dataref array index rather than a value (SW lights put INDEX in A). */
			[[nodiscard]] bool IsIndex(const size_t index) const { return StripUnderscores(Word(index)) == "INDEX"; }

		private:
			const LightRecord &m_Record;
			const LightDefinition &m_Definition;
			const std::vector<float> &m_Values;
		};

		void ReadColor(const ArgumentReader &args, float color[4])
		{
			for (size_t i = 0; i < 4; ++i)
				color[i] = args.Value(i, 1.0f);
			if (args.IsIndex(3))
				color[3] = 1.0f;
		}

		void ReadDirection(const ArgumentReader &args, const size_t first, float direction[3])
		{
			for (size_t i = 0; i < 3; ++i)
				direction[i] = args.Value(first + i);
		}

		/* @brief A cell size / column / row argument as an int: saturated, since a damaged file's 1e27 doesn't fit one. */
		int ToCell(const float value)
		{
			return std::isfinite(value) ? static_cast<int>(std::clamp(value, -65536.0f, 65536.0f)) : 0;
		}

		/* @brief Texture rectangle of a cell given as size, column and row in units of the cell size. */
		void CellToUv(const int cellSize, const int column, const int row, const int divisionsX, const int divisionsY, float uv[4])
		{
			const float du = static_cast<float>(std::max(cellSize, 1)) / static_cast<float>(std::max(divisionsX, 1));
			const float dv = static_cast<float>(std::max(cellSize, 1)) / static_cast<float>(std::max(divisionsY, 1));
			uv[0] = static_cast<float>(column) * du;
			uv[1] = static_cast<float>(row) * dv;
			uv[2] = uv[0] + du;
			uv[3] = uv[1] + dv;
		}

		float Length3(const float v[3])
		{
			return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		}

		float SmoothStep(const float edge0, const float edge1, const float x)
		{
			const float t = std::clamp((x - edge0) / std::max(edge1 - edge0, 1e-6f), 0.0f, 1.0f);
			return t * t * (3.0f - 2.0f * t);
		}

		float Fraction(const double value)
		{
			return static_cast<float>(value - std::floor(value));
		}
	}

	// -------------------------------------------------------

	bool LightSpillSpec::IsSpot() const
	{
		return Length3(direction) > 1e-4f && width > -0.999f && width < 0.999f;
	}

	float LightSpillSpec::GetRange() const
	{
		if (IsPhotometric())
			return std::clamp(std::sqrt(intensity / SPILL_CUTOFF_LUX), 1.0f, 400.0f);
		return std::max(std::abs(size), 0.5f);
	}

	// -------------------------------------------------------

	bool LightCatalog::Parse(const std::string_view text, std::string *error)
	{
		m_Lights.clear();
		m_Texture.clear();
		m_DivisionsX = 16;
		m_DivisionsY = 8;

		LineReader reader(text);
		std::string_view line;
		std::vector<std::string_view> t;

		// Header: "A" / "I", version, LIGHT_SPECS
		int headerLine = 0;
		while (headerLine < 3 && reader.Next(line))
		{
			line = Trim(line);
			if (line.empty())
				continue;

			const bool ok = (headerLine == 0 && (line == "A" || line == "I")) || (headerLine == 1 && ParseInt(line, 0) > 0) ||
							(headerLine == 2 && line == "LIGHT_SPECS");
			if (!ok)
			{
				if (error)
					*error = "Not a lights.txt file (bad header line '" + std::string(line) + "')";
				return false;
			}
			++headerLine;
		}

		if (headerLine < 3)
		{
			if (error)
				*error = "Not a lights.txt file (truncated header)";
			return false;
		}

		while (reader.Next(line))
		{
			if (const size_t comment = line.find('#'); comment != std::string_view::npos)
				line = line.substr(0, comment);
			Tokenize(line, t);
			if (t.empty())
				continue;

			const std::string_view cmd = t[0];
			if (cmd == "TEXTURE" && t.size() > 1)
			{
				if (m_Texture.empty())
					m_Texture = std::string(RestOfLine(line, 1)); // the first TEXTURE is the close-range one
				continue;
			}
			if (cmd == "X_DIVISIONS" && t.size() > 1)
			{
				m_DivisionsX = std::max(1, ParseInt(t[1], 16));
				continue;
			}
			if (cmd == "Y_DIVISIONS" && t.size() > 1)
			{
				m_DivisionsY = std::max(1, ParseInt(t[1], 8));
				continue;
			}

			if (cmd == "LIGHT_PARAM_DEF" && t.size() > 2)
			{
				LightDefinition &definition = m_Lights[std::string(t[1])];
				definition.name = std::string(t[1]);
				definition.params.clear();
				const size_t count = static_cast<size_t>(std::max(0, ParseInt(t[2], 0)));
				for (size_t i = 0; i < count && 3 + i < t.size(); ++i)
					definition.params.emplace_back(t[3 + i]);
				continue;
			}

			LightRecordType type;
			if (t.size() < 2 || !ParseRecordType(cmd, type))
				continue; // LIGHT_PARAMS and other global settings the editor doesn't need

			LightDefinition &definition = m_Lights[std::string(t[1])];
			definition.name = std::string(t[1]);

			LightRecord record;
			record.type = type;
			for (size_t i = 2; i < t.size(); ++i)
				record.args.emplace_back(t[i]);
			definition.records.push_back(std::move(record));
		}

		if (m_Lights.empty())
		{
			if (error)
				*error = "lights.txt defines no lights";
			return false;
		}
		return true;
	}

	bool LightCatalog::Load(const std::filesystem::path &path, std::string *error)
	{
		std::string text;
		if (!ReadTextFile(path, text))
		{
			if (error)
				*error = "Could not read '" + path.string() + "'";
			return false;
		}

		if (!Parse(text, error))
			return false;

		m_Path = path;
		return true;
	}

	const LightDefinition *LightCatalog::Find(const std::string_view name) const
	{
		const auto it = m_Lights.find(std::string(name));
		return it != m_Lights.end() ? &it->second : nullptr;
	}

	bool LightCatalog::Resolve(const Obj8Light &light, ResolvedLight &out, std::string *warning) const
	{
		out = ResolvedLight{};
		const std::vector<float> &p = light.params;
		const auto param = [&p](const size_t i, const float fallback = 0.0f) { return i < p.size() ? p[i] : fallback; };

		switch (light.kind)
		{
			case Obj8Light::Kind::Custom:
			{
				// LIGHT_CUSTOM <x y z> r g b a s s1 t1 s2 t2 <dataref>
				LightBillboardSpec billboard;
				for (size_t i = 0; i < 4; ++i)
					billboard.color[i] = param(i, 1.0f);
				billboard.size = param(4, 1.0f);
				billboard.uv[0] = param(5);
				billboard.uv[1] = param(6);
				billboard.uv[2] = param(7, 1.0f);
				billboard.uv[3] = param(8, 1.0f);
				billboard.customTexture = true;
				billboard.processor = GetLightProcessor(light.dataref.name);
				out.billboards.push_back(billboard);
				return true;
			}

			case Obj8Light::Kind::SpillCustom:
			{
				// LIGHT_SPILL_CUSTOM <x y z> r g b a s dx dy dz semi <dataref>
				LightSpillSpec spill;
				for (size_t i = 0; i < 4; ++i)
					spill.color[i] = param(i, 1.0f);
				spill.size = param(4, 1.0f);
				for (size_t i = 0; i < 3; ++i)
					spill.direction[i] = param(5 + i);
				spill.width = param(8, 1.0f);
				spill.processor = GetLightProcessor(light.dataref.name);
				out.spills.push_back(spill);
				return true;
			}

			case Obj8Light::Kind::Legacy:
			{
				// VLIGHT color; values outside 0..1 select v7 special lights, which the editor draws as plain ones.
				LightBillboardSpec billboard;
				for (size_t i = 0; i < 3; ++i)
					billboard.color[i] = std::clamp(param(i, 1.0f), 0.0f, 1.0f);
				CellToUv(1, 0, 0, m_DivisionsX, m_DivisionsY, billboard.uv);
				out.billboards.push_back(billboard);
				return true;
			}

			case Obj8Light::Kind::Named:
			case Obj8Light::Kind::Param:
				break;
		}

		const LightDefinition *definition = Find(light.name);
		if (!definition)
		{
			if (warning)
				*warning = "Unknown light '" + light.name + "'";
			return false;
		}

		for (const LightRecord &record : definition->records)
		{
			const ArgumentReader args(record, *definition, light.params);
			switch (record.type)
			{
				case LightRecordType::BillboardHW:
				case LightRecordType::BillboardSW:
				{
					LightBillboardSpec billboard;
					ReadColor(args, billboard.color);

					bool candela = false;
					const float size = args.Value(4, 1.0f, &candela);
					billboard.intensity = candela ? std::max(size, 0.0f) : 0.0f;
					billboard.size = candela ? 1.0f : std::abs(size);
					billboard.axial = !candela && size < 0.0f;

					CellToUv(ToCell(args.Value(5, 1.0f)), ToCell(args.Value(6)), ToCell(args.Value(7)), m_DivisionsX,
						m_DivisionsY, billboard.uv);
					ReadDirection(args, 8, billboard.direction);
					billboard.width = args.Value(11, 1.0f);

					if (record.type == LightRecordType::BillboardHW)
					{
						billboard.frequency = args.Value(12);
						billboard.phase = args.Value(13);
						billboard.amplitude = args.Value(14);
						billboard.day = args.Value(15) > 0.5f;
					}
					else
					{
						billboard.processor = GetLightProcessor(args.Word(12));
					}
					out.billboards.push_back(billboard);
					break;
				}

				case LightRecordType::SpillHWDir:
				case LightRecordType::SpillHWFlash:
				case LightRecordType::SpillSW:
				{
					LightSpillSpec spill;
					ReadColor(args, spill.color);

					bool candela = false;
					const float size = args.Value(4, 1.0f, &candela);
					spill.intensity = candela ? std::max(size, 0.0f) : 0.0f;
					spill.size = candela ? 1.0f : std::abs(size);

					if (record.type == LightRecordType::SpillHWFlash)
					{
						spill.frequency = args.Value(5);
						spill.phase = args.Value(6);
						spill.amplitude = args.Value(7);
						spill.day = args.Value(8) > 0.5f;
					}
					else
					{
						ReadDirection(args, 5, spill.direction);
						spill.width = args.Value(8, 1.0f);
						if (record.type == LightRecordType::SpillHWDir)
							spill.day = args.Value(9) > 0.5f;
						else
							spill.processor = GetLightProcessor(args.Word(9));
					}
					out.spills.push_back(spill);
					break;
				}

				case LightRecordType::SpillGround:
				case LightRecordType::SpillGroundReverse:
					break; // v8 ground halos: X-Plane 12 lights the ground with spill instead
			}
		}

		if (out.billboards.empty() && out.spills.empty() && warning)
			*warning = "Light '" + light.name + "' has nothing the editor draws";
		return true;
	}

	// -------------------------------------------------------

	float EvaluateFlash(const float frequency, const float phase, const float amplitude, const double timeSeconds)
	{
		if (frequency <= 0.0f)
			return 1.0f;

		const float x = Fraction((timeSeconds + static_cast<double>(phase)) * static_cast<double>(frequency));
		if (amplitude > 1000.0f)
			return x < (amplitude - 1000.0f) / 1000.0f ? 1.0f : 0.0f; // square wave

		const float triangle = 1.0f - std::abs(2.0f * x - 1.0f);
		if (amplitude <= 0.0f)
			return triangle;

		const float value = triangle * amplitude;
		return std::clamp(amplitude > 1.0f ? value - (amplitude - 1.0f) : value + (1.0f - amplitude), 0.0f, 1.0f);
	}

	float EvaluateBillboardDirectionality(const LightBillboardSpec &spec, const float directionWorld[3], const float toCamera[3])
	{
		const float length = Length3(directionWorld);
		if (length < 1e-4f || spec.width >= 1.0f)
			return 1.0f; // omnidirectional

		const float d = directionWorld[0] * toCamera[0] + directionWorld[1] * toCamera[1] + directionWorld[2] * toCamera[2];
		if (spec.IsPhotometric() && length < 1.01f)
		{
			// Beam: full inside the cone's inner half, fading out at its edge (cos = width).
			if (spec.width <= -1.0f)
				return 1.0f;
			const float cosine = d / length;
			return SmoothStep(spec.width, spec.width + (1.0f - spec.width) * 0.5f, cosine);
		}

		return std::clamp(d + spec.width, 0.0f, 1.0f);
	}

	float GetBillboardRadius(const LightBillboardSpec &spec, const float distanceMeters)
	{
		const float sizeConstant = spec.IsPhotometric() ? 0.5f * std::pow(spec.intensity, 0.25f) : spec.size;
		return sizeConstant * BILLBOARD_SCALE * std::pow(std::max(distanceMeters, 1.0f), BILLBOARD_DECAY);
	}

	LightProcessor GetLightProcessor(const std::string_view dataref)
	{
		if (dataref.empty() || dataref == "NULL" || dataref == "none" || dataref == "no_ref")
			return LightProcessor::None;

		const size_t slash = dataref.rfind('/');
		const std::string_view leaf = slash == std::string_view::npos ? dataref : dataref.substr(slash + 1);
		if (leaf.find("rabbit") != std::string_view::npos)
			return LightProcessor::Rabbit;
		if (leaf.find("wigwag") != std::string_view::npos)
			return LightProcessor::WigWag;
		if (leaf.find("vasi") != std::string_view::npos || leaf.find("papi") != std::string_view::npos)
			return LightProcessor::Vasi;
		if (leaf.find("strobe") != std::string_view::npos)
			return LightProcessor::Strobe;
		return LightProcessor::None;
	}

	float EvaluateProcessor(const LightProcessor processor, const double timeSeconds, const float phaseSeed)
	{
		switch (processor)
		{
			case LightProcessor::Strobe:
				return Fraction(timeSeconds + phaseSeed) < 0.06f ? 1.0f : 0.0f;
			case LightProcessor::Rabbit:
				return Fraction(timeSeconds * 2.0 + phaseSeed) < 0.08f ? 1.0f : 0.0f;
			case LightProcessor::WigWag:
				return Fraction(timeSeconds + phaseSeed) < 0.5f ? 1.0f : 0.0f;
			case LightProcessor::Vasi:
			case LightProcessor::None:
				break;
		}
		return 1.0f;
	}

}

// -------------------------------------------------------
