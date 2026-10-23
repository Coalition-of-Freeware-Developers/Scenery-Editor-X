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
 * xp_lights.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_obj8.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * X-Plane's built-in light definitions (Resources/bitmaps/world/lites/lights.txt), described in
	 * https://developer.x-plane.com/article/lights-txt-file-format/
	 *
	 * A named light is a group of records: camera-facing billboards cut from the lights texture and "spill" that
	 * lights the surroundings. OBJ lights (LIGHT_NAMED, LIGHT_PARAM, LIGHT_CUSTOM, LIGHT_SPILL_CUSTOM, LIGHTS) are
	 * resolved against the catalog into concrete billboard and spill specs; drawing them is the scene's job.
	 *
	 * Conventions (from the file): a light's front faces south (+Z); billboard cells are "size column row" in units
	 * of the cell size with (0, 0) in the lower-left corner; a size written as "<n>cd" is a photometric intensity.
	 *
	 * This header only depends on the standard library so the catalog can be unit tested without the engine.
	 */

	/* @brief Record types of lights.txt. */
	enum class LightRecordType : uint8_t
	{
		BillboardHW,		// R G B A SIZE CELL_SIZE X Y DX DY DZ WIDTH FREQ PHASE AMP DAY
		BillboardSW,		// R G B A SIZE CELL_SIZE X Y DX DY DZ WIDTH DREF
		SpillHWDir,			// R G B A SIZE DX DY DZ WIDTH DAY
		SpillHWFlash,		// R G B A SIZE FREQ PHASE AMP DAY
		SpillSW,			// R G B A SIZE DX DY DZ WIDTH DREF
		SpillGround,		// SIZE CELL_SIZE X Y (v8 ground quads)
		SpillGroundReverse
	};

	/* @brief One overload line, with its arguments kept as written (numbers, "<n>cd" or parameter names). */
	struct LightRecord
	{
		LightRecordType type = LightRecordType::BillboardHW;
		std::vector<std::string> args;
	};

	/* @brief Every record of one light name, plus its LIGHT_PARAM_DEF parameter names in OBJ argument order. */
	struct LightDefinition
	{
		std::string name;
		std::vector<std::string> params;
		std::vector<LightRecord> records;
	};

	/* @brief How an X-Plane "processing" dataref drives a software light the editor emulates. */
	enum class LightProcessor : uint8_t
	{
		None,		// Steady (no dataref, or one the editor can't emulate)
		Strobe,		// Short flash once a second
		Rabbit,		// Sequenced flashes, twice a second
		WigWag,		// Alternates once a second
		Vasi		// White above the glide slope, red below
	};

	/* @brief A billboard ready to place: everything except the camera-dependent terms. */
	struct LightBillboardSpec
	{
		float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };	// Tint; alpha is a master dimmer
		float size = 1.0f;								// Billboard size constant (legacy lights)
		float intensity = 0.0f;							// Candela when > 0 (X-Plane 12 photometric lights)
		float uv[4] = { 0.0f, 0.0f, 1.0f, 1.0f };		// s0 t0 s1 t1, lower-left origin
		float direction[3] = { 0.0f, 0.0f, 0.0f };		// Object space; zero = omnidirectional
		float width = 1.0f;								// See EvaluateBillboardDirectionality()
		float frequency = 0.0f;							// Flash (Hz), 0 = steady
		float phase = 0.0f;								// Seconds
		float amplitude = 0.0f;
		bool day = false;								// DAY flag: also on in daylight
		bool customTexture = false;						// LIGHT_CUSTOM: uv addresses the object's own texture
		bool axial = false;								// Negative size: hangs off the direction axis
		LightProcessor processor = LightProcessor::None;

		[[nodiscard]] bool IsPhotometric() const { return intensity > 0.0f; }
	};

	/* @brief Light cast onto the surroundings (a point or spot light). */
	struct LightSpillSpec
	{
		float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float size = 1.0f;								// Reach in meters (legacy lights)
		float intensity = 0.0f;							// Candela when > 0
		float direction[3] = { 0.0f, 0.0f, 0.0f };		// Object space; zero = omnidirectional
		float width = 1.0f;								// Cosine of the half cone angle; >= 1 = omnidirectional
		float frequency = 0.0f;
		float phase = 0.0f;
		float amplitude = 0.0f;
		bool day = false;
		LightProcessor processor = LightProcessor::None;

		[[nodiscard]] bool IsPhotometric() const { return intensity > 0.0f; }
		[[nodiscard]] bool IsSpot() const;

		/* @brief Distance in meters the spill reaches. */
		[[nodiscard]] float GetRange() const;
	};

	/* @brief What an OBJ light command turned into. */
	struct ResolvedLight
	{
		std::vector<LightBillboardSpec> billboards;
		std::vector<LightSpillSpec> spills;
	};

	/**
	 * @class LightCatalog
	 * @brief The parsed lights.txt: light definitions by name and the billboard texture grid.
	 */
	class LightCatalog
	{
	public:
		/**
		 * @brief Parses lights.txt text.
		 * @param text The whole file.
		 * @param error Receives a description when the header is wrong (optional).
		 * @return False when the text is not a lights.txt file.
		 */
		bool Parse(std::string_view text, std::string *error = nullptr);

		/* @brief Reads and parses a lights.txt file. */
		bool Load(const std::filesystem::path &path, std::string *error = nullptr);

		[[nodiscard]] bool IsLoaded() const { return !m_Lights.empty(); }
		[[nodiscard]] const std::filesystem::path &GetPath() const { return m_Path; }

		/* @brief The billboard texture as written (relative to the lights.txt folder). */
		[[nodiscard]] const std::string &GetTexture() const { return m_Texture; }
		[[nodiscard]] int GetDivisionsX() const { return m_DivisionsX; }
		[[nodiscard]] int GetDivisionsY() const { return m_DivisionsY; }
		[[nodiscard]] size_t GetLightCount() const { return m_Lights.size(); }

		/* @brief A light by name, or nullptr. */
		[[nodiscard]] const LightDefinition *Find(std::string_view name) const;

		/**
		 * @brief Turns an OBJ light into billboard and spill specs.
		 * Named and parameterized lights come from the catalog (LIGHT_PARAM arguments replace the definition's
		 * parameter names); custom and legacy lights are built from their own arguments.
		 * @param light The OBJ light.
		 * @param out Receives the specs; cleared first.
		 * @param warning Receives why the light resolved to nothing (optional).
		 * @return False when nothing could be resolved (unknown name).
		 */
		bool Resolve(const Obj8Light &light, ResolvedLight &out, std::string *warning = nullptr) const;

	private:
		std::filesystem::path m_Path;
		std::string m_Texture;
		int m_DivisionsX = 16;
		int m_DivisionsY = 8;
		std::unordered_map<std::string, LightDefinition> m_Lights;
	};

	// -------------------------------------------------------
	// Light evaluation (camera and clock dependent terms)
	// -------------------------------------------------------

	/**
	 * @brief Flash brightness (0..1) for lights.txt FREQ / PHASE / AMP.
	 * A triangle wave: amplitudes above 1 clip it into a strobe, below 1 keep a floor of (1 - amp), and amplitudes
	 * above 1000 switch to a square wave with a pulse width of (amp - 1000) / 1000.
	 */
	float EvaluateFlash(float frequency, float phase, float amplitude, double timeSeconds);

	/**
	 * @brief How much of a directional billboard is visible from a direction.
	 * Photometric (candela) lights with a unit direction treat width as the cosine of the beam's half angle;
	 * legacy lights use X-Plane's constant term model, saturate(dot(direction, toCamera) + width), where the direction
	 * is pre-scaled to sharpen the beam.
	 * @param spec The billboard.
	 * @param directionWorld Its direction in world space (same length as the spec's).
	 * @param toCamera Unit vector from the light to the camera.
	 */
	float EvaluateBillboardDirectionality(const LightBillboardSpec &spec, const float directionWorld[3], const float toCamera[3]);

	/**
	 * @brief World-space half size of a billboard seen from a distance. Billboards shrink slower than perspective
	 * (lights.txt's decay exponent) so distant lights stay visible; photometric lights grow with intensity.
	 */
	float GetBillboardRadius(const LightBillboardSpec &spec, float distanceMeters);

	/* @brief The emulated processor for a software light's dataref (sim/graphics/animation/lights/...). */
	LightProcessor GetLightProcessor(std::string_view dataref);

	/**
	 * @brief Brightness (0..1) of a processed light at a time; Vasi returns 1 (its color changes instead).
	 * @param phaseSeed Spreads sequenced lights (rabbit, wig-wag) apart, e.g. from the light's position.
	 */
	float EvaluateProcessor(LightProcessor processor, double timeSeconds, float phaseSeed);

}

// -------------------------------------------------------
