/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* RefTest.cpp
* -------------------------------------------------------
* Tests for the Ref smart pointer implementation
* -------------------------------------------------------
*/
#include <catch2/catch_test_macros.hpp>
#include <SceneryEditorX/core/pointers.h>
#include <memory>
#include <string>

namespace SceneryEditorX
{
	namespace Tests
	{
		// Sample reference-counted classes for testing
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
		
		// Helper for tracking object destruction
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
		
		TEST_CASE("Ref basic functionality", "[Ref]")
		{
		    SECTION("Default constructor creates null reference")
			{
		        Ref<TestObject> ref;
		        REQUIRE(ref == nullptr);
		        REQUIRE_FALSE(ref);
		        REQUIRE(ref.Get() == nullptr);
		        REQUIRE(ref.UseCount() == 0);
		    }
		    
		    SECTION("Constructor from nullptr creates null reference")
			{
		        Ref<TestObject> ref(nullptr);
		        REQUIRE(ref == nullptr);
		        REQUIRE_FALSE(ref);
		    }
		    
		    SECTION("Constructor from raw pointer")
			{
		        auto* rawPtr = new TestObject(42);
		        Ref<TestObject> ref(rawPtr);
		        
		        REQUIRE(ref != nullptr);
		        REQUIRE(ref);
		        REQUIRE(ref.Get() == rawPtr);
		        REQUIRE(ref.UseCount() == 1);
		        REQUIRE(ref->GetValue() == 42);
		        REQUIRE((*ref).GetValue() == 42);
		    }
		    
		    SECTION("CreateRef helper function")
			{
		        auto ref = CreateRef<TestObject>(42);
		        
		        REQUIRE(ref != nullptr);
		        REQUIRE(ref.UseCount() == 1);
		        REQUIRE(ref->GetValue() == 42);
		    }
		}
		
		TEST_CASE("Ref copy operations", "[Ref]")
		{
		    SECTION("Copy constructor")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        Ref<TestObject> ref2(ref1);
		        
		        REQUIRE(ref1.UseCount() == 2);
		        REQUIRE(ref2.UseCount() == 2);
		        REQUIRE(ref1.Get() == ref2.Get());
		        REQUIRE(ref1->GetValue() == 42);
		        REQUIRE(ref2->GetValue() == 42);
		    }
		    
		    SECTION("Copy assignment operator")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        Ref<TestObject> ref2;
		        
		        ref2 = ref1;
		        
		        REQUIRE(ref1.UseCount() == 2);
		        REQUIRE(ref2.UseCount() == 2);
		        REQUIRE(ref1.Get() == ref2.Get());
		        
		        // Self-assignment should be safe
		        ref1 = ref1;
		        REQUIRE(ref1.UseCount() == 2);
		    }
		    
