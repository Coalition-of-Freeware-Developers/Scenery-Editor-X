/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ref.h
* -------------------------------------------------------
* Created: 10/4/2025
* -------------------------------------------------------
*/
#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief RefCounted is a base class for reference counted objects.
	 * It provides a simple interface for incrementing and decrementing the reference count.
	 */
	class RefCounted
	{
	public:
		virtual ~RefCounted() = default;

		void IncRefCount() const
		{
			++RefCount;
		}
		void DecRefCount() const
		{
			--RefCount;
		}

		uint32_t GetRefCount() const { return RefCount.load(); }
	private:
		mutable std::atomic<uint32_t> RefCount = 0;
	};

	// -------------------------------------------------------

	namespace RefUtils
	{
		void AddToLiveReferences(void* instance);
		void RemoveFromLiveReferences(void* instance);
		bool IsLive(void* instance);
	}

	// -------------------------------------------------------

	/**
	 * @brief Ref is a smart pointer that manages the lifetime of a RefCounted object.
	 * It increments the reference count when a new Ref is created and decrements it when the Ref is destroyed.
	 * @tparam T
	 */
	template<typename T>
	class Ref
	{
	public:
		Ref() : instancePtr(nullptr) {}
		
		Ref(std::nullptr_t n) : instancePtr(nullptr) {}

		Ref(T* instance) : instancePtr(instance)
		{
			static_assert(std::is_base_of_v<RefCounted, T>, "Class is not RefCounted!");

			IncRef();
		}

		template<typename T2>
		requires(std::is_base_of_v<T2, T> || std::is_base_of_v<T, T2>)
		Ref(const Ref<T2>& other)
		{
			instancePtr = static_cast<T *>(other.instancePtr);
			IncRef();
		}

		template<typename T2>
		Ref(Ref<T2>&& other)
		{
			instancePtr = (T*)other.instancePtr;
			other.instancePtr = nullptr;
		}

		static Ref<T> CopyWithoutIncrement(const Ref<T>& other)
		{
			Ref<T> result = nullptr;
			result->instancePtr = other.instancePtr;
			return result;
		}

		~Ref()
		{
			DecRef();
		}

		Ref(const Ref<T>& other) : instancePtr(other.instancePtr)
		{
			IncRef();
		}

		Ref& operator=(std::nullptr_t)
		{
			DecRef();
			instancePtr = nullptr;
			return *this;
		}

		Ref& operator=(const Ref<T>& other)
		{
			if (this == &other)
				return *this;

			other.IncRef();
			DecRef();

			instancePtr = other.instancePtr;
			return *this;
		}

		template<typename T2>
		Ref& operator=(const Ref<T2>& other)
		{
			other.IncRef();
			DecRef();

			instancePtr = other.instancePtr;
			return *this;
		}

		template<typename T2>
		Ref& operator=(Ref<T2>&& other)
		{
			DecRef();

			instancePtr = other.instancePtr;
			other.instancePtr = nullptr;
			return *this;
		}

		operator bool() { return instancePtr != nullptr; }
		operator bool() const { return instancePtr != nullptr; }

		T* operator->() { return instancePtr; }
		const T* operator->() const { return instancePtr; }

		T& operator*() { return *instancePtr; }
		const T& operator*() const { return *instancePtr; }

		T* Raw() { return  instancePtr; }
		const T* Raw() const { return  instancePtr; }

		void Reset(T* instance = nullptr)
		{
			DecRef();
			instancePtr = instance;
		}

		template<typename T2>
		requires(std::is_base_of_v<T2, T> || std::is_base_of_v<T, T2>)
		Ref<T2> As() const
		{
			return Ref<T2>(*this);
		}

        template <typename... Args>
        static Ref<T> Create(Args &&...args)
        {
        #ifdef SEDX_PLATFORM_WINDOWS
            return Ref<T>(new T(std::forward<Args>(args)...));
        #else
            return Ref<T>(new T(std::forward<Args>(args)...));
        #endif
        }

		bool operator==(const Ref<T>& other) const
		{
			return instancePtr == other.instancePtr;
		}

		bool operator!=(const Ref<T>& other) const
		{
			return !(*this == other);
		}

		bool EqualsObject(const Ref<T>& other)
		{
			if (!instancePtr || !other.instancePtr)
				return false;
			
			return *instancePtr == *other.instancePtr;
		}
	private:
		void IncRef() const
		{
			if (instancePtr)
			{
				instancePtr->IncRefCount();
				RefUtils::AddToLiveReferences((void*)instancePtr);
			}
		}

		void DecRef() const
		{
			if (instancePtr)
			{
				instancePtr->DecRefCount();
				
				if (instancePtr->GetRefCount() == 0)
				{
					delete instancePtr;
					RefUtils::RemoveFromLiveReferences((void*)instancePtr);
					instancePtr = nullptr;
				}
			}
		}

		template<class T2>
		friend class Ref;
		mutable T* instancePtr;
	};

	// -------------------------------------------------------

	/**
	 * @brief WeakRef is a weak reference to a RefCounted object. It does not increment the reference count of the object,
	 * @tparam T
	 */
	template<typename T>
	class WeakRef
	{
	public:
		WeakRef() = default;

		WeakRef(Ref<T> ref)
		{
			instancePtr = ref.Raw();
		}

		WeakRef(T* instance)
		{
			instancePtr = instance;
		}

		T* operator->() { return instancePtr; }
		const T* operator->() const { return instancePtr; }

		T& operator*() { return *instancePtr; }
		const T& operator*() const { return *instancePtr; }

		bool IsValid() const { return instancePtr ? RefUtils::IsLive(instancePtr) : false; }
		operator bool() const { return IsValid(); }

		template<typename T2>
		WeakRef<T2> As() const
		{
			return WeakRef<T2>(dynamic_cast<T2*>(instancePtr));
		}
	private:
		T* instancePtr = nullptr;
	};

	// -------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------
