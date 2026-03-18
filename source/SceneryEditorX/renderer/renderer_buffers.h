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
 * renderer_buffers.h
 * -------------------------------------------------------
 * Created: 16/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <colors.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// low frequency - updates once per frame
	struct ConstantBuffer_Frame
	{
		Matrix view;
		Matrix view_inv;
		Matrix view_previous;
		Matrix projection;
		Matrix projection_inv;
		Matrix projection_previous;
		Matrix view_projection;
		Matrix view_projection_inv;
		Matrix view_projection_ortho;
		Matrix view_projection_unjittered;
		Matrix view_projection_previous;
		Matrix view_projection_previous_unjittered;

		Vec2 resolution_render;
		Vec2 resolution_output;

		Vec2 taa_jitter_current;
		Vec2 taa_jitter_previous;

		float camera_aperture;
		float delta_time;
		uint32_t frame;
		uint32_t options;

		Vec3 camera_position;
		float camera_near;

		Vec3 camera_forward;
		float camera_far;

		float camera_last_movement_time;
		float hdr_enabled;
		float hdr_max_nits;
		float padding;

		Vec3 camera_position_previous;
		float resolution_scale;

		double time;
		float camera_fov;
		float padding2;

		Vec3 wind;
		float gamma;

		Vec3 camera_right;
		float camera_exposure;

		// clouds
		float cloud_coverage;
		float cloud_shadows;
		float padding3;
		float padding4;

		void SetBit(const bool set, const uint32_t bit)
		{
			options = set ? (options |= bit) : (options & ~bit);
		}
	};

	struct ShaderBuffer_Material
	{
		Vec4 color = Vec4::Zero;

		Vec2 tiling_uv = Vec2::Zero;
		Vec2 offset_uv = Vec2::Zero;
		Vec2 invert_uv = Vec2::Zero;
		float roughness_mul = 0.0f;
		float metallic_mul  = 0.0f;
		float normal_mul    = 0.0f;
		float height_mul    = 0.0f;

		uint32_t flags    = 0;
		float local_width = 0.0f;
		float padding;
		float subsurface_scattering;

		float sheen;
		float local_height   = 0.0f;
		float world_space_uv = 0.0f;
		float padding2;

		float anisotropic;
		float anisotropic_rotation;
		float clearcoat;
		float clearcoat_roughness;
	};

	struct ShaderBuffer_Light
	{
		Color color;
		Vec3 position;
		float intensity;
		Vec3 direction;
		float range;
		float angle;
		uint32_t flags;
		uint32_t screen_space_shadows_slice_index;
		float area_width;  // area light width in meters
		float area_height; // area light height in meters
		Matrix view_projection[6];
		Vec2 atlas_offsets[6];
		Vec2 atlas_scales[6];
		Vec2 atlas_texel_sizes[6];
	};

	struct ShaderBuffer_Aabb
	{
		Vec3 min;
		float is_occluder;
		Vec3 max;
		float padding2;
	};

	// per-blas-instance offsets into the global geometry buffer (indexed by InstanceIndex() in rt shaders)
	struct ShaderBuffer_GeometryInfo
	{
		uint32_t vertex_offset;
		uint32_t index_offset;
	};

	// gpu-driven indirect draw arguments (matches VkDrawIndexedIndirectCommand layout)
	struct ShaderBuffer_IndirectDrawArgs
	{
		uint32_t index_count    = 0;
		uint32_t instance_count = 0;
		uint32_t first_index    = 0;
		int32_t  vertex_offset  = 0;
		uint32_t first_instance = 0;
	};

	// per-draw data for gpu-driven rendering (indexed by draw_id in shaders)
	struct ShaderBuffer_DrawData
	{
		Matrix transform;          // current world transform
		Matrix transform_previous; // previous frame world transform
		uint32_t material_index = 0;     // index into the bindless material parameters array
		uint32_t is_transparent = 0;     // transparency flag
		uint32_t aabb_index     = 0;     // index into the aabb buffer for culling
		uint32_t padding        = 0;
	};

	// gpu particle (matches hlsl Particle struct, 64 bytes)
	struct ShaderBuffer_Particle
	{
		Vec3 position;
		float lifetime      = 0.0f; // remaining lifetime
		Vec3 velocity;
		float max_lifetime  = 0.0f; // initial lifetime
		Vec4 color;			// current rgba
		float size          = 0.0f; // current size
		float padding1      = 0.0f;
		float padding2      = 0.0f;
		float padding3      = 0.0f;
	};

	// gpu emitter parameters (matches hlsl EmitterParams struct)
	struct ShaderBuffer_EmitterParams
	{
		Vec3 position;
		float emission_rate   = 0.0f;
		float lifetime        = 0.0f;
		float start_speed     = 0.0f;
		float start_size      = 0.0f;
		float end_size        = 0.0f;
		Color start_color;
		Color end_color;
		float gravity_modifier = 0.0f;
		float radius           = 0.0f;
		float delta_time       = 0.0f;
		uint32_t max_particles = 0;
		uint32_t frame         = 0;
		uint32_t emitter_count = 0; // number of active emitters this frame
		float padding1         = 0.0f;
		float padding2         = 0.0f;
	};

}

// -------------------------------------------------------
