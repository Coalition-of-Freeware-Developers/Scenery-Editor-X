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
 * xp_obj8.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * X-Plane OBJ8 model data, parsed from the text format described in
	 * https://developer.x-plane.com/article/obj8-file-format-specification/
	 *
	 * The parser keeps the file's own conventions: right-handed, +Y up, +X east, +Z south, meters, texture
	 * coordinates with the origin in the lower-left corner and clockwise front faces. Converting to the
	 * renderer's conventions is the GPU asset builder's job (xp_object_asset.cpp).
	 *
	 * This header only depends on the standard library so the parser can be unit tested without the engine.
	 */

	/* @brief A dataref reference as written in an OBJ ("sim/time/total_running_time_sec", "foo/bar[2]"). */
	struct DatarefRef
	{
		std::string name;	// Empty when the command had no dataref (or "none" / "no_ref" / "NULL")
		int index = -1;		// Array index from a "[n]" suffix, -1 when the dataref is a scalar

		[[nodiscard]] bool IsSet() const { return !name.empty(); }

		/* @brief Parses "name" or "name[n]"; "none", "no_ref" and "NULL" give an empty reference. */
		static DatarefRef Parse(std::string_view text);

		/* @brief The reference written back in OBJ form ("name" or "name[n]"). */
		[[nodiscard]] std::string ToString() const;

		bool operator==(const DatarefRef &) const = default;
	};

	/* @brief One keyframe of a translation: the offset reached at a dataref value. */
	struct Obj8TransKey
	{
		float value = 0.0f;
		float offset[3] = { 0.0f, 0.0f, 0.0f };
	};

	/* @brief One keyframe of a rotation: the angle (degrees, right-hand rule about the axis) at a dataref value. */
	struct Obj8RotKey
	{
		float value = 0.0f;
		float angleDeg = 0.0f;
	};

	/**
	 * @struct Obj8AnimOp
	 * @brief One transform inside an ANIM_begin / ANIM_end block (ANIM_trans*, ANIM_rotate*).
	 * Simple ANIM_trans / ANIM_rotate become a two-key table, so evaluation is the same for both forms.
	 */
	struct Obj8AnimOp
	{
		enum class Type : uint8_t
		{
			Translate,
			Rotate
		};

		Type type = Type::Translate;
		float axis[3] = { 0.0f, 1.0f, 0.0f };	// Rotation axis, normalized at parse time
		std::vector<Obj8TransKey> transKeys;	// Type::Translate
		std::vector<Obj8RotKey> rotKeys;		// Type::Rotate
		DatarefRef dataref;						// Unset: the op is static and evaluates at its first key
		float loop = 0.0f;						// ANIM_keyframe_loop period, 0 when the op does not loop
	};

	/**
	 * @struct Obj8AnimNode
	 * @brief An animation group (ANIM_begin ... ANIM_end). Nodes nest through parent.
	 */
	struct Obj8AnimNode
	{
		int parent = -1;					// Parent node index, -1 for a top-level group
		std::vector<Obj8AnimOp> ops;		// Applied in file order: the first op is the outermost transform
		bool implicit = false;				// Created for animation commands that followed geometry inside a group
	};

	/* @brief ANIM_hide / ANIM_show: hides or shows the geometry that follows while the dataref is in [v1, v2]. */
	struct Obj8VisibilityCmd
	{
		bool hide = true;
		float v1 = 0.0f;
		float v2 = 0.0f;
		DatarefRef dataref;
		float loop = 0.0f;	// ANIM_keyframe_loop period (flashing lights), 0 when the rule does not loop
	};

	/* @brief A level of detail (ATTR_LOD near far): visible from near (inclusive) to far (exclusive) meters. */
	struct Obj8Lod
	{
		float nearDistance = 0.0f;
		float farDistance = 0.0f;
	};

	/* @brief ATTR_layer_group / ATTR_layer_group_draped value. */
	struct Obj8LayerGroup
	{
		std::string name = "objects";
		int offset = 0;
	};

	/**
	 * @struct Obj8DrawState
	 * @brief The ATTR_* state a TRIS command was drawn with. Batches only merge when their states compare equal.
	 */
	struct Obj8DrawState
	{
		bool blend = true;					// ATTR_blend (default) vs ATTR_no_blend
		float alphaCutoff = 0.5f;			// ATTR_no_blend ratio
		bool shadowBlend = false;			// ATTR_shadow_blend
		bool cull = true;					// ATTR_cull (default) vs ATTR_no_cull
		bool depth = true;					// ATTR_depth / ATTR_no_depth
		bool draped = false;				// ATTR_draped / ATTR_no_draped
		bool castShadow = true;				// ATTR_shadow / ATTR_no_shadow
		bool flatShading = false;			// ATTR_shade_flat (deprecated)
		bool solidCamera = false;			// ATTR_solid_camera
		bool drawEnabled = true;			// ATTR_draw_enable / ATTR_draw_disable
		bool cockpit = false;				// Any ATTR_cockpit* region
		std::string hardSurface;			// ATTR_hard / ATTR_hard_deck surface ("" + hard == smooth)
		bool hard = false;
		bool hardDeck = false;
		int polygonOffset = 0;				// ATTR_poly_os
		float shinyRatio = 0.0f;			// ATTR_shiny_rat
		bool hasLightLevel = false;			// ATTR_light_level
		float lightLevelV1 = 0.0f;
		float lightLevelV2 = 1.0f;
		DatarefRef lightLevelDataref;
		float lightLevelBrightness = 0.0f;	// Optional photometric brightness (nts), 0 when not given

		bool operator==(const Obj8DrawState &) const = default;
	};

	/**
	 * @struct Obj8Batch
	 * @brief Consecutive TRIS commands that share an animation group, LOD, draw state and visibility rules.
	 */
	struct Obj8Batch
	{
		struct Range
		{
			uint32_t offset = 0;	// First entry in Obj8Data::indices
			uint32_t count = 0;		// Number of indices (a multiple of 3)
		};

		std::vector<Range> ranges;
		int node = -1;						// Animation group, -1 when the geometry is not animated
		int lod = -1;						// Index into Obj8Data::lods, -1 when the object has no ATTR_LOD
		Obj8DrawState state;
		std::vector<uint32_t> visibility;	// Obj8Data::visibility entries in effect, in evaluation order

		[[nodiscard]] uint32_t GetIndexCount() const;
	};

	/* @brief A light command (LIGHT_NAMED, LIGHT_PARAM, LIGHT_CUSTOM, LIGHT_SPILL_CUSTOM or a legacy LIGHTS vertex). */
	struct Obj8Light
	{
		enum class Kind : uint8_t
		{
			Named,
			Param,
			Custom,
			SpillCustom,
			Legacy
		};

		Kind kind = Kind::Named;
		std::string name;					// Named / Param light name
		float position[3] = { 0.0f, 0.0f, 0.0f };
		std::vector<float> params;			// Remaining numeric arguments, in file order
		std::vector<std::string> extraArgs;	// Non-numeric trailing LIGHT_PARAM arguments
		DatarefRef dataref;					// Custom / spill lights
		int node = -1;
		int lod = -1;
		std::vector<uint32_t> visibility;
	};

	/* @brief A VT entry: position, normal and texture coordinate. */
	struct Obj8Vertex
	{
		float position[3] = { 0.0f, 0.0f, 0.0f };
		float normal[3] = { 0.0f, 1.0f, 0.0f };
		float uv[2] = { 0.0f, 0.0f };
	};

	/* @brief A dataref the object reads, with the range its keyframes cover (for editor sliders). */
	struct Obj8DatarefUsage
	{
		DatarefRef dataref;
		float minValue = 0.0f;
		float maxValue = 0.0f;
		float loop = 0.0f;					// Largest ANIM_keyframe_loop seen for this dataref
		uint32_t useCount = 0;
	};

	/**
	 * @struct Obj8Data
	 * @brief Everything the editor needs from an OBJ8 file to draw and animate it.
	 */
	struct Obj8Data
	{
		std::filesystem::path filePath;		// Source file, when parsed from disk
		uint32_t version = 0;				// 800

		// Textures, as written in the file (relative to the OBJ's folder)
		std::string texture;
		std::string textureLit;
		std::string textureNormal;
		std::string textureDraped;
		std::string textureDrapedNormal;
		std::string textureMaterialGloss;	// TEXTURE_MAP material_gloss (X-Plane 12)
		std::string textureGloss;			// TEXTURE_MAP gloss (X-Plane 12)
		float normalRatio = 1.0f;			// TEXTURE_NORMAL ratio (repeats per albedo tile)
		bool normalMetalness = false;		// NORMAL_METALNESS: normal map blue = metalness
		bool drapedNormalMetalness = false;
		bool blendGlass = false;			// BLEND_GLASS

		// Global state
		bool hasGlobalNoBlend = false;
		float globalNoBlend = 0.5f;
		bool hasGlobalShadowBlend = false;
		float globalShadowBlend = 0.5f;
		bool hasGlobalSpecular = false;
		float globalSpecular = 0.0f;
		float globalTintAlbedo = 0.0f;
		float globalTintEmissive = 0.0f;
		float globalLuminance = 0.0f;
		bool globalNoShadow = false;
		bool globalCockpitLit = false;
		bool tilted = false;
		Obj8LayerGroup layerGroup;
		Obj8LayerGroup layerGroupDraped = { "objects", -5 };
		float lodDraped = 0.0f;
		std::string particleSystem;

		// Geometry
		std::vector<Obj8Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<Obj8Vertex> lineVertices;	// VLINE: position + rgb in normal[]
		std::vector<Obj8Vertex> lightVertices;	// VLIGHT: position + rgb in normal[]

		// Structure
		std::vector<Obj8Lod> lods;
		std::vector<Obj8AnimNode> nodes;
		std::vector<Obj8VisibilityCmd> visibility;
		std::vector<Obj8Batch> batches;
		std::vector<Obj8Light> lights;
		std::vector<Obj8DatarefUsage> datarefs;

		// Diagnostics
		uint32_t trisCommandCount = 0;
		uint32_t linesCommandCount = 0;
		uint32_t manipulatorCount = 0;
		uint32_t unknownCommandCount = 0;
		std::vector<std::string> warnings;

		/* @brief True when every LOD starts at 0 (X-Plane draws all LODs whose far distance is beyond the camera). */
		[[nodiscard]] bool HasAdditiveLods() const;

		/* @brief True when any group carries an animation op or a hide/show rule. */
		[[nodiscard]] bool IsAnimated() const { return !nodes.empty() || !visibility.empty(); }

		/* @brief Total triangle count over every batch. */
		[[nodiscard]] uint32_t GetTriangleCount() const;
	};

	/**
	 * @brief Parses OBJ8 text.
	 * @param text The whole file.
	 * @param out Receives the model; cleared first.
	 * @param error Receives a description when parsing fails (optional).
	 * @return False when the header is not an OBJ8 header or the file is structurally invalid.
	 */
	bool ParseObj8(std::string_view text, Obj8Data &out, std::string *error = nullptr);

	/**
	 * @brief Reads and parses an OBJ8 file.
	 * @param path File to read.
	 * @param out Receives the model; its filePath is set to path.
	 * @param error Receives a description when reading or parsing fails (optional).
	 * @return False on I/O or parse failure.
	 */
	bool LoadObj8(const std::filesystem::path &path, Obj8Data &out, std::string *error = nullptr);

	// -------------------------------------------------------
	// Animation evaluation
	// -------------------------------------------------------

	/**
	 * @brief Supplies dataref values while evaluating animation (name, array index or -1).
	 * Unknown datarefs should return 0, matching an unset dataref in X-Plane.
	 */
	using DatarefReader = std::function<float(const DatarefRef &)>;

	/**
	 * @struct RigidTransform
	 * @brief Rotation (unit quaternion x, y, z, w) followed by a translation: p' = q * p + t.
	 */
	struct RigidTransform
	{
		float rotation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float translation[3] = { 0.0f, 0.0f, 0.0f };

		/* @brief Composition: applies inner first, then this. */
		[[nodiscard]] RigidTransform Compose(const RigidTransform &inner) const;

		/* @brief Transforms a point. */
		void TransformPoint(const float in[3], float out[3]) const;
	};

	/**
	 * @brief Reduces a dataref value by an ANIM_keyframe_loop period (no-op when loop <= 0).
	 */
	float ApplyKeyframeLoop(float value, float loop);

	/**
	 * @brief Evaluates one animation op for a dataref value: linear interpolation between the bracketing
	 * keys and linear extrapolation from the nearest pair outside the table.
	 */
	RigidTransform EvaluateAnimOp(const Obj8AnimOp &op, float datarefValue);

	/**
	 * @brief Evaluates an animation group's local transform (its ops composed in file order).
	 */
	RigidTransform EvaluateAnimNode(const Obj8AnimNode &node, const DatarefReader &reader);

	/**
	 * @brief Evaluates hide / show rules in order. Geometry starts visible.
	 * @param data The object (its visibility table).
	 * @param rules Indices into data.visibility.
	 */
	bool EvaluateVisibility(const Obj8Data &data, const std::vector<uint32_t> &rules, const DatarefReader &reader);

}

// -------------------------------------------------------
