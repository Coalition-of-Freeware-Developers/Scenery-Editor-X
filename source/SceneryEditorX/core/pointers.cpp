/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* pointers.cpp
* -------------------------------------------------------
* Created: 31/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/pointers.h>
#include <unordered_map>
#include <mutex>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace Internal
	{
		/**
		 * @brief Control block for managing weak references to an object
		 * 
		 * The ControlBlock is responsible for tracking weak references to an object
		 * even after the object itself has been destroyed. It maintains:
		 * 1. A pointer to the actual object (which becomes nullptr when the object is destroyed)
		 * 2. A count of weak references pointing to this control block
		 * 
		 * When an object is destroyed but weak references to it still exist, the control
		 * block remains alive (with m_Ptr set to nullptr) until all weak references are gone.
		 * 
		 * @tparam T The type of object being referenced
		 */
		template <typename T>
		class ControlBlock
		{
		public:
		    /**
		     * @brief Constructs a control block for the specified object
		     * 
		     * @param ptr Pointer to the object being tracked
		     */
		    explicit ControlBlock(T *ptr) noexcept : m_Ptr(ptr), m_WeakCount(0) {}
		
		    /**
		     * @brief Increments the weak reference count
		     * 
		     * Called when a new WeakRef is created or copied to point to this object
		     */
		    void IncWeakCount() noexcept { ++m_WeakCount; }
		
		    /**
		     * @brief Decrements the weak reference count
		     * 
		     * When the weak count reaches zero and the object pointer is nullptr
		     * (indicating the object has been destroyed), the control block
		     * deletes itself as it's no longer needed.
		     */
		    void DecWeakCount() noexcept
		    {
		        if (--m_WeakCount == 0 && m_Ptr == nullptr)
		        {
		            delete this;
		        }
		    }
		
		    /**
		     * @brief Gets the pointer to the managed object
		     * 
		     * @return The pointer to the object, or nullptr if the object has been destroyed
		     */
		    T *GetPtr() const noexcept { return m_Ptr; }
		
		    /**
		     * @brief Sets the object pointer
		     * 
		     * This is typically called with nullptr when the object is being destroyed
		     * to indicate that the object is no longer valid.
		     * 
		     * @param ptr The new object pointer value
		     */
		    void SetPtr(T *ptr) noexcept
		    {
		        m_Ptr = ptr;
		    }
		
		    /**
		     * @brief Gets the current weak reference count
		     * 
		     * @return The number of weak references pointing to this control block
		     */
		    uint32_t GetWeakCount() const noexcept
		    {
		        return m_WeakCount;
		    }
		
		private:
		    T *m_Ptr;                          ///< Pointer to the managed object, or nullptr if destroyed
		    std::atomic<uint32_t> m_WeakCount; ///< Number of weak references to this object
		};
		

		template <typename T>
		class ControlBlockRegistry
		{
		public:
		    /**
		     * @brief Get the singleton instance of the registry
		     * 
		     * @return A reference to the singleton instance
		     */
		    static ControlBlockRegistry &GetInstance()
		    {
		        static ControlBlockRegistry instance;
		        return instance;
		    }

		    /**
		     * @brief Get or create a control block for the specified object pointer
		     * 
		     * If a control block already exists for the given pointer, it returns that block.
		     * Otherwise, it creates a new control block, registers it, and returns it.
		     * 
		     * @param ptr Pointer to the object for which to get/create a control block
		     * @return Pointer to the control block, or nullptr if ptr is nullptr
		     */
		    ControlBlock<T> *GetControlBlock(T *ptr)
		    {
		        if (!ptr)
		            return nullptr;
		
		        std::lock_guard<std::mutex> lock(m_Mutex);
		        auto it = m_Blocks.find(ptr);
		        if (it != m_Blocks.end())
		        {
		            return it->second;
		        }
		
		        auto block = new Internal::ControlBlock<T>(ptr);
		        m_Blocks[ptr] = block;
		        return block;
		    }
		
		    /**
		     * @brief Remove the control block associated with the specified object pointer
		     * 
		     * This method is called when an object is being destroyed. It sets the object pointer
		     * in the control block to nullptr to indicate that the object is no longer valid.
		     * If there are no weak references to the object, the control block itself is deleted.
		     * 
		     * @param ptr Pointer to the object whose control block should be removed
		     */
		    void RemoveControlBlock(T *ptr)
		    {
		        if (!ptr)
		            return;
		
		        std::lock_guard<std::mutex> lock(m_Mutex);
		        auto it = m_Blocks.find(ptr);
		        if (it != m_Blocks.end())
		        {
		            it->second->SetPtr(nullptr);
		            if (it->second->GetWeakCount() == 0)
		            {
		                delete it->second;
		            }
		            m_Blocks.erase(it);
		        }
		    }
		
		private:
		    /**
		     * @brief Private constructor to enforce singleton pattern
		     */
		    ControlBlockRegistry() = default;
		    ~ControlBlockRegistry()
		    {
		        for (auto &[ptr, block] : m_Blocks)
		        {
		            delete block;
		        }
		    }
		
		    std::unordered_map<T *, Internal::ControlBlock<T> *> m_Blocks;
		    std::mutex m_Mutex;
		};
	} // namespace Internal
	
    /**
	 * @brief Increments the reference count for the object.
	 * 
	 * This method is called when a new reference to the object is created.
	 * It safely increments the internal reference counter of the pointed object
	 * if the pointer is not null.
	 * 
	 * @tparam T The type of the reference-counted object
	 * @return void
	 */
	template <typename T>
	void Ref<T>::InternalAddRef() const noexcept
	{
	    if (m_Ptr)
	    {
	        m_Ptr->IncRefCount();
	    }
	}

    /**
	 * @brief Releases a reference to the object and potentially deletes it.
	 * 
	 * This method decrements the reference count of the pointed object.
	 * If the reference count reaches zero, it updates any weak references
	 * through the ControlBlockRegistry to indicate that the object is no longer valid,
	 * and then deletes the object.
	 * 
	 * The method ensures that:
	 * 1. Weak references can detect that the object has been destroyed
	 * 2. The object memory is properly freed when no more strong references exist
	 * 3. The internal pointer is set to nullptr after the release
	 * 
	 * @tparam T The type of the reference-counted object
	 * @return void
	 */
    template <typename T>
    void Ref<T>::InternalRelease() noexcept
    {
        if (m_Ptr)
        {
            if (m_Ptr->DecRefCount() == 0)
            {
                /// Update any weak references before deleting the object
                Internal::ControlBlockRegistry<T>::GetInstance().RemoveControlBlock(m_Ptr);
                delete m_Ptr;
            }
            m_Ptr = nullptr;
        }
    }
	
	/**
	 * @brief Constructs a weak reference from a strong reference.
	 * 
	 * This constructor creates a WeakRef that weakly references the same object
	 * as the provided strong reference (Ref<U>). The constructor supports proper
	 * type conversion through the template parameter U, which must be convertible to T.
	 * 
	 * The implementation:
	 * 1. Checks if the provided reference is valid
	 * 2. Retrieves or creates a control block for the referenced object
	 * 3. Increments the weak reference count in the control block
	 * 
	 * @tparam U The type of the source reference, must be convertible to T
	 * @param ref The strong reference to create a weak reference from
	 * @note This constructor enables implicit conversion from Ref<U> to WeakRef<T>
	 *       when U is convertible to T
	 */
    template <typename T>
    template <typename U, typename>
    WeakRef<T>::WeakRef(const Ref<U> &ref) noexcept
    {
        if (ref)
        {
            m_ControlBlock =
                Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(static_cast<T *>(ref.Get()));
            if (m_ControlBlock)
            {
                m_ControlBlock->IncWeakCount();
            }
        }
    }

    /**
     * @brief Copy constructor for weak references.
     * 
     * This constructor creates a new WeakRef that weakly references the same object
     * as the provided source WeakRef. If the source WeakRef is valid (points to a
     * control block), this constructor:
     * 1. Copies the control block pointer from the source
     * 2. Increments the weak reference count in that control block
     * 
     * @param other The source WeakRef to copy from
     * @note This maintains proper reference counting without affecting the 
     *       lifetime of the referenced object
     */
    template <typename T>
    WeakRef<T>::WeakRef(const WeakRef &other) noexcept : m_ControlBlock(other.m_ControlBlock)
    {
        if (m_ControlBlock)
        {
            m_ControlBlock->IncWeakCount();
        }
    }

	/**
	 * @brief Copy conversion constructor for WeakRef objects of different but compatible types.
	 * 
	 * This constructor allows creation of a WeakRef<T> from a WeakRef<U> where U is convertible to T
	 * (typically through inheritance relationships). It properly maintains the weak reference counting
	 * through the control block system.
	 * 
	 * The implementation:
	 * 1. Checks if the source WeakRef has a valid control block
	 * 2. Retrieves or creates a control block for T* through the registry using a static_cast
	 * 3. Increments the weak reference count if a valid control block is found
	 * 
	 * @tparam U Source type that is convertible to T
	 * @param other The source WeakRef<U> to convert from
	 * @note This constructor only participates in overload resolution if U* is convertible to T*
	 */
    template <typename T>
    template <typename U, typename>
    WeakRef<T>::WeakRef(const WeakRef<U> &other) noexcept
    {
        if (other.m_ControlBlock)
        {
            m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(
                static_cast<T *>(other.m_ControlBlock->GetPtr()));
            if (m_ControlBlock)
            {
                m_ControlBlock->IncWeakCount();
            }
        }
    }
	
	/**
	 * @brief Move constructor for weak references.
	 * 
	 * This constructor creates a new WeakRef by transferring ownership of the control block
	 * from the source WeakRef. After the move, the source WeakRef no longer references
	 * any object (its control block pointer is set to nullptr).
	 * 
	 * The implementation:
	 * 1. Takes ownership of the control block pointer from the source WeakRef
	 * 2. Sets the source WeakRef's control block pointer to nullptr to prevent
	 *    both instances from managing the same control block
	 * 
	 * Unlike the copy constructor, this constructor doesn't increment the weak reference count
	 * since ownership is being transferred rather than shared.
	 * 
	 * @param other The source WeakRef to move from
	 */
    template <typename T>
    WeakRef<T>::WeakRef(WeakRef &&other) noexcept : m_ControlBlock(other.m_ControlBlock)
    {
        other.m_ControlBlock = nullptr;
    }
	
	/**
	 * @brief Move conversion constructor for weak references of different but compatible types.
	 * 
	 * This constructor moves a WeakRef<U> to a WeakRef<T> where U is convertible to T
	 * (typically through inheritance relationships). Unlike the regular move constructor,
	 * this constructor performs a type conversion which requires finding or creating a
	 * control block for the target type.
	 * 
	 * The implementation:
	 * 1. Checks if the source WeakRef has a valid control block
	 * 2. If valid, retrieves or creates a control block for the target type through the registry
	 * 3. Sets the source WeakRef's control block to nullptr to transfer ownership
	 * 4. No increment of weak reference count is needed as ownership is transferred
	 * 
	 * @tparam U Source type that is convertible to T
	 * @param other The source WeakRef<U> to move from
	 * @note This constructor only participates in overload resolution if U* is convertible to T*
	 *       (enforced by the SFINAE template parameter)
	 */
    template <typename T>
    template <typename U, typename>
    WeakRef<T>::WeakRef(WeakRef<U> &&other) noexcept
    {
        if (other.m_ControlBlock)
        {
            m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(
                static_cast<T *>(other.m_ControlBlock->GetPtr()));
            other.m_ControlBlock = nullptr;
        }
    }
	
	/**
	 * @brief Destructor for the weak reference.
	 * 
	 * This destructor properly cleans up resources associated with the weak reference.
	 * When a WeakRef is destroyed, it decrements the weak reference count in the associated
	 * control block. If this was the last weak reference and the object has already been 
	 * destroyed (control block's pointer is null), the control block itself will be deleted.
	 * 
	 * The destruction process ensures that:
	 * 1. All weak references are properly tracked
	 * 2. Control blocks are cleaned up when no longer needed
	 * 3. No memory leaks occur when weak references go out of scope
	 */
    template <typename T>
    WeakRef<T>::~WeakRef()
    {
        if (m_ControlBlock)
        {
            m_ControlBlock->DecWeakCount();
        }
    }
	
	/**
	 * @brief Copy assignment operator for weak references.
	 * 
	 * This operator assigns the content of another WeakRef to this WeakRef.
	 * If this WeakRef is already referencing an object, it decrements the 
	 * weak reference count in that object's control block. Then it copies 
	 * the control block pointer from the source WeakRef and increments the 
	 * weak reference count if the control block is valid.
	 * 
	 * The implementation:
	 * 1. Checks for self-assignment to avoid unnecessary operations
	 * 2. Decrements the weak reference count in the current control block (if any)
	 * 3. Copies the control block pointer from the source WeakRef
	 * 4. Increments the weak reference count in the new control block (if valid)
	 * 
	 * @param other The source WeakRef to copy from
	 * @return A reference to this WeakRef after the assignment
	 * @note This operator maintains proper reference counting without affecting the 
	 *       lifetime of the referenced object
	 */
    template <typename T>
    WeakRef<T> &WeakRef<T>::operator=(const WeakRef &other) noexcept
    {
        if (this != &other)
        {
            if (m_ControlBlock)
            {
                m_ControlBlock->DecWeakCount();
            }

            m_ControlBlock = other.m_ControlBlock;

            if (m_ControlBlock)
            {
                m_ControlBlock->IncWeakCount();
            }
        }
        return *this;
    }
	
	/**
	 * @brief Copy conversion assignment operator for WeakRef objects of different but compatible types.
	 * 
	 * This operator allows assignment of a WeakRef<U> to a WeakRef<T> where U is convertible to T
	 * (typically through inheritance relationships). It properly maintains the weak reference counting
	 * through the control block system.
	 * 
	 * The implementation:
	 * 1. Decrements the weak reference count for this WeakRef's current control block (if any)
	 * 2. Retrieves or creates a control block for the T* pointer obtained by static_casting the U* pointer
	 *    from the source WeakRef's control block
	 * 3. Increments the weak reference count if a valid control block is found
	 * 
	 * @tparam U Source type that is convertible to T
	 * @param other The source WeakRef<U> to assign from
	 * @return A reference to this WeakRef after the assignment
	 * @note This operator only participates in overload resolution if U* is convertible to T*
	 *       (enforced by the SFINAE template parameter)
	 */
    template <typename T>
    template <typename U, typename>
    WeakRef<T> &WeakRef<T>::operator=(const WeakRef<U> &other) noexcept
    {
        if (m_ControlBlock)
        {
            m_ControlBlock->DecWeakCount();
            m_ControlBlock = nullptr;
        }

        if (other.m_ControlBlock)
        {
            m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(
                static_cast<T *>(other.m_ControlBlock->GetPtr()));
            if (m_ControlBlock)
            {
                m_ControlBlock->IncWeakCount();
            }
        }

        return *this;
    }
	
	/**
	 * @brief Move assignment operator for weak references.
	 * 
	 * This operator assigns the content of another WeakRef to this WeakRef through move semantics.
	 * Move assignment is more efficient than copy assignment as it transfers ownership of the 
	 * control block pointer rather than copying it and incrementing reference counts.
	 * 
	 * The implementation:
	 * 1. Checks for self-assignment to avoid unnecessary operations
	 * 2. Decrements the weak reference count in the current control block (if any)
	 * 3. Takes ownership of the control block pointer from the source WeakRef
	 * 4. Sets the source WeakRef's control block pointer to nullptr to prevent
	 *    both instances from managing the same control block
	 * 
	 * @param other The source WeakRef to move from
	 * @return A reference to this WeakRef after the assignment
	 */
    template <typename T>
    WeakRef<T> &WeakRef<T>::operator=(WeakRef &&other) noexcept
    {
        if (this != &other)
        {
            if (m_ControlBlock)
            {
                m_ControlBlock->DecWeakCount();
            }

            m_ControlBlock = other.m_ControlBlock;
            other.m_ControlBlock = nullptr;
        }
        return *this;
    }
	
	/**
	 * @brief Move conversion assignment operator for weak references of different but compatible types.
	 * 
	 * This operator moves a WeakRef<U> to a WeakRef<T> where U is convertible to T
	 * (typically through inheritance relationships). Unlike the regular move constructor,
	 * this operator performs a type conversion which requires finding or creating a
	 * control block for the target type.
	 * 
	 * The implementation:
	 * 1. Decrements the weak reference count of the current control block (if any)
	 * 2. If the source WeakRef has a valid control block, retrieves or creates a control block 
	 *    for the target type through the registry
	 * 3. Sets the source WeakRef's control block to nullptr to prevent both instances 
	 *    from managing the same control block
	 * 4. No increment of weak reference count is needed as ownership is transferred
	 * 
	 * @tparam U Source type that is convertible to T
	 * @param other The source WeakRef<U> to move from
	 * @return A reference to this WeakRef after the assignment
	 * @note This operator only participates in overload resolution if U* is convertible to T*
	 *       (enforced by the SFINAE template parameter)
	 */
    template <typename T>
    template <typename U, typename>
    WeakRef<T> &WeakRef<T>::operator=(WeakRef<U> &&other) noexcept
    {
        if (m_ControlBlock)
        {
            m_ControlBlock->DecWeakCount();
            m_ControlBlock = nullptr;
        }

        if (other.m_ControlBlock)
        {
            m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(
                static_cast<T *>(other.m_ControlBlock->GetPtr()));
            other.m_ControlBlock = nullptr;
        }

        return *this;
    }
	
	/**
	 * @brief Assignment operator that assigns a strong reference to a weak reference.
	 * 
	 * This operator assigns a strong reference (Ref<U>) to this weak reference (WeakRef<T>).
	 * It properly maintains weak reference counting through the control block system.
	 * 
	 * The implementation:
	 * 1. Decrements the weak reference count in the current control block (if any)
	 * 2. Clears the current control block pointer
	 * 3. If the source reference is valid, retrieves or creates a control block for the referenced object
	 * 4. Increments the weak reference count if a valid control block is found
	 * 
	 * @tparam U Source type that is convertible to T
	 * @param ref The source Ref<U> to assign from
	 * @return A reference to this WeakRef after the assignment
	 * @note This operator only participates in overload resolution if U* is convertible to T*
	 *       (enforced by the SFINAE template parameter)
	 */
    template <typename T>
    template <typename U, typename>
    WeakRef<T> &WeakRef<T>::operator=(const Ref<U> &ref) noexcept
    {
        if (m_ControlBlock)
        {
            m_ControlBlock->DecWeakCount();
            m_ControlBlock = nullptr;
        }

        if (ref)
        {
            m_ControlBlock =
                Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(static_cast<T *>(ref.Get()));
            if (m_ControlBlock)
            {
                m_ControlBlock->IncWeakCount();
            }
        }

        return *this;
    }
	
	/**
	 * @brief Assignment operator for assigning nullptr to a weak reference.
	 * 
	 * This operator allows assigning nullptr to a WeakRef, which effectively
	 * resets the weak reference. It decrements the weak reference count in the
	 * associated control block (if any) and sets the control block pointer to nullptr.
	 * 
	 * The implementation:
	 * 1. Decrements the weak reference count if a valid control block exists
	 * 2. Sets the control block pointer to nullptr
	 * 
	 * @param unused Nullptr value (not used in the implementation)
	 * @return A reference to this WeakRef after the assignment
	 */
    template <typename T>
    WeakRef<T> &WeakRef<T>::operator=(std::nullptr_t) noexcept
    {
        if (m_ControlBlock)
        {
            m_ControlBlock->DecWeakCount();
            m_ControlBlock = nullptr;
        }
        return *this;
    }
	
	/**
	 * @brief Checks if the object pointed to by the weak reference has been destroyed.
	 * 
	 * This method determines whether the WeakRef is expired by checking if:
	 * 1. The control block is null (indicating an empty weak reference), or
	 * 2. The pointer stored in the control block is null (indicating the referenced object has been destroyed)
	 * 
	 * A WeakRef becomes expired when the last Ref pointing to the same object is destroyed,
	 * which triggers the object's deletion. The control block maintains this information
	 * even after the object is gone.
	 * 
	 * @return true if the referenced object has been destroyed or if this is an empty WeakRef
	 * @return false if the referenced object is still alive
	 */
    template <typename T>
    bool WeakRef<T>::Expired() const noexcept
    {
        return !m_ControlBlock || m_ControlBlock->GetPtr() == nullptr;
    }
	
	/**
	 * @brief Attempts to convert a weak reference to a strong reference.
	 * 
	 * This method tries to obtain a strong reference (Ref<T>) from the weak reference.
	 * If the object the WeakRef points to is still alive (not expired), it creates
	 * and returns a new Ref<T> pointing to that object, which increments the reference
	 * count of the object. If the object has been destroyed, it returns an empty Ref<T>.
	 * 
	 * The implementation:
	 * 1. Checks if the control block exists and the object is still alive
	 * 2. If both conditions are met, creates a new strong reference to the object
	 * 3. Otherwise, returns an empty (null) strong reference
	 * 
	 * @tparam T The type of the referenced object
	 * @return Ref<T> A strong reference to the object if it's still alive, or an empty reference otherwise
	 */
    template <typename T>
    Ref<T> WeakRef<T>::Lock() const noexcept
    {
        if (!m_ControlBlock || m_ControlBlock->GetPtr() == nullptr)
            return Ref<T>();

        return Ref<T>(m_ControlBlock->GetPtr());
    }
	
	/**
	 * @brief Resets this weak reference to empty state.
	 * 
	 * This method explicitly releases the weak reference to any object it might be pointing to.
	 * It decrements the weak reference count in the associated control block, and if this
	 * was the last weak reference and the object has already been destroyed, the control block
	 * itself will be deleted.
	 * 
	 * After calling Reset(), the weak reference will be in an empty state (similar to a
	 * default-constructed WeakRef) and will return true for Expired() and nullptr for Lock().
	 * 
	 * @note This method is often used to explicitly release resources before the WeakRef
	 *       goes out of scope, or to prepare the WeakRef for reuse.
	 */
    template <typename T>
    void WeakRef<T>::Reset() noexcept
    {
        if (m_ControlBlock)
        {
            m_ControlBlock->DecWeakCount();
            m_ControlBlock = nullptr;
        }
    }
	
	/**
	 * @brief Gets the current number of strong references (Ref<T>) to the object.
	 * 
	 * This method returns the reference count of the object that this WeakRef
	 * points to. If the WeakRef is expired (the object has been destroyed) or
	 * if it's an empty WeakRef, the method returns 0.
	 * 
	 * This is useful for debugging and testing purposes, or for algorithms that
	 * need to make decisions based on the reference count of an object.
	 * 
	 * @tparam T The type of the referenced object
	 * @return The number of strong references to the object, or 0 if the WeakRef is expired
	 */
    template <typename T>
    uint32_t WeakRef<T>::UseCount() const noexcept
    {
        if (m_ControlBlock && m_ControlBlock->GetPtr())
        {
            return m_ControlBlock->GetPtr()->GetRefCount();
        }
        return 0;
    }
	
	/**
	 * @brief Equality comparison operator for WeakRef objects.
	 * 
	 * This operator determines if two WeakRef objects reference the same underlying object.
	 * The comparison is done in the following order:
	 * 1. First checks if both WeakRefs have the same control block pointer (fast path)
	 * 2. If control blocks differ, checks if either is nullptr (meaning one reference is empty)
	 * 3. Finally compares the actual object pointers stored in the control blocks
	 *
	 * This enables WeakRef objects to be used in containers that require equality comparison,
	 * such as std::set, std::map, or for general comparison operations.
	 * 
	 * @param other The WeakRef to compare with
	 * @return true if both WeakRef objects reference the same object or are both empty
	 * @return false if the WeakRef objects reference different objects or one is empty and one is not
	 */
    template <typename T>
    bool WeakRef<T>::operator==(const WeakRef &other) const noexcept
    {
        if (m_ControlBlock == other.m_ControlBlock)
            return true;

        if (!m_ControlBlock || !other.m_ControlBlock)
            return false;

        return m_ControlBlock->GetPtr() == other.m_ControlBlock->GetPtr();
    }
	
	/**
	 * @brief Inequality comparison operator for WeakRef objects.
	 * 
	 * This operator determines if two WeakRef objects reference different underlying objects.
	 * It is implemented by calling the equality operator and negating the result.
	 * 
	 * Two WeakRef objects are considered not equal if:
	 * 1. They reference different objects, or
	 * 2. One references an object and the other is empty, or
	 * 3. They are both empty but have different control blocks
	 *
	 * @param other The WeakRef to compare with
	 * @return true if the WeakRef objects reference different objects or have different empty states
	 * @return false if both WeakRef objects reference the same object or are both empty
	 */
    template <typename T>
    bool WeakRef<T>::operator!=(const WeakRef &other) const noexcept
    {
        return !(*this == other);
    }
	
	/**
	 * @brief Constructor that creates a strong reference from a weak reference.
	 * 
	 * This constructor attempts to convert a WeakRef<T> to a Ref<T> by checking if
	 * the object pointed to by the weak reference is still alive. If the weak reference
	 * is valid (not expired), this constructor creates a new strong reference to the object
	 * and increments its reference count. If the weak reference is expired (the object
	 * has been destroyed), the constructor creates an empty Ref (m_Ptr = nullptr).
	 * 
	 * The implementation:
	 * 1. Retrieves the object pointer from the weak reference's control block if it exists
	 * 2. Initializes the internal pointer to the retrieved object or nullptr
	 * 3. Calls InternalAddRef() to increment the reference count if the pointer is valid
	 * 
	 * @param weak The weak reference to convert to a strong reference
	 * @note This enables safe conversion from WeakRef<T> to Ref<T>, preventing access to destroyed objects
	 */
    template <typename T>
    Ref<T>::Ref(const WeakRef<T> &weak) noexcept : m_Ptr(weak.m_ControlBlock ? weak.m_ControlBlock->GetPtr() : nullptr)
    {
        InternalAddRef();
    }
	
	/**
	 * @brief Explicit template instantiation for Ref<RefCounted>
	 * 
	 * This explicit instantiation ensures that the compiler generates all the code
	 * for Ref<RefCounted> at this point, making it available to all translation units
	 * that include this header without having to recompile the template for each use.
	 * 
	 * RefCounted is the base class for all reference-counted objects in the system,
	 * so this instantiation is particularly important for the smart pointer system.
	 */
    template class Ref<RefCounted>;

    /**
	 * @brief Explicit template instantiation for WeakRef<RefCounted>
	 * 
	 * This explicit instantiation ensures that the compiler generates all the code
	 * for WeakRef<RefCounted> at this point, making it available to all translation units
	 * that include this header without having to recompile the template for each use.
	 * 
	 * Weak references to RefCounted objects allow tracking objects without preventing their
	 * deletion when all strong references (Ref<T>) are gone, which is essential for
	 * breaking reference cycles and implementing observer patterns.
	 */
    template class WeakRef<RefCounted>;
	
} // namespace SceneryEditorX

/// -------------------------------------------------------
