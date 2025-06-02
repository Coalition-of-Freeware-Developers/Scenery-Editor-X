/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* pointers.h
* -------------------------------------------------------
* Created: 31/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

/// -------------------------------------------------------

////////////////////////////////////////////////////////////
///				Pointer Templates & Alias				 ///
////////////////////////////////////////////////////////////
namespace SceneryEditorX
{
	
	/**
	 * @brief Base class for objects that can be reference-counted.
	 * 
	 * Provides thread-safe reference counting capabilities that can be used by smart pointers.
	 * Objects inheriting from this class can be managed by Ref<T>.
	 */
	class RefCounted
	{
	public:
		/**
		 * @brief Default constructor initializes reference count to 0.
		 */
		RefCounted() = default;
		
		/**
		 * @brief Copy constructor maintains the reference count at 0.
		 * 
		 * When an object is copied, the new instance starts with a fresh reference count.
		 */
		RefCounted(const RefCounted&) noexcept {}
		
		/**
		 * @brief Copy assignment operator doesn't affect reference count.
		 * 
		 * Reference count is associated with object identity, not with its contents.
		 */
		RefCounted& operator=(const RefCounted&) noexcept { return *this; }
		
		/**
		 * @brief Move constructor maintains the reference count at 0.
		 */
		RefCounted(RefCounted&&) noexcept {}
		
		/**
		 * @brief Move assignment operator doesn't affect reference count.
		 */
		RefCounted& operator=(RefCounted&&) noexcept { return *this; }
		
		/**
		 * @brief Virtual destructor for proper polymorphic behavior.
		 */
		virtual ~RefCounted() = default;
		
		/**
		 * @brief Increments the reference count.
		 * @return The new reference count.
		 */
		uint32_t IncRefCount() const noexcept { return ++m_RefCount; }
		
		/**
		 * @brief Decrements the reference count.
		 * @return The new reference count.
		 */
		uint32_t DecRefCount() const noexcept
		{
			assert(m_RefCount > 0 && "Reference count is already 0");
			return --m_RefCount;
		}
		
		/**
		 * @brief Gets the current reference count.
		 * @return The current reference count.
		 */
		uint32_t GetRefCount() const noexcept { return m_RefCount; }
		
	private:
		/// Using mutable to allow const objects to be reference counted
		mutable std::atomic<uint32_t> m_RefCount{0};
	};

    /// -------------------------------------------------------

	/**
	 * @brief Alias template for a unique pointer to type T.
	 * @tparam T The type to manage.
	 */
	template <typename T>
	using Scope = std::unique_ptr<T>;
	
	/**
	 * @brief Creates a unique pointer to an object of type T.
	 * 
	 * Creates and returns a unique pointer that has exclusive ownership
	 * of the newly created object.
	 * 
	 * @tparam T The type to manage.
	 * @tparam Args The types of the arguments to pass to the constructor of T.
	 * @param args The arguments to pass to the constructor of T.
	 * @return A unique pointer to an object of type T.
	 */
	template <typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args &&...args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

    /// ----------------------------------------------------------

    namespace Internal
    {
    /**
     * @brief Control block for weak reference handling.
     * 
     * This class provides the implementation for tracking weak references to objects.
     * When an object with Ref pointers is destroyed, the control block remains
     * until all WeakRef instances pointing to it are also destroyed.
     * 
     * @tparam T The type of the managed object.
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
        explicit ControlBlock(T *ptr) noexcept : m_Ptr(ptr), m_WeakCount(0)
        {
        }

        /**
         * @brief Increments the weak reference count
         * 
         * Called when a new WeakRef is created or copied to point to this object
         */
        void IncWeakCount() noexcept
        {
            ++m_WeakCount;
        }

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
                delete this;
        }

