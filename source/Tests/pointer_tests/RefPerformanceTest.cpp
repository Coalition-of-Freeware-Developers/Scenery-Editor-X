/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* RefPerformanceTest.cpp
* -------------------------------------------------------
* Performance and stress tests for the reference counting system
* -------------------------------------------------------
*/
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <random>
#include <SceneryEditorX/core/pointers.h>
#include <thread>
#include <vector>

namespace SceneryEditorX
{
	namespace Tests
	{
		// Simple reference-counted object
		class PerfTestObject : public RefCounted
		{
		public:
		    explicit PerfTestObject(int id = 0) : m_Id(id) {}
		    int GetId() const { return m_Id; }
		    void SetId(int id) { m_Id = id; }
		
		private:
		    int m_Id;
		};
		
		// Performance test for creating and destroying Ref objects
		TEST_CASE("Ref creation and destruction performance", "[Ref][performance]")
		{
		    constexpr size_t iterations = 1000000;
		    
		    SECTION("Create and destroy Refs in sequence")
			{
		        auto startTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i)
				{
		            auto ref = CreateRef<PerfTestObject>(static_cast<int>(i));
		            REQUIRE(ref->GetId() == static_cast<int>(i));
		        }
		        
		        auto endTime = std::chrono::high_resolution_clock::now();
		        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		        
		        INFO("Time to create and destroy " << iterations << " Refs: " << duration << " ms");
		        // No assertion, just measuring performance
		    }
		    
		    SECTION("Create and store Refs in a vector")
			{
		        std::vector<Ref<PerfTestObject>> refs;
		        refs.reserve(iterations);
		        
		        auto startTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i)
				{
		            refs.push_back(CreateRef<PerfTestObject>(static_cast<int>(i)));
		        }
		        
		        // Clear the vector to destroy all Refs
		        refs.clear();
		        
