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
	
	    uint32_t GetRefCount() const
	    {
	        return RefCount_.load();
	    }
	
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
	class SRef
	{
	public:
		SRef()
			: InstancePtr_(nullptr)
		{
		}
		
		SRef(std::nullptr_t n)
			: InstancePtr_(nullptr)
		{
		}

		SRef(T* instance)
			: InstancePtr_(instance)
		{
			static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");

			IncRef();
		}

		template<typename T2>
		requires(std::is_base_of_v<T2, T> || std::is_base_of_v<T, T2>)
		SRef(const SRef<T2>& other)
		{
			InstancePtr_ = (T*)other.InstancePtr_;
			IncRef();
		}

		template<typename T2>
		SRef(SRef<T2>&& other)
		{
			InstancePtr_ = (T*)other.InstancePtr_;
			other.InstancePtr_ = nullptr;
		}

		static SRef<T> CopyWithoutIncrement(const SRef<T>& other)
		{
			SRef<T> result = nullptr;
			result->InstancePtr_ = other.InstancePtr_;
			return result;
		}

		~SRef()
		{
			DecRef();
		}

		SRef(const SRef<T>& other)
			: InstancePtr_(other.InstancePtr_)
		{
			IncRef();
		}

		SRef& operator=(std::nullptr_t)
		{
			DecRef();
			InstancePtr_ = nullptr;
			return *this;
		}

		SRef& operator=(const SRef<T>& other)
		{
			if (this == &other)
				return *this;

			other.IncRef();
			DecRef();

			InstancePtr_ = other.InstancePtr_;
			return *this;
		}

		template<typename T2>
		SRef& operator=(const SRef<T2>& other)
		{
			other.IncRef();
			DecRef();

			InstancePtr_ = other.InstancePtr_;
			return *this;
		}

		template<typename T2>
		SRef& operator=(SRef<T2>&& other)
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
		SRef<T2> As() const
		{
			return SRef<T2>(*this);
		}

		template<typename... Args>
		static SRef<T> Create(Args&&... args)
		{
			return SRef<T>(new T(std::forward<Args>(args)...));
		}

		bool operator==(const SRef<T>& other) const
		{
			return InstancePtr_ == other.InstancePtr_;
		}

		bool operator!=(const SRef<T>& other) const
		{
			return !(*this == other);
		}

		bool EqualsObject(const SRef<T>& other)
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
		friend class SRef;
		mutable T* InstancePtr_;
	};

	// -------------------------------------------------------

	template<typename T>
	class WeakRef
	{
	public:
		WeakRef() = default;

		WeakRef(SRef<T> ref)
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