		    SECTION("Copy assignment from nullptr")
			{
		        auto ref = CreateRef<TestObject>(42);
		        REQUIRE(ref.UseCount() == 1);
		        
		        ref = nullptr;
		        REQUIRE(ref == nullptr);
		        REQUIRE(ref.UseCount() == 0);
		    }
		}
		
		TEST_CASE("Ref move operations", "[Ref]")
		{
		    SECTION("Move constructor")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        auto* rawPtr = ref1.Get();
		        Ref<TestObject> ref2(std::move(ref1));
		        
		        REQUIRE(ref1 == nullptr);
		        REQUIRE(ref2 != nullptr);
		        REQUIRE(ref2.Get() == rawPtr);
		        REQUIRE(ref2.UseCount() == 1);
		    }
		    
		    SECTION("Move assignment operator")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        auto* rawPtr = ref1.Get();
		        Ref<TestObject> ref2;
		        
		        ref2 = std::move(ref1);
		        
		        REQUIRE(ref1 == nullptr);
		        REQUIRE(ref2 != nullptr);
		        REQUIRE(ref2.Get() == rawPtr);
		        REQUIRE(ref2.UseCount() == 1);
		        
		        // Self-move should be safe (though normally not used)
		        Ref<TestObject> ref3 = CreateRef<TestObject>(42);
		        ref3 = std::move(ref3);
		        
		        // This behavior is implementation-defined, but our implementation
		        // should set the pointer to nullptr
		        REQUIRE(ref3 == nullptr);
		    }
		}
		
		TEST_CASE("Ref type conversion", "[Ref]")
		{
		    SECTION("Upcast via constructor")
			{
		        auto derived = CreateRef<DerivedTestObject>(42, "Test");
		        Ref<TestObject> base(derived);
		        
		        REQUIRE(derived.UseCount() == 2);
		        REQUIRE(base.UseCount() == 2);
		        REQUIRE(derived->GetValue() == 42);
		        REQUIRE(base->GetValue() == 42);
		    }
		    
		    SECTION("Upcast via assignment")
			{
		        auto derived = CreateRef<DerivedTestObject>(42, "Test");
		        Ref<TestObject> base;
		        
		        base = derived;
		        
		        REQUIRE(derived.UseCount() == 2);
		        REQUIRE(base.UseCount() == 2);
		        REQUIRE(derived->GetValue() == 42);
		        REQUIRE(base->GetValue() == 42);
		    }
		    
		    SECTION("Downcast using As() - static_cast")
			{
		        auto base = CreateRef<TestObject>(42);
		        auto derived = CreateRef<DerivedTestObject>(42, "Test");
		        Ref<TestObject> baseRef = derived;
		        
		        // Valid downcast
		        auto derivedAgain = baseRef.As<DerivedTestObject>();
		        REQUIRE(derivedAgain != nullptr);
		        REQUIRE(derivedAgain->GetName() == "Test");
		        
		        // Invalid downcast (undefined behavior in real code, but we test the mechanism)
		        auto invalidDowncast = base.As<DerivedTestObject>();
		        // Even though this would be unsafe in real code, the static_cast will succeed
		        REQUIRE(invalidDowncast != nullptr);
		    }
		    
		    SECTION("Downcast using DynamicCast() - dynamic_cast")
			{
		        auto base = CreateRef<TestObject>(42);
		        auto derived = CreateRef<DerivedTestObject>(42, "Test");
		        Ref<TestObject> baseRef = derived;
		        
		        // Valid downcast
		        auto derivedAgain = baseRef.DynamicCast<DerivedTestObject>();
		        REQUIRE(derivedAgain != nullptr);
		        REQUIRE(derivedAgain->GetName() == "Test");
		        
		        // Invalid downcast (safely returns null)
		        auto invalidDowncast = base.DynamicCast<DerivedTestObject>();
		        REQUIRE(invalidDowncast == nullptr);
		    }
		}
		
		TEST_CASE("Ref resource management", "[Ref]")
		{
		    SECTION("Destruction on going out of scope")
			{
		        bool destroyed = false;
		        {
		            auto ref = CreateRef<DestructionTracker>(&destroyed);
		            REQUIRE_FALSE(destroyed);
		        }
		        REQUIRE(destroyed);
		    }
		    
		    SECTION("Destruction on last reference")
			{
		        bool destroyed = false;
		        auto ref1 = CreateRef<DestructionTracker>(&destroyed);
		        {
		            auto ref2 = ref1;
		            auto ref3 = ref1;
		            REQUIRE_FALSE(destroyed);
		        }
		        REQUIRE_FALSE(destroyed);
		        ref1 = nullptr;
		        REQUIRE(destroyed);
		    }
		    
		    SECTION("Reset method")
			{
		        bool destroyed1 = false;
		        bool destroyed2 = false;
		        
		        auto ref = CreateRef<DestructionTracker>(&destroyed1);
		        REQUIRE_FALSE(destroyed1);
		        
		        ref.Reset(new DestructionTracker(&destroyed2));
		        REQUIRE(destroyed1);
		        REQUIRE_FALSE(destroyed2);
		        
		        ref.Reset();
		        REQUIRE(destroyed2);
		        REQUIRE(ref == nullptr);
		    }
		    
		    SECTION("IsUnique method")
			{
		        auto ref1 = CreateRef<TestObject>();
		        REQUIRE(ref1.IsUnique());
		        
		        auto ref2 = ref1;
		        REQUIRE_FALSE(ref1.IsUnique());
		        REQUIRE_FALSE(ref2.IsUnique());
		        
		        ref2 = nullptr;
		        REQUIRE(ref1.IsUnique());
		    }
		}
		
		TEST_CASE("Ref interoperability with std::shared_ptr", "[Ref]")
		{
		    SECTION("Construction from shared_ptr")
			{
		        auto shared = std::make_shared<TestObject>(42);
		        Ref<TestObject> ref(shared);
		        
		        REQUIRE(ref != nullptr);
		        REQUIRE(ref->GetValue() == 42);
		        // Reference count will include both the shared_ptr and the Ref
		        REQUIRE(ref.UseCount() >= 1);
		    }
		    
		    SECTION("Conversion to shared_ptr")
			{
		        auto ref = CreateRef<TestObject>(42);
		        auto shared = ref.ToSharedPtr();
		        
		        REQUIRE(shared != nullptr);
		        REQUIRE(shared->GetValue() == 42);
		        REQUIRE(shared.use_count() >= 1);
		        REQUIRE(ref.UseCount() >= 1);
		    }
		}
		
		TEST_CASE("Ref WeakRef integration", "[Ref][WeakRef]")
		{
		    SECTION("Creating WeakRef from Ref")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak(ref);
		        
		        REQUIRE_FALSE(weak.Expired());
		        REQUIRE(weak.UseCount() == 1);
		    }
		    
		    SECTION("WeakRef expiration")
			{
		        WeakRef<TestObject> weak;
		        {
		            auto ref = CreateRef<TestObject>(42);
		            weak = ref;
		            REQUIRE_FALSE(weak.Expired());
		        }
		        REQUIRE(weak.Expired());
		        REQUIRE(weak.UseCount() == 0);
		    }
		    
		    SECTION("Locking WeakRef to get Ref")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak(ref);
		        
		        auto lockedRef = weak.Lock();
		        REQUIRE(lockedRef != nullptr);
		        REQUIRE(lockedRef->GetValue() == 42);
		        REQUIRE(lockedRef.UseCount() == 2);
		        
		        ref = nullptr;
		        auto stillLockedRef = weak.Lock();
		        REQUIRE(stillLockedRef != nullptr);
		        REQUIRE(stillLockedRef.UseCount() == 1);
		        
		        stillLockedRef = nullptr;
		        REQUIRE(weak.Expired());
		        auto expiredRef = weak.Lock();
		        REQUIRE(expiredRef == nullptr);
		    }
		    
		    SECTION("Constructing Ref from WeakRef")
			{
		        auto ref = CreateRef<TestObject>(42);
		        WeakRef<TestObject> weak(ref);
		        
		        Ref<TestObject> newRef(weak);
		        REQUIRE(newRef != nullptr);
		        REQUIRE(newRef->GetValue() == 42);
		        REQUIRE(newRef.UseCount() == 2);
		        
		        ref = nullptr;
		        weak.Reset();
		        Ref<TestObject> nullRef(weak);
		        REQUIRE(nullRef == nullptr);
		    }
		}
		
		TEST_CASE("Ref comparison operators", "[Ref]")
		{
		    SECTION("Equality with another Ref")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        auto ref2 = ref1;
		        auto ref3 = CreateRef<TestObject>(42);
		        
		        REQUIRE(ref1 == ref2);
		        REQUIRE_FALSE(ref1 == ref3);
		    }
		    
		    SECTION("Inequality with another Ref")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        auto ref2 = ref1;
		        auto ref3 = CreateRef<TestObject>(42);
		        
		        REQUIRE_FALSE(ref1 != ref2);
		        REQUIRE(ref1 != ref3);
		    }
		    
		    SECTION("Equality with nullptr")
			{
		        Ref<TestObject> ref1;
		        auto ref2 = CreateRef<TestObject>(42);
		        
		        REQUIRE(ref1 == nullptr);
		        REQUIRE_FALSE(ref2 == nullptr);
		    }
		    
		    SECTION("Inequality with nullptr")
			{
		        Ref<TestObject> ref1;
		        auto ref2 = CreateRef<TestObject>(42);
		        
		        REQUIRE_FALSE(ref1 != nullptr);
		        REQUIRE(ref2 != nullptr);
		    }
		    
		    SECTION("Object equality with EqualsObject")
			{
		        class ComparableObject : public RefCounted
				{
		        public:
		            explicit ComparableObject(int id) : m_Id(id) {}
		            bool operator==(const ComparableObject& other) const
					{
		                return m_Id == other.m_Id;
		            }
		        private:
		            int m_Id;
		        };
		        
		        auto ref1 = CreateRef<ComparableObject>(42);
		        auto ref2 = CreateRef<ComparableObject>(42);
		        auto ref3 = CreateRef<ComparableObject>(43);
		        
		        // Same value but different objects
		        REQUIRE(ref1.EqualsObject(ref2));
		        REQUIRE_FALSE(ref1.EqualsObject(ref3));
		        
		        // Same object
		        auto ref4 = ref1;
		        REQUIRE(ref1.EqualsObject(ref4));
		    }
		}
		
		TEST_CASE("Ref misc operations", "[Ref]")
		{
		    SECTION("Swap method")
			{
		        auto ref1 = CreateRef<TestObject>(42);
		        auto ref2 = CreateRef<TestObject>(24);
		        
		        auto* ptr1 = ref1.Get();
		        auto* ptr2 = ref2.Get();
		        
		        ref1.Swap(ref2);
		        
		        REQUIRE(ref1.Get() == ptr2);
		        REQUIRE(ref2.Get() == ptr1);
		        REQUIRE(ref1->GetValue() == 24);
		        REQUIRE(ref2->GetValue() == 42);
		    }
		    
		    SECTION("IsValid method")
			{
		        Ref<TestObject> ref1;
		        auto ref2 = CreateRef<TestObject>();
		        
		        REQUIRE_FALSE(ref1.IsValid());
		        REQUIRE(ref2.IsValid());
		    }
		}
	
	}  // namespace Tests
}  // namespace SceneryEditorX
