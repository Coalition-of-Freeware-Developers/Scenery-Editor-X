/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* RefThreadSafetyTest.cpp
* -------------------------------------------------------
* Thread safety tests for the reference counting system
* -------------------------------------------------------
*/
#include <atomic>
#include <barrier>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <future>
#include <random>
#include <SceneryEditorX/utils/pointers.h>
#include <thread>
#include <vector>

/// -------------------------------------------------------------------

namespace SceneryEditorX
{
	namespace Tests
	{
		// Simple reference-counted object for thread safety tests
		class ThreadTestObject : public RefCounted
		{
		public:
		    explicit ThreadTestObject(const int id = 0) : m_Id(id) {}
		    int GetId() const { return m_Id; }
		    void SetId(const int id) { m_Id = id; }
		    
		    // Atomic counter for tracking construction/destruction
		    static std::atomic<int> s_InstanceCount;
		    
		    ThreadTestObject() : m_Id(0)
            {
                ++s_InstanceCount;
            }

            virtual ~ThreadTestObject() override { --s_InstanceCount; }
		
		private:
		    int m_Id;
		};
		
		// Initialize static counter
		std::atomic<int> ThreadTestObject::s_InstanceCount{0};
	
		// Test class with both strong and weak references
		class ComplexThreadTestObject : public RefCounted
		{
		public:
		    explicit ComplexThreadTestObject(const int id = 0) : m_Id(id) {}
		    
		    void SetStrongNext(const Ref<ComplexThreadTestObject>& next)
			{
		        m_StrongNext = next;
		    }
		    
		    void SetWeakNext(const Ref<ComplexThreadTestObject>& next)
			{
		        m_WeakNext = next;
		    }
		    
		    Ref<ComplexThreadTestObject> GetStrongNext() const { return m_StrongNext; }
		    
		    Ref<ComplexThreadTestObject> GetWeakNext() const { return m_WeakNext.Lock(); }
		    
		    int GetId() const { return m_Id; }
		    
		private:
		    int m_Id;
		    Ref<ComplexThreadTestObject> m_StrongNext;
		    WeakRef<ComplexThreadTestObject> m_WeakNext;
		};
	
		// Test concurrent creation and destruction of Ref objects
		TEST_CASE("Ref concurrent creation and destruction", "[Ref][thread][safety]")
		{
		    SECTION("Multiple threads creating and destroying Refs")
			{
		        constexpr int threadCount = 10;

                // Reset the instance counter
		        ThreadTestObject::s_InstanceCount = 0;
		        
		        // Create threads that will create and destroy objects
		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);
		        
		        for (int t = 0; t < threadCount; ++t)
                {
                    constexpr int objectsPerThread = 1000;
                    threads.emplace_back([t, objectsPerThread]()
					{
		                std::vector<Ref<ThreadTestObject>> objects;
		                objects.reserve(objectsPerThread);
		                
		                // Create objects
		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    objects.push_back(CreateRef<ThreadTestObject>(t * objectsPerThread + i));
		                }
		                
		                // Verify all objects exist
		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    REQUIRE(objects[i]->GetId() == t * objectsPerThread + i);
		                }
		                
		                // Destroy half the objects
		                for (int i = 0; i < objectsPerThread / 2; ++i)
						{
		                    objects[i] = nullptr;
		                }
		                
		                // Create some more objects
		                for (int i = 0; i < objectsPerThread / 2; ++i)
						{
		                    objects[i] = CreateRef<ThreadTestObject>(t * objectsPerThread + i + objectsPerThread);
		                }
		                
		                // Let objects go out of scope and be destroyed
		            });
		        }
		        
		        // Join all threads
		        for (auto& thread : threads)
				{
		            thread.join();
		        }
		        
