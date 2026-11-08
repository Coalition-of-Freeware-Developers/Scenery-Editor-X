/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* WeakRefTest.cpp
* -------------------------------------------------------
* Tests for the WeakRef weak pointer implementation
* -------------------------------------------------------
*/
#include <catch2/catch_all.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <SceneryEditorX/utils/pointers.h>
#include <string>
#include <thread>
#include <vector>

/// -------------------------------------------------------------------

namespace SceneryEditorX
{
    namespace Tests::PointerTests
    {

        /// -------------------------------------------------------------------

		///< Test classes for weak reference testing
		class WeakTestObject : public RefCounted
		{
		public:
		    explicit WeakTestObject(const int value = 42) : m_Value(value) {}
		    virtual ~WeakTestObject() override = default;

		    int GetValue() const { return m_Value; }
		    void SetValue(const int value) { m_Value = value; }

		    static int GetDestroyCount() { return s_DestroyCount; }
		    static void ResetDestroyCount() { s_DestroyCount = 0; }

		protected:
		    virtual void OnDestroy() { ++s_DestroyCount; }

		private:
		    int m_Value;
		    static int s_DestroyCount;
		};

        /// -------------------------------------------------------------------

		int WeakTestObject::s_DestroyCount = 0;

        /// -------------------------------------------------------------------

		class DerivedWeakTestObject : public WeakTestObject
		{
		public:
		    explicit DerivedWeakTestObject(int value = 100, const std::string& name = "derived") : WeakTestObject(value), m_Name(name) {}

		    const std::string& GetName() const { return m_Name; }
		    void SetName(const std::string& name) { m_Name = name; }

		private:
		    std::string m_Name;
		};

        /// -------------------------------------------------------------------

		/**
		 * @brief Always placed in the same fixed slot, so each new object takes the address of the one before it.
		 *
		 * Heaps often do the same. A WeakRef whose control block was not expired when its object died would then
		 * see the next object as its own: these objects turn that use-after-free into a test failure even without
		 * a sanitizer. Only one can be alive at a time.
		 */
		class SlotWeakTestObject : public DerivedWeakTestObject
		{
		public:
		    using DerivedWeakTestObject::DerivedWeakTestObject;
		    ~SlotWeakTestObject() override { ++s_Destroyed; }

		    static void *operator new(std::size_t size)
		    {
		        if (size > sizeof(s_Slot) || s_SlotInUse)
		            throw std::bad_alloc();
		        s_SlotInUse = true;
		        return s_Slot;
		    }
		    static void operator delete(void *) noexcept { s_SlotInUse = false; }

		    static const void *SlotAddress() { return s_Slot; }

		    static inline int s_Destroyed = 0;

		private:
		    alignas(std::max_align_t) static inline unsigned char s_Slot[256];
		    static inline bool s_SlotInUse = false;
		};

		static_assert(sizeof(SlotWeakTestObject) <= 256 && alignof(SlotWeakTestObject) <= alignof(std::max_align_t));

        /// -------------------------------------------------------------------

		/// Comes before WeakTestObject in OffsetWeakTestObject, so its RefCounted part is not at the start of the object
		struct WeakTestHeader
		{
		    virtual ~WeakTestHeader() = default;
		    std::int64_t m_Header = 0x5EDC;
		};

		/// RefCounted at a non-zero offset: a WeakTestObject* to it is a different address than the object's own
		class OffsetWeakTestObject : public WeakTestHeader, public DerivedWeakTestObject
		{
		public:
		    using DerivedWeakTestObject::DerivedWeakTestObject;
		    ~OffsetWeakTestObject() override { ++s_Destroyed; }

		    static inline int s_Destroyed = 0;
		};

        /// -------------------------------------------------------------------

		/// RefCounted as a virtual base of both sides of a diamond: one RefCounted part, reached through either side
		class VirtualLeftWeakTestObject : public virtual RefCounted
		{
		public:
		    int m_Left = 1;
		};

		class VirtualRightWeakTestObject : public virtual RefCounted
		{
		public:
		    int m_Right = 2;
		};

