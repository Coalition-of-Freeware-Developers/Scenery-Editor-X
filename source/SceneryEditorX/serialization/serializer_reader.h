/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* serializer_reader.h
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vulkan/vk_buffers.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class SerializeReader
	{
	public:
		virtual ~SerializeReader() = default;

        [[nodiscard]] virtual bool IsStreamGood() const = 0;
		virtual uint64_t GetStreamPosition() = 0;
		virtual void SetStreamPosition(uint64_t position) = 0;
		virtual bool ReadData(char* destination, size_t size) = 0;

        explicit operator bool() const { return IsStreamGood(); }

		void ReadBuffer(Buffer& buffer, uint32_t size = 0);
		void ReadString(std::string& string);

		template<typename T>
		void ReadRaw(T& type)
		{
			const bool success = ReadData((char*)&type, sizeof(T));
			SEDX_CORE_ASSERT(success);
		}

		template<typename T>
		void ReadObject(T& obj)
		{
			T::Deserialize(this, obj);
		}

		template<typename Key, typename Value>
		void ReadMap(std::map<Key, Value>& map, uint32_t size = 0)
		{
			if (size == 0)
				ReadRaw<uint32_t>(size);

			for (uint32_t i = 0; i < size; i++)
			{
				Key key;
				if constexpr (std::is_trivial<Key>())
					ReadRaw<Key>(key);
				else
					ReadObject<Key>(key);

				if constexpr (std::is_trivial<Value>())
					ReadRaw<Value>(map[key]);
				else
					ReadObject<Value>(map[key]);
			}
		}

		template<typename Key, typename Value>
		void ReadMap(std::unordered_map<Key, Value>& map, uint32_t size = 0)
		{
			if (size == 0)
				ReadRaw<uint32_t>(size);

			for (uint32_t i = 0; i < size; i++)
			{
				Key key;
				if constexpr (std::is_trivial<Key>())
					ReadRaw<Key>(key);
				else
					ReadObject<Key>(key);

				if constexpr (std::is_trivial<Value>())
					ReadRaw<Value>(map[key]);
				else
					ReadObject<Value>(map[key]);
			}
		}

		template<typename Value>
		void ReadMap(std::unordered_map<std::string, Value>& map, uint32_t size = 0)
		{
			if (size == 0)
				ReadRaw<uint32_t>(size);

			for (uint32_t i = 0; i < size; i++)
			{
				std::string key;
				ReadString(key);

				if constexpr (std::is_trivial<Value>())
					ReadRaw<Value>(map[key]);
				else
					ReadObject<Value>(map[key]);
			}
		}

		template<typename T>
		void ReadArray(std::vector<T>& array, uint32_t size = 0)
		{
			if (size == 0)
				ReadRaw<uint32_t>(size);

			array.resize(size);

			for (uint32_t i = 0; i < size; i++)
			{
				if constexpr (std::is_trivial<T>())
					ReadRaw<T>(array[i]);
				else
					ReadObject<T>(array[i]);
			}
		}

	};

	template<>
	inline void SerializeReader::ReadArray(std::vector<std::string>& array, uint32_t size)
	{
		if (size == 0)
			ReadRaw<uint32_t>(size);

		array.resize(size);

		for (uint32_t i = 0; i < size; i++)
			ReadString(array[i]);
	}

}

/// -------------------------------------------------------
