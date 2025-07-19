/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* json_serializer.h
* -------------------------------------------------------
* Created: 12/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <nlohmann/json.hpp>
#include <SceneryEditorX/asset/asset.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>

/// -------------------------------------------------------

namespace nlohmann
{
	/// GLM vector type JSON conversions
	template<>
	struct adl_serializer<glm::ivec2>
	{
		static void to_json(json& j, const glm::ivec2& v)
		{
			j = json::array({v.x, v.y});
		}

		static void from_json(const json& j, glm::ivec2& v)
		{
			if (j.is_array() && j.size() == 2)
			{
				v.x = j[0].get<int>();
				v.y = j[1].get<int>();
			}
		}
	};

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::ivec3>
	{
		static void to_json(json& j, const glm::ivec3& v)
		{
			j = json::array({v.x, v.y, v.z});
		}

		static void from_json(const json& j, glm::ivec3& v)
		{
			if (j.is_array() && j.size() == 3)
			{
				v.x = j[0].get<int>();
				v.y = j[1].get<int>();
				v.z = j[2].get<int>();
			}
		}
	};

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::ivec4>
	{
		static void to_json(json& j, const glm::ivec4& v)
		{
			j = json::array({v.x, v.y, v.z, v.w});
		}

		static void from_json(const json& j, glm::ivec4& v)
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

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::bvec2>
	{
		static void to_json(json& j, const glm::bvec2& v)
		{
			j = json::array({v.x, v.y});
		}

		static void from_json(const json& j, glm::bvec2& v)
		{
			if (j.is_array() && j.size() == 2)
			{
				v.x = j[0].get<bool>();
				v.y = j[1].get<bool>();
			}
		}
	};

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::bvec3>
	{
		static void to_json(json& j, const glm::bvec3& v)
		{
			j = json::array({v.x, v.y, v.z});
		}

		static void from_json(const json& j, glm::bvec3& v)
		{
			if (j.is_array() && j.size() == 3)
			{
				v.x = j[0].get<bool>();
				v.y = j[1].get<bool>();
				v.z = j[2].get<bool>();
			}
		}
	};

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::bvec4>
	{
		static void to_json(json& j, const glm::bvec4& v)
		{
			j = json::array({v.x, v.y, v.z, v.w});
		}

		static void from_json(const json& j, glm::bvec4& v)
		{
			if (j.is_array() && j.size() == 4)
			{
				v.x = j[0].get<bool>();
				v.y = j[1].get<bool>();
				v.z = j[2].get<bool>();
				v.w = j[3].get<bool>();
			}
		}
	};

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::vec2>
	{
		static void to_json(json& j, const glm::vec2& v)
		{
			j = json::array({v.x, v.y});
		}

		static void from_json(const json& j, glm::vec2& v)
		{
			if (j.is_array() && j.size() == 2)
			{
				v.x = j[0].get<float>();
				v.y = j[1].get<float>();
			}
		}
	};

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::vec3>
	{
		static void to_json(json& j, const glm::vec3& v)
		{
			j = json::array({v.x, v.y, v.z});
		}

		static void from_json(const json& j, glm::vec3& v)
		{
			if (j.is_array() && j.size() == 3)
			{
				v.x = j[0].get<float>();
				v.y = j[1].get<float>();
				v.z = j[2].get<float>();
			}
		}
	};

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::vec4>
	{
		static void to_json(json& j, const glm::vec4& v)
		{
			j = json::array({v.x, v.y, v.z, v.w});
		}

		static void from_json(const json& j, glm::vec4& v)
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

    /// -------------------------------------------------------

	template<>
	struct adl_serializer<glm::quat>
	{
		static void to_json(json& j, const glm::quat& v)
		{
			j = json::array({v.w, v.x, v.y, v.z});
		}

		static void from_json(const json& j, glm::quat& v)
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

    /// -------------------------------------------------------

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

    /// -------------------------------------------------------

}

/// -------------------------------------------------------
