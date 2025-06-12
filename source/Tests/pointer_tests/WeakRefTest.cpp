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
#include <catch2/catch_test_macros.hpp>
#include <SceneryEditorX/core/pointers.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace SceneryEditorX
{
	namespace Tests
	{
		
		// Reuse the test classes from RefTest.cpp
		class TestObject : public RefCounted
		{
		public:
		    TestObject() : m_Value(0) {}
		    explicit TestObject(int value) : m_Value(value) {}
		    
		    int GetValue() const { return m_Value; }
		    void SetValue(int value) { m_Value = value; }
		    
		private:
		    int m_Value;
		};
		
		class DerivedTestObject : public TestObject
		{
		public:
		    DerivedTestObject() : TestObject(0), m_Name("Derived") {}
		    explicit DerivedTestObject(int value, const std::string& name) 
		        : TestObject(value), m_Name(name) {}
		    
		    const std::string& GetName() const { return m_Name; }
		    void SetName(const std::string& name) { m_Name = name; }
		    
		private:
		    std::string m_Name;
		};
		
		class DestructionTracker : public RefCounted
		{
		public:
		    DestructionTracker(bool* destroyed) : m_Destroyed(destroyed)
			{
		        if (m_Destroyed) *m_Destroyed = false;
		    }
		    
		    ~DestructionTracker() override
			{
		        if (m_Destroyed) *m_Destroyed = true;
		    }
		    
		private:
		    bool* m_Destroyed;
		};
		
		TEST_CASE("WeakRef basic functionality", "[WeakRef]")
		{
		    SECTION("Default constructor creates null weak reference")
			{
		        WeakRef<TestObject> weak;
		        REQUIRE(weak.Expired());
		        REQUIRE(weak.UseCount() == 0);
		        REQUIRE(weak.Lock() == nullptr);
		    }
		    
		    SECTION("Constructor from nullptr creates null weak reference")
			{
		        WeakRef<TestObject> weak(nullptr);
		        REQUIRE(weak.Expired());
		        REQUIRE(weak.UseCount() == 0);
		    }
		    
		    SECTION("Constructor from Ref")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak(ref);
		        
		        REQUIRE_FALSE(weak.Expired());
		        REQUIRE(weak.UseCount() == 1);
		        
		        auto locked = weak.Lock();
		        REQUIRE(locked != nullptr);
		        REQUIRE(locked->GetValue() == 42);
		        REQUIRE(locked.UseCount() == 2);
		    }
		}
		
		TEST_CASE("WeakRef copy operations", "[WeakRef]")
		{
		    SECTION("Copy constructor")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak1(ref);
		        WeakRef<TestObject> weak2(weak1);
		        
		        REQUIRE_FALSE(weak1.Expired());
		        REQUIRE_FALSE(weak2.Expired());
		        REQUIRE(weak1.UseCount() == 1);
		        REQUIRE(weak2.UseCount() == 1);
		        
		        auto locked1 = weak1.Lock();
		        auto locked2 = weak2.Lock();
		        REQUIRE(locked1.Get() == locked2.Get());
		    }
		    
		    SECTION("Copy assignment operator")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak1(ref);
		        WeakRef<TestObject> weak2;
		        
		        weak2 = weak1;
		        
		        REQUIRE_FALSE(weak1.Expired());
		        REQUIRE_FALSE(weak2.Expired());
		        REQUIRE(weak1.UseCount() == 1);
		        REQUIRE(weak2.UseCount() == 1);
		        
		        // Self-assignment should be safe
		        weak1 = weak1;
		        REQUIRE_FALSE(weak1.Expired());
		    }
		    
		    SECTION("Copy assignment from Ref")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak;
		        
		        weak = ref;
		        
		        REQUIRE_FALSE(weak.Expired());
		        REQUIRE(weak.UseCount() == 1);
		        
		        auto locked = weak.Lock();
		        REQUIRE(locked != nullptr);
		        REQUIRE(locked->GetValue() == 42);
		    }
		    
		    SECTION("Copy assignment from nullptr")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak(ref);
		        
		        weak = nullptr;
		        
		        REQUIRE(weak.Expired());
		        REQUIRE(weak.UseCount() == 0);
		    }
		}
		
		TEST_CASE("WeakRef move operations", "[WeakRef]")
		{
		    SECTION("Move constructor")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak1(ref);
		        WeakRef<TestObject> weak2(std::move(weak1));
		        
		        // Original weak ref should be reset
		        REQUIRE(weak1.Expired());
		        REQUIRE(weak1.UseCount() == 0);
		        
		        // New weak ref should point to the object
		        REQUIRE_FALSE(weak2.Expired());
		        REQUIRE(weak2.UseCount() == 1);
		        
		        auto locked = weak2.Lock();
		        REQUIRE(locked != nullptr);
		        REQUIRE(locked->GetValue() == 42);
		    }
		    
		    SECTION("Move assignment operator")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak1(ref);
		        WeakRef<TestObject> weak2;
		        
		        weak2 = std::move(weak1);
		        
		        // Original weak ref should be reset
		        REQUIRE(weak1.Expired());
		        REQUIRE(weak1.UseCount() == 0);
		        
		        // New weak ref should point to the object
		        REQUIRE_FALSE(weak2.Expired());
		        REQUIRE(weak2.UseCount() == 1);
		        
		        // Self-move should be handled safely
		        WeakRef<TestObject> weak3(ref);
		        weak3 = std::move(weak3);
		        // This behavior depends on implementation, but our implementation should leave it expired
		        REQUIRE(weak3.Expired());
		    }
		}
		
		TEST_CASE("WeakRef type conversion", "[WeakRef]")
		{
		    SECTION("Upcast via constructor")
			{
		        auto derived = CreateRef<DerivedTestObject>(42, "Test");
		        WeakRef<DerivedTestObject> derivedWeak(derived);
		        WeakRef<TestObject> baseWeak(derivedWeak);
		        
		        REQUIRE_FALSE(derivedWeak.Expired());
		        REQUIRE_FALSE(baseWeak.Expired());
		        
		        auto derivedLocked = derivedWeak.Lock();
		        auto baseLocked = baseWeak.Lock();
		        
		        REQUIRE(derivedLocked != nullptr);
		        REQUIRE(baseLocked != nullptr);
		        REQUIRE(derivedLocked->GetValue() == 42);
		        REQUIRE(baseLocked->GetValue() == 42);
		    }
		    
		    SECTION("Upcast via assignment")
			{
		        auto derived = CreateRef<DerivedTestObject>(42, "Test");
		        WeakRef<DerivedTestObject> derivedWeak(derived);
		        WeakRef<TestObject> baseWeak;
		        
		        baseWeak = derivedWeak;
		        
		        REQUIRE_FALSE(derivedWeak.Expired());
		        REQUIRE_FALSE(baseWeak.Expired());
		        
		        auto derivedLocked = derivedWeak.Lock();
		        auto baseLocked = baseWeak.Lock();
		        
		        REQUIRE(derivedLocked != nullptr);
		        REQUIRE(baseLocked != nullptr);
		        REQUIRE(derivedLocked->GetValue() == 42);
		        REQUIRE(baseLocked->GetValue() == 42);
		    }
		    
		    SECTION("Locking and downcasting")
			{
		        auto derived = CreateRef<DerivedTestObject>(42, "Test");
		        WeakRef<TestObject> baseWeak(derived);
		        
		        auto baseLocked = baseWeak.Lock();
		        REQUIRE(baseLocked != nullptr);
		        
		        // Downcast using DynamicCast
		        auto derivedAgain = baseLocked.DynamicCast<DerivedTestObject>();
		        REQUIRE(derivedAgain != nullptr);
		        REQUIRE(derivedAgain->GetName() == "Test");
		    }
		}
		
		TEST_CASE("WeakRef lifetime management", "[WeakRef]")
		{
		    SECTION("Expiration when Ref is destroyed")
			{
		        WeakRef<TestObject> weak;
		        {
		            auto ref = CreateRef<TestObject>(42);
		            weak = ref;
		            REQUIRE_FALSE(weak.Expired());
		        }
		        // Ref has been destroyed, weak ref should be expired
		        REQUIRE(weak.Expired());
		        REQUIRE(weak.Lock() == nullptr);
		    }
		    
		    SECTION("Reset method")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak(ref);
		        
		        REQUIRE_FALSE(weak.Expired());
		        
		        weak.Reset();
		        
		        REQUIRE(weak.Expired());
		        REQUIRE(weak.UseCount() == 0);
		    }
		    
		    SECTION("Control block cleanup")
			{
		        bool destroyed = false;
		        WeakRef<DestructionTracker> weak;
		        {
		            auto ref = CreateRef<DestructionTracker>(&destroyed);
		            weak = ref;
		        }
		        // Object should be destroyed when ref goes out of scope
		        REQUIRE(destroyed);
		        REQUIRE(weak.Expired());
		        
		        // Lock should return null for expired weak ref
		        auto locked = weak.Lock();
		        REQUIRE(locked == nullptr);
		    }
		}
		
		TEST_CASE("WeakRef comparison operators", "[WeakRef]")
		{
		    SECTION("Equality with another WeakRef")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        auto ref2 = CreateRef<TestObject>(42);
		        
		        WeakRef<TestObject> weak1(ref1);
		        WeakRef<TestObject> weak2(ref1);
		        WeakRef<TestObject> weak3(ref2);
		        
		        REQUIRE(weak1 == weak2);
		        REQUIRE_FALSE(weak1 == weak3);
		    }
		    
		    SECTION("Inequality with another WeakRef")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        auto ref2 = CreateRef<TestObject>(42);
		        
		        WeakRef<TestObject> weak1(ref1);
		        WeakRef<TestObject> weak2(ref1);
		        WeakRef<TestObject> weak3(ref2);
		        
		        REQUIRE_FALSE(weak1 != weak2);
		        REQUIRE(weak1 != weak3);
		    }
		    
		    SECTION("Comparing expired WeakRefs")
			{
		        WeakRef<TestObject> weak1;
		        WeakRef<TestObject> weak2;
		        
		        REQUIRE(weak1 == weak2);
		        REQUIRE_FALSE(weak1 != weak2);
		        
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak3(ref);
		        
		        REQUIRE_FALSE(weak1 == weak3);
		        REQUIRE(weak1 != weak3);
		    }
		}
		
		TEST_CASE("WeakRef thread safety", "[WeakRef][thread]")
		{
		    SECTION("Multiple threads accessing the same WeakRef")
			{
		        auto ref = CreateRef<TestObject>(0);
		        WeakRef<TestObject> weak(ref);
		        
		        // Create multiple threads that increment the value
		        constexpr int threadCount = 10;
		        constexpr int incrementsPerThread = 1000;
		        
		        std::vector<std::thread> threads;
		        for (int i = 0; i < threadCount; ++i)
				{
		            threads.emplace_back([&weak, incrementsPerThread]()
					{
		                for (int j = 0; j < incrementsPerThread; ++j)
						{
		                    if (auto locked = weak.Lock())
							{
		                        int value = locked->GetValue();
		                        locked->SetValue(value + 1);
		                    }
		                }
		            });
		        }
		        
		        // Join all threads
		        for (auto& thread : threads) { thread.join(); }
		        
		        // Check final value
		        auto locked = weak.Lock();
		        REQUIRE(locked != nullptr);
		        REQUIRE(locked->GetValue() == threadCount * incrementsPerThread);
		    }
		    
		    SECTION("Thread creating and destroying Refs while others use WeakRefs")
			{
		        constexpr int iterations = 100;
		        
		        for (int i = 0; i < iterations; ++i)
				{
		            WeakRef<TestObject> weak;
		            
		            // Thread 1: Create and destroy Ref objects
		            std::thread t1([&weak]()
					{
		                for (int j = 0; j < 10; ++j)
						{
		                    auto ref = CreateRef<TestObject>(j);
		                    weak = ref;
		                    std::this_thread::yield(); // Allow other thread to run
		                }
		            });
		            
		            // Thread 2: Try to lock the weak ref
		            std::thread t2([&weak]()
					{
		                for (int j = 0; j < 20; ++j)
						{
		                    auto locked = weak.Lock();
		                    if (locked)
							{
		                        // If we got a valid ref, the value should be 0-9
		                        int value = locked->GetValue();
		                        REQUIRE(value >= 0);
		                        REQUIRE(value < 10);
		                    }
		                    std::this_thread::yield(); // Allow other thread to run
		                }
		            });
		            
		            t1.join();
		            t2.join();
		        }
		    }
		}
		
		TEST_CASE("WeakRef cycle breaking", "[WeakRef]")
		{
		    SECTION("Preventing reference cycles")
			{
		        // Create a class that contains both Ref and WeakRef
		        class Node : public RefCounted
				{
		        public:
		            void SetStrongNext(const Ref<Node>& next) { m_StrongNext = next; }
		            void SetWeakNext(const Ref<Node>& next) { m_WeakNext = next; }
		            
		            Ref<Node> GetStrongNext() const { return m_StrongNext; }
		            Ref<Node> GetWeakNext() const { return m_WeakNext.Lock(); }
		            
		        private:
		            Ref<Node> m_StrongNext;
		            WeakRef<Node> m_WeakNext;
		        };
		        
		        bool nodeADestroyed = false;
		        bool nodeBDestroyed = false;
		        
		        // Create the cycle using weak references
		        {
		            auto nodeA = CreateRef<Node>();
		            auto nodeB = CreateRef<Node>();
		            
		            // This would create a cycle if we used strong references for both
		            nodeA->SetWeakNext(nodeB);
		            nodeB->SetWeakNext(nodeA);
		            
		            // Verify they're connected
		            REQUIRE(nodeA->GetWeakNext() == nodeB);
		            REQUIRE(nodeB->GetWeakNext() == nodeA);
		        }
		        // Both nodes should be destroyed because the cycle was broken by WeakRef
		        REQUIRE((nodeADestroyed || nodeBDestroyed));
		        
		        // Now create a real cycle with strong references
		        nodeADestroyed = false;
		        nodeBDestroyed = false;
		        {
		            auto nodeA = CreateRef<Node>();
		            auto nodeB = CreateRef<Node>();
		            
		            // Create a strong reference cycle
		            nodeA->SetStrongNext(nodeB);
		            nodeB->SetStrongNext(nodeA);
		            
		            // Verify they're connected
		            REQUIRE(nodeA->GetStrongNext() == nodeB);
		            REQUIRE(nodeB->GetStrongNext() == nodeA);
		            
		            // Break the cycle manually
		            nodeA->SetStrongNext(nullptr);
		            nodeB->SetStrongNext(nullptr);
		        }
		        // Now both nodes should be destroyed
		        REQUIRE((nodeADestroyed || nodeBDestroyed));
		    }
		}
	
	}  // namespace Tests
}  // namespace SceneryEditorX
