/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene_ref.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/core/memory.h>
#include <atomic>
#include <cstddef>
#include <type_traits>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class RefCounted
	{
	public:
	    virtual ~RefCounted() = default;
	
	    void IncRefCount() const
	    {
	        ++RefCount_;
	    }
	    void DecRefCount() const
	    {
	        --RefCount_;
	    }
	
	    uint32_t GetRefCount() const { return RefCount_.load(); }
	
	private:
	    mutable std::atomic<uint32_t> RefCount_ = 0;
	};

	// -------------------------------------------------------

	namespace RefUtils
    {
		void AddToLiveReferences(void *instance);
		void RemoveFromLiveReferences(void *instance);
		bool IsLive(void *instance);
    } // namespace RefUtils

	template<typename T>
	class Ref
	{
	public:
		Ref()
			: InstancePtr_(nullptr)
		{
		}
		
		Ref(std::nullptr_t n)
			: InstancePtr_(nullptr)
		{
		}

		Ref(T* instance)
			: InstancePtr_(instance)
		{
			static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");

			IncRef();
		}

		template<typename T2>
		requires(std::is_base_of_v<T2, T> || std::is_base_of_v<T, T2>)
		Ref(const Ref<T2>& other)
		{
			InstancePtr_ = (T*)other.InstancePtr_;
			IncRef();
		}

		template<typename T2>
		Ref(Ref<T2>&& other)
		{
			InstancePtr_ = (T*)other.InstancePtr_;
			other.InstancePtr_ = nullptr;
		}

		static Ref<T> CopyWithoutIncrement(const Ref<T>& other)
		{
			Ref<T> result = nullptr;
			result->InstancePtr_ = other.InstancePtr_;
			return result;
		}

		~Ref()
		{
			DecRef();
		}

		Ref(const Ref<T>& other)
			: InstancePtr_(other.InstancePtr_)
		{
			IncRef();
		}

		Ref& operator=(std::nullptr_t)
		{
			DecRef();
			InstancePtr_ = nullptr;
			return *this;
		}

		Ref& operator=(const Ref<T>& other)
		{
			if (this == &other)
				return *this;

			other.IncRef();
			DecRef();

			InstancePtr_ = other.InstancePtr_;
			return *this;
		}

		template<typename T2>
		Ref& operator=(const Ref<T2>& other)
		{
			other.IncRef();
			DecRef();

			InstancePtr_ = other.InstancePtr_;
			return *this;
		}

		template<typename T2>
		Ref& operator=(Ref<T2>&& other)
		{
			DecRef();

			InstancePtr_ = other.InstancePtr_;
			other.InstancePtr_ = nullptr;
			return *this;
		}

		operator bool() { return InstancePtr_ != nullptr; }
		operator bool() const { return InstancePtr_ != nullptr; }

		T* operator->() { return InstancePtr_; }
		const T* operator->() const { return InstancePtr_; }

		T& operator*() { return *InstancePtr_; }
		const T& operator*() const { return *InstancePtr_; }

		T* Raw() { return  InstancePtr_; }
		const T* Raw() const { return  InstancePtr_; }

		void Reset(T* instance = nullptr)
		{
			DecRef();
			InstancePtr_ = instance;
		}

		template<typename T2>
		requires(std::is_base_of_v<T2, T> || std::is_base_of_v<T, T2>)
		Ref<T2> As() const
		{
			return Ref<T2>(*this);
		}

		template<typename... Args>
		static Ref<T> Create(Args&&... args)
		{
			return Ref<T>(new T(std::forward<Args>(args)...));
		}

		bool operator==(const Ref<T>& other) const
		{
			return InstancePtr_ == other.InstancePtr_;
		}

		bool operator!=(const Ref<T>& other) const
		{
			return !(*this == other);
		}

		bool EqualsObject(const Ref<T>& other)
		{
			if (!InstancePtr_ || !other.InstancePtr_)
				return false;
			
			return *InstancePtr_ == *other.InstancePtr_;
		}
	private:
		void IncRef() const
		{
			if (InstancePtr_)
			{
				InstancePtr_->IncRefCount();
				RefUtils::AddToLiveReferences((void*)InstancePtr_);
			}
		}

		void DecRef() const
		{
			if (InstancePtr_)
			{
				InstancePtr_->DecRefCount();
				
				if (InstancePtr_->GetRefCount() == 0)
				{
					delete InstancePtr_;
					RefUtils::RemoveFromLiveReferences((void*)InstancePtr_);
					InstancePtr_ = nullptr;
				}
			}
		}

		template<class T2>
		friend class Ref;
		mutable T* InstancePtr_;
	};

	// -------------------------------------------------------

	template<typename T>
	class WeakRef
	{
	public:
		WeakRef() = default;

		WeakRef(Ref<T> ref)
		{
			InstancePtr_ = ref.Raw();
		}

		WeakRef(T* instance)
		{
			InstancePtr_ = instance;
		}

		T* operator->() { return InstancePtr_; }
		const T* operator->() const { return InstancePtr_; }

		T& operator*() { return *InstancePtr_; }
		const T& operator*() const { return *InstancePtr_; }

		bool IsValid() const { return InstancePtr_ ? RefUtils::IsLive(InstancePtr_) : false; }
		operator bool() const { return IsValid(); }

		template<typename T2>
		WeakRef<T2> As() const
		{
			return WeakRef<T2>(dynamic_cast<T2*>(InstancePtr_));
		}
	private:
		T* InstancePtr_ = nullptr;
	};


} // namespace SceneryEditorX

// -------------------------------------------------------
