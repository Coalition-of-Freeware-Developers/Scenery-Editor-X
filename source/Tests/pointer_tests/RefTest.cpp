/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* RefTest.cpp
* -------------------------------------------------------
* Tests for the Ref<T> smart pointer class
* -------------------------------------------------------
*/
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/pointers.h>
#include "../SimpleTestHelper.h"
#include <memory>
#include <string>

/// -------------------------------------------------------------------

namespace SceneryEditorX
{
    namespace Tests
    {
        namespace PointerTests
        {
            // Test class for reference counting tests
            class TestObject : public RefCounted
            {
            public:
                explicit TestObject(const int value = 42) : m_Value(value) {}
                virtual ~TestObject() override { ++s_DestroyCount; }

                int GetValue() const { return m_Value; }
                void SetValue(const int value) { m_Value = value; }

                static int GetDestroyCount() { return s_DestroyCount; }
                static void ResetDestroyCount() { s_DestroyCount = 0; }

            private:
                int m_Value;
                static int s_DestroyCount;
            };

            int TestObject::s_DestroyCount = 0;

            class DerivedTestObject : public TestObject
            {
            public:
                explicit DerivedTestObject(const int value = 100, std::string name = "derived") : TestObject(value), m_Name(std::move(name)) {}
                const std::string& GetName() const { return m_Name; }
                void SetName(const std::string& name) { m_Name = name; }

            private:
                std::string m_Name;
            };

            // Test fixture for Ref tests
            class RefTestFixture
            {
            public:
                RefTestFixture()
                {
                    TestObject::ResetDestroyCount();
                }

                ~RefTestFixture()
                {
                    TestObject::ResetDestroyCount();
                }
            };
        }

        using namespace PointerTests;

