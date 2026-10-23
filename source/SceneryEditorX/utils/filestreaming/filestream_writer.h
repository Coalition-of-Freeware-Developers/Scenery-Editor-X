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
 * filestream_writer.h
 * -------------------------------------------------------
 * Created: 11/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/core/memory/buffer.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class StreamWriter
	 * @brief A base class for writing data to a stream.
	 * This class provides an interface for writing data to various types of streams.
	 * Derived classes must implement the pure virtual functions to handle specific stream types.
	 */
	class StreamWriter
	{
	public:
		virtual ~StreamWriter() = default;

		[[nodiscard]] virtual bool IsStreamGood() const = 0;
		virtual uint64_t GetStreamPosition() = 0;
		virtual void SetStreamPosition(uint64_t position) = 0;
		virtual bool WriteData(const char* data, size_t size) = 0;

		operator bool() const { return IsStreamGood(); }

		void WriteBuffer(Memory::Buffer buffer, bool writeSize = true);
		void WriteZero(uint64_t size);
		void WriteString(const std::string& string);

		template<typename T>
		void WriteRaw(const T& type)
		{
			bool success = WriteData((char*)&type, sizeof(T));
			SEDX_CORE_ASSERT(success);
		}

		template<typename T>
		void WriteObject(const T& obj)
		{
			T::Serialize(this, obj);
		}

		template<typename Key, typename Value>
		void WriteMap(const std::map<Key, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				WriteRaw<uint32_t>((uint32_t)map.size());

			for (const auto& [key, value] : map)
			{
				if constexpr (std::is_trivial<Key>())
					WriteRaw<Key>(key);
				else
					WriteObject<Key>(key);

				if constexpr (std::is_trivial<Value>())
					WriteRaw<Value>(value);
				else
					WriteObject<Value>(value);
			}
		}

		template<typename Key, typename Value>
		void WriteMap(const std::unordered_map<Key, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				WriteRaw<uint32_t>((uint32_t)map.size());

			for (const auto& [key, value] : map)
			{
				if constexpr (std::is_trivial<Key>())
					WriteRaw<Key>(key);
				else
					WriteObject<Key>(key);

				if constexpr (std::is_trivial<Value>())
					WriteRaw<Value>(value);
				else
					WriteObject<Value>(value);
			}
		}

		template<typename Value>
		void WriteMap(const std::unordered_map<std::string, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				WriteRaw<uint32_t>((uint32_t)map.size());

			for (const auto& [key, value] : map)
			{
				WriteString(key);

				if constexpr (std::is_trivial<Value>())
					WriteRaw<Value>(value);
				else
					WriteObject<Value>(value);
			}
		}

		template<typename T>
		void WriteArray(const std::vector<T>& array, bool writeSize = true)
		{
			if (writeSize)
				WriteRaw<uint32_t>((uint32_t)array.size());

			for (const auto& element : array)
			{
				if constexpr (std::is_trivial<T>())
					WriteRaw<T>(element);
				else
					WriteObject<T>(element);
			}
		}

	};

	template<>
	inline void StreamWriter::WriteArray(const std::vector<std::string>& array, bool writeSize)
	{
		if (writeSize)
			WriteRaw<uint32_t>((uint32_t)array.size());

		for (const auto& element : array)
			WriteString(element);
	}

}

// -------------------------------------------------------
