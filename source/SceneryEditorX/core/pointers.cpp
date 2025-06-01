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
#include <mutex>
#include <SceneryEditorX/core/pointers.h>
#include <unordered_set>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    namespace Internal
    {
        /// Control block for WeakRef implementation
        template <typename T>
        class ControlBlock
        {
        public:
            explicit ControlBlock(T* ptr) noexcept : m_Ptr(ptr), m_WeakCount(0) {}

            void IncWeakCount() noexcept
            {
                ++m_WeakCount;
            }

            void DecWeakCount() noexcept
            {
                if (--m_WeakCount == 0 && m_Ptr == nullptr)
                {
                    delete this;
                }
            }

            T* GetPtr() const noexcept
            {
                return m_Ptr;
            }

            void SetPtr(T* ptr) noexcept
            {
                m_Ptr = ptr;
            }

        private:
            T* m_Ptr;
            std::atomic<uint32_t> m_WeakCount;
        };
        
        /// Global registry to manage control blocks
        template <typename T>
        class ControlBlockRegistry
        {
        public:
            static ControlBlockRegistry& GetInstance()
            {
                static ControlBlockRegistry instance;
                return instance;
            }
            
            Internal::ControlBlock<T>* GetControlBlock(T* ptr)
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
            
            void RemoveControlBlock(T* ptr)
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
            ControlBlockRegistry() = default;
            ~ControlBlockRegistry()
            {
                for (auto& [ptr, block] : m_Blocks)
                {
                    delete block;
                }
            }
            
            std::unordered_map<T*, Internal::ControlBlock<T>*> m_Blocks;
            std::mutex m_Mutex;
        };
    }

    /// ---------------------------------------------------------

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
        WeakRef(const Ref<U>& ref) noexcept
        {
            if (ref)
            {
                m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(static_cast<T*>(ref.Get()));
                if (m_ControlBlock)
                    m_ControlBlock->IncWeakCount();
            }
        }
        
        /**
         * @brief Copy constructor.
         * 
         * @param other The WeakRef to copy from.
         */
        WeakRef(const WeakRef& other) noexcept : m_ControlBlock(other.m_ControlBlock)
        {
            if (m_ControlBlock)
                m_ControlBlock->IncWeakCount();
        }
        
        /**
         * @brief Copy constructor with type conversion.
         * 
         * @tparam U The type of the other WeakRef.
         * @param other The WeakRef to copy from.
         */
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        WeakRef(const WeakRef<U>& other) noexcept
        {
            if (other.m_ControlBlock)
            {
                m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(static_cast<T*>(other.m_ControlBlock->GetPtr()));
                if (m_ControlBlock)
                    m_ControlBlock->IncWeakCount();
            }
        }
        
        /**
         * @brief Move constructor.
         * 
         * @param other The WeakRef to move from.
         */
        WeakRef(WeakRef&& other) noexcept : m_ControlBlock(other.m_ControlBlock)
        {
            other.m_ControlBlock = nullptr;
        }
        
        /**
         * @brief Move constructor with type conversion.
         * 
         * @tparam U The type of the other WeakRef.
         * @param other The WeakRef to move from.
         */
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        WeakRef(WeakRef<U>&& other) noexcept
        {
            if (other.m_ControlBlock)
            {
                m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(static_cast<T*>(other.m_ControlBlock->GetPtr()));
                other.m_ControlBlock = nullptr;
            }
        }
        
        /**
         * @brief Destructor.
         */
        ~WeakRef()
        {
            if (m_ControlBlock)
                m_ControlBlock->DecWeakCount();
        }
        
        /**
         * @brief Copy assignment operator.
         * 
         * @param other The WeakRef to copy from.
         * @return Reference to this WeakRef.
         */
        WeakRef& operator=(const WeakRef& other) noexcept
        {
            if (this != &other)
            {
                if (m_ControlBlock)
                    m_ControlBlock->DecWeakCount();

                m_ControlBlock = other.m_ControlBlock;
                
                if (m_ControlBlock)
                    m_ControlBlock->IncWeakCount();
            }
            return *this;
        }
        
        /**
         * @brief Copy assignment operator with type conversion.
         * 
         * @tparam U The type of the other WeakRef.
         * @param other The WeakRef to copy from.
         * @return Reference to this WeakRef.
         */
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        WeakRef& operator=(const WeakRef<U>& other) noexcept
        {
            if (m_ControlBlock)
            {
                m_ControlBlock->DecWeakCount();
                m_ControlBlock = nullptr;
            }
            
            if (other.m_ControlBlock)
            {
                m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(static_cast<T*>(other.m_ControlBlock->GetPtr()));
                if (m_ControlBlock)
                {
                    m_ControlBlock->IncWeakCount();
                }
            }
            
            return *this;
        }
        
        /**
         * @brief Move assignment operator.
         * 
         * @param other The WeakRef to move from.
         * @return Reference to this WeakRef.
         */
        WeakRef& operator=(WeakRef&& other) noexcept
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
         * @brief Move assignment operator with type conversion.
         * 
         * @tparam U The type of the other WeakRef.
         * @param other The WeakRef to move from.
         * @return Reference to this WeakRef.
         */
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        WeakRef& operator=(WeakRef<U>&& other) noexcept
        {
            if (m_ControlBlock)
            {
                m_ControlBlock->DecWeakCount();
                m_ControlBlock = nullptr;
            }
            
            if (other.m_ControlBlock)
            {
                m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(static_cast<T*>(other.m_ControlBlock->GetPtr()));
                other.m_ControlBlock = nullptr;
            }
            
            return *this;
        }
        
        /**
         * @brief Assignment operator from Ref<T>.
         * 
         * @param ref The Ref<T> to observe.
         * @return Reference to this WeakRef.
         */
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        WeakRef& operator=(const Ref<U>& ref) noexcept
        {
            if (m_ControlBlock)
            {
                m_ControlBlock->DecWeakCount();
                m_ControlBlock = nullptr;
            }
            
            if (ref)
            {
                m_ControlBlock = Internal::ControlBlockRegistry<T>::GetInstance().GetControlBlock(static_cast<T*>(ref.Get()));
                if (m_ControlBlock)
                {
                    m_ControlBlock->IncWeakCount();
                }
            }
            
            return *this;
        }
        
        /**
         * @brief Assignment operator from nullptr.
         * 
         * @return Reference to this WeakRef.
         */
        WeakRef& operator=(std::nullptr_t) noexcept
        {
            if (m_ControlBlock)
            {
                m_ControlBlock->DecWeakCount();
                m_ControlBlock = nullptr;
            }
            return *this;
        }
        
        /**
         * @brief Checks if the WeakRef is expired.
         * 
         * A WeakRef is expired if the object it points to has been destroyed.
         * 
         * @return True if the WeakRef is expired, false otherwise.
         */
        bool Expired() const noexcept
        {
            return !m_ControlBlock || m_ControlBlock->GetPtr() == nullptr;
        }
        
        /**
         * @brief Attempts to get a strong reference to the object.
         * 
         * @return A Ref<T> to the object, or an empty Ref<T> if the object has been destroyed.
         */
        Ref<T> Lock() const noexcept
        {
            if (!m_ControlBlock || m_ControlBlock->GetPtr() == nullptr)
                return Ref<T>();
            
            return Ref<T>(m_ControlBlock->GetPtr());
        }
        
        /**
         * @brief Resets the WeakRef.
         */
        void Reset() noexcept
        {
            if (m_ControlBlock)
            {
                m_ControlBlock->DecWeakCount();
                m_ControlBlock = nullptr;
            }
        }
        
        /**
         * @brief Gets the reference count of the object.
         * 
         * @return The number of Ref<T> instances that share ownership of the object, or 0 if the WeakRef is expired.
         */
        uint32_t UseCount() const noexcept
        {
            if (m_ControlBlock && m_ControlBlock->GetPtr())
            {
                return m_ControlBlock->GetPtr()->GetRefCount();
            }
            return 0;
        }
        
        /**
         * @brief Equality operator.
         * 
         * @param other The WeakRef to compare with.
         * @return True if both WeakRefs observe the same object, false otherwise.
         */
        bool operator==(const WeakRef& other) const noexcept
        {
            if (m_ControlBlock == other.m_ControlBlock)
                return true;
            
            if (!m_ControlBlock || !other.m_ControlBlock)
                return false;
            
            return m_ControlBlock->GetPtr() == other.m_ControlBlock->GetPtr();
        }
        
        /**
         * @brief Inequality operator.
         * 
         * @param other The WeakRef to compare with.
         * @return True if the WeakRefs observe different objects, false otherwise.
         */
        bool operator!=(const WeakRef& other) const noexcept
        {
            return !(*this == other);
        }
        
    private:
        Internal::ControlBlock<T>* m_ControlBlock = nullptr;
        
        /// Allow Ref<T> to access m_ControlBlock
        template <typename U>
        friend class Ref;
    };

    /// ---------------------------------------------------------

    /// Implementation of Ref<T> member function from WeakRef
    template <typename T>
    Ref<T>::Ref(const WeakRef<T>& weak) noexcept
        : m_Ptr(weak.m_ControlBlock ? weak.m_ControlBlock->GetPtr() : nullptr)
    {
        InternalAddRef();
    }

    /// Helper functions for the Ref<T> class
    template <typename T>
    void Ref<T>::InternalAddRef() const noexcept
    {
        if (m_Ptr)
        {
            m_Ptr->IncRefCount();
        }
    }

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