		        auto endTime = std::chrono::high_resolution_clock::now();
		        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		        
		        INFO("Time to create, store, and destroy " << iterations << " Refs: " << duration << " ms");
		        // No assertion, just measuring performance
		    }
		}
		
		// Performance test for copying and moving Ref objects
		TEST_CASE("Ref copy and move performance", "[Ref][performance]")
		{
		    constexpr size_t iterations = 1000000;
		    
		    SECTION("Copy Refs")
			{
		        auto original = CreateRef<PerfTestObject>(42);
		        
		        auto startTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i)
				{
		            Ref<PerfTestObject> copy = original;
		            REQUIRE(copy->GetId() == 42);
		        }
		        
		        auto endTime = std::chrono::high_resolution_clock::now();
		        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		        
		        INFO("Time to copy " << iterations << " Refs: " << duration << " ms");
		    }
		    
		    SECTION("Move Refs")
			{
		        auto startTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i)
				{
		            auto original = CreateRef<PerfTestObject>(42);
		            Ref<PerfTestObject> moved = std::move(original);
		            REQUIRE(moved->GetId() == 42);
		            REQUIRE(original == nullptr);
		        }
		        
		        auto endTime = std::chrono::high_resolution_clock::now();
		        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		        
		        INFO("Time to move " << iterations << " Refs: " << duration << " ms");
		    }
		}
		
		// Stress test for the reference counting system with many references to the same object
		TEST_CASE("Ref reference counting stress test", "[Ref][stress]")
		{
		    constexpr size_t refCount = 10000;
		    
		    SECTION("Many Refs to the same object")
			{
		        auto original = CreateRef<PerfTestObject>(42);
		        std::vector<Ref<PerfTestObject>> refs(refCount);
		        
		        // Create many references to the same object
		        for (auto& ref : refs) { ref = original; }
		        
		        REQUIRE(original.UseCount() == refCount + 1);
		        
		        // Destroy half of the references
		        for (size_t i = 0; i < refCount / 2; ++i) { refs[i] = nullptr; }
		        
		        REQUIRE(original.UseCount() == refCount / 2 + 1);
		        
		        // Destroy the rest of the references
		        refs.clear();
		        
		        REQUIRE(original.UseCount() == 1);
		        REQUIRE(original->GetId() == 42);
		    }
		}
		
		// Stress test for WeakRef with many weak references to the same object
		TEST_CASE("WeakRef reference counting stress test", "[WeakRef][stress]")
		{
		    constexpr size_t weakRefCount = 10000;
		    
		    SECTION("Many WeakRefs to the same object")
			{
		        auto original = CreateRef<PerfTestObject>(42);
		        std::vector<WeakRef<PerfTestObject>> weakRefs(weakRefCount);
		        
		        // Create many weak references to the same object
		        for (auto& weakRef : weakRefs) { weakRef = original; }
		        
		        // UseCount should still be 1 as weak references don't affect the reference count
		        REQUIRE(original.UseCount() == 1);
		        
		        // Lock half of the weak references
		        std::vector<Ref<PerfTestObject>> lockedRefs;
		        lockedRefs.reserve(weakRefCount / 2);
		        
		        for (size_t i = 0; i < weakRefCount / 2; ++i)
				{
		            lockedRefs.push_back(weakRefs[i].Lock());
		        }
		        
		        // Now the reference count should include the locked refs
		        REQUIRE(original.UseCount() == weakRefCount / 2 + 1);
		        
		        // Destroy the original reference
		        original = nullptr;
		        
		        // Locked refs should still be valid
		        for (const auto& ref : lockedRefs)
				{
		            REQUIRE(ref != nullptr);
		            REQUIRE(ref->GetId() == 42);
		        }
		        
		        // Weak refs that weren't locked should be expired
		        for (size_t i = weakRefCount / 2; i < weakRefCount; ++i)
				{
		            REQUIRE(weakRefs[i].Expired());
		            REQUIRE(weakRefs[i].Lock() == nullptr);
		        }
		        
		        // Destroy the locked refs
		        lockedRefs.clear();
		        
		        // Now all weak refs should be expired
		        for (const auto& weakRef : weakRefs)
				{
		            REQUIRE(weakRef.Expired());
		            REQUIRE(weakRef.Lock() == nullptr);
		        }
		    }
		}
		
		// Multi-threaded stress test for the reference counting system
		TEST_CASE("Ref multi-threaded stress test", "[Ref][stress][thread]")
		{
		    constexpr size_t threadCount = 8;
		    constexpr size_t objectsPerThread = 1000;
		    constexpr size_t operationsPerObject = 100;
		    
		    SECTION("Multiple threads creating and sharing Refs")
			{
		        // Shared pool of objects that threads will access
		        std::array<Ref<PerfTestObject>, threadCount * objectsPerThread> sharedObjects;
		        
		        // Create the initial objects
		        for (size_t i = 0; i < sharedObjects.size(); ++i)
				{
		            sharedObjects[i] = CreateRef<PerfTestObject>(static_cast<int>(i));
		        }
		        
		        // Create threads that will randomly access objects
		        std::vector<std::thread> threads;
		        
		        for (size_t t = 0; t < threadCount; ++t)
				{
		            threads.emplace_back([&sharedObjects, t, objectsPerThread, operationsPerObject]()
					{
		                std::random_device rd;
		                std::mt19937 gen(rd());
		                std::uniform_int_distribution<size_t> dist(0, sharedObjects.size() - 1);
		                
		                std::vector<Ref<PerfTestObject>> localRefs;
		                localRefs.reserve(objectsPerThread);
		                
		                // Each thread does a mix of operations:
		                // 1. Create a new reference to a shared object
		                // 2. Reset its references
		                // 3. Create new objects
		                for (size_t i = 0; i < objectsPerThread; ++i)
						{
		                    for (size_t op = 0; op < operationsPerObject; ++op)
							{
		                        size_t action = op % 3;
		                        
		                        switch (action)
								{
		                            case 0:
									{
		                                // Copy a shared object
		                                size_t index = dist(gen);
		                                localRefs.push_back(sharedObjects[index]);
		                                break;
		                            }
		                            case 1:
									{
		                                // Reset some references if we have any
		                                if (!localRefs.empty())
										{
		                                    size_t index = gen() % localRefs.size();
		                                    localRefs[index] = nullptr;
		                                }
		                                break;
		                            }
		                            case 2:
									{
		                                // Create a new object
		                                int id = static_cast<int>(t * objectsPerThread * operationsPerObject + i * operationsPerObject + op);
		                                localRefs.push_back(CreateRef<PerfTestObject>(id));
		                                break;
		                            }
		                        }
		                    }
		                }
		                
		                // Clean up local references
		                localRefs.clear();
		            });
		        }
		        
		        // Join all threads
		        for (auto& thread : threads)
				{
		            thread.join();
		        }
		        
		        // Verify that all shared objects still have their original IDs
		        for (size_t i = 0; i < sharedObjects.size(); ++i)
				{
		            REQUIRE(sharedObjects[i]->GetId() == static_cast<int>(i));
		        }
		        
		        // Clean up
		        for (auto& obj : sharedObjects)
				{
		            obj = nullptr;
		        }
		    }
		}
		
		// Memory leak test for the reference counting system
		TEST_CASE("Ref and WeakRef memory leak test", "[Ref][WeakRef][memory]")
		{
		    constexpr size_t iterations = 10000;
		    
		    SECTION("Creating and destroying objects with cycles")
			{
		        class Node : public RefCounted
				{
		        public:
		            void SetNext(const Ref<Node>& next) { m_Next = next; }
		            void SetPrev(const Ref<Node>& prev) { m_Prev = prev; }
		            void SetWeakNext(const Ref<Node>& next) { m_WeakNext = next; }
		            
		        private:
		            Ref<Node> m_Next;
		            Ref<Node> m_Prev;
		            WeakRef<Node> m_WeakNext;
		        };
		        
		        // Track the number of destroyed objects
		        size_t destroyedCount = 0;
		        
		        // Create a custom object that increments a counter when destroyed
		        class TrackedNode : public RefCounted
				{
		        public:
		            explicit TrackedNode(size_t* counter) : m_Counter(counter) {}
		            
		            ~TrackedNode() override
					{
		                if (m_Counter) {
		                    ++(*m_Counter);
		                }
		            }
		            
		            void SetNext(const Ref<TrackedNode>& next) { m_Next = next; }
		            void SetPrev(const Ref<TrackedNode>& prev) { m_Prev = prev; }
		            void SetWeakNext(const Ref<TrackedNode>& next) { m_WeakNext = next; }
		            
		        private:
		            size_t* m_Counter;
		            Ref<TrackedNode> m_Next;
		            Ref<TrackedNode> m_Prev;
		            WeakRef<TrackedNode> m_WeakNext;
		        };
		        
		        // Create cycles that should be manually broken
		        for (size_t i = 0; i < iterations; ++i)
				{
		            auto nodeA = CreateRef<TrackedNode>(&destroyedCount);
		            auto nodeB = CreateRef<TrackedNode>(&destroyedCount);
		            
		            // Create a reference cycle
		            nodeA->SetNext(nodeB);
		            nodeB->SetPrev(nodeA);
		            
		            // Break the cycle
		            nodeA->SetNext(nullptr);
		            nodeB->SetPrev(nullptr);
		        }
		        
		        // Create cycles that use WeakRef to avoid leaks
		        for (size_t i = 0; i < iterations; ++i)
				{
		            auto nodeA = CreateRef<TrackedNode>(&destroyedCount);
		            auto nodeB = CreateRef<TrackedNode>(&destroyedCount);
		            
		            // Create a cycle using WeakRef for one direction
		            nodeA->SetNext(nodeB);
		            nodeB->SetWeakNext(nodeA);
		        }
		        
		        // All nodes should be destroyed
		        REQUIRE(destroyedCount == iterations * 4);
		    }
		}
		
		// Tests comparing Ref with std::shared_ptr
		TEST_CASE("Ref vs std::shared_ptr performance comparison", "[Ref][performance][comparison]")
		{
		    constexpr size_t iterations = 1000000;
		    
		    SECTION("Creation and destruction")
			{
		        // Test Ref
		        auto refStartTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i)
				{
		            auto ref = CreateRef<PerfTestObject>(static_cast<int>(i));
		        }
		        
		        auto refEndTime = std::chrono::high_resolution_clock::now();
		        auto refDuration = std::chrono::duration_cast<std::chrono::milliseconds>(refEndTime - refStartTime).count();
		        
		        // Test std::shared_ptr
		        auto sharedStartTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i)
				{
		            auto shared = std::make_shared<PerfTestObject>(static_cast<int>(i));
		        }
		        
		        auto sharedEndTime = std::chrono::high_resolution_clock::now();
		        auto sharedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sharedEndTime - sharedStartTime).count();
		        
		        INFO("Ref creation/destruction time: " << refDuration << " ms");
		        INFO("std::shared_ptr creation/destruction time: " << sharedDuration << " ms");
		    }
		    
		    SECTION("Copying")
			{
		        auto ref = CreateRef<PerfTestObject>(42);
		        auto shared = std::make_shared<PerfTestObject>(42);
		        
		        // Test Ref copying
		        auto refStartTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i) { Ref<PerfTestObject> copy = ref; }
		        
		        auto refEndTime = std::chrono::high_resolution_clock::now();
		        auto refDuration = std::chrono::duration_cast<std::chrono::milliseconds>(refEndTime - refStartTime).count();
		        
		        // Test std::shared_ptr copying
		        auto sharedStartTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i) { std::shared_ptr<PerfTestObject> copy = shared; }
		        
		        auto sharedEndTime = std::chrono::high_resolution_clock::now();
		        auto sharedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sharedEndTime - sharedStartTime).count();
		        
		        INFO("Ref copying time: " << refDuration << " ms");
		        INFO("std::shared_ptr copying time: " << sharedDuration << " ms");
		    }
		    
		    SECTION("Interoperability")
			{
		        // Test converting Ref to std::shared_ptr
		        auto ref = CreateRef<PerfTestObject>(42);
		        
		        auto toSharedStartTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i)
				{
		            auto shared = ref.ToSharedPtr();
		            REQUIRE(shared->GetId() == 42);
		        }
		        
		        auto toSharedEndTime = std::chrono::high_resolution_clock::now();
		        auto toSharedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(toSharedEndTime - toSharedStartTime).count();
		        
		        // Test converting std::shared_ptr to Ref
		        auto shared = std::make_shared<PerfTestObject>(42);
		        
		        auto toRefStartTime = std::chrono::high_resolution_clock::now();
		        
		        for (size_t i = 0; i < iterations; ++i)
				{
		            Ref<PerfTestObject> convertedRef(shared);
		            REQUIRE(convertedRef->GetId() == 42);
		        }
		        
		        auto toRefEndTime = std::chrono::high_resolution_clock::now();
		        auto toRefDuration = std::chrono::duration_cast<std::chrono::milliseconds>(toRefEndTime - toRefStartTime).count();
		        
		        INFO("Ref to std::shared_ptr conversion time: " << toSharedDuration << " ms");
		        INFO("std::shared_ptr to Ref conversion time: " << toRefDuration << " ms");
		    }
		}

	}  // namespace Tests
}  // namespace SceneryEditorX
