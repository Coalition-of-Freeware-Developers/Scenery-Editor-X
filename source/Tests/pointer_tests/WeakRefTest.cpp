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
#include <memory>
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

		///< Test fixture for weak reference tests
		class WeakRefTestFixture
		{
		public:
		    WeakRefTestFixture() { WeakTestObject::ResetDestroyCount(); }
		    ~WeakRefTestFixture() { WeakTestObject::ResetDestroyCount(); }
		};
    }
}

using namespace SceneryEditorX::Tests::PointerTests;

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