		class VirtualDiamondWeakTestObject final : public VirtualLeftWeakTestObject, public VirtualRightWeakTestObject
		{
		public:
		    ~VirtualDiamondWeakTestObject() override { ++s_Destroyed; }

		    static inline int s_Destroyed = 0;
		};

        /// -------------------------------------------------------------------

		/// A WeakRef to a destroyed object: expired, never locks, and counts no owners
		template <typename T>
		void RequireExpired(const WeakRef<T> &weak)
		{
		    REQUIRE(weak.Expired());
		    REQUIRE_FALSE(weak.Lock());
		    REQUIRE(weak.UseCount() == 0);
		}

		template <typename... W>
		void RequireAllExpired(const W &...weak)
		{
		    (RequireExpired(weak), ...);
		}

		/// True if both WeakRefs use one control block (they may observe the object as different types)
		template <typename A, typename B>
		bool SameControlBlock(const WeakRef<A> &a, const WeakRef<B> &b)
		{
		    return static_cast<const void *>(a.GetControlBlock()) == static_cast<const void *>(b.GetControlBlock());
		}

		/**
		 * @brief Puts a new SlotWeakTestObject where the destroyed one was and checks the expired WeakRefs ignore it.
		 *
		 * A WeakRef to the new object must get a control block of its own, none of the expired ones.
		 */
		template <typename... W>
		void RequireStayExpiredWhenTheSlotIsReused(const W &...weak)
		{
		    const auto next = CreateRef<SlotWeakTestObject>(999, "next");
		    REQUIRE(static_cast<const void *>(next.Get()) == SlotWeakTestObject::SlotAddress());

		    const WeakRef<WeakTestObject> weakNext = next;
		    REQUIRE(weakNext.Lock().Get() == next.Get());
		    const bool sharesAnExpiredBlock = (SameControlBlock(weakNext, weak) || ...);
		    REQUIRE_FALSE(sharesAnExpiredBlock);
		    RequireAllExpired(weak...);
		}

        /// -------------------------------------------------------------------

		///< Test fixture for weak reference tests
		class WeakRefTestFixture
		{
		public:
		    WeakRefTestFixture()
		    {
		        WeakTestObject::ResetDestroyCount();
		        SlotWeakTestObject::s_Destroyed = 0;
		        OffsetWeakTestObject::s_Destroyed = 0;
		        VirtualDiamondWeakTestObject::s_Destroyed = 0;
		    }
		    ~WeakRefTestFixture() { WeakTestObject::ResetDestroyCount(); }
		};
    }
}