		        // All objects should be destroyed when threads finish
		        REQUIRE(ThreadTestObject::s_InstanceCount == 0);
		    }
		}
	
		// Test concurrent access to shared Ref objects
		TEST_CASE("Ref concurrent access to shared objects", "[Ref][thread][safety]")
		{
		    SECTION("Multiple threads sharing and modifying Refs")
			{
		        constexpr int threadCount = 10;
		        constexpr int objectCount = 100;

                // Create shared objects
		        std::vector<Ref<ThreadTestObject>> sharedObjects;
		        sharedObjects.reserve(objectCount);
		        
		        for (int i = 0; i < objectCount; ++i)
				{
		            sharedObjects.push_back(CreateRef<ThreadTestObject>(i));
		        }
		        
		        // Use a barrier to ensure all threads start at the same time
		        std::barrier sync_point(threadCount + 1);
		        
		        // Create threads that will concurrently access and modify the shared objects
		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);
		        
		        for (int t = 0; t < threadCount; ++t)
                {
                    constexpr int operationsPerThread = 10000;
                    threads.emplace_back([t, operationsPerThread, &sharedObjects, &sync_point]()
					{
		                std::random_device rd;
		                std::mt19937 gen(rd());
		                std::uniform_int_distribution<int> objectDist(0, static_cast<int>(sharedObjects.size()) - 1);
		                
		                // Local copies of shared objects
		                std::vector<Ref<ThreadTestObject>> localCopies;
		                localCopies.reserve(operationsPerThread / 4);
		                
		                // Wait for all threads to be ready
		                sync_point.arrive_and_wait();
		                
		                for (int op = 0; op < operationsPerThread; ++op)
						{
		                    const int action = op % 4;
		                    const int objectIndex = objectDist(gen);
		                    
		                    switch (action)
							{
		                        case 0:
								{
		                            // Copy a shared object
		                            localCopies.push_back(sharedObjects[objectIndex]);
		                            break;
		                        }
		                        case 1:
								{
		                            // Modify a shared object
		                            const int oldValue = sharedObjects[objectIndex]->GetId();
		                            sharedObjects[objectIndex]->SetId(oldValue + 1);
		                            sharedObjects[objectIndex]->SetId(oldValue); // Restore original value
		                            break;
		                        }
		                        case 2:
								{
		                            // Replace a shared object
		                            int oldId = sharedObjects[objectIndex]->GetId();
		                            sharedObjects[objectIndex] = CreateRef<ThreadTestObject>(oldId);
		                            break;
		                        }
		                        case 3:
								{
		                            // Clear some local copies if we have any
		                            if (!localCopies.empty())
									{
		                                const int count = localCopies.size() / 2;
		                                localCopies.resize(count);
		                            }
		                            break;
		                        }
		                    }
		                }
		                
		                // Wait for all threads to finish operations
		                sync_point.arrive_and_wait();
		                
		                // Clear local copies
		                localCopies.clear();
		            });
		        }
		        
		        // Start the test
		        sync_point.arrive_and_wait();
		        
		        // Wait for threads to finish
		        sync_point.arrive_and_wait();
		        
		        // Join all threads
		        for (auto& thread : threads) { thread.join(); }
		        
		        // Verify the objects have their original IDs
		        for (int i = 0; i < objectCount; ++i) { REQUIRE(sharedObjects[i]->GetId() == i); }
		        
		        // Clean up
		        sharedObjects.clear();
		        
		        // All objects should be destroyed
		        REQUIRE(ThreadTestObject::s_InstanceCount == 0);
		    }
		}
	
		// Test concurrent operations on the same WeakRef objects
		TEST_CASE("WeakRef concurrent operations", "[WeakRef][thread][safety]")
		{
		    SECTION("Multiple threads using WeakRefs to the same objects")
			{
		        constexpr int threadCount = 10;
		        constexpr int objectCount = 100;

                // Create objects
		        std::vector<Ref<ThreadTestObject>> objects;
		        std::vector<WeakRef<ThreadTestObject>> weakRefs;
		        
		        objects.reserve(objectCount);
		        weakRefs.reserve(objectCount);
		        
		        for (int i = 0; i < objectCount; ++i)
				{
		            objects.push_back(CreateRef<ThreadTestObject>(i));
		            weakRefs.emplace_back(objects[i]);
		        }
		        
		        // Use a barrier to ensure all threads start at the same time
		        std::barrier sync_point(threadCount + 1);
		        
		        // Create threads that will concurrently access the weak references
		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);
		        
		        for (int t = 0; t < threadCount; ++t)
                {
                    constexpr int operationsPerThread = 10000;
                    threads.emplace_back([t, operationsPerThread, &objects, &weakRefs, &sync_point]()
					{
		                std::random_device rd;
		                std::mt19937 gen(rd());
		                std::uniform_int_distribution<int> objectDist(0, static_cast<int>(objects.size()) - 1);
		                
		                // Local storage for locked refs
		                std::vector<Ref<ThreadTestObject>> lockedRefs;
		                lockedRefs.reserve(operationsPerThread / 4);
		                
		                // Wait for all threads to be ready
		                sync_point.arrive_and_wait();
		                
		                for (int op = 0; op < operationsPerThread; ++op)
						{
		                    const int action = op % 4;
		                    const int objectIndex = objectDist(gen);
		                    
		                    switch (action)
							{
		                        case 0:
								{
		                            // Lock a weak reference
                                    if (auto locked = weakRefs[objectIndex].Lock())
									{
		                                lockedRefs.push_back(locked);
		                            }
		                            break;
		                        }
		                        case 1:
								{
		                            // Reset a weak reference
		                            weakRefs[objectIndex].Reset();
		                            // Re-assign it
		                            weakRefs[objectIndex] = objects[objectIndex];
		                            break;
		                        }
		                        case 2:
								{
		                            // Check if a weak reference is expired
		                            const bool expired = weakRefs[objectIndex].Expired();
		                            // It shouldn't be expired as long as objects are alive
		                            REQUIRE_FALSE(expired);
		                            break;
		                        }
		                        case 3:
								{
		                            // Clear some locked refs if we have any
		                            if (!lockedRefs.empty())
									{
		                                const int count = lockedRefs.size() / 2;
		                                lockedRefs.resize(count);
		                            }
		                            break;
		                        }
		                    }
		                }
		                
		                // Wait for all threads to finish operations
		                sync_point.arrive_and_wait();
		                
		                // Clear locked references
		                lockedRefs.clear();
		            });
		        }
		        
		        // Start the test
		        sync_point.arrive_and_wait();
		        
		        // Wait for threads to finish
		        sync_point.arrive_and_wait();
		        
		        // Join all threads
		        for (auto& thread : threads) { thread.join(); }
		        
		        // Verify the objects still have their original IDs
		        for (int i = 0; i < objectCount; ++i)
				{
		            REQUIRE(objects[i]->GetId() == i);
		            // WeakRefs should all be valid
		            REQUIRE_FALSE(weakRefs[i].Expired());
		            auto locked = weakRefs[i].Lock();
		            REQUIRE(locked != nullptr);
		            REQUIRE(locked->GetId() == i);
		        }
		        
		        // Now destroy the objects and verify weak refs are expired
		        objects.clear();
		        
		        for (int i = 0; i < objectCount; ++i)
				{
		            REQUIRE(weakRefs[i].Expired());
		            REQUIRE(weakRefs[i].Lock() == nullptr);
		        }
		        
		        // All objects should be destroyed
		        REQUIRE(ThreadTestObject::s_InstanceCount == 0);
		    }
		}
	
		// Test the control block registry's thread safety
		TEST_CASE("Control block registry thread safety", "[WeakRef][thread][registry]")
		{
		    SECTION("Multiple threads creating and destroying objects with WeakRefs")
			{
		        constexpr int threadCount = 10;

                // Reset the instance counter
		        ThreadTestObject::s_InstanceCount = 0;
		        
		        // Use a barrier to ensure all threads start at the same time
		        std::barrier sync_point(threadCount + 1);
		        
		        // Create threads that will create and destroy objects with WeakRefs
		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);
		        
		        for (int t = 0; t < threadCount; ++t)
                {
                    constexpr int objectsPerThread = 1000;
                    constexpr int operationsPerObject = 10;
                    threads.emplace_back([t, objectsPerThread, operationsPerObject, &sync_point]()
										 {
		                // Wait for all threads to be ready
		                sync_point.arrive_and_wait();
		                
		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    // Create an object
		                    auto obj = CreateRef<ThreadTestObject>(t * objectsPerThread + i);
		                    // Create a weak reference to it
		                    WeakRef<ThreadTestObject> weak(obj);
		                    
		                    // Perform operations on the object and weak reference
		                    for (int op = 0; op < operationsPerObject; ++op)
							{

                                switch (int action = op % 5)
								{
		                            case 0:
									{
		                                // Lock the weak reference
                                        if (auto locked = weak.Lock())
										{
		                                    REQUIRE(locked->GetId() == t * objectsPerThread + i);
		                                }
		                                break;
		                            }
		                            case 1:
									{
		                                // Reset the weak reference
		                                weak.Reset();
		                                // Re-assign it
		                                weak = obj;
		                                break;
		                            }
		                            case 2:
									{
		                                // Check if the weak reference is expired
		                                bool expired = weak.Expired();
		                                REQUIRE_FALSE(expired);
		                                break;
		                            }
		                            case 3:
									{
		                                // Reset the object temporarily
		                                auto temp = obj;
		                                obj = nullptr;
		                                // Check that weak ref is now expired
		                                REQUIRE(weak.Expired());
		                                // Restore the object
		                                obj = temp;
		                                break;
		                            }
		                            case 4:
									{
		                                // Create more weak references
		                                WeakRef<ThreadTestObject> weak2(obj);
                                        const WeakRef<ThreadTestObject> &weak3 = weak2;
		                                REQUIRE_FALSE(weak2.Expired());
		                                REQUIRE_FALSE(weak3.Expired());
		                                break;
		                            }
		                        }
		                    }
		                    
		                    // Let the object be destroyed
		                    obj = nullptr;
		                    REQUIRE(weak.Expired());
		                }
		            });
		        }
		        
		        // Start the test
		        sync_point.arrive_and_wait();
		        
		        // Join all threads
		        for (auto& thread : threads) {  thread.join(); }
		        
		        // All objects should be destroyed
		        REQUIRE(ThreadTestObject::s_InstanceCount == 0);
		    }
		}
	
		// Test for race conditions in the reference counting system
		TEST_CASE("Ref and WeakRef race condition test", "[Ref][WeakRef][thread][race]")
		{
			SECTION("Race between creating and destroying Refs with WeakRefs")
			{
			    constexpr int iterations = 1000;
			    
			    for (int iter = 0; iter < iterations; ++iter)
				{
			        // Create a shared object
			        auto obj = CreateRef<ThreadTestObject>(iter);
			        WeakRef<ThreadTestObject> weak(obj);
			        
			        // Launch two threads:
			        // 1. One that keeps creating and destroying Refs from the WeakRef
			        // 2. One that keeps setting the original Ref to nullptr and back
			        
			        std::atomic<bool> stop{false};
			        std::atomic<int> lockSuccesses{0};
			        std::atomic<int> lockFailures{0};
			        
			        // Thread 1: Keep locking and unlocking the weak reference
			        auto lockThread = std::thread([&weak, &stop, &lockSuccesses, &lockFailures]()
					{
			            while (!stop)
						{
                            if (auto locked = weak.Lock())
							{
			                    ++lockSuccesses;
			                }
							else
							{
			                    ++lockFailures;
			                }
			            }
			        });
			        
			        // Thread 2: Keep setting the original reference to nullptr and back
			        auto resetThread = std::thread([&obj, &stop]()
					{
			            const auto backup = obj;
			            for (int i = 0; i < 100; ++i)
						{
			                obj = nullptr;
			                std::this_thread::yield(); // Increase chance of race condition
			                obj = backup;
			                std::this_thread::yield();
			            }
			            stop = true;
			        });
			        
			        // Let the threads run
			        resetThread.join();
			        lockThread.join();
			        
			        // Either the weak ref should be expired (if obj is null) or valid (if obj is not null)
			        if (obj)
					{
			            REQUIRE_FALSE(weak.Expired());
			            auto locked = weak.Lock();
			            REQUIRE(locked != nullptr);
			            REQUIRE(locked->GetId() == iter);
			        }
					else
					{
			            REQUIRE(weak.Expired());
			            REQUIRE(weak.Lock() == nullptr);
			        }
			        
			        // Output statistics
			        INFO("Iteration " << iter << ": Lock successes: " << lockSuccesses << ", lock failures: " << lockFailures);
			    }
			}
			
			SECTION("Race between WeakRef operations")
			{
                constexpr int iterations = 100;
			    
			    for (int iter = 0; iter < iterations; ++iter)
				{
                    constexpr int threadCount = 10;
                    // Create shared data
			        auto obj = CreateRef<ThreadTestObject>(iter);
			        std::vector<WeakRef<ThreadTestObject>> weakRefs(threadCount);
			        
			        // Initialize all weak refs to point to the object
			        for (auto& weak : weakRefs) { weak = obj; }
			        
			        // Create threads that will perform various operations on the weak refs
			        std::vector<std::future<void>> futures;
			        
			        for (int t = 0; t < threadCount; ++t)
					{
			            futures.push_back(std::async(std::launch::async, [t, &weakRefs, &obj]()
						{
			                // Each thread does different operations based on its ID
			                switch (t % 5)
							{
			                    case 0:
								{
			                        // Lock and unlock
			                        for (int i = 0; i < 100; ++i)
									{
                                        if (const auto locked = weakRefs[t].Lock())
										{
			                                REQUIRE(locked->GetId() == weakRefs[t].Lock()->GetId());
			                            }
			                        }
			                        break;
			                    }
			                    case 1:
								{
			                        // Reset and reassign
			                        for (int i = 0; i < 100; ++i)
									{
			                            weakRefs[t].Reset();
			                            weakRefs[t] = obj;
			                        }
			                        break;
			                    }
			                    case 2:
								{
			                        // Check expiration
			                        for (int i = 0; i < 100; ++i)
									{
			                            bool expired = weakRefs[t].Expired();
			                            if (!expired)
										{
			                                auto locked = weakRefs[t].Lock();
			                                REQUIRE(locked != nullptr);
			                            }
			                        }
			                        break;
			                    }
			                    case 3:
								{
			                        // Copy between weak refs
			                        for (int i = 0; i < 100; ++i)
									{
			                            int otherIndex = (t + 1) % threadCount;
			                            weakRefs[t] = weakRefs[otherIndex];
			                        }
			                        break;
			                    }
			                    case 4:
								{
			                        // Move between weak refs
			                        for (int i = 0; i < 100; ++i)
									{
			                            int otherIndex = (t + 1) % threadCount;
			                            WeakRef<ThreadTestObject> temp = weakRefs[otherIndex];
			                            weakRefs[t] = std::move(temp);
			                            weakRefs[t] = obj; // Restore
			                        }
			                        break;
			                    }
			                }
			            }));
			        }
			        
			        // Wait for half the threads
			        for (int t = 0; t < threadCount / 2; ++t) { futures[t].wait(); }
			        
			        // Reset the object halfway through
			        obj = nullptr;
			        
			        // Wait for the rest of the threads
			        for (int t = threadCount / 2; t < threadCount; ++t) { futures[t].wait(); }
			        
			        // All weak refs should now be expired
			        for (const auto& weak : weakRefs)
					{
			            REQUIRE(weak.Expired());
			            REQUIRE(weak.Lock() == nullptr);
			        }
			    }
			}
		}
	
		// Test for scenarios involving complex objects with both Ref and WeakRef members
		TEST_CASE("Complex object with Ref and WeakRef members", "[Ref][WeakRef][thread][complex]")
		{
		    SECTION("Multiple threads creating and manipulating complex objects")
			{
		        constexpr int threadCount = 8;

                // Use a barrier to ensure all threads start at the same time
		        std::barrier sync_point(threadCount + 1);
		        
		        // Shared collection of objects that threads can connect to
		        std::vector<Ref<ComplexThreadTestObject>> sharedObjects;
		        std::mutex sharedObjectsMutex;
		        
		        // Create threads that will create complex objects
		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);
		        
		        for (int t = 0; t < threadCount; ++t)
                {
                    constexpr int objectsPerThread = 100;
                    threads.emplace_back([t, objectsPerThread, &sharedObjects, &sharedObjectsMutex, &sync_point]()
					{
		                std::random_device rd;
		                std::mt19937 gen(rd());
		                
		                // Create local objects
		                std::vector<Ref<ComplexThreadTestObject>> localObjects;
		                localObjects.reserve(objectsPerThread);
		                
		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    localObjects.push_back(CreateRef<ComplexThreadTestObject>(t * objectsPerThread + i));
		                }
		                
		                // Wait for all threads to have created their objects
		                sync_point.arrive_and_wait();
		                
		                // Make connections between objects
		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    // Connect to another random local object with strong reference
		                    int localTargetIdx = gen() % objectsPerThread;
		                    localObjects[i]->SetStrongNext(localObjects[localTargetIdx]);
		                    
		                    // Connect to a shared object with weak reference
		                    {
		                        std::lock_guard<std::mutex> lock(sharedObjectsMutex);
		                        if (!sharedObjects.empty())
								{
		                            int sharedTargetIdx = gen() % sharedObjects.size();
		                            localObjects[i]->SetWeakNext(sharedObjects[sharedTargetIdx]);
		                        }
		                    }
		                    
		                    // Add this object to the shared collection
		                    if (i % 10 == 0)
							{
		                        std::lock_guard<std::mutex> lock(sharedObjectsMutex);
		                        sharedObjects.push_back(localObjects[i]);
		                    }
		                }
		                
		                // Wait for all threads to have made connections
		                sync_point.arrive_and_wait();
		                
		                // Now traverse the object graph
		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    // Follow strong references
                            if (auto next = localObjects[i]->GetStrongNext())
							{
		                        REQUIRE(next->GetId() >= t * objectsPerThread);
		                        REQUIRE(next->GetId() < (t + 1) * objectsPerThread);
		                    }
		                    
		                    // Follow weak references
                            if (auto weakNext = localObjects[i]->GetWeakNext())
							{
		                        // The ID range for weak references is not predictable
		                        // since they can point to objects from any thread
		                        REQUIRE(weakNext->GetId() >= 0);
		                        REQUIRE(weakNext->GetId() < threadCount * objectsPerThread);
		                    }
		                }
		                
		                // Wait for all threads to have traversed the graph
		                sync_point.arrive_and_wait();
		                
		                // Break cycles to prevent memory leaks
		                for (auto& obj : localObjects)
						{
		                    obj->SetStrongNext(nullptr);
		                    obj->SetWeakNext(nullptr);
		                }
		                
		                // Clear local objects
		                localObjects.clear();
		            });
		        }
		        
		        // Start the test
		        sync_point.arrive_and_wait();
		        
		        // Wait for threads to make connections
		        sync_point.arrive_and_wait();
		        
		        // Wait for threads to traverse the graph
		        sync_point.arrive_and_wait();
		        
		        // Join all threads
		        for (auto& thread : threads)
				{
		            thread.join();
		        }
		        
		        // Clear shared objects
		        sharedObjects.clear();
		        
		        // All objects should be properly cleaned up
		        // (Can't easily verify this without a global counter)
		    }
		}
	
	}

    /// -------------------------------------------------------------------

}

/// -------------------------------------------------------------------
