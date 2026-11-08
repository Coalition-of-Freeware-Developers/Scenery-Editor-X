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
 * pointers.h
 * -------------------------------------------------------
 * Created: 31/5/2025
 * -------------------------------------------------------
 * Scenery Editor X - Smart Pointer System (Consolidated)
 * -------------------------------------------------------
 * This header previously contained two competing implementations of the
 * engine smart pointer system. The duplicate (control-block based) version
 * injected ahead of the original project version has been removed to resolve
 * redefinition and ODR errors. The surviving implementation below is the
 * original project design that other engine modules expect (m_Ptr +
 * InternalAddRef/Release + ControlBlockRegistry for weak refs).
 * -------------------------------------------------------
 */
 #pragma once
 #include <atomic>
 #include <cassert>
 #include <cstdint>
 #include <memory>
 #include <mutex>
 #include <type_traits>
 #include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace Internal
	{
		class ControlBlockRegistry;
	}

	/**
	 * @brief Base class for objects that can be reference-counted.
	 *
	 * Provides thread-safe reference counting capabilities that can be used by smart pointers.
	 * Objects inheriting from this class can be managed by Ref<T>.
	 */
	class RefCounted
	{
	public:
		/** @brief Default constructor initializes reference count to 0. */
		RefCounted() = default;

		/**
		 * @brief Copy constructor maintains the reference count at 0.
		 * When an object is copied, the new instance starts with a fresh reference count.
		 */
		RefCounted(const RefCounted&) noexcept {}

		/**
		 * @brief Copy assignment operator doesn't affect reference count.
		 * Reference count is associated with object identity, not with its contents.
		 */
		RefCounted& operator=(const RefCounted&) noexcept { return *this; }

		/** @brief Move constructor maintains the reference count at 0. */
		RefCounted(RefCounted&&) noexcept {}

		/** @brief Move assignment operator doesn't affect reference count. */
		RefCounted& operator=(RefCounted&&) noexcept { return *this; }

		/**
		 * @brief Virtual destructor for proper polymorphic behavior.
		 *
		 * Expires the object's WeakRefs if it still has a control block, which happens only when it was destroyed
		 * without its last Ref release (that release expires them first): deleted by hand, or a stack or member
		 * object a Ref pointed to. Both are misuse, but the registry would otherwise keep the block of an address the
		 * next object may get, and the old WeakRefs would lock that object. Objects that never had a WeakRef, or
		 * whose last Ref expired them, skip the registry (one atomic load).
		 *
		 * Only the last Ref release is safe against other threads: it expires the WeakRefs before any destructor
		 * runs. In the misuse paths this runs last, so another thread's Lock() can still get the object while its
		 * derived part is being destroyed.
		 */
		virtual ~RefCounted();

		/**
		 * @brief Increments the reference count.
		 * @return The new reference count.
		 */
		uint32_t IncRefCount() const noexcept { return ++m_RefCount; }

		/**
		 * @brief Increments the reference count unless it is 0.
		 *
		 * A count of 0 means the last Ref is releasing the object, which must not be brought back:
		 * WeakRef::Lock() uses this, so a lock racing with the last release either keeps the object
		 * alive (it incremented first) or fails (the release got to 0 first).
		 *
		 * @return True if the count was incremented.
		 */
		bool TryIncRefCount() const noexcept
		{
			uint32_t count = m_RefCount.load(std::memory_order_relaxed);
			while (count != 0)
			{
				if (m_RefCount.compare_exchange_weak(count, count + 1, std::memory_order_acq_rel, std::memory_order_relaxed))
					return true;
			}
			return false;
		}

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
		friend class Internal::ControlBlockRegistry;

		// Using mutable to allow const objects to be reference counted
		mutable std::atomic<uint32_t> m_RefCount{0};

		// Set when the first WeakRef to this object is made, cleared when its control block is removed: only while it
		// is set does the object have a control block to expire. Not copied, like the count: it belongs to the
		// object's identity.
		mutable std::atomic<bool> m_HasWeakRefs{false};
	};

	// -------------------------------------------------------

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

	// ----------------------------------------------------------

	namespace Internal
	{
		/**
		 * @brief Control block for managing weak references to an object
		 *
		 * The ControlBlock is responsible for tracking weak references to an object
		 * even after the object itself has been destroyed. It maintains:
		 * 1. A pointer to the object's RefCounted part (which becomes nullptr when the object is destroyed)
		 * 2. A count of weak references pointing to this control block
		 *
		 * There is one control block per object, whatever the static types of the Refs that own it and the
		 * WeakRefs that observe it: a WeakRef<Base> and a WeakRef<Derived> to the same object share it, so the
		 * last release of the object expires both, through a Ref of either type. The block stores the object's
		 * RefCounted part (the same address whatever the static type, with multiple or virtual inheritance
		 * too); each WeakRef keeps its own pointer of its type.
		 *
		 * When an object is destroyed but weak references to it still exist, the control
		 * block remains alive (with its pointer set to nullptr) until all weak references are gone.
		 *
		 * Like std::weak_ptr's control block, the weak count includes one reference held on
		 * behalf of the living object (released by ControlBlockRegistry::RemoveControlBlock when
		 * the object is destroyed), so whichever release brings the count to 0 deletes the block,
		 * exactly once, whether that is the object's destruction or the last WeakRef.
		 */
		class ControlBlock
		{
		public:
			/**
			 * @brief Constructs a control block for the specified (living) object
			 * @param object The RefCounted part of the object being tracked
			 */
			explicit ControlBlock(const RefCounted *object) noexcept : m_Ptr(object), m_WeakCount(1) {}

			/**
			 * @brief Increments the weak reference count
			 * Called when a new WeakRef is created or copied to point to this object
			 */
			void IncWeakCount() noexcept { m_WeakCount.fetch_add(1, std::memory_order_relaxed); }

			/**
			 * @brief Decrements the weak reference count
			 *
			 * Deletes the control block when the count reaches zero: no WeakRef uses it any more
			 * and the object has been destroyed (it holds a reference while alive).
			 */
			void DecWeakCount() noexcept
			{
				if (m_WeakCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					delete this;
				}
			}

			/**
			 * @brief Gets the RefCounted part of the managed object
			 *
			 * Only a snapshot: another thread may be destroying the object. Use WeakRef::Lock()
			 * to get a pointer that stays valid.
			 *
			 * @return The object's RefCounted part, or nullptr if the object has been destroyed
			 */
			const RefCounted *GetPtr() const noexcept { return m_Ptr.load(std::memory_order_acquire); }

			/**
			 * @brief Sets the object pointer
			 *
			 * This is typically called with nullptr when the object is being destroyed
			 * to indicate that the object is no longer valid.
			 *
			 * @param object The new object pointer value
			 */
			void SetPtr(const RefCounted *object) noexcept { m_Ptr.store(object, std::memory_order_release); }

			/**
			 * @brief Gets the current weak reference count
			 * @return The number of weak references pointing to this control block, plus one while the object is alive
			 */
			uint32_t GetWeakCount() const noexcept { return m_WeakCount.load(std::memory_order_relaxed); }

		private:
			// The object's RefCounted part, or nullptr once it is destroyed (it never comes back)
			std::atomic<const RefCounted *> m_Ptr;

			// Number of weak references to this block, plus one while the object is alive
			std::atomic<uint32_t> m_WeakCount;
		};

		/**
		 * @brief Registry for managing control blocks for weak references
		 *
		 * This class maintains a mapping from objects to their control blocks, allowing weak
		 * references to locate the control block for an object they're referencing. It ensures
		 * that all weak references to the same object share the same control block.
		 *
		 * One registry serves every type, keyed by the object's RefCounted part: Ref<T> releases
		 * find the object's block whatever T is. It is a singleton (never destroyed, so it outlives
		 * every static Ref and WeakRef).
		 */
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
				static ControlBlockRegistry* instance = new ControlBlockRegistry();
				return *instance;
			}

			/**
			 * @brief Expires the WeakRefs to an object whose last Ref was just released
			 *
			 * Called by the last release, before the object is deleted, through a Ref of any type, and by
			 * ~RefCounted() for an object destroyed some other way. Objects without a control block (they never had
			 * a WeakRef, or it was already removed) skip the registry and its mutex.
			 *
			 * The m_HasWeakRefs flag is set (under the registry's mutex) by a thread holding a strong reference,
			 * and that reference's release comes before the count reaches 0, so the last release sees it.
			 *
			 * @param object The RefCounted part of the object (its reference count is 0)
			 */
			static void ExpireWeakRefs(const RefCounted *object) noexcept
			{
				if (object && object->m_HasWeakRefs.load(std::memory_order_acquire))
					GetInstance().RemoveControlBlock(object);
			}

			/**
			 * @brief Get or create the control block for an object and take a weak reference to it
			 *
			 * If a control block already exists for the given object, it returns that block.
			 * Otherwise, it creates a new control block, registers it, and returns it. Either way the
			 * block's weak count is incremented for the caller, who releases it with DecWeakCount().
			 *
			 * @param object The RefCounted part of a living object the caller holds a strong reference to
			 * @return Pointer to the control block, or nullptr if object is nullptr or the registry is shut down
			 */
			ControlBlock *AcquireControlBlock(const RefCounted *object)
			{
				if (!object)
					return nullptr;

				std::lock_guard<std::mutex> lock(m_Mutex);
				if (m_IsShutDown.load())
					return nullptr;

				auto it = m_Blocks.find(object);
				ControlBlock *block = nullptr;
				if (it != m_Blocks.end())
				{
					block = it->second;
				}
				else
				{
					block = new ControlBlock(object);
					m_Blocks[object] = block;
					object->m_HasWeakRefs.store(true, std::memory_order_release);
				}

				block->IncWeakCount();
				return block;
			}

			/**
			 * @brief Gets a strong reference to the object of a control block, if it is still alive
			 *
			 * The object is only read under the registry's mutex: RemoveControlBlock() clears the
			 * block under the same mutex before the object is deleted, so a non-null pointer read
			 * here points to memory that is still valid. The reference count may already be 0 (the
			 * last Ref is on its way into RemoveControlBlock()), in which case the lock fails.
			 *
			 * @param block A control block the caller holds a weak reference to
			 * @return True if the object is alive and its reference count was incremented for the caller
			 */
			bool TryLock(ControlBlock *block)
			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				const RefCounted *object = block->GetPtr();
				if (!object)
					return false;
				if (object->TryIncRefCount())
					return true;

				ExpireDying(block);
				return false;
			}

			/**
			 * @brief Gets the reference count of the object of a control block
			 * @param block A control block the caller holds a weak reference to
			 * @return The number of strong references, or 0 if the object has been destroyed
			 */
			uint32_t UseCount(ControlBlock *block)
			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				const RefCounted *object = block->GetPtr();
				if (!object)
					return 0;

				const uint32_t count = object->GetRefCount();
				if (count == 0)
					ExpireDying(block);
				return count;
			}

			/**
			 * @brief Remove the control block associated with the specified object
			 *
			 * This method is called when an object is being destroyed, before it is deleted. It sets
			 * the object pointer in the control block to nullptr to indicate that the object is no
			 * longer valid and releases the weak reference held for the living object; the control
			 * block is deleted when no WeakRef uses it any more. It also clears the object's
			 * m_HasWeakRefs, so its destructor doesn't look for the block again.
			 *
			 * @param object The RefCounted part of the object whose control block should be removed
			 */
			void RemoveControlBlock(const RefCounted *object)
			{
				if (!object || m_IsShutDown.load()) return;

				ControlBlock *block = nullptr;
				{
					std::lock_guard<std::mutex> lock(m_Mutex);
					auto it = m_Blocks.find(object);
					if (it == m_Blocks.end())
						return;

					block = it->second;
					m_Blocks.erase(it);
					block->SetPtr(nullptr);
					object->m_HasWeakRefs.store(false, std::memory_order_release);
				}

				// May delete the block, if no WeakRef uses it
				block->DecWeakCount();
			}

			/**
			 * @brief Shuts down the control block registry, releasing all resources.
			 *
			 * This method expires every control block managed by the registry (WeakRefs that still
			 * use one see their object as destroyed and delete the block when they are released)
			 * and prevents any further allocations or access to control blocks.
			 */
			void Shutdown()
			{
				std::unordered_map<const RefCounted *, ControlBlock *> blocks;
				{
					std::lock_guard<std::mutex> lock(m_Mutex);
					blocks.swap(m_Blocks);
					for (auto &[_, block] : blocks)
						block->SetPtr(nullptr);
					m_IsShutDown = true;
				}

				for (auto &[_, block] : blocks)
					block->DecWeakCount();
			}

			/**
			 * @brief Checks if the registry has been shut down.
			 * @return True if the registry is shut down, false otherwise.
			 */
			bool IsShutDown() const noexcept { return m_IsShutDown.load(); }

		private:
			/**
			 * @brief Clears a control block whose object's reference count was seen at 0 (call under m_Mutex)
			 *
			 * The last Ref is on its way into RemoveControlBlock(), and a count of 0 never goes up again
			 * (TryIncRefCount() refuses it), so the block can be cleared now. WeakRef::Expired() reads only
			 * the pointer, without the mutex, and so agrees with a failed lock from here on.
			 * RemoveControlBlock() still erases the block and releases the object's weak reference.
			 */
			static void ExpireDying(ControlBlock *block) noexcept { block->SetPtr(nullptr); }

			/**
			 * @brief Private constructor to enforce singleton pattern
			 */
			ControlBlockRegistry() = default;
			~ControlBlockRegistry()
			{
				for (auto &pair : m_Blocks)
				{
					delete pair.second;
				}
			}

			// Mapping from objects (their RefCounted parts) to their control blocks
			std::unordered_map<const RefCounted *, ControlBlock *> m_Blocks;

			// Mutex for thread-safe access to the registry
			std::mutex m_Mutex;

			// Indicates whether the registry is shut down
			std::atomic<bool> m_IsShutDown{false};
		};

	} // namespace Internal

	inline RefCounted::~RefCounted() { Internal::ControlBlockRegistry::ExpireWeakRefs(this); }

	// -----------------------------------------------------------

	template <typename T>
	class WeakRef;

	// ---------------------------------------------------------

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
		/** @brief Default constructor creates a null reference. */
		constexpr Ref() : m_Ptr(nullptr) {};

		/** @brief Constructor from nullptr creates a null reference. */
		constexpr Ref(std::nullptr_t) : m_Ptr(nullptr) {};

		/**
		 * @brief Constructor from raw pointer. Takes ownership of the object.
		 *
		 * Adds a reference to the object's own (intrusive) count, so a Ref made from a pointer to an object other
		 * Refs already own shares it with them. Implicit: passing `this` where a Ref is expected makes one.
		 *
		 * @warning In a constructor or destructor no Ref owns the object (yet, or any more): a Ref made from `this`
		 * there takes the count from 0 to 1, and its release brings it back to 0, which deletes the object (the one
		 * under construction, or a second time). So a temporary made from `this` (`this` passed as a `const Ref<T>&`
		 * argument, `WeakRef<T>(Ref<T>(this))`) must not be the only one. Pass a plain reference or pointer instead,
		 * or hold a count over the constructor with IncRefCount() / DecRefCount() (DecRefCount() never deletes), as
		 * Device's constructor does.
		 *
		 * @param instance Pointer to the object to manage.
		 */
		Ref(T *instance) : m_Ptr(instance)
		{
			static_assert(std::is_base_of_v<RefCounted, T>, "Class is not RefCounted!");

			InternalAddRef();
		};

		/**
		 * @brief Constructor from raw pointer. Takes ownership of the object.
		 *
		 * Adds a reference like Ref(T*), with the same warning about `this` in constructors and destructors.
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
		 * @param other The Ref to copy from.
		 */
		Ref(const Ref& other) noexcept : m_Ptr(other.m_Ptr) { InternalAddRef(); }

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
		 * @param other The Ref to move from.
		 */
		Ref(Ref&& other) noexcept : m_Ptr(other.m_Ptr) { other.m_Ptr = nullptr; }

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
		Ref(Ref<U>&& other) noexcept : m_Ptr(other.Get()) { other.m_Ptr = nullptr; }

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
		 * If the reference count reaches 0, the object is destroyed.
		 */
		~Ref() { InternalRelease(); }

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
			// Self-assignment or assigning same pointer: do nothing
			if (static_cast<const void*>(this) == static_cast<const void*>(&other) || m_Ptr == other.Get())
			{
				return *this;
			}

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
			// Self-move or moving same pointer: do nothing
			if (static_cast<void*>(this) == static_cast<void*>(&other) || m_Ptr == other.Get())
			{
				return *this;
			}

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
		 * @return Reference to the managed object.
		 */
		T& operator*() const noexcept
		{
			assert(m_Ptr && "Dereferencing null Ref");
			return *m_Ptr;
		}

		/**
		 * @brief Arrow operator. Provides access to the managed object's members.
		 * @return Pointer to the managed object.
		 */
		T* operator->() const noexcept
		{
			assert(m_Ptr && "Accessing member of null Ref");
			return m_Ptr;
		}

		/**
		 * @brief Boolean conversion operator. Checks if the Ref is not null.
		 * @return True if the Ref is not null, false otherwise.
		 */
		explicit operator bool() const noexcept { return m_Ptr != nullptr; }

		/**
		 * @brief Gets the raw pointer to the managed object.
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
		 * @return True if the reference count is 1, false otherwise or if null.
		 */
		bool IsUnique() const noexcept { return m_Ptr && m_Ptr->GetRefCount() == 1; }

		/**
		 * @brief Converts this Ref to a Ref of another type using static_cast.
		 * @tparam U The type to convert to.
		 * @return A Ref<U> managing the same object.
		 */
		template <typename U>
		Ref<U> As() const noexcept { return Ref<U>(static_cast<U*>(m_Ptr)); }

		/**
		 * @brief Converts this Ref to a Ref of another type using dynamic_cast.
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
			{
				return nullptr;
			}

			// Increment the ref count for the shared_ptr
			InternalAddRef();

			// Create a shared_ptr with a custom deleter that releases its reference like a Ref: when it is the
			// last owner, the WeakRefs to the object expire before it is deleted
			return std::shared_ptr<T>(m_Ptr, [](T* ptr)
			{
				if (ptr)
					ReleaseObject(ptr);
			});
		}

		/**
		 * @brief Swaps the contents of this Ref with another.
		 * @param other The Ref to swap with.
		 */
		void Swap(Ref& other) noexcept
		{
			std::swap(m_Ptr, other.m_Ptr);
		}

		/**
		 * @brief Checks if the Ref is not null.
		 * @return True if the Ref is not null, false otherwise.
		 */
		bool IsValid() const noexcept { return m_Ptr != nullptr; }

		/**
		 * @brief Gets the reference count of the managed object.
		 * @return The reference count, or 0 if the Ref is null.
		 */
		uint32_t UseCount() const noexcept { return m_Ptr ? m_Ptr->GetRefCount() : 0; }

		/**
		 * @brief Equality operator. Compares the managed objects.
		 * @param other The Ref to compare with.
		 * @return True if both Refs manage the same object, false otherwise.
		 */
		bool operator==(const Ref& other) const noexcept { return m_Ptr == other.m_Ptr; }

		/**
		 * @brief Inequality operator. Compares the managed objects.
		 * @param other The Ref to compare with.
		 * @return True if the Refs manage different objects, false otherwise.
		 */
		bool operator!=(const Ref& other) const noexcept { return m_Ptr != other.m_Ptr; }

		/**
		 * @brief Equality operator with nullptr. Checks if the Ref is null.
		 * @return True if the Ref is null, false otherwise.
		 */
		bool operator==(std::nullptr_t) const noexcept { return m_Ptr == nullptr; }

		/**
		 * @brief Inequality operator with nullptr. Checks if the Ref is not null.
		 * @return True if the Ref is not null, false otherwise.
		 */
		bool operator!=(std::nullptr_t) const noexcept { return m_Ptr != nullptr; }

	private:
		// Helper for SFINAE-based object comparison
		template <typename U>
		static auto HasEqualityOperator(int) -> decltype(std::declval<U>() == std::declval<U>(), std::true_type{});

		template <typename U>
		static std::false_type HasEqualityOperator(...);

		template <typename U>
		static bool CompareObjectsImpl(const U& a, const U& b, std::true_type)
		{
			return a == b;
		}

		template <typename U>
		static bool CompareObjectsImpl(const U& a, const U& b, std::false_type)
		{
			// No equality operator available
			return false; 
		}

	public:
		/**
		 * @brief Compares the managed objects for object equality.
		 *
		 * This method compares the objects themselves, not just the pointers.
		 * If `T` does not define `operator==`, the comparison returns false.
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

			// Use compile-time type trait detection instead of runtime function calls
			using HasEquality = decltype(HasEqualityOperator<T>(0));
			return CompareObjectsImpl(*m_Ptr, *other.m_Ptr, HasEquality{});
		 }

	private:
		// Pointer to the managed object
		mutable T* m_Ptr = nullptr;

		// Internal method to add a reference
		void InternalAddRef() const noexcept;

		// Internal method to release the reference
		void InternalRelease() noexcept;

		// Releases one reference to a non-null object: the last one expires its WeakRefs and deletes it
		static void ReleaseObject(T* ptr) noexcept;

		// Grant access to specific classes or functions
		template <typename U>
		friend class Ref;

		// Allow WeakRef to access m_Ptr
		template <typename U>
		friend class WeakRef;
	};

	// -------------------------------------------------------

	/**
	 * @brief A weak reference to an object managed by Ref<T>.
	 *
	 * WeakRef allows observing an object without affecting its lifetime.
	 * Unlike Ref<T>, WeakRef does not prevent the object from being destroyed.
	 *
	 * Like std::weak_ptr:
	 * - It is made from a Ref (or another WeakRef), never from a raw pointer, so it needs an object a Ref
	 *   already owns. Don't make one from `this` in a constructor: the temporary Ref that takes would delete the
	 *   object (see Ref(T*)). Make it once the object is owned, for example in an Init() called after CreateRef().
	 * - An object whose reference count is 0 counts as destroyed, even while its last release is still running:
	 *   Lock() returns null and UseCount() 0, and from then on Expired() is true. It never comes back.
	 *
	 * @tparam T The type of the managed object.
	 */
	template <typename T>
	class WeakRef
	{
	public:
		/** @brief Default constructor creates an empty weak reference. */
		constexpr WeakRef() noexcept = default;

		/** @brief Constructor from nullptr creates an empty weak reference. */
		constexpr WeakRef(std::nullptr_t) noexcept {}

		/**
		 * @brief Constructor from a Ref<T>. Creates a weak reference to the object managed by ref.
		 *
		 * The object must be owned by a Ref that outlives this call, not a temporary made from `this` in a
		 * constructor (see the class notes).
		 *
		 * @param ref The Ref<T> to observe.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef(const Ref<U>& ref) noexcept;

		/**
		 * @brief Copy constructor.
		 * @param other The WeakRef to copy from.
		 */
		WeakRef(const WeakRef& other) noexcept;

		/**
		 * @brief Copy constructor with type conversion.
		 * @tparam U The type of the other WeakRef.
		 * @param other The WeakRef to copy from.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef(const WeakRef<U>& other) noexcept;

		/**
		 * @brief Move constructor.
		 * @param other The WeakRef to move from.
		 */
		WeakRef(WeakRef&& other) noexcept;

		/**
		 * @brief Move constructor with type conversion.
		 * @tparam U The type of the other WeakRef.
		 * @param other The WeakRef to move from.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef(WeakRef<U>&& other) noexcept;

		~WeakRef();

		// Internal method to access the control block
		Internal::ControlBlock *GetControlBlock() const noexcept { return m_ControlBlock; }

		/**
		 * @brief Copy assignment operator.
		 * @param other The WeakRef to copy from.
		 * @return Reference to this WeakRef.
		 */
		WeakRef& operator=(const WeakRef& other) noexcept;

		/**
		 * @brief Copy assignment operator with type conversion.
		 * @tparam U The type of the other WeakRef.
		 * @param other The WeakRef to copy from.
		 * @return Reference to this WeakRef.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef& operator=(const WeakRef<U>& other) noexcept;

		/**
		 * @brief Move assignment operator.
		 * @param other The WeakRef to move from.
		 * @return Reference to this WeakRef.
		 */
		WeakRef& operator=(WeakRef&& other) noexcept;

		/**
		 * @brief Move assignment operator with type conversion.
		 * @tparam U The type of the other WeakRef.
		 * @param other The WeakRef to move from.
		 * @return Reference to this WeakRef.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef& operator=(WeakRef<U>&& other) noexcept;

		/**
		 * @brief Assignment operator from Ref<T>.
		 * @param ref The Ref<T> to observe.
		 * @return Reference to this WeakRef.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		WeakRef& operator=(const Ref<U>& ref) noexcept;

		/**
		 * @brief Assignment operator from nullptr.
		 * @return Reference to this WeakRef.
		 */
		WeakRef& operator=(std::nullptr_t) noexcept;

		/**
		 * @brief Checks if the WeakRef is expired.
		 * A WeakRef is expired if the object it points to has been destroyed.
		 * @return True if the WeakRef is expired, false otherwise.
		 */
		bool Expired() const noexcept;

		/**
		 * @brief Attempts to get a strong reference to the object.
		 * @return A Ref<T> to the object, or an empty Ref<T> if the object has been destroyed.
		 */
		Ref<T> Lock() const noexcept;

		/** @brief Resets the WeakRef. */
		void Reset() noexcept;

		/**
		 * @brief Gets the reference count of the object.
		 * @return The number of Ref<T> instances that share ownership of the object, or 0 if the WeakRef is expired.
		 */
		uint32_t UseCount() const noexcept;

		/**
		 * @brief Equality operator.
		 * @param other The WeakRef to compare with.
		 * @return True if both WeakRefs observe the same object, false otherwise.
		 */
		bool operator==(const WeakRef& other) const noexcept;

		/**
		 * @brief Inequality operator.
		 * @param other The WeakRef to compare with.
		 * @return True if the WeakRefs observe different objects, false otherwise.
		 */
		bool operator!=(const WeakRef& other) const noexcept;

	private:
		// The object's control block (one per object, shared by WeakRefs of every type), or nullptr if empty
		Internal::ControlBlock* m_ControlBlock = nullptr;

		// The object as a T. Like std::weak_ptr's stored pointer, it is only used once a lock proved the object
		// alive: after the object is destroyed it dangles. It may be at another address than the RefCounted part
		// the control block stores (multiple or virtual inheritance), which is why the WeakRef keeps it.
		T* m_Ptr = nullptr;

		// Allow all WeakRef instantiations to access each other's private members
		template <typename> friend class WeakRef;

		// Allow Ref<T> to access m_ControlBlock
		template <typename U>
		friend class Ref;
	};

	// -------------------------------------------------------

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
		static_assert(std::is_constructible_v<T, Args &&...>, "Type T is not constructible with the provided arguments. Check that T's constructor matches Args.");
		return Ref<T>(new T(std::forward<Args>(args)...));
	}

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
			ReleaseObject(m_Ptr);
			m_Ptr = nullptr;
		}
	}

	/**
	 * @brief Releases one reference to an object, deleting it if that was the last one.
	 *
	 * Every last release goes through here (Ref<T> of any T, and the std::shared_ptr deleter of
	 * ToSharedPtr()). The object's single control block is found by its RefCounted part, so the
	 * WeakRefs to it expire whatever their static type and whatever the static type of the last
	 * owner. Lock-free unless the object ever had a WeakRef.
	 *
	 * @tparam T The type of the reference-counted object
	 * @param ptr The object, not null
	 */
	template <typename T>
	void Ref<T>::ReleaseObject(T* ptr) noexcept
	{
		if (ptr->DecRefCount() == 0)
		{
			Internal::ControlBlockRegistry::ExpireWeakRefs(ptr);
			delete ptr;
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
	 * 2. Retrieves or creates the object's control block (the same one for every static type)
	 * 3. Increments the weak reference count in the control block
	 *
	 * @tparam U The type of the source reference, must be convertible to T
	 * @param ref The strong reference to create a weak reference from
	 * @note - This constructor enables implicit conversion from Ref<U> to WeakRef<T>
	 *       when U is convertible to T
	 */
	template <typename T>
	template <typename U, typename>
	WeakRef<T>::WeakRef(const Ref<U> &ref) noexcept
	{
		if (ref)
		{
			T *ptr = ref.Get();
			m_ControlBlock = Internal::ControlBlockRegistry::GetInstance().AcquireControlBlock(ptr);
			if (m_ControlBlock)
				m_Ptr = ptr;
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
	 * @note - This maintains proper reference counting without affecting the
	 *       lifetime of the referenced object
	 */
	template <typename T>
	WeakRef<T>::WeakRef(const WeakRef &other) noexcept : m_ControlBlock(other.m_ControlBlock), m_Ptr(other.m_Ptr)
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
	 * 1. Locks the source WeakRef: converting a U* to a T* may read the object (a virtual base), which
	 *    must not be destroyed meanwhile
	 * 2. If it is alive, shares the source's control block (one per object, whatever the static type)
	 *    and keeps the object as a T*
	 *
	 * An expired source gives an empty WeakRef.
	 *
	 * @tparam U Source type that is convertible to T
	 * @param other The source WeakRef<U> to convert from
	 * @note - This constructor only participates in overload resolution if U* is convertible to T*
	 */
	template <typename T>
	template <typename U, typename>
	WeakRef<T>::WeakRef(const WeakRef<U> &other) noexcept
	{
		Internal::ControlBlock *block = other.m_ControlBlock;
		if (!block)
			return;

		if (const Ref<U> strong = other.Lock())
		{
			block->IncWeakCount();
			m_ControlBlock = block;
			m_Ptr = strong.Get();
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
	WeakRef<T>::WeakRef(WeakRef &&other) noexcept : m_ControlBlock(other.m_ControlBlock), m_Ptr(other.m_Ptr)
	{
		other.m_ControlBlock = nullptr;
		other.m_Ptr = nullptr;
	}

	/**
	 * @brief Move conversion constructor for weak references of different but compatible types.
	 *
	 * This constructor moves a WeakRef<U> to a WeakRef<T> where U is convertible to T
	 * (typically through inheritance relationships). Unlike the regular move constructor,
	 * this constructor performs a type conversion, which needs the object alive.
	 *
	 * The implementation:
	 * 1. Converts like the copy conversion constructor (locking the source and sharing its control block)
	 * 2. Resets the source WeakRef, releasing its weak reference
	 *
	 * @tparam U Source type that is convertible to T
	 * @param other The source WeakRef<U> to move from
	 * @note - This constructor only participates in overload resolution if U* is convertible to T*
	 *       (enforced by the SFINAE template parameter)
	 */
	template <typename T>
	template <typename U, typename>
	WeakRef<T>::WeakRef(WeakRef<U> &&other) noexcept : WeakRef(static_cast<const WeakRef<U> &>(other))
	{
		other.Reset();
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
	 * @note - This operator maintains proper reference counting without affecting the
	 *       lifetime of the referenced object
	 */
	template <typename T>
	WeakRef<T> &WeakRef<T>::operator=(const WeakRef &other) noexcept
	{
		if (this != &other)
		{
			// Take the new reference before releasing the old one
			Internal::ControlBlock *block = other.m_ControlBlock;
			if (block)
			{
				block->IncWeakCount();
			}

			if (m_ControlBlock)
			{
				m_ControlBlock->DecWeakCount();
			}

			m_ControlBlock = block;
			m_Ptr = other.m_Ptr;
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
	 * 1. Converts the source like the copy conversion constructor (locking it while its pointer is
	 *    converted, and sharing its control block)
	 * 2. Releases this WeakRef's current control block (if any) and takes the converted one
	 *
	 * @tparam U Source type that is convertible to T
	 * @param other The source WeakRef<U> to assign from
	 * @return A reference to this WeakRef after the assignment
	 * @note - This operator only participates in overload resolution if U* is convertible to T*
	 *       (enforced by the SFINAE template parameter)
	 */
	template <typename T>
	template <typename U, typename>
	WeakRef<T> &WeakRef<T>::operator=(const WeakRef<U> &other) noexcept
	{
		return *this = WeakRef<T>(other);
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
			m_Ptr = other.m_Ptr;
			other.m_ControlBlock = nullptr;
			other.m_Ptr = nullptr;
		}

		return *this;
	}

	/**
	 * @brief Move conversion assignment operator for weak references of different but compatible types.
	 *
	 * This operator moves a WeakRef<U> to a WeakRef<T> where U is convertible to T
	 * (typically through inheritance relationships). Unlike the regular move assignment operator,
	 * this operator performs a type conversion, which needs the object alive.
	 *
	 * The implementation:
	 * 1. Converts like the move conversion constructor (sharing the source's control block, and the
	 *    source WeakRef is reset)
	 * 2. Releases this WeakRef's current control block (if any) and takes the converted one
	 *
	 * @tparam U Source type that is convertible to T
	 * @param other The source WeakRef<U> to move from
	 * @return A reference to this WeakRef after the assignment
	 * @note - This operator only participates in overload resolution if U* is convertible to T*
	 *       (enforced by the SFINAE template parameter)
	 */
	template <typename T>
	template <typename U, typename>
	WeakRef<T> &WeakRef<T>::operator=(WeakRef<U> &&other) noexcept
	{
		return *this = WeakRef<T>(std::move(other));
	}

	/**
	 * @brief Assignment operator that assigns a strong reference to a weak reference.
	 *
	 * This operator assigns a strong reference (Ref<U>) to this weak reference (WeakRef<T>).
	 * It properly maintains weak reference counting through the control block system.
	 *
	 * The implementation:
	 * 1. If the source reference is valid, retrieves or creates a control block for the referenced object
	 *    and takes a weak reference to it
	 * 2. Releases the current control block (if any) and keeps the new one
	 *
	 * @tparam U Source type that is convertible to T
	 * @param ref The source Ref<U> to assign from
	 * @return A reference to this WeakRef after the assignment
	 * @note - This operator only participates in overload resolution if U* is convertible to T*
	 *       (enforced by the SFINAE template parameter)
	 */
	template <typename T>
	template <typename U, typename>
	WeakRef<T> &WeakRef<T>::operator=(const Ref<U> &ref) noexcept
	{
		Internal::ControlBlock *block = nullptr;
		T *ptr = nullptr;
		if (ref)
		{
			ptr = ref.Get();
			block = Internal::ControlBlockRegistry::GetInstance().AcquireControlBlock(ptr);
			if (!block)
				ptr = nullptr;
		}

		if (m_ControlBlock)
		{
			m_ControlBlock->DecWeakCount();
		}

		m_ControlBlock = block;
		m_Ptr = ptr;
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
		Reset();
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
	 * Takes no lock: it reads only the control block's pointer, which is cleared once and for
	 * all when the object is released. It agrees with Lock(): it never says a live object is
	 * expired, and once Lock() or UseCount() has failed or returned 0 it returns true (they
	 * clear the pointer of an object whose count they see at 0). While the last Ref is being
	 * released on another thread it may still return false for a moment; as with
	 * std::weak_ptr::expired(), "not expired" is only a hint, and Lock() is the reliable test.
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
	 * If the object of the WeakRef points to is still alive (not expired), it creates
	 * and returns a new Ref<T> pointing to that object, which increments the reference
	 * count of the object. If the object has been destroyed, it returns an empty Ref<T>.
	 *
	 * Safe against another thread releasing the last Ref at the same time: the reference
	 * count is only incremented if it is not 0 yet, and only under the registry's mutex,
	 * which the releasing thread takes before the object is deleted (see
	 * ControlBlockRegistry::TryLock), so the lock either keeps the object alive or fails.
	 *
	 * @tparam T The type of the referenced object
	 * @return Ref<T> A strong reference to the object if it's still alive, or an empty reference otherwise
	 */
	template <typename T>
	Ref<T> WeakRef<T>::Lock() const noexcept
	{
		// A destroyed object never comes back, so an expired WeakRef needs no lock
		Ref<T> ref;
		if (!m_ControlBlock || m_ControlBlock->GetPtr() == nullptr) {
			return ref;
		}

		// TryLock already incremented the count for the returned Ref. The object is alive, so this
		// WeakRef's pointer of type T to it is valid (the block only knows its RefCounted part).
		if (Internal::ControlBlockRegistry::GetInstance().TryLock(m_ControlBlock))
			ref.m_Ptr = m_Ptr;
		return ref;
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
	 * @note - This method is often used to explicitly release resources before the WeakRef
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
		m_Ptr = nullptr;
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
			// Read under the registry's mutex: the object may be being destroyed on another thread
			return Internal::ControlBlockRegistry::GetInstance().UseCount(m_ControlBlock);
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
	 * It is implemented by negating the result of the equality operator.
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
	 * The implementation locks the weak reference (WeakRef::Lock()), which is safe against
	 * another thread releasing the last strong reference at the same time.
	 *
	 * @param weak The weak reference to convert to a strong reference
	 * @note - This enables safe conversion from WeakRef<T> to Ref<T>, preventing access to destroyed objects
	 */
	template <typename T>
	inline Ref<T>::Ref(const WeakRef<T> &weak) noexcept : Ref(weak.Lock())
	{
	}

	/**
	 * @brief Equality operator for comparing objects through two Ref instances.
	 *
	 * This operator compares the objects managed by two Ref instances for equality.
	 * It first checks if the Ref instances point to the same address (identity comparison).
	 * If not, it checks if both Refs are valid, and if so, compares the objects using
	 * their operator== implementation.
	 *
	 * This operator requires that type T has a valid operator== defined.
	 *
	 * @tparam T The type of objects managed by the Ref instances
	 * @param lhs The left-hand side Ref for comparison
	 * @param rhs The right-hand side Ref for comparison
	 * @return True if both Refs manage equal objects, false otherwise
	 */
	template <typename T>
	bool operator==(const Ref<T> &lhs, const Ref<T> &rhs) noexcept
	{
		if (lhs.Get() == rhs.Get())
			return true;

		if (!lhs || !rhs)
			return false;

		return *lhs == *rhs;
	}

	/**
	 * @brief Inequality operator for comparing objects through two Ref instances.
	 *
	 * This operator compares the objects managed by two Ref instances for inequality.
	 * It is implemented by negating the result of the equality operator.
	 *
	 * This operator requires that type T has a valid operator== defined.
	 *
	 * @tparam T The type of objects managed by the Ref instances
	 * @param lhs The left-hand side Ref for comparison
	 * @param rhs The right-hand side Ref for comparison
	 * @return True if the Refs manage different objects, false if they manage equal objects
	 */
	template <typename T>
	bool operator!=(const Ref<T> &lhs, const Ref<T> &rhs) noexcept
	{
		return !(lhs == rhs);
	}

	/**
	 * @brief Equality operator for comparing objects through two WeakRef instances.
	 *
	 * This operator compares the objects managed by two WeakRef instances for equality.
	 * It first checks if both WeakRefs are not expired and refer to valid objects.
	 * If so, it locks the WeakRefs to obtain strong references (Refs) and compares
	 * the objects using the operator== implementation for Ref<T>.
	 *
	 * This operator requires that type T has a valid operator== defined.
	 *
	 * @tparam T The type of objects managed by the WeakRef instances
	 * @param lhs The left-hand side WeakRef for comparison
	 * @param rhs The right-hand side WeakRef for comparison
	 * @return True if both WeakRefs manage equal objects, false otherwise
	 */
	template <typename T>
	bool operator==(const WeakRef<T> &lhs, const WeakRef<T> &rhs) noexcept
	{
		if (lhs.Expired() || rhs.Expired())
			return false;

		auto lhsLocked = lhs.Lock();
		auto rhsLocked = rhs.Lock();

		return lhsLocked == rhsLocked;
	}

	/**
	 * @brief Inequality operator for comparing objects through two WeakRef instances.
	 *
	 * This operator compares the objects managed by two WeakRef instances for inequality.
	 * It is implemented by negating the result of the equality operator.
	 *
	 * This operator requires that type T has a valid operator== defined.
	 *
	 * @tparam T The type of objects managed by the WeakRef instances
	 * @param lhs The left-hand side WeakRef for comparison
	 * @param rhs The right-hand side WeakRef for comparison
	 * @return True if the WeakRefs manage different objects, false if they manage equal objects
	 */
	template <typename T>
	bool operator!=(const WeakRef<T> &lhs, const WeakRef<T> &rhs) noexcept
	{
		return !(lhs == rhs);
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

}

// -------------------------------------------------------
