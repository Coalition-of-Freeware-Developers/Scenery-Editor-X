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
    template<typename ElementType>
    using InitializerList = std::initializer_list<ElementType>;

    template<typename ElementType>
    class Array;

	template<typename ElementType>
	class ArrayView;

    template<typename ElementType>
    class List
    {
    public:
        List() : Impl() {}
		List(const ArrayView<ElementType>& view);
        List(size_t count) : Impl(count) {}
        List(size_t count, const ElementType& defaultValue) : Impl(count, defaultValue) {}
        List(std::initializer_list<ElementType> list) : Impl(list) {}
		List(std::vector<ElementType> list) : Impl(list) {}

        ElementType& operator[](size_t index) { return (ElementType&)Impl[index]; }

        const ElementType& operator[](size_t index) const { return Impl[index]; }
		const ElementType& At(size_t index) const { return Impl[index]; }

		ElementType& At(size_t index) { return Impl[index]; }
        ElementType& GetFirst() { return Impl[0]; }

        const ElementType& GetFirst() const { return Impl[0]; }

        ElementType& GetLast() { return Impl[GetSize() - 1]; }
        const ElementType& GetLast() const { return Impl[GetSize() - 1]; }
        bool IsEmpty() const { return Impl.empty(); }

        ElementType* GetData()
        {
			if (IsEmpty())
				return nullptr;
            return Impl.data();
        }

        const ElementType* GetData() const
        {
			if (IsEmpty())
				return nullptr;
            return Impl.data();
        }

        size_t GetSize() const
        {
            return Impl.size();
        }

        void Add(const ElementType& item)
        {
            Impl.push_back(item);
        }

		void InsertAt(int index, const ElementType& item)
		{
			if (Impl.empty())
			{
				Impl.push_back(item);
				return;
			}
			Impl.insert(Impl.begin() + index, item);
		}

        template<typename Pred>
        void InsertSorted(const ElementType& item, Pred pred)
        {
            InsertSortedUpperBound(item, pred);
        }

        template<typename Pred>
        void InsertSortedUpperBound(const ElementType& item, Pred pred)
        {
            Impl.insert(std::upper_bound(Impl.begin(), Impl.end(), item, pred), item);
        }

        template<typename Pred>
        void InsertSortedLowerBound(const ElementType& item, Pred pred)
        {
            Impl.insert(std::lower_bound(Impl.begin(), Impl.end(), item, pred), item);
        }

		void InsertRange(int index, std::initializer_list<ElementType> elements)
		{
			Impl.insert(Impl.begin() + index, elements);
		}

		void InsertRange(int index, const List<ElementType>& elements)
		{
			Impl.insert(Impl.begin() + index, elements.begin().ptr, elements.end().ptr);
		}

        int32_t IndexOf(const ElementType& item) const
        {
            for (int i = 0; i < Impl.size(); i++)
            {
                if (Impl[i] == item)
                    return i;

            }
            return -1;
        }

        int32_t IndexOf(std::function<bool(const ElementType&)> pred) const
        {
            for (int i = 0; i < Impl.size(); i++)
            {
                if (pred(Impl[i]))
                    return i;

            }
            return -1;
        }

        template<typename... Args>
        ElementType& EmplaceBack(Args&&... args)
        {
            return Impl.emplace_back(args...);
        }

        void AddRange(std::initializer_list<ElementType> elements)
        {
            for (auto it = elements.begin(); it != elements.end(); ++it)
            {
                Impl.push_back(*it);
            }
        }

        void AddRange(List<ElementType> elements)
        {
            for (auto it = elements.begin(); it != elements.end(); ++it)
            {
                Impl.push_back(*it);
            }
        }

        void Remove(const ElementType& item)
        {
            for (int i = 0; i < Impl.size(); i++)
            {
                if (Impl[i] == item)
                {
                    Impl.erase(Impl.begin() + i);
                    return;
                }
            }
        }

        void RemoveAll(std::function<bool(const ElementType& item)> pred)
        {
            List<uint32_t> indicesToRemove{};
            for (int i = Impl.size() - 1; i >= 0; i--)
            {
                if (pred(Impl[i]))
                    indicesToRemove.Add(i);

            }

            for (auto idx : indicesToRemove)
            {
                if (idx >= 0 && idx < Impl.size() - 1)
                    RemoveAt(idx);
            }
        }

        void RemoveFirst(std::function<bool(const ElementType& item)> pred)
        {
            List<uint32_t> indicesToRemove{};
            for (int i = 0; i < Impl.size(); i++)
            {
                if (pred(Impl[i]))
                {
                    RemoveAt(i);
                    break;
                }
            }
        }

		template<typename T>
		List<T> Transform(std::function<T(ElementType&)> selector)
		{
			List<T> result{};
            result.Reserve(GetSize());
			for (int i = 0; i < GetSize(); i++)
			{
				result.Add(selector(At(i)));
			}
			return result;
		}

		template<typename T>
		List<T> Transform(std::function<T(const ElementType&)> selector) const
		{
			List<T> result{};
            result.Reserve(GetSize());
			for (int i = 0; i < GetSize(); i++)
			{
				result.Add(selector(At(i)));
			}
			return result;
		}

		List<ElementType> Where(std::function<bool(const ElementType& item)> predicate) const
		{
			List<ElementType> result{};
            result.Reserve(GetSize());
			for (int i = 0; i < GetSize(); i++)
			{
				if (predicate(At(i)))
				{
					result.Add(At(i));
				}
			}
			return result;
		}

		List<ElementType> Where(std::function<bool(const ElementType& item, int index)> predicate) const
		{
			List<ElementType> result{};
            result.Reserve(GetSize());
			for (int i = 0; i < GetSize(); i++)
			{
				if (predicate(At(i), i))
				{
					result.Add(At(i));
				}
			}
			return result;
		}

        void RemoveAt(uint32_t index)
        {
            Impl.erase(Impl.begin() + index);
        }

        void Clear()
        {
            Impl.clear();
        }

        void Resize(uint32_t newSize)
        {
            Impl.resize(newSize);
        }

        void Reserve(uint32_t capacity)
        {
            Impl.reserve(capacity);
        }

        bool Exists(const ElementType& item) const
        {
            for (int i = 0; i < Impl.size(); i++)
            {
                if (Impl[i] == item)
                    return true;
            }
            return false;
        }

        bool Exists(std::function<bool(const ElementType&)> func) const
        {
            for (int i = 0; i < Impl.size(); i++)
            {
                if (func(Impl[i]))
                    return true;
            }
            return false;
        }

        /*
         *  Iterators
         */

        struct Iterator
        {
            friend class List;
            friend class Array<ElementType>;

            using iterator_category = std::contiguous_iterator_tag;
            using difference_type	= std::ptrdiff_t;
            using value_type		= ElementType;
            using pointer			= ElementType*;  // or also value_type*
            using reference			= ElementType&;  // or also value_type&

            Iterator(pointer ptr) : ptr(ptr) {}

            // De-reference ops
            reference operator*() const { return *ptr; }
            pointer operator->() { return ptr; }
			operator pointer() { return ptr; }

            // Increment ops
            Iterator& operator++() { ptr++; return *this; }
            Iterator operator++(int) { Iterator Temp = *this; ++(*this); return Temp; }
			Iterator operator+(difference_type rhs) const { return Iterator(ptr + rhs); }
            Iterator& operator+=(difference_type rhs) { ptr += rhs; return *this; }
			Iterator operator+(Iterator rhs) const { return ptr + rhs.ptr; }

            // Decrement ops
            Iterator& operator--() { ptr--; return *this; }
            Iterator operator--(int) { Iterator Temp = *this; --(*this); return Temp; }
			Iterator operator-(difference_type rhs) const { return Iterator(ptr - rhs); }
			difference_type operator-(Iterator rhs) const { return ptr - rhs.ptr; }

            friend bool operator== (const Iterator& A, const Iterator& B) { return A.ptr == B.ptr; };
            friend bool operator!= (const Iterator& A, const Iterator& B) { return A.ptr != B.ptr; };

        private:
            pointer ptr;
        };

        struct ConstIterator
        {
            friend class List;
            friend class Array<ElementType>;

            using iterator_category = std::contiguous_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = ElementType;
            using pointer = const ElementType*;  // or also value_type*
            using reference = const ElementType&;  // or also value_type&

            ConstIterator(pointer ptr) : ptr(ptr) {}

            // De-reference ops
            reference operator*() const { return *ptr; }
            pointer operator->() const { return ptr; }
			operator pointer() const { return ptr; }

            // Increment ops
            ConstIterator& operator++() { ptr++; return *this; }
            ConstIterator operator++(int) { ConstIterator temp = *this; ++(*this); return temp; }
			ConstIterator operator+(difference_type rhs) const { return ConstIterator(ptr + rhs); }
			//ConstIterator operator+(size_t rhs) { return ConstIterator(ptr + rhs); }
			difference_type operator+(const ConstIterator& rhs) const { return (ptr + rhs.ptr); }
            // Decrement ops
            ConstIterator& operator--() { ptr--; return *this; }
            ConstIterator operator--(int) { ConstIterator temp = *this; --(*this); return temp; }
            ConstIterator operator-(difference_type rhs) const { return ConstIterator(ptr - rhs); }
			ConstIterator operator-(size_t rhs) { return Iterator(ptr - rhs); }
			difference_type operator-(const ConstIterator& rhs) const { return (ptr - rhs.ptr); }

            friend bool operator== (const ConstIterator& A, const ConstIterator& B) { return A.ptr == B.ptr; };
            friend bool operator!= (const ConstIterator& A, const ConstIterator& B) { return A.ptr != B.ptr; };

        private:
            pointer ptr;
        };

        Iterator begin() { return Iterator{ Impl.data() }; }
        Iterator end() { return Iterator{ Impl.data() + Impl.size() }; }

        const ConstIterator begin() const { return ConstIterator{ Impl.data() }; }
        const ConstIterator end() const { return ConstIterator{ Impl.data() + Impl.size() }; }

        Iterator Begin() { return begin(); }
        Iterator End() { return end(); }

		template<class TPred>
		inline void Sort(TPred pred)
		{
			std::sort(Impl.begin(), Impl.end(), pred);
		}

    protected:
        std::vector<ElementType> Impl;

        friend class Variant;
    };

    template<typename ElementType = uint8_t>
    class Array : public List<ElementType>
    {
    public:
        using Type = ElementType;
        using Super = List<ElementType>;

        Array() : Super(), ElementTypeId(GetTypeId<ElementType>()) {}

		Array(const ArrayView<ElementType>& view);

        Array(size_t count) : Super(count), ElementTypeId(GetTypeId<ElementType>()) {}

        Array(size_t count, const ElementType& defaultValue) : Super(count, defaultValue), ElementTypeId(GetTypeId<ElementType>()) {}

        Array(std::initializer_list<ElementType> list) : Super(list), ElementTypeId(GetTypeId<ElementType>()) {}

		Array(std::vector<ElementType> list) : Super(list), ElementTypeId(GetTypeId<ElementType>()) {}

		template<size_t Size>
		Array(const ElementType inArray[Size]) : Super(Size), ElementTypeId(GetTypeId<ElementType>())
		{
			for (int i = 0; i < Size; i++)
			{
				Super::Impl[i] = inArray[i];
			}
		}

        Array(const Array& copy)
        {
            Super::Impl = copy.Super::Impl;
            ElementTypeId = copy.ElementTypeId;
        }

        Array& operator=(const Array& copy)
        {
            Super::Impl = copy.Super::Impl;
            ElementTypeId = copy.ElementTypeId;
            return *this;
        }

        Array(Array&& move) noexcept
        {
            Super::Impl = std::move(move.Super::Impl);
            ElementTypeId = move.ElementTypeId;
        }

        TypeId GetElementTypeId() const
        {
            return ElementTypeId;
        }

        ElementType& operator[](size_t index)
        {
            return (ElementType&)Super::Impl[index];
        }

        const ElementType& operator[](size_t index) const
        {
            return Super::Impl[index];
        }

        const ElementType& At(size_t index) const
        {
            return Super::Impl[index];
        }

        ElementType& At(size_t index)
        {
            return Super::Impl[index];
        }

        ElementType& GetFirst()
        {
            return Super::Impl[0];
        }

        const ElementType& GetFirst() const
        {
            return Super::Impl[0];
        }

        ElementType& GetLast()
        {
            return Super::Impl[GetSize() - 1];
        }

        const ElementType& GetLast() const
        {
            return Super::Impl[GetSize() - 1];
        }

		ElementType& Top()
		{
			return GetLast();
		}

        const ElementType& Top() const
        {
            return GetLast();
        }

        inline bool IsEmpty() const
        {
            return Super::Impl.empty();
        }

        inline bool NotEmpty() const
        {
            return !IsEmpty();
        }

        inline ElementType* GetData()
        {
            return Super::Impl.data();
        }

        inline const ElementType* GetData() const
        {
            return Super::Impl.data();
        }

        inline size_t GetSize() const
        {
            return Super::Impl.size();
        }

        inline void Add(const ElementType& item)
        {
			Super::Add(item);
        }

		inline void InsertAt(size_t index, const ElementType& item)
		{
			Super::InsertAt(index, item);
		}

        template<typename Pred>
        void InsertSorted(const ElementType& item, Pred pred)
        {
            Super::InsertSorted(item, pred);
        }

        void Push(const ElementType& item)
        {
            Super::Impl.push_back(item);
        }

        void Pop()
        {
            Super::Impl.pop_back();
        }

        int64_t IndexOf(const ElementType& item) const
        {
            for (int i = 0; i < Super::Impl.size(); i++)
            {
                if (Super::Impl[i] == item)
                    return i;
            }
            return -1;
        }

		int64_t IndexOf(std::function<bool(const ElementType&)> pred) const
        {
            for (int i = 0; i < Super::Impl.size(); i++)
            {
                if (pred(Super::Impl[i]))
                    return i;
            }
            return -1;
        }

		template<typename T>
		Array<T> Transform(std::function<T(ElementType&)> selector)
		{
			Array<T> result{};
            result.Reserve(GetSize());
			for (int i = 0; i < GetSize(); i++)
			{
				result.Add(selector(At(i)));
			}
			return result;
		}

		template<typename T>
		Array<T> Transform(std::function<T(const ElementType&)> selector) const
		{
			Array<T> result{};
            result.Reserve(GetSize());
			for (int i = 0; i < GetSize(); i++)
			{
				result.Add(selector(At(i)));
			}
			return result;
		}

		Array<ElementType> Where(std::function<bool(const ElementType& item)> predicate) const
		{
			Array<ElementType> result{};
            result.Reserve(GetSize());
			for (int i = 0; i < GetSize(); i++)
			{
				if (predicate(At(i)))
					result.Add(At(i));
			}
			return result;
		}

		Array<ElementType> Where(std::function<bool(const ElementType& item, int index)> predicate) const
		{
			Array<ElementType> result{};
            result.Reserve(GetSize());
			for (int i = 0; i < GetSize(); i++)
			{
				if (predicate(At(i), i))
				{
					result.Add(At(i));
				}
			}
			return result;
		}

        template<typename... Args>
        inline ElementType& EmplaceBack(Args&&... args)
        {
            return Super::Impl.emplace_back(args...);
        }

        inline void AddRange(std::initializer_list<ElementType> elements)
        {
            for (auto it = elements.begin(); it != elements.end(); ++it)
            {
                Super::Impl.push_back(*it);
            }
        }

        inline void AddRange(const Array<ElementType>& elements)
        {
            for (auto it = elements.begin(); it != elements.end(); ++it)
            {
                Super::Impl.push_back(*it);
            }
        }

		inline void InsertRange(int index, std::initializer_list<ElementType> elements)
		{
			Super::InsertRange(index, elements);
		}

		inline void InsertRange(int index, const Array<ElementType>& elements)
		{
			Super::InsertRange(index, (const List<ElementType>&)elements);
		}

        bool Remove(const ElementType& item)
        {
            for (int i = 0; i < GetSize(); i++)
            {
                if (Super::Impl[i] == item)
                {
                    Super::Impl.erase(Super::Impl.begin() + i);
                    return true;
                }
            }

			return false;
        }

        void RemoveAll(std::function<bool(const ElementType& item)> pred)
        {
            Array<uint32_t> indicesToRemove{};
            for (int i = GetSize() - 1; i >= 0; i--)
            {
                if (pred(Super::Impl[i]))
                {
                    indicesToRemove.Add(i);
                }
            }

            for (int i = 0; i < indicesToRemove.GetSize(); i++)
            {
                uint32_t idx = indicesToRemove[i];
                if (idx >= 0 && idx < GetSize())
                    RemoveAt(idx);
            }
        }

        void RemoveFirst(std::function<bool(const ElementType& item)> pred)
        {
            Array<uint32_t> indicesToRemove{};
            for (int i = 0; i < Super::Impl.size(); i++)
            {
                if (pred(Super::Impl[i]))
                {
                    RemoveAt(i);
                    break;
                }
            }
        }

        inline void RemoveAt(size_t index)
        {
            Super::Impl.erase(Super::Impl.begin() + index);
        }

        inline void Clear()
        {
            List<ElementType>::Impl.clear();
        }

        inline void Resize(uint32_t newSize)
        {
            List<ElementType>::Impl.resize(newSize);
        }

		inline void Resize(uint32_t newSize, const ElementType& defaultValue)
		{
			List<ElementType>::Impl.resize(newSize, defaultValue);
		}

        void Reserve(uint32_t capacity)
        {
            List<ElementType>::Reserve(capacity);
        }

        bool Exists(const ElementType& item) const
        {
            for (int i = 0; i < Super::Impl.size(); i++)
            {
                if (List<ElementType>::Impl[i] == item)
                    return true;
            }
            return false;
        }

        bool Exists(std::function<bool(const ElementType&)> func) const
        {
            for (int i = 0; i < Super::Impl.size(); i++)
            {
                if (func(Super::Impl[i]))
                    return true;
            }
            return false;
        }

        using Iterator = typename Super::Iterator;
        using ConstIterator = typename Super::ConstIterator;

        auto begin() { return Iterator{ List<ElementType>::Impl.data() }; }
        auto end() { return Iterator{ List<ElementType>::Impl.data() + List<ElementType>::Impl.size() }; }

        const ConstIterator begin() const { return ConstIterator{ List<ElementType>::Impl.data() }; }
        const ConstIterator end() const { return ConstIterator{ List<ElementType>::Impl.data() + List<ElementType>::Impl.size() }; }

		Iterator Begin() { return begin(); }
		Iterator End() { return end(); }

    private:
        TypeId ElementTypeId;

        friend class Variant;
    };

}
 */
/// -------------------------------------------------------