        /**
         * @brief Gets the pointer to the managed object
         * 
         * @return The pointer to the object, or nullptr if the object has been destroyed
         */
        T *GetPtr() const noexcept
        {
            return m_Ptr;
        }

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
        std::atomic<uint32_t> m_WeakCount; ///< Number of weak references to this control block
    };

    /**
     * @brief Registry that manages control blocks for weak references
     * 
     * This singleton class maintains a mapping between object pointers and their
     * corresponding control blocks, allowing WeakRef instances to find the
     * appropriate control block for an object.
     * 
     * @tparam T The type of the managed object.
     */
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
                return it->second;

            auto *block = new ControlBlock<T>(ptr);
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
                ControlBlock<T> *block = it->second;
                m_Blocks.erase(it);

                if (block->GetWeakCount() == 0)
                    delete block;
                else
                    block->SetPtr(nullptr); // Mark the object as destroyed
            }
        }

    private:
        /**
         * @brief Private constructor to enforce singleton pattern
         */
        ControlBlockRegistry() = default;

        ~ControlBlockRegistry()
        {
            // Delete any remaining control blocks
            std::lock_guard<std::mutex> lock(m_Mutex);
            for (auto &pair : m_Blocks)
                delete pair.second;
            m_Blocks.clear();
        }

        std::unordered_map<T *, ControlBlock<T> *> m_Blocks; ///< Map of object pointers to control blocks
        std::mutex m_Mutex;                                  ///< Mutex to protect concurrent access to the map
    };
    } // namespace Internal

    /// -----------------------------------------------------------


	template <typename T>
	class WeakRef;

    /// ---------------------------------------------------------

	/**
	 * @brief A reference-counting smart pointer that manages shared ownership of objects.
	 * 
	 * The Ref class provides a reference-counting ownership mechanism where multiple
	 * Ref instances can share ownership of a single object. The object is destroyed
	 * when the last Ref pointing to it is destroyed or reset.
	 * 
	 * @tparam T The type of the managed object.
	 */
	template <typename T>
	class Ref
	{
	public:
		/**
		 * @brief Default constructor creates a null reference.
		 */
		constexpr Ref() noexcept = default;
		
		/**
		 * @brief Constructor from nullptr creates a null reference.
		 */
		constexpr Ref(std::nullptr_t) noexcept {}
		
		/**
		 * @brief Constructor from raw pointer. Takes ownership of the object.
		 * 
		 * This constructor increments the reference count of the object.
		 * 
		 * @param ptr Pointer to the object to manage.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		explicit Ref(U* ptr) noexcept : m_Ptr(ptr)
		{
			InternalAddRef();
		}
		
		/**
		 * @brief Copy constructor. Shares ownership of the object.
		 * 
		 * This constructor increments the reference count of the object.
		 * 
		 * @param other The Ref to copy from.
		 */
		Ref(const Ref& other) noexcept : m_Ptr(other.m_Ptr)
		{
			InternalAddRef();
		}
		
		/**
		 * @brief Copy constructor with type conversion. Shares ownership of the object.
		 * 
		 * This constructor increments the reference count of the object and allows
		 * converting between compatible types.
		 * 
		 * @tparam U The type of the other Ref.
		 * @param other The Ref to copy from.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		Ref(const Ref<U>& other) noexcept : m_Ptr(other.Get())
		{
			InternalAddRef();
		}
		
		/**
		 * @brief Move constructor. Takes ownership from another Ref.
		 * 
		 * This constructor doesn't change the reference count of the object.
		 * 
		 * @param other The Ref to move from.
		 */
		Ref(Ref&& other) noexcept : m_Ptr(other.m_Ptr)
		{
			other.m_Ptr = nullptr;
		}
		
		/**
		 * @brief Move constructor with type conversion. Takes ownership from another Ref.
		 * 
		 * This constructor doesn't change the reference count of the object and allows
		 * converting between compatible types.
		 * 
		 * @tparam U The type of the other Ref.
		 * @param other The Ref to move from.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		Ref(Ref<U>&& other) noexcept : m_Ptr(other.Get())
		{
			other.m_Ptr = nullptr;
		}
		
		/**
		 * @brief Constructor from std::shared_ptr. Shares ownership of the object.
		 * 
		 * This constructor allows interoperability with std::shared_ptr.
		 * 
		 * @param shared The std::shared_ptr to convert from.
		 */
		explicit Ref(const std::shared_ptr<T>& shared) noexcept : m_Ptr(shared.get())
		{
			InternalAddRef();
		}
		
		/**
		 * @brief Constructor from WeakRef. Obtains a strong reference if available.
		 * 
		 * This constructor attempts to obtain a strong reference from a WeakRef.
		 * If the WeakRef has expired, the Ref will be null.
		 * 
		 * @param weak The WeakRef to convert from.
		 */
		explicit Ref(const WeakRef<T>& weak) noexcept;
		
		/**
		 * @brief Destructor. Decrements the reference count of the object.
		 * 
		 * If the reference count reaches 0, the object is destroyed.
		 */
		~Ref()
		{
			InternalRelease();
		}
		
		/**
		 * @brief Copy assignment operator. Shares ownership of the object.
		 * 
		 * This operator increments the reference count of the assigned object
		 * and decrements the reference count of the previously managed object.
		 * 
		 * @param other The Ref to copy from.
		 * @return Reference to this Ref.
		 */
		Ref& operator=(const Ref& other) noexcept
		{
			if (this != &other)
			{
				InternalRelease();
				m_Ptr = other.m_Ptr;
				InternalAddRef();
			}
			return *this;
		}
		
		/**
		 * @brief Copy assignment operator with type conversion. Shares ownership of the object.
		 * 
		 * This operator increments the reference count of the assigned object,
		 * decrements the reference count of the previously managed object, and allows
		 * converting between compatible types.
		 * 
		 * @tparam U The type of the other Ref.
		 * @param other The Ref to copy from.
		 * @return Reference to this Ref.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		Ref& operator=(const Ref<U>& other) noexcept
		{
			InternalRelease();
			m_Ptr = other.Get();
			InternalAddRef();
			return *this;
		}
		
		/**
		 * @brief Move assignment operator. Takes ownership from another Ref.
		 * 
		 * This operator decrements the reference count of the previously managed object
		 * and takes ownership of the object from the other Ref without changing its
		 * reference count.
		 * 
		 * @param other The Ref to move from.
		 * @return Reference to this Ref.
		 */
		Ref& operator=(Ref&& other) noexcept
		{
			if (this != &other)
			{
				InternalRelease();
				m_Ptr = other.m_Ptr;
				other.m_Ptr = nullptr;
			}
			return *this;
		}
		
		/**
		 * @brief Move assignment operator with type conversion. Takes ownership from another Ref.
		 * 
		 * This operator decrements the reference count of the previously managed object,
		 * takes ownership of the object from the other Ref without changing its reference count,
		 * and allows converting between compatible types.
		 * 
		 * @tparam U The type of the other Ref.
		 * @param other The Ref to move from.
		 * @return Reference to this Ref.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		Ref& operator=(Ref<U>&& other) noexcept
		{
			InternalRelease();
			m_Ptr = other.Get();
			other.m_Ptr = nullptr;
			return *this;
		}
		
		/**
		 * @brief Assignment operator from nullptr. Resets the Ref.
		 * 
		 * This operator decrements the reference count of the previously managed object
		 * and sets the Ref to null.
		 * 
		 * @return Reference to this Ref.
		 */
		Ref& operator=(std::nullptr_t) noexcept
		{
			InternalRelease();
			return *this;
		}
		
		/**
		 * @brief Dereference operator. Provides access to the managed object.
		 * 
		 * @return Reference to the managed object.
		 */
		T& operator*() const noexcept
		{
			assert(m_Ptr && "Dereferencing null Ref");
			return *m_Ptr;
		}
		
		/**
		 * @brief Arrow operator. Provides access to the managed object's members.
		 * 
		 * @return Pointer to the managed object.
		 */
		T* operator->() const noexcept
		{
			assert(m_Ptr && "Accessing member of null Ref");
			return m_Ptr;
		}
		
		/**
		 * @brief Boolean conversion operator. Checks if the Ref is not null.
		 * 
		 * @return True if the Ref is not null, false otherwise.
		 */
		explicit operator bool() const noexcept { return m_Ptr != nullptr; }
		
		/**
		 * @brief Gets the raw pointer to the managed object.
		 * 
		 * @return Pointer to the managed object.
		 */
		T* Get() const noexcept { return m_Ptr; }
		
		/**
		 * @brief Resets the Ref to null or to manage a new object.
		 * 
		 * This method decrements the reference count of the previously managed object
		 * and sets the Ref to manage a new object or to null if no object is provided.
		 * 
		 * @param ptr Pointer to the new object to manage, or nullptr.
		 */
		void Reset(T* ptr = nullptr) noexcept
		{
			InternalRelease();
			m_Ptr = ptr;
			InternalAddRef();
		}
		
		/**
		 * @brief Checks if this Ref is the only one managing the object.
		 * 
		 * @return True if the reference count is 1, false otherwise or if null.
		 */
		bool IsUnique() const noexcept
		{
			return m_Ptr && m_Ptr->GetRefCount() == 1;
		}
		
		/**
		 * @brief Converts this Ref to a Ref of another type using static_cast.
		 * 
		 * @tparam U The type to convert to.
		 * @return A Ref<U> managing the same object.
		 */
		template <typename U>
		Ref<U> As() const noexcept
		{
			return Ref<U>(static_cast<U*>(m_Ptr));
		}
		
		/**
		 * @brief Converts this Ref to a Ref of another type using dynamic_cast.
		 * 
		 * @tparam U The type to convert to.
		 * @return A Ref<U> managing the same object, or null if the cast fails.
		 */
		template <typename U>
		Ref<U> DynamicCast() const noexcept
		{
			if (U* cast = dynamic_cast<U*>(m_Ptr))
				return Ref<U>(cast);
			return Ref<U>();
		}
		
		/**
		 * @brief Converts this Ref to a std::shared_ptr.
		 * 
		 * This method creates a std::shared_ptr from this Ref with a custom deleter
		 * that decrements the reference count. This allows interoperability with
		 * functions that expect std::shared_ptr.
		 * 
		 * @return A std::shared_ptr managing the same object.
		 */
		std::shared_ptr<T> ToSharedPtr() const noexcept
		{
			if (!m_Ptr)
				return nullptr;
				
			/// Increment the ref count for the shared_ptr
			InternalAddRef();
			
			/// Create a shared_ptr with a custom deleter that decrements the ref count
			return std::shared_ptr<T>(m_Ptr, [](T* ptr)
			{
				if (ptr && ptr->DecRefCount() == 0)
					delete ptr;
			});
		}
		
		/**
		 * @brief Swaps the contents of this Ref with another.
		 * 
		 * @param other The Ref to swap with.
		 */
		void Swap(Ref& other) noexcept
		{
			std::swap(m_Ptr, other.m_Ptr);
		}
		
		/**
		 * @brief Checks if the Ref is not null.
		 * 
		 * @return True if the Ref is not null, false otherwise.
		 */
		bool IsValid() const noexcept { return m_Ptr != nullptr; }
		
		/**
		 * @brief Gets the reference count of the managed object.
		 * 
		 * @return The reference count, or 0 if the Ref is null.
		 */
		uint32_t UseCount() const noexcept
		{
			return m_Ptr ? m_Ptr->GetRefCount() : 0;
		}
		
		/**
		 * @brief Equality operator. Compares the managed objects.
		 * 
		 * @param other The Ref to compare with.
		 * @return True if both Refs manage the same object, false otherwise.
		 */
		bool operator==(const Ref& other) const noexcept
		{
			return m_Ptr == other.m_Ptr;
		}
		
		/**
		 * @brief Inequality operator. Compares the managed objects.
		 * 
		 * @param other The Ref to compare with.
		 * @return True if the Refs manage different objects, false otherwise.
		 */
		bool operator!=(const Ref& other) const noexcept
		{
			return m_Ptr != other.m_Ptr;
		}
		
		/**
		 * @brief Equality operator with nullptr. Checks if the Ref is null.
		 * 
		 * @return True if the Ref is null, false otherwise.
		 */
		bool operator==(std::nullptr_t) const noexcept
		{
			return m_Ptr == nullptr;
		}
		
		/**
		 * @brief Inequality operator with nullptr. Checks if the Ref is not null.
		 * 
		 * @return True if the Ref is not null, false otherwise.
		 */
		bool operator!=(std::nullptr_t) const noexcept
		{
			return m_Ptr != nullptr;
		}
		
		/**
		 * @brief Compares the managed objects for object equality.
		 * 
		 * Unlike operator==, this method compares the objects themselves, not just
		 * the pointers. This requires that T has an operator== defined.
		 * 
		 * @param other The Ref to compare with.
		 * @return True if both objects are equal, false otherwise.
		 */
		bool EqualsObject(const Ref& other) const noexcept
		{
			if (m_Ptr == other.m_Ptr)
				return true;
			
			if (!m_Ptr || !other.m_Ptr)
				return false;
			
			return *m_Ptr == *other.m_Ptr;
		}
		
	private:
		T* m_Ptr = nullptr;
		
		void InternalAddRef() const noexcept;
		void InternalRelease() noexcept;
		
		/// Allow WeakRef to access private members
		template <typename U>
		friend class WeakRef;
	};

    /// -------------------------------------------------------

	/**
	 * @brief A weak reference to an object managed by Ref<T>.
	 * 
	 * WeakRef allows observing an object without affecting its lifetime.
	 * Unlike Ref<T>, WeakRef does not prevent the object from being destroyed.
	 * 
	 * @tparam T The type of the managed object.
	 */
	template <typename T>
	class WeakRef
	{
	public:
		/**
		 * @brief Default constructor creates an empty weak reference.
		 */
		constexpr WeakRef() noexcept = default;
		
		/**
		 * @brief Constructor from nullptr creates an empty weak reference.
		 */
		constexpr WeakRef(std::nullptr_t) noexcept {}
		
		/**
		 * @brief Constructor from a Ref<T>. Creates a weak reference to the object managed by ref.
		 * 
		 * @param ref The Ref<T> to observe.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef(const Ref<U>& ref) noexcept;
		
		/**
		 * @brief Copy constructor.
		 * 
		 * @param other The WeakRef to copy from.
		 */
		WeakRef(const WeakRef& other) noexcept;
		
		/**
		 * @brief Copy constructor with type conversion.
		 * 
		 * @tparam U The type of the other WeakRef.
		 * @param other The WeakRef to copy from.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef(const WeakRef<U>& other) noexcept;
		
		/**
		 * @brief Move constructor.
		 * 
		 * @param other The WeakRef to move from.
		 */
		WeakRef(WeakRef&& other) noexcept;
		
		/**
		 * @brief Move constructor with type conversion.
		 * 
		 * @tparam U The type of the other WeakRef.
		 * @param other The WeakRef to move from.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef(WeakRef<U>&& other) noexcept;
		
		/**
		 * @brief Destructor.
		 */
		~WeakRef();
		
		/**
		 * @brief Copy assignment operator.
		 * 
		 * @param other The WeakRef to copy from.
		 * @return Reference to this WeakRef.
		 */
		WeakRef& operator=(const WeakRef& other) noexcept;
		
		/**
		 * @brief Copy assignment operator with type conversion.
		 * 
		 * @tparam U The type of the other WeakRef.
		 * @param other The WeakRef to copy from.
		 * @return Reference to this WeakRef.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef& operator=(const WeakRef<U>& other) noexcept;
		
		/**
		 * @brief Move assignment operator.
		 * 
		 * @param other The WeakRef to move from.
		 * @return Reference to this WeakRef.
		 */
		WeakRef& operator=(WeakRef&& other) noexcept;
		
		/**
		 * @brief Move assignment operator with type conversion.
		 * 
		 * @tparam U The type of the other WeakRef.
		 * @param other The WeakRef to move from.
		 * @return Reference to this WeakRef.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef& operator=(WeakRef<U>&& other) noexcept;
		
		/**
		 * @brief Assignment operator from Ref<T>.
		 * 
		 * @param ref The Ref<T> to observe.
		 * @return Reference to this WeakRef.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef& operator=(const Ref<U>& ref) noexcept;
		
		/**
		 * @brief Assignment operator from nullptr.
		 * 
		 * @return Reference to this WeakRef.
		 */
		WeakRef& operator=(std::nullptr_t) noexcept;
		
		/**
		 * @brief Checks if the WeakRef is expired.
		 * 
		 * A WeakRef is expired if the object it points to has been destroyed.
		 * 
		 * @return True if the WeakRef is expired, false otherwise.
		 */
		bool Expired() const noexcept;
		
		/**
		 * @brief Attempts to get a strong reference to the object.
		 * 
		 * @return A Ref<T> to the object, or an empty Ref<T> if the object has been destroyed.
		 */
		Ref<T> Lock() const noexcept;
		
		/**
		 * @brief Resets the WeakRef.
		 */
		void Reset() noexcept;
		
		/**
		 * @brief Gets the reference count of the object.
		 * 
		 * @return The number of Ref<T> instances that share ownership of the object, or 0 if the WeakRef is expired.
		 */
		uint32_t UseCount() const noexcept;
		
		/**
		 * @brief Equality operator.
		 * 
		 * @param other The WeakRef to compare with.
		 * @return True if both WeakRefs observe the same object, false otherwise.
		 */
		bool operator==(const WeakRef& other) const noexcept;
		
		/**
		 * @brief Inequality operator.
		 * 
		 * @param other The WeakRef to compare with.
		 * @return True if the WeakRefs observe different objects, false otherwise.
		 */
		bool operator!=(const WeakRef& other) const noexcept;
		
	private:
		Internal::ControlBlock<T>* m_ControlBlock = nullptr;
		
		/// Allow Ref<T> to access m_ControlBlock
		template <typename U>
		friend class Ref;
	};

    /// -------------------------------------------------------

	/**
	 * @brief Creates a reference-counted object of type T.
	 * 
	 * This function creates a new instance of T and wraps it in a Ref<T>.
	 * The type T must inherit from RefCounted.
	 * 
	 * @tparam T The type to create.
	 * @tparam Args The types of the arguments to pass to the constructor of T.
	 * @param args The arguments to pass to the constructor of T.
	 * @return A Ref<T> managing the new object.
	 */
	template <typename T, typename... Args>
	Ref<T> CreateRef(Args&&... args)
	{
		static_assert(std::is_base_of_v<RefCounted, T>, "Type must inherit from RefCounted");
		return Ref<T>(new T(std::forward<Args>(args)...));
	}

} // namespace SceneryEditorX

/// -------------------------------------------------------