using namespace SceneryEditorX::Tests::PointerTests;
using SceneryEditorX::CreateRef;
using SceneryEditorX::Ref;
using SceneryEditorX::WeakRef;

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Basic Construction and Functionality", "[SmartPointers][WeakRef][Basic]")
{
    SECTION("Default construction creates null weak reference")
    {
        SceneryEditorX::WeakRef<WeakTestObject> weakRef;
        REQUIRE(weakRef.Expired());
        REQUIRE(!weakRef.Lock());
    }

    SECTION("Construction from strong reference")
    {
        auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>(123);
        SceneryEditorX::WeakRef<WeakTestObject> weakRef = strongRef;

        REQUIRE(!weakRef.Expired());
        auto locked = weakRef.Lock();
        REQUIRE(locked);
        REQUIRE(locked.Get() == strongRef.Get());
        REQUIRE(locked->GetValue() == 123);
    }

    SECTION("Weak reference does not affect reference count")
    {
        auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>();
        REQUIRE(strongRef.UseCount() == 1);

        SceneryEditorX::WeakRef<WeakTestObject> weakRef = strongRef;
        REQUIRE(strongRef.UseCount() == 1); // Weak ref doesn't increase count

        auto locked = weakRef.Lock();
        REQUIRE(strongRef.UseCount() == 2); // Locked ref increases count
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Expiration Detection", "[SmartPointers][WeakRef][Expiration]")
{
    SECTION("Weak reference expires when strong reference is destroyed")
    {
        SceneryEditorX::WeakRef<WeakTestObject> weakRef;

        {
            auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>(456);
            weakRef = strongRef;
            REQUIRE(!weakRef.Expired());
        } // strongRef goes out of scope

        REQUIRE(weakRef.Expired());
        REQUIRE(!weakRef.Lock());
    }

    SECTION("Multiple weak references expire together")
    {
        SceneryEditorX::WeakRef<WeakTestObject> weakRef1;
        SceneryEditorX::WeakRef<WeakTestObject> weakRef2;

        {
            auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>(789);
            weakRef1 = strongRef;
            weakRef2 = strongRef;

            REQUIRE(!weakRef1.Expired());
            REQUIRE(!weakRef2.Expired());
        } // strongRef goes out of scope

        REQUIRE(weakRef1.Expired());
        REQUIRE(weakRef2.Expired());
        REQUIRE(!weakRef1.Lock());
        REQUIRE(!weakRef2.Lock());
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Copy Operations", "[SmartPointers][WeakRef][Copy]")
{
    SECTION("Copy construction from weak reference")
    {
        auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>(101112);
        SceneryEditorX::WeakRef<WeakTestObject> original = strongRef;
        SceneryEditorX::WeakRef<WeakTestObject> copy = original;

        REQUIRE(!original.Expired());
        REQUIRE(!copy.Expired());

        auto locked1 = original.Lock();
        auto locked2 = copy.Lock();

        REQUIRE(locked1);
        REQUIRE(locked2);
        REQUIRE(locked1.Get() == locked2.Get());
    }

    SECTION("Copy assignment from weak reference")
    {
        auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>(131415);
        SceneryEditorX::WeakRef<WeakTestObject> original = strongRef;
        SceneryEditorX::WeakRef<WeakTestObject> copy;

        copy = original;

        REQUIRE(!original.Expired());
        REQUIRE(!copy.Expired());

        auto locked1 = original.Lock();
        auto locked2 = copy.Lock();

        REQUIRE(locked1.Get() == locked2.Get());
    }

    SECTION("Copy from expired weak reference")
    {
        SceneryEditorX::WeakRef<WeakTestObject> original;
        SceneryEditorX::WeakRef<WeakTestObject> copy;

        {
            auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>();
            original = strongRef;
        } // strongRef expires

        copy = original; // Copy expired reference

        REQUIRE(original.Expired());
        REQUIRE(copy.Expired());
        REQUIRE(!copy.Lock());
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Move Operations", "[SmartPointers][WeakRef][Move]")
{
    SECTION("Move construction transfers weak reference")
    {
        auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>(161718);
        SceneryEditorX::WeakRef<WeakTestObject> original = strongRef;
        SceneryEditorX::WeakRef<WeakTestObject> moved = std::move(original);

        REQUIRE(!moved.Expired());
        auto locked = moved.Lock();
        REQUIRE(locked);
        REQUIRE(locked->GetValue() == 161718);
    }

    SECTION("Move assignment transfers weak reference")
    {
        auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>(192021);
        SceneryEditorX::WeakRef<WeakTestObject> original = strongRef;
        SceneryEditorX::WeakRef<WeakTestObject> moved;

        moved = std::move(original);

        REQUIRE(!moved.Expired());
        auto locked = moved.Lock();
        REQUIRE(locked);
        REQUIRE(locked->GetValue() == 192021);
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Type Conversions", "[SmartPointers][WeakRef][Conversion]")
{
    SECTION("Create weak reference from derived type")
    {
        auto derived = SceneryEditorX::CreateRef<DerivedWeakTestObject>(222324, "test");
        SceneryEditorX::WeakRef<WeakTestObject> weakBase = derived.As<WeakTestObject>();

        REQUIRE(!weakBase.Expired());
        auto locked = weakBase.Lock();
        REQUIRE(locked);
        REQUIRE(locked->GetValue() == 222324);
    }

    SECTION("Dynamic cast of weak reference")
    {
        auto derived = SceneryEditorX::CreateRef<DerivedWeakTestObject>(252627, "dynamic");
        SceneryEditorX::WeakRef<WeakTestObject> weakBase = derived.As<WeakTestObject>();

        // Try to get back to derived through weak reference
        auto lockedBase = weakBase.Lock();
        REQUIRE(lockedBase);

        auto backToDerived = lockedBase.DynamicCast<DerivedWeakTestObject>();
        REQUIRE(backToDerived);
        REQUIRE(backToDerived->GetName() == "dynamic");
    }
}

// One object has one control block, whatever the static types of the Refs that own it and the WeakRefs that observe
// it: the WeakRefs expire when the last owner is released, of whichever type it is.
TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Owners and observers of different static types", "[SmartPointers][WeakRef][Conversion]")
{
    SECTION("WeakRef<Base> from a Ref<Derived>")
    {
        auto derived = CreateRef<SlotWeakTestObject>(1, "first");
        const WeakRef<WeakTestObject> weakBase = derived;
        const WeakRef<WeakTestObject> weakBaseAs = derived.As<WeakTestObject>();
        WeakRef<WeakTestObject> weakBaseAssigned;
        weakBaseAssigned = derived;

        REQUIRE(SameControlBlock(weakBase, weakBaseAs));
        REQUIRE(SameControlBlock(weakBase, weakBaseAssigned));
        REQUIRE(weakBase.UseCount() == 1);
        REQUIRE(weakBase.Lock().Get() == derived.Get());

        SECTION("Last owner is the Ref<Derived>")
        {
            derived.Reset();
        }

        SECTION("Last owner is a Ref<Base>")
        {
            Ref<WeakTestObject> base = derived;
            derived.Reset();
            REQUIRE_FALSE(weakBase.Expired());
            REQUIRE(weakBase.UseCount() == 1);
            base.Reset();
        }

        REQUIRE(SlotWeakTestObject::s_Destroyed == 1);
        RequireAllExpired(weakBase, weakBaseAs, weakBaseAssigned);
        RequireStayExpiredWhenTheSlotIsReused(weakBase, weakBaseAs, weakBaseAssigned);
    }

    SECTION("WeakRef<Base> converted from a WeakRef<Derived>")
    {
        auto derived = CreateRef<SlotWeakTestObject>(2, "convert");
        const WeakRef<SlotWeakTestObject> weakDerived = derived;
        const WeakRef<WeakTestObject> weakBase = weakDerived;
        WeakRef<WeakTestObject> weakBaseAssigned;
        weakBaseAssigned = weakDerived;
        const WeakRef<WeakTestObject> weakBaseMoved = WeakRef<SlotWeakTestObject>(weakDerived);
        WeakRef<WeakTestObject> weakBaseMoveAssigned;
        weakBaseMoveAssigned = WeakRef<SlotWeakTestObject>(weakDerived);

        REQUIRE(SameControlBlock(weakBase, weakDerived));
        REQUIRE(SameControlBlock(weakBaseAssigned, weakDerived));
        REQUIRE(SameControlBlock(weakBaseMoved, weakDerived));
        REQUIRE(SameControlBlock(weakBaseMoveAssigned, weakDerived));
        REQUIRE(weakBase.Lock().Get() == derived.Get());
        REQUIRE(weakBaseMoved.Lock()->GetValue() == 2);

        SECTION("Last owner is the Ref<Derived>")
        {
            derived.Reset();
        }

        SECTION("Last owner is a Ref<Base> locked from the converted WeakRef")
        {
            Ref<WeakTestObject> base = weakBase.Lock();
            derived.Reset();
            REQUIRE_FALSE(weakDerived.Expired());
            REQUIRE(weakDerived.UseCount() == 1);
            base.Reset();
        }

        REQUIRE(SlotWeakTestObject::s_Destroyed == 1);
        RequireAllExpired(weakDerived, weakBase, weakBaseAssigned, weakBaseMoved, weakBaseMoveAssigned);
        RequireStayExpiredWhenTheSlotIsReused(weakDerived, weakBase, weakBaseAssigned, weakBaseMoved, weakBaseMoveAssigned);
    }

    SECTION("WeakRef<Derived> from a WeakRef<Base> through DynamicCast")
    {
        auto object = CreateRef<SlotWeakTestObject>(3, "dynamic");
        const WeakRef<WeakTestObject> weakBase = object.As<WeakTestObject>();
        const WeakRef<DerivedWeakTestObject> weakDerived = weakBase.Lock().DynamicCast<DerivedWeakTestObject>();

        REQUIRE(SameControlBlock(weakDerived, weakBase));
        REQUIRE(weakDerived.Lock().Get() == object.Get());
        REQUIRE(weakDerived.Lock()->GetName() == "dynamic");
        REQUIRE(weakDerived.UseCount() == 1);

        SECTION("Last owner is a Ref<Derived>")
        {
            Ref<DerivedWeakTestObject> derived = weakDerived.Lock();
            object.Reset();
            REQUIRE_FALSE(weakBase.Expired());
            derived.Reset();
        }

        SECTION("Last owner is a Ref<Base>")
        {
            Ref<WeakTestObject> base = weakBase.Lock();
            object.Reset();
            REQUIRE_FALSE(weakDerived.Expired());
            base.Reset();
        }

        REQUIRE(SlotWeakTestObject::s_Destroyed == 1);
        RequireAllExpired(weakBase, weakDerived);
        RequireStayExpiredWhenTheSlotIsReused(weakBase, weakDerived);
    }

    SECTION("DynamicCast to a type the object is not gives an empty WeakRef")
    {
        const auto plain = CreateRef<WeakTestObject>(4);
        const WeakRef<WeakTestObject> weakBase = plain;
        const WeakRef<DerivedWeakTestObject> weakDerived = weakBase.Lock().DynamicCast<DerivedWeakTestObject>();

        REQUIRE(weakDerived.GetControlBlock() == nullptr);
        RequireExpired(weakDerived);
        REQUIRE(weakBase.Lock().Get() == plain.Get());
    }

    SECTION("Last owner is a std::shared_ptr from ToSharedPtr")
    {
        auto derived = CreateRef<SlotWeakTestObject>(5, "shared");
        const WeakRef<WeakTestObject> weakBase = derived;
        std::shared_ptr<SlotWeakTestObject> shared = derived.ToSharedPtr();

        derived.Reset();
        REQUIRE_FALSE(weakBase.Expired());
        REQUIRE(weakBase.UseCount() == 1);

        shared.reset();
        REQUIRE(SlotWeakTestObject::s_Destroyed == 1);
        RequireExpired(weakBase);
        RequireStayExpiredWhenTheSlotIsReused(weakBase);
    }

    SECTION("RefCounted at a non-zero offset in the object")
    {
        auto object = CreateRef<OffsetWeakTestObject>(6, "offset");
        WeakTestObject *const base = object.Get();
        REQUIRE(static_cast<void *>(base) != static_cast<void *>(object.Get()));

        const WeakRef<OffsetWeakTestObject> weakObject = object;
        const WeakRef<WeakTestObject> weakBase = object;
        const WeakRef<WeakTestObject> weakConverted = weakObject;

        REQUIRE(SameControlBlock(weakBase, weakObject));
        REQUIRE(SameControlBlock(weakConverted, weakObject));
        REQUIRE(weakObject.Lock().Get() == object.Get());
        REQUIRE(weakBase.Lock().Get() == base);
        REQUIRE(weakConverted.Lock().Get() == base);
        REQUIRE(weakConverted.Lock()->GetValue() == 6);
        REQUIRE(weakObject.Lock()->m_Header == 0x5EDC);

        SECTION("Last owner is the Ref<Derived>")
        {
            object.Reset();
        }

        SECTION("Last owner is a Ref<Base>")
        {
            Ref<WeakTestObject> owner = object;
            object.Reset();
            REQUIRE(weakObject.UseCount() == 1);
            owner.Reset();
        }

        REQUIRE(OffsetWeakTestObject::s_Destroyed == 1);
        RequireAllExpired(weakObject, weakBase, weakConverted);
    }

    SECTION("RefCounted as a virtual base")
    {
        auto diamond = CreateRef<VirtualDiamondWeakTestObject>();
        const WeakRef<VirtualLeftWeakTestObject> weakLeft = diamond;
        const WeakRef<VirtualRightWeakTestObject> weakRight = diamond;
        const WeakRef<SceneryEditorX::RefCounted> weakRoot = weakRight;

        REQUIRE(SameControlBlock(weakLeft, weakRight));
        REQUIRE(SameControlBlock(weakRoot, weakRight));
        REQUIRE(weakLeft.Lock().Get() == static_cast<VirtualLeftWeakTestObject *>(diamond.Get()));
        REQUIRE(weakRight.Lock().Get() == static_cast<VirtualRightWeakTestObject *>(diamond.Get()));
        REQUIRE(weakLeft.Lock()->m_Left == 1);
        REQUIRE(weakRight.Lock()->m_Right == 2);
        REQUIRE(weakRoot.UseCount() == 1);

        SECTION("Last owner is a Ref to one side")
        {
            Ref<VirtualRightWeakTestObject> owner = diamond;
            diamond.Reset();
            REQUIRE_FALSE(weakLeft.Expired());
            owner.Reset();
        }

        SECTION("Last owner is a Ref<RefCounted>")
        {
            Ref<SceneryEditorX::RefCounted> owner = weakRoot.Lock();
            diamond.Reset();
            REQUIRE_FALSE(weakRight.Expired());
            owner.Reset();
        }

        REQUIRE(VirtualDiamondWeakTestObject::s_Destroyed == 1);
        RequireAllExpired(weakLeft, weakRight, weakRoot);
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Reset Operations", "[SmartPointers][WeakRef][Reset]")
{
    SECTION("Reset weak reference to null")
    {
        auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>();
        SceneryEditorX::WeakRef<WeakTestObject> weakRef = strongRef;

        REQUIRE(!weakRef.Expired());

        weakRef.Reset();

        REQUIRE(weakRef.Expired());
        REQUIRE(!weakRef.Lock());
    }

    SECTION("Reset to new strong reference")
    {
        auto strongRef1 = SceneryEditorX::CreateRef<WeakTestObject>(282930);
        auto strongRef2 = SceneryEditorX::CreateRef<WeakTestObject>(313233);
        SceneryEditorX::WeakRef<WeakTestObject> weakRef = strongRef1;

        weakRef = strongRef2; // Reset to different reference

        auto locked = weakRef.Lock();
        REQUIRE(locked);
        REQUIRE(locked->GetValue() == 313233);
        REQUIRE(locked.Get() == strongRef2.Get());
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Observer Pattern Implementation", "[SmartPointers][WeakRef][Observer]")
{
    SECTION("Observer list with weak references")
    {
        std::vector<SceneryEditorX::WeakRef<WeakTestObject>> observers;

        // Add observers
        {
            auto observer1 = SceneryEditorX::CreateRef<WeakTestObject>(1);
            auto observer2 = SceneryEditorX::CreateRef<WeakTestObject>(2);
            auto observer3 = SceneryEditorX::CreateRef<WeakTestObject>(3);

            observers.push_back(observer1);
            observers.push_back(observer2);
            observers.push_back(observer3);

            // All observers should be accessible
            int validCount = 0;
            for (auto& weakObs : observers)
			{
                if (auto obs = weakObs.Lock())
				{
                    validCount++;
                }
            }
            REQUIRE(validCount == 3);
        } // observers go out of scope

        // All weak references should be expired
        int expiredCount = 0;
        for (auto& weakObs : observers)
		{
            if (weakObs.Expired())
			{
                expiredCount++;
            }
        }
        REQUIRE(expiredCount == 3);
    }

    SECTION("Clean up expired observers")
    {
        std::vector<SceneryEditorX::WeakRef<WeakTestObject>> observers;

        // Add some observers, let some expire
        auto persistentObserver = SceneryEditorX::CreateRef<WeakTestObject>(100);
        observers.push_back(persistentObserver);

        {
            auto temporaryObserver = SceneryEditorX::CreateRef<WeakTestObject>(200);
            observers.push_back(temporaryObserver);
        } // temporaryObserver expires

        // Clean up expired observers
        std::erase_if(observers,[](const SceneryEditorX::WeakRef<WeakTestObject>& weak) { return weak.Expired(); });

        REQUIRE(observers.size() == 1);
        auto locked = observers[0].Lock();
        REQUIRE(locked);
        REQUIRE(locked->GetValue() == 100);
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Circular Reference Breaking", "[SmartPointers][WeakRef][Circular]")
{
    SECTION("Parent-child relationship without circular references")
    {
        struct Node : SceneryEditorX::RefCounted
        {
            Node(const int value) : m_Value(value) {}

            void AddChild(const SceneryEditorX::Ref<Node>& child, const SceneryEditorX::Ref<Node>& self)
            {
                m_Children.push_back(child);          // Strong reference to child
                child->m_Parent = self;               // Weak reference to parent via Ref<Node>
            }

            [[nodiscard]] SceneryEditorX::Ref<Node> GetParent() const
            {
                return m_Parent.Lock(); // Safe access to parent
            }

            int m_Value;
            std::vector<SceneryEditorX::Ref<Node>> m_Children;
            SceneryEditorX::WeakRef<Node> m_Parent;
        };

        auto parent = SceneryEditorX::CreateRef<Node>(1);
        auto child1 = SceneryEditorX::CreateRef<Node>(2);
        auto child2 = SceneryEditorX::CreateRef<Node>(3);

    parent->AddChild(child1, parent);
    parent->AddChild(child2, parent);

        // Verify relationships
        REQUIRE(parent->m_Children.size() == 2);
        REQUIRE(child1->GetParent() == parent);
        REQUIRE(child2->GetParent() == parent);

        // Parent should have reference count of 1 (no circular references)
        REQUIRE(parent.UseCount() == 1);
        REQUIRE(child1.UseCount() == 2); // parent->children + child1
        REQUIRE(child2.UseCount() == 2); // parent->children + child2

        // When parent is destroyed, children should lose access to parent
        SceneryEditorX::WeakRef<Node> weakParent = parent;
        parent.Reset();

        REQUIRE(weakParent.Expired());
        REQUIRE(!child1->GetParent());
        REQUIRE(!child2->GetParent());
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Thread Safety Considerations", "[SmartPointers][WeakRef][Threading]")
{
    SECTION("Concurrent access to weak reference")
    {
        const auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>(424344);
        SceneryEditorX::WeakRef<WeakTestObject> weakRef = strongRef;

        std::atomic<int> successCount{0};
        std::vector<std::thread> threads;

        // Multiple threads trying to lock the weak reference
        for (int i = 0; i < 10; ++i)
		{
            threads.emplace_back([&weakRef, &successCount]()
								 {
                for (int j = 0; j < 100; ++j)
				{
                    if (auto locked = weakRef.Lock())
					{
                        ++successCount;
                    }
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        REQUIRE(successCount.load() == 1000);
    }
}

TEST_CASE_METHOD(WeakRefTestFixture, "WeakRef - Edge Cases and Error Handling", "[SmartPointers][WeakRef][EdgeCases]")
{
    SECTION("Multiple resets of weak reference")
    {
        SceneryEditorX::WeakRef<WeakTestObject> weakRef;
        weakRef.Reset(); // Should be safe on empty weak ref

        const auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>();
        weakRef = strongRef;
        weakRef.Reset();
        weakRef.Reset(); // Should be safe multiple times

        REQUIRE(weakRef.Expired());
    }

    SECTION("Lock expired weak reference multiple times")
    {
        SceneryEditorX::WeakRef<WeakTestObject> weakRef;

        {
            const auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>();
            weakRef = strongRef;
        } // strongRef expires

        // Multiple attempts to lock should all fail safely
        for (int i = 0; i < 10; ++i)
		{
            auto locked = weakRef.Lock();
            REQUIRE(!locked);
        }
    }

    SECTION("Self assignment with weak reference")
    {
        const auto strongRef = SceneryEditorX::CreateRef<WeakTestObject>();
        SceneryEditorX::WeakRef<WeakTestObject> weakRef = strongRef;

        weakRef = weakRef; // Self assignment should be safe

        REQUIRE(!weakRef.Expired());
        auto locked = weakRef.Lock();
        REQUIRE(locked);
    }
}
