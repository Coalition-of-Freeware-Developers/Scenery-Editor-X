/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* dynamic_array.h
* -------------------------------------------------------
* Created: 22/7/2025
* -------------------------------------------------------
*/
#pragma once
/* #include <cstdint>
#include <vector>
#include <functional>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	template<typename T, size_t Size>
	class StaticArray
	{
	public:
		static constexpr size_t size = Size;
		StaticArray() {}
		StaticArray(std::initializer_list<T> list)
		{
			int i = 0;
			for (const auto& item : list)
			{
				if (i >= impl.size())
					break;
				impl[i++] = item;
			}
		}

		inline T& operator[](size_t index) { return impl[index]; }
		inline const T& operator[](size_t index) const { return impl[index]; }

		inline auto begin() { return impl.begin(); }
		inline auto end() { return impl.end(); }

		inline const auto begin() const { return impl.begin(); }
		inline const auto end() const { return impl.end(); }

		constexpr inline bool IsEmpty() const { return false; }
		constexpr inline const T* GetData() const { return impl.data(); }

		constexpr inline T* GetData() { return impl.data(); }
		constexpr inline size_t GetSize() const { return impl.size(); }
		constexpr inline size_t GetCapacity() const { return impl.size(); }

	private:
		std::array<T, Size> impl{};
	};

	template<typename T, size_t Size>
	class FixedArray
	{
	public:

		FixedArray() {}
		FixedArray(std::initializer_list<T> list)
		{
			for (const auto& item : list)
			{
				Add(item);
			}
		}

		inline T& operator[](size_t index) { return impl[index]; }
		inline const T& operator[](size_t index) const { return impl[index]; }

		inline auto begin() { return impl.begin(); }
		inline auto end() { return impl.begin() + count; }

		inline const auto begin() const { return impl.begin(); }
		inline const auto end() const { return impl.begin() + count; }

		inline bool IsEmpty() const { return count == 0; }
		inline void Clear() { count = 0; }

		inline void Resize(uint32_t newSize)
		{
			uint32_t oldCount = count;
			count = newSize < impl.size() ? newSize : impl.size();

			for (int i = oldCount; i < count; i++)
			{
				impl[i] = {};
			}
		}

		inline void Add(const T& item)
		{
			if (count >= GetCapacity())
				return;

			impl[count++] = item;
		}

		inline void Insert(const T& item, uint32_t index)
		{
			if (count >= GetCapacity())
				return;

			count++;

			for (int i = count - 1; i >= index + 1; i--)
			{
				impl[i] = impl[i - 1];
			}

			impl[index] = item;
		}

		void RemoveAt(uint32_t index)
		{
			if (index >= count)
				return;

			for (int i = index; i < count - 1; i++)
			{
				impl[i] = impl[i + 1];
			}

			count--;
		}

		void Remove(const T& item)
		{
			for (int i = 0; i < count; i++)
			{
				if (impl[i] == item)
				{
					RemoveAt(i);
					return;
				}
			}
		}

		constexpr inline const T& GetFirst() const { return *impl.begin(); }
		constexpr inline T& GetFirst() { return *impl.begin(); }

		constexpr inline const T& GetLast() const { return *(impl.begin() + count - 1); }
		constexpr inline T& GetLast() { return *(impl.begin() + count - 1); }

		constexpr inline T* GetData()
		{
			if (IsEmpty())
				return nullptr;
			return impl.data();
		}

		inline uint32_t GetSize() const { return count; }

		constexpr inline uint32_t GetCapacity() const { return impl.size(); }

	private:
		std::array<T, Size> impl{};
		uint32_t count = 0;
	};

}
 */
/// -------------------------------------------------------