        TEST_CASE_METHOD(RefTestFixture, "Ref default construction", "[Ref][construction]")
        {
            TEST_CASE_LOG("Ref default construction", "[Ref][construction]");

            SECTION("Default constructor creates null reference")
            {
                SECTION_LOG("Default constructor creates null reference");
                LOG_TEST_INFO("Testing default constructor behavior");

                Ref<TestObject> ref;
                LOG_TEST_INFO("Created default Ref<TestObject>");

                REQUIRE_FALSE(ref);
                LOG_ASSERTION("ref", !ref, "Default ref should be false/null");

                REQUIRE(ref.Get() == nullptr);
                LOG_ASSERTION("ref.Get() == nullptr", ref.Get() == nullptr, "Get() should return nullptr");

                REQUIRE(ref.UseCount() == 0);
                LOG_ASSERTION("ref.UseCount() == 0", ref.UseCount() == 0, "UseCount should be 0");

                REQUIRE_FALSE(ref.IsValid());
                LOG_ASSERTION("!ref.IsValid()", !ref.IsValid(), "IsValid should return false");
            }

            SECTION("nullptr constructor creates null reference")
            {
                SECTION_LOG("nullptr constructor creates null reference");
                LOG_TEST_INFO("Testing nullptr constructor behavior");

                Ref<TestObject> ref(nullptr);
                LOG_TEST_INFO("Created Ref<TestObject> with nullptr");

                REQUIRE_FALSE(ref);
                LOG_ASSERTION("ref", !ref, "nullptr ref should be false/null");

                REQUIRE(ref.Get() == nullptr);
                LOG_ASSERTION("ref.Get() == nullptr", ref.Get() == nullptr, "Get() should return nullptr");

                REQUIRE(ref.UseCount() == 0);
                LOG_ASSERTION("ref.UseCount() == 0", ref.UseCount() == 0, "UseCount should be 0");

                REQUIRE_FALSE(ref.IsValid());
                LOG_ASSERTION("!ref.IsValid()", !ref.IsValid(), "IsValid should return false");
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref construction from raw pointer", "[Ref][construction]")
        {
            SECTION("Constructor from raw pointer takes ownership")
            {
                auto* rawPtr = new TestObject(123);
                Ref<TestObject> ref(rawPtr);

                REQUIRE(ref);
                REQUIRE(ref.Get() == rawPtr);
                REQUIRE(ref->GetValue() == 123);
                REQUIRE(ref.UseCount() == 1);
                REQUIRE(ref.IsValid());
                REQUIRE(ref.IsUnique());
            }

            SECTION("Multiple refs from same raw pointer share ownership")
            {
                auto* rawPtr = new TestObject(456);
                Ref<TestObject> ref1(rawPtr);
                Ref<TestObject> ref2(rawPtr);

                REQUIRE(ref1.UseCount() == 2);
                REQUIRE(ref2.UseCount() == 2);
                REQUIRE(ref1.Get() == ref2.Get());
                REQUIRE_FALSE(ref1.IsUnique());
                REQUIRE_FALSE(ref2.IsUnique());
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref copy construction", "[Ref][construction][copy]")
        {
            SECTION("Copy constructor shares ownership")
            {
                Ref<TestObject> original = CreateRef<TestObject>(789);
                const Ref<TestObject> &copy(original);

                REQUIRE(original.UseCount() == 2);
                REQUIRE(copy.UseCount() == 2);
                REQUIRE(original.Get() == copy.Get());
                REQUIRE(original->GetValue() == 789);
                REQUIRE(copy->GetValue() == 789);
            }

            SECTION("Copy constructor with type conversion")
            {
                Ref<DerivedTestObject> derived = CreateRef<DerivedTestObject>(987, "test");
                Ref<TestObject> base(derived);

                REQUIRE(derived.UseCount() == 2);
                REQUIRE(base.UseCount() == 2);
                REQUIRE(base->GetValue() == 987);
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref move construction", "[Ref][construction][move]")
        {
            SECTION("Move constructor transfers ownership")
            {
                Ref<TestObject> original = CreateRef<TestObject>(654);
                TestObject* originalPtr = original.Get();
                Ref<TestObject> moved(std::move(original));

                REQUIRE_FALSE(original);
                REQUIRE(original.Get() == nullptr);
                REQUIRE(moved);
                REQUIRE(moved.Get() == originalPtr);
                REQUIRE(moved.UseCount() == 1);
                REQUIRE(moved->GetValue() == 654);
            }

            SECTION("Move constructor with type conversion")
            {
                Ref<DerivedTestObject> derived = CreateRef<DerivedTestObject>(321, "moved");
                TestObject* originalPtr = derived.Get();
                Ref<TestObject> base(std::move(derived));

                REQUIRE_FALSE(derived);
                REQUIRE(base.Get() == originalPtr);
                REQUIRE(base.UseCount() == 1);
                REQUIRE(base->GetValue() == 321);
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref assignment operators", "[Ref][assignment]")
        {
            SECTION("Copy assignment shares ownership")
            {
                Ref<TestObject> ref1 = CreateRef<TestObject>(111);
                Ref<TestObject> ref2 = CreateRef<TestObject>(222);

                REQUIRE(TestObject::GetDestroyCount() == 0);

                ref2 = ref1;

                REQUIRE(TestObject::GetDestroyCount() == 1); // Object with value 222 destroyed
                REQUIRE(ref1.UseCount() == 2);
                REQUIRE(ref2.UseCount() == 2);
                REQUIRE(ref1.Get() == ref2.Get());
                REQUIRE(ref1->GetValue() == 111);
                REQUIRE(ref2->GetValue() == 111);
            }

            SECTION("Move assignment transfers ownership")
            {
                Ref<TestObject> ref1 = CreateRef<TestObject>(333);
                Ref<TestObject> ref2 = CreateRef<TestObject>(444);
                TestObject* ref1Ptr = ref1.Get();

                REQUIRE(TestObject::GetDestroyCount() == 0);

                ref2 = std::move(ref1);

                REQUIRE(TestObject::GetDestroyCount() == 1); // Object with value 444 destroyed
                REQUIRE_FALSE(ref1);
                REQUIRE(ref2.Get() == ref1Ptr);
                REQUIRE(ref2.UseCount() == 1);
                REQUIRE(ref2->GetValue() == 333);
            }

            SECTION("Assignment to nullptr resets reference")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(555);

                REQUIRE(TestObject::GetDestroyCount() == 0);

                ref = nullptr;

                REQUIRE(TestObject::GetDestroyCount() == 1);
                REQUIRE_FALSE(ref);
                REQUIRE(ref.Get() == nullptr);
                REQUIRE(ref.UseCount() == 0);
            }

            SECTION("Self-assignment is safe")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(666);
                TestObject* originalPtr = ref.Get();

                ref = ref; // Self-assignment

                REQUIRE(ref.Get() == originalPtr);
                REQUIRE(ref.UseCount() == 1);
                REQUIRE(ref->GetValue() == 666);
                REQUIRE(TestObject::GetDestroyCount() == 0);
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref object access", "[Ref][access]")
        {
            SECTION("Dereference operator provides object access")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(777);
                TestObject& obj = *ref;

                REQUIRE(obj.GetValue() == 777);

                obj.SetValue(888);
                REQUIRE(ref->GetValue() == 888);
            }

            SECTION("Arrow operator provides member access")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(999);

                REQUIRE(ref->GetValue() == 999);
                ref->SetValue(1010);
                REQUIRE(ref->GetValue() == 1010);
            }

            SECTION("Get() returns raw pointer")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(1111);
                TestObject* rawPtr = ref.Get();

                REQUIRE(rawPtr != nullptr);
                REQUIRE(rawPtr->GetValue() == 1111);
                REQUIRE(rawPtr == ref.Get()); // Same pointer
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref type conversions", "[Ref][conversion]")
        {
            SECTION("As() performs static cast")
            {
                Ref<DerivedTestObject> derived = CreateRef<DerivedTestObject>(1212, "base");
                Ref<TestObject> base = derived.As<TestObject>();

                REQUIRE(base);
                REQUIRE(base.UseCount() == 2);
                REQUIRE(base->GetValue() == 1212);
            }

            SECTION("DynamicCast() performs dynamic cast - success")
            {
                Ref<TestObject> base = CreateRef<DerivedTestObject>(1313, "derived");
                Ref<DerivedTestObject> derived = base.DynamicCast<DerivedTestObject>();

                REQUIRE(derived);
                REQUIRE(derived.UseCount() == 2);
                REQUIRE(derived->GetValue() == 1313);
                REQUIRE(derived->GetName() == "derived");
            }

            SECTION("DynamicCast() performs dynamic cast - failure")
            {
                Ref<TestObject> base = CreateRef<TestObject>(1414);
                Ref<DerivedTestObject> derived = base.DynamicCast<DerivedTestObject>();

                REQUIRE_FALSE(derived);
                REQUIRE(derived.Get() == nullptr);
                REQUIRE(base.UseCount() == 1); // Original reference unaffected
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref reset and swap", "[Ref][utility]")
        {
            SECTION("Reset() with no argument sets to null")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(1515);

                REQUIRE(TestObject::GetDestroyCount() == 0);

                ref.Reset();

                REQUIRE(TestObject::GetDestroyCount() == 1);
                REQUIRE_FALSE(ref);
                REQUIRE(ref.UseCount() == 0);
            }

            SECTION("Reset() with pointer changes managed object")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(1616);
                auto* newObject = new TestObject(1717);

                REQUIRE(TestObject::GetDestroyCount() == 0);

                ref.Reset(newObject);

                REQUIRE(TestObject::GetDestroyCount() == 1); // Old object destroyed
                REQUIRE(ref.Get() == newObject);
                REQUIRE(ref->GetValue() == 1717);
                REQUIRE(ref.UseCount() == 1);
            }

            SECTION("Swap() exchanges managed objects")
            {
                Ref<TestObject> ref1 = CreateRef<TestObject>(1818);
                Ref<TestObject> ref2 = CreateRef<TestObject>(1919);
                TestObject* ptr1 = ref1.Get();
                TestObject* ptr2 = ref2.Get();

                ref1.Swap(ref2);

                REQUIRE(ref1.Get() == ptr2);
                REQUIRE(ref2.Get() == ptr1);
                REQUIRE(ref1->GetValue() == 1919);
                REQUIRE(ref2->GetValue() == 1818);
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref comparison operators", "[Ref][comparison]")
        {
            SECTION("Equality comparison")
            {
                Ref<TestObject> ref1 = CreateRef<TestObject>(2020);
                Ref<TestObject> ref2 = ref1;
                Ref<TestObject> ref3 = CreateRef<TestObject>(2020); // Same value, different object

                REQUIRE(ref1 == ref2);
                REQUIRE_FALSE(ref1 == ref3);
                REQUIRE(ref1 != ref3);
                REQUIRE_FALSE(ref1 != ref2);
            }

            SECTION("Comparison with nullptr")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(2121);
                Ref<TestObject> nullRef;

                REQUIRE_FALSE(ref == nullptr);
                REQUIRE(nullRef == nullptr);
                REQUIRE(ref != nullptr);
                REQUIRE_FALSE(nullRef != nullptr);
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref interoperability with std::shared_ptr", "[Ref][interop]")
        {
            SECTION("Construction from std::shared_ptr")
            {
                auto sharedPtr = std::make_shared<TestObject>(2222);
                Ref<TestObject> ref(sharedPtr);

                REQUIRE(ref);
                REQUIRE(ref->GetValue() == 2222);
                // Note: UseCount might be different due to shared_ptr's control block
            }

            SECTION("Conversion to std::shared_ptr")
            {
                Ref<TestObject> ref = CreateRef<TestObject>(2323);
                auto sharedPtr = ref.ToSharedPtr();

                REQUIRE(sharedPtr);
                REQUIRE(sharedPtr->GetValue() == 2323);
                REQUIRE(sharedPtr.get() == ref.Get());
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "Ref memory management", "[Ref][memory]")
        {
            SECTION("Object destroyed when last reference is released")
            {
                {
                    Ref<TestObject> ref1 = CreateRef<TestObject>(2424);
                    {
                        Ref<TestObject> ref2 = ref1;
                        REQUIRE(ref1.UseCount() == 2);
                        REQUIRE(TestObject::GetDestroyCount() == 0);
                    }
                    REQUIRE(ref1.UseCount() == 1);
                    REQUIRE(TestObject::GetDestroyCount() == 0);
                }
                REQUIRE(TestObject::GetDestroyCount() == 1);
            }

            SECTION("Multiple objects managed independently")
            {
                Ref<TestObject> ref1 = CreateRef<TestObject>(2525);
                Ref<TestObject> ref2 = CreateRef<TestObject>(2626);

                REQUIRE(ref1.UseCount() == 1);
                REQUIRE(ref2.UseCount() == 1);
                REQUIRE(TestObject::GetDestroyCount() == 0);

                ref1 = nullptr;

                REQUIRE(TestObject::GetDestroyCount() == 1);
                REQUIRE(ref2.UseCount() == 1);
                REQUIRE(ref2->GetValue() == 2626);

                ref2 = nullptr;

                REQUIRE(TestObject::GetDestroyCount() == 2);
            }
        }

        TEST_CASE_METHOD(RefTestFixture, "CreateRef factory function", "[Ref][factory]")
        {
            SECTION("Creates object with correct arguments")
            {
                auto ref = CreateRef<TestObject>(2727);

                REQUIRE(ref);
                REQUIRE(ref->GetValue() == 2727);
                REQUIRE(ref.UseCount() == 1);
                REQUIRE(ref.IsUnique());
            }

            SECTION("Creates derived object")
            {
                auto ref = CreateRef<DerivedTestObject>(2828, "factory");

                REQUIRE(ref);
                REQUIRE(ref->GetValue() == 2828);
                REQUIRE(ref->GetName() == "factory");
                REQUIRE(ref.UseCount() == 1);
            }
        }
    }

    /// -------------------------------------------------------------------

}

/// -------------------------------------------------------------------
