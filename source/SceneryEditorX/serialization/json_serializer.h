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
 * json_serializer.h
 * -------------------------------------------------------
 * Created: 12/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include <nlohmann/json.hpp>
#include "SceneryEditorX/asset/asset.h"

// -------------------------------------------------------

namespace nlohmann
{

    /*
	/// (Legacy GLM vector type JSON conversions removed; using internal types.)
	template<>
	struct adl_serializer<iVec2>
	{
		static void to_json(json& j, const iVec2& v)
		{
			j = json::array({v.x, v.y});
		}

		static void from_json(const json& j, iVec2& v)
		{
			if (j.is_array() && j.size() == 2)
			{
				v.x = j[0].get<int>();
				v.y = j[1].get<int>();
			}
		}
	};

	// -------------------------------------------------------

	template<>
	struct adl_serializer<Bool3>
	{
		static void to_json(json& j, const Bool3& v)
		{
			j = json::array({ v.x, v.y, v.z });
		}

		static void from_json(const json& j, Bool3& v)
		{
			if (j.is_array() && j.size() == 3)
			{
				v.x = j[0].get<bool>();
				v.y = j[1].get<bool>();
				v.z = j[2].get<bool>();
			}
		}
	};

	template<>
	struct adl_serializer<iVec3>
	{
		static void to_json(json& j, const iVec3& v)
		{
			j = json::array({v.x, v.y, v.z});
		}

		static void from_json(const json& j, iVec3& v)
		{
			if (j.is_array() && j.size() == 3)
			{
				v.x = j[0].get<int>();
				v.y = j[1].get<int>();
				v.z = j[2].get<int>();
			}
		}
	};

    // -------------------------------------------------------

	template<>
	struct adl_serializer<iVec4>
	{
		static void to_json(json& j, const iVec4& v)
		{
			j = json::array({v.x, v.y, v.z, v.w});
		}

		static void from_json(const json& j, iVec4& v)
		{
			if (j.is_array() && j.size() == 4)
			{
				v.x = j[0].get<int>();
				v.y = j[1].get<int>();
				v.z = j[2].get<int>();
				v.w = j[3].get<int>();
			}
		}
	};

    // -------------------------------------------------------

	// (Removed legacy glm::bvec2/3/4 serializers – replaced by Bool2/Bool3/Bool4.)

	template<>
	struct adl_serializer<Vec2>
	{
		static void to_json(json& j, const Vec2& v)
		{
			j = json::array({v.x, v.y});
		}

		static void from_json(const json& j, Vec2& v)
		{
			if (j.is_array() && j.size() == 2)
			{
				v.x = j[0].get<float>();
				v.y = j[1].get<float>();
			}
		}
	};

    // -------------------------------------------------------

	template<>
	struct adl_serializer<Vec3>
	{
		static void to_json(json& j, const Vec3& v)
		{
			j = json::array({v.x, v.y, v.z});
		}

		static void from_json(const json& j, Vec3& v)
		{
			if (j.is_array() && j.size() == 3)
			{
				v.x = j[0].get<float>();
				v.y = j[1].get<float>();
				v.z = j[2].get<float>();
			}
		}
	};

    // -------------------------------------------------------

	template<>
	struct adl_serializer<Vec4>
	{
		static void to_json(json& j, const Vec4& v)
		{
			j = json::array({v.x, v.y, v.z, v.w});
		}

		static void from_json(const json& j, Vec4& v)
		{
			if (j.is_array() && j.size() == 4)
			{
				v.x = j[0].get<float>();
				v.y = j[1].get<float>();
				v.z = j[2].get<float>();
				v.w = j[3].get<float>();
			}
		}
	};

    // -------------------------------------------------------

	template<>
	struct adl_serializer<Quat>
	{
		static void to_json(json& j, const Quat& v)
		{
			j = json::array({v.w, v.x, v.y, v.z});
		}

		static void from_json(const json& j, Quat& v)
		{
			if (j.is_array() && j.size() == 4)
			{
				v.w = j[0].get<float>();
				v.x = j[1].get<float>();
				v.y = j[2].get<float>();
				v.z = j[3].get<float>();
			}
		}
	};

    // -------------------------------------------------------

	template<>
	struct adl_serializer<uint64_t>
	{
		static void to_json(json& j, const uint64_t& handle)
		{
			j = static_cast<uint64_t>(handle);
		}

		static void from_json(const json& j, uint64_t& handle)
		{
			if (j.is_number_unsigned())
			{
				handle = j.get<uint64_t>();
			}
		}
	};
	*/

    // -------------------------------------------------------

}

// -------------------------------------------------------
