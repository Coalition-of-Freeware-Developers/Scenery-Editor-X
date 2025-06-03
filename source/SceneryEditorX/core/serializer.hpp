/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* serializer.hpp
* -------------------------------------------------------
* Created: 9/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <nlohmann/json.hpp>
#include <SceneryEditorX/scene/asset_manager.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/*using Json = nlohmann::json;

	/// -------------------------------------------------------

	inline void to_json(Json& j, const Vec3& v)
	{
	    j = Json{v.x, v.y, v.z};
	}
	
	inline void to_json(Json& j, const Vec4& v)
	{
	    j = Json{v.x, v.y, v.z, v.w};
	}
	
	inline void from_json(const Json& j, Vec4& v)
	{
	    if (j.is_array() && j.size() == 4)
		{
	        v.x = j[0];
	        v.y = j[1];
	        v.z = j[2];
	        v.w = j[3];
	    }
	}
	
	inline void from_json(const Json& j, Vec3& v)
	{
	    if (j.is_array() && j.size() == 3)
		{
	        v.x = j[0];
	        v.y = j[1];
	        v.z = j[2];
	    }
	}

    /// -------------------------------------------------------

	struct BinaryStorage
	{
	    std::vector<uint8_t> data;
	
	    uint32_t Push(void *ptr, uint32_t size)
		{
	        uint32_t offset = data.size();
	        data.resize(data.size() + size);
	        memcpy(data.data() + offset, ptr, size);
	        return offset;
	    }
	
	    void* Get(uint32_t offset)
		{
	        return data.data() + offset;
	    }
	};

    /// -------------------------------------------------------

	struct Serializer
	{
	    Json& j;
	    BinaryStorage& storage;
	    AssetManager& manager;
	    int directory = 0;

		/// -------------------------------------------------------

	    std::filesystem::path filename;
	    LOCAL constexpr int LOAD = 0;
	    LOCAL constexpr int SAVE = 1;

		/// -------------------------------------------------------

	    Serializer(Json& j, BinaryStorage& storage, int directory, AssetManager& manager) : j(j) , storage(storage) , manager(manager) , directory(directory) {}

	    /// -------------------------------------------------------

	    template<typename T>
	    void Serialize(Ref<T>& object)
		{
	        if (directory == LOAD)
			{
                if (!j.contains("type") || !j.contains("name") || !j.contains("uuid"))
                {
                    SEDX_CORE_ERROR_TAG("Serializer", "JSON object doesn't contain required fields (type, name, uuid)");
                }
                ObjectType type = j["type"];
                std::string name = j["name"];
                uint32_t uuid = j["uuid"];
                auto createdObject = manager.CreateObject(type, name, uuid);
                object = std::dynamic_pointer_cast<T>(createdObject);
                if (!object)
                {
                    SEDX_CORE_ERROR_TAG("Serializer", "Failed to create object of requested type");
                }
                Serializer ser(j, storage, directory, manager);
                object->Serialize(ser);
            }
			else
			{
	            Serializer ser(j, storage, directory, manager);
	            j["type"] = object->type;
	            j["name"] = object->name;
	            j["uuid"] = object->uuid;
	            object->Serialize(ser);
	        }
	    }

	    /// -------------------------------------------------------
	
	    template<typename T>
	    void operator()(const std::string& field, T& value)
		{
	        if (directory == SAVE)
			{
	            to_json(j[field], value);
	        }
			else if (j.contains(field))
			{
	            from_json(j[field], value);
	        }
	    }

	    /// -------------------------------------------------------

	    template<typename T>
	    void Vector(const std::string& field, std::vector<T>& v)
		{
	        if (directory == SAVE) {
	            uint32_t size = v.size() * sizeof(T);
	            uint32_t offset = storage.Push(v.data(), size);
	            j[field] = Json::object();
	            j[field]["offset"] = offset;
	            j[field]["size"] = size;
                j[field]["count"] = v.size();
	        }
			else if (j.contains(field))
			{
	            uint32_t size = j[field]["size"];
	            uint32_t offset = j[field]["offset"];
                uint32_t count = j[field]["count"];
                v.resize(count);
                memcpy(v.data(), storage.Get(offset), size);
	        }
	    }

	    /// -------------------------------------------------------

	    template<typename T>
	    void VectorRef(const std::string& field, std::vector<T>& v)
		{
	         if (directory == SAVE)
			 {
	             Json childrenArray = Json::array();
	             for (auto& x : v)
				 {
	                 Serializer childSerializer(childrenArray.emplace_back(), storage, directory, manager);
	                 childSerializer.Serialize(x);
	             }
	             j[field] = childrenArray;

	         }
             else if (j.contains(field))
             {
                 v.reserve(j[field].size());
                 for (auto &value : j[field])
                 {
                     ObjectType type = value["type"];
                     std::string name = value["name"];
                     uint32_t uuid = value["uuid"];
                     Serializer childSerializer(value, storage, directory, manager);
                     auto &child = v.emplace_back();
                     childSerializer.Serialize(child);
                 }
             }
	    }

	    /// -------------------------------------------------------

	    template <typename T>
	    void Asset(const std::string& field, Ref<T>& object)
		{
	        if (directory == SAVE)
			{
	            if (object)
				{
	                j[field] = object->uuid;
	            }
				else
				{
	                j[field] = 0;
	            }
	        }
			else if (j.contains(field) && j[field] != 0)
			{
	            object = manager.Get<T>(j[field]);
	        }
	    }

		/// -------------------------------------------------------

	    template <typename T>
	    void Node(const std::string& field, Ref<T>& node, SceneAsset* scene)
		{
	        if (directory == SAVE)
			{
	            if (node)
				{
	                j[field] = node->uuid;
	            }
				else
				{
	                j[field] = 0;
	            }
	        }
			else if (j.contains(field) && j[field] != 0)
			{
	            node = scene->Get<T>(j[field]);
	        }
	    }

		/// -------------------------------------------------------
	};
	*/

} // namespace SceneryEditorX

/// -------------------------------------------------------
