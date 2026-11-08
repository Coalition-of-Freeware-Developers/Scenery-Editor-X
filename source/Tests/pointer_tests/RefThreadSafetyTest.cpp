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
*
* Two rules shape every test here:
* - Catch2's assertion macros are not thread-safe (this build does not define CATCH_CONFIG_THREAD_SAFE_ASSERTIONS),
*   so worker threads never call REQUIRE / CHECK. They record failures in a ThreadChecks object and the main
*   thread asserts after join().
* - Like std::shared_ptr / std::weak_ptr, one Ref or WeakRef *variable* is not safe to change while another
*   thread reads or changes it. What must be safe is many threads using their own Ref / WeakRef variables that
*   point at the same object, so the tests share objects between threads, never a variable some thread changes.
* -------------------------------------------------------
*/
#include <atomic>
#include <barrier>
#include <catch2/catch_test_macros.hpp>
#include <latch>
#include <mutex>
#include <random>
#include <SceneryEditorX/utils/pointers.h>
#include <string>
#include <thread>
#include <vector>

/// -------------------------------------------------------------------

namespace SceneryEditorX
{
	namespace Tests
	{
		/**
		 * @brief Failures seen on worker threads, reported by the main thread after join().
		 *
		 * Only the first failing check stores its expression and line; join() makes them visible to the main thread.
		 */
		class ThreadChecks
		{
		public:
		    void Check(const bool ok, const char *expression, const int line) noexcept
		    {
		        if (ok)
		            return;
		        if (m_Failures.fetch_add(1) == 0)
		        {
		            m_FirstExpression = expression;
		            m_FirstLine = line;
		        }
		    }

		    int Failures() const noexcept { return m_Failures.load(); }

		    std::string Describe() const
		    {
		        if (Failures() == 0)
		            return "no failed checks on worker threads";
		        return std::to_string(Failures()) + " failed check(s) on worker threads, first: " + m_FirstExpression +
		               " (RefThreadSafetyTest.cpp:" + std::to_string(m_FirstLine) + ")";
		    }

		private:
		    std::atomic<int> m_Failures{0};
		    const char *m_FirstExpression = "";
		    int m_FirstLine = 0;
		};

		/// Records a failed condition on a worker thread (never throws, never touches Catch2).
		#define THREAD_CHECK(checks, ...) (checks).Check(static_cast<bool>(__VA_ARGS__), #__VA_ARGS__, __LINE__)

		/// Main thread only: fails the test if any worker thread recorded a failure.
		#define REQUIRE_NO_THREAD_FAILURES(checks)                                                                   \
		    do                                                                                                       \
		    {                                                                                                        \
		        INFO((checks).Describe());                                                                           \
		        REQUIRE((checks).Failures() == 0);                                                                   \
		    } while (false)

		// Simple reference-counted object for thread safety tests
		class ThreadTestObject : public RefCounted
		{
		public:
		    explicit ThreadTestObject(const int id = 0) : m_Id(id) { ++s_InstanceCount; }
		    ~ThreadTestObject() override { --s_InstanceCount; }

		    int GetId() const { return m_Id; }

		    // Live instances, so a test can check that every object it made was destroyed exactly once
		    inline static std::atomic<int> s_InstanceCount{0};

		private:
		    const int m_Id;
		};

		/**
		 * @brief Counts its destructions in a counter that outlives it.
		 *
		 * Tells "destroyed once" from "destroyed twice" (a double free) and lets a thread holding a Ref check that
		 * the object was not destroyed under it, without reading freed memory.
		 */
		class TrackedObject : public RefCounted
		{
		public:
		    TrackedObject(const int id, std::atomic<int> &destructions) : m_Id(id), m_Destructions(destructions) {}
		    ~TrackedObject() override { ++m_Destructions; }

		    int GetId() const { return m_Id; }

		private:
		    const int m_Id;
		    std::atomic<int> &m_Destructions;
		};

		// Test class with both strong and weak references
		class ComplexThreadTestObject : public RefCounted
		{
		public:
		    explicit ComplexThreadTestObject(const int id = 0) : m_Id(id) { ++s_InstanceCount; }
		    ~ComplexThreadTestObject() override { --s_InstanceCount; }

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

		    inline static std::atomic<int> s_InstanceCount{0};

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
		        constexpr int objectsPerThread = 1000;

		        const int instancesBefore = ThreadTestObject::s_InstanceCount;
		        ThreadChecks checks;

		        // Create threads that will create and destroy objects
		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);

		        for (int t = 0; t < threadCount; ++t)
		        {
		            threads.emplace_back([t, &checks]()
					{
		                std::vector<Ref<ThreadTestObject>> objects;
		                objects.reserve(objectsPerThread);

		                // Create objects
		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    objects.push_back(CreateRef<ThreadTestObject>(t * objectsPerThread + i));
		                }

		                // Verify all objects exist and are owned once
		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    THREAD_CHECK(checks, objects[i]->GetId() == t * objectsPerThread + i);
		                    THREAD_CHECK(checks, objects[i].UseCount() == 1);
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
		                    THREAD_CHECK(checks, objects[i]->GetId() == t * objectsPerThread + i + objectsPerThread);
		                }

		                // Let objects go out of scope and be destroyed
		            });
		        }

		        for (auto& thread : threads)
				{
		            thread.join();
		        }

		        REQUIRE_NO_THREAD_FAILURES(checks);

		        // All objects should be destroyed when threads finish
		        REQUIRE(ThreadTestObject::s_InstanceCount == instancesBefore);
		    }
		}

		// Test concurrent access to shared Ref objects
		TEST_CASE("Ref concurrent access to shared objects", "[Ref][thread][safety]")
		{
		    SECTION("Multiple threads copying, moving and dropping Refs to the same objects")
			{
		        constexpr int threadCount = 10;
		        constexpr int objectCount = 100;
		        constexpr int operationsPerThread = 10000;

		        const int instancesBefore = ThreadTestObject::s_InstanceCount;
		        ThreadChecks checks;

		        // Shared objects. The vector and its Refs are only read while the threads run: every thread copies
		        // from them, and only its own copies are changed or dropped.
		        std::vector<Ref<ThreadTestObject>> sharedObjects;
		        sharedObjects.reserve(objectCount);

		        for (int i = 0; i < objectCount; ++i)
				{
		            sharedObjects.push_back(CreateRef<ThreadTestObject>(i));
		        }

		        // Ensure all threads start at the same time
		        std::latch start(threadCount + 1);

		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);

		        for (int t = 0; t < threadCount; ++t)
		        {
		            threads.emplace_back([t, &sharedObjects, &checks, &start]()
					{
		                std::mt19937 gen(static_cast<unsigned>(t) + 1u);
		                std::uniform_int_distribution<int> objectDist(0, static_cast<int>(sharedObjects.size()) - 1);

		                // Copies of shared objects, and objects only this thread owns
		                std::vector<Ref<ThreadTestObject>> localCopies;
		                localCopies.reserve(operationsPerThread / 2);
		                std::vector<Ref<ThreadTestObject>> ownObjects;
		                for (int i = 0; i < 16; ++i)
		                    ownObjects.push_back(CreateRef<ThreadTestObject>(-1));

		                start.arrive_and_wait();

		                for (int op = 0; op < operationsPerThread; ++op)
						{
		                    const int objectIndex = objectDist(gen);

		                    switch (op % 5)
							{
		                        case 0:
								{
		                            // Copy a shared object
		                            localCopies.push_back(sharedObjects[objectIndex]);
		                            THREAD_CHECK(checks, localCopies.back()->GetId() == objectIndex);
		                            break;
		                        }
		                        case 1:
								{
		                            // Copy a copy: the shared vector, the local copy and this one all hold it
		                            if (!localCopies.empty())
									{
		                                Ref<ThreadTestObject> copy = localCopies[gen() % localCopies.size()];
		                                THREAD_CHECK(checks, copy.UseCount() >= 3);
		                                localCopies.push_back(std::move(copy));
		                            }
		                            break;
		                        }
		                        case 2:
								{
		                            // Replace one of this thread's own objects (creation and destruction alongside the sharing)
		                            ownObjects[gen() % ownObjects.size()] = CreateRef<ThreadTestObject>(-1);
		                            break;
		                        }
		                        case 3:
								{
		                            // Move one local copy over another
		                            if (localCopies.size() >= 2)
									{
		                                const size_t from = gen() % localCopies.size();
		                                const size_t to = gen() % localCopies.size();
		                                if (from != to)
		                                {
		                                    localCopies[to] = std::move(localCopies[from]);
		                                    THREAD_CHECK(checks, localCopies[to] != nullptr);
		                                    localCopies[from] = sharedObjects[objectIndex];
		                                }
		                            }
		                            break;
		                        }
		                        case 4:
								{
		                            // Drop half of the local copies
		                            localCopies.resize(localCopies.size() / 2);
		                            break;
		                        }
		                    }
		                }

		                localCopies.clear();
		                ownObjects.clear();
		            });
		        }

		        start.arrive_and_wait();

		        for (auto& thread : threads) { thread.join(); }

		        REQUIRE_NO_THREAD_FAILURES(checks);

		        // The objects keep their IDs and every copy made on the threads was released
		        for (int i = 0; i < objectCount; ++i)
		        {
		            REQUIRE(sharedObjects[i]->GetId() == i);
		            REQUIRE(sharedObjects[i].UseCount() == 1);
		        }

		        sharedObjects.clear();

		        // All objects should be destroyed
		        REQUIRE(ThreadTestObject::s_InstanceCount == instancesBefore);
		    }
		}

		// Test concurrent operations on WeakRefs to the same objects
		TEST_CASE("WeakRef concurrent operations", "[WeakRef][thread][safety]")
		{
		    SECTION("Multiple threads using their own WeakRefs to the same objects")
			{
		        constexpr int threadCount = 10;
		        constexpr int objectCount = 100;
		        constexpr int operationsPerThread = 10000;

		        const int instancesBefore = ThreadTestObject::s_InstanceCount;
		        ThreadChecks checks;

		        // Objects and WeakRefs shared by all threads; only read while the threads run
		        std::vector<Ref<ThreadTestObject>> objects;
		        std::vector<WeakRef<ThreadTestObject>> weakRefs;

		        objects.reserve(objectCount);
		        weakRefs.reserve(objectCount);

		        for (int i = 0; i < objectCount; ++i)
				{
		            objects.push_back(CreateRef<ThreadTestObject>(i));
		            weakRefs.emplace_back(objects[i]);
		        }

		        std::latch start(threadCount + 1);

		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);

		        for (int t = 0; t < threadCount; ++t)
		        {
		            threads.emplace_back([t, &objects, &weakRefs, &checks, &start]()
					{
		                std::mt19937 gen(static_cast<unsigned>(t) + 1u);
		                std::uniform_int_distribution<int> objectDist(0, static_cast<int>(objects.size()) - 1);

		                std::vector<Ref<ThreadTestObject>> lockedRefs;
		                lockedRefs.reserve(operationsPerThread / 2);

		                start.arrive_and_wait();

		                // This thread's own WeakRefs, copied from the shared ones at the same time as the other threads
		                std::vector<WeakRef<ThreadTestObject>> myWeakRefs = weakRefs;

		                for (int op = 0; op < operationsPerThread; ++op)
						{
		                    const int objectIndex = objectDist(gen);

		                    switch (op % 5)
							{
		                        case 0:
								{
		                            // Lock a shared WeakRef: every object is alive, so it must succeed
		                            auto locked = weakRefs[objectIndex].Lock();
		                            THREAD_CHECK(checks, locked != nullptr);
		                            if (locked)
		                            {
		                                THREAD_CHECK(checks, locked->GetId() == objectIndex);
		                                lockedRefs.push_back(std::move(locked));
		                            }
		                            break;
		                        }
		                        case 1:
								{
		                            // Reset this thread's WeakRef and point it at the object again (through the registry):
		                            // it must find the object's existing control block
		                            auto &mine = myWeakRefs[objectIndex];
		                            mine.Reset();
		                            THREAD_CHECK(checks, mine.Expired());
		                            mine = objects[objectIndex];
		                            THREAD_CHECK(checks, !mine.Expired());
		                            THREAD_CHECK(checks, mine.GetControlBlock() == weakRefs[objectIndex].GetControlBlock());
		                            break;
		                        }
		                        case 2:
								{
		                            // Copy a shared WeakRef into this thread's own
		                            auto &mine = myWeakRefs[objectIndex];
		                            mine = weakRefs[objectIndex];
		                            THREAD_CHECK(checks, !mine.Expired());
		                            THREAD_CHECK(checks, mine.GetControlBlock() == weakRefs[objectIndex].GetControlBlock());
		                            break;
		                        }
		                        case 3:
								{
		                            // Lock this thread's own WeakRef
		                            auto locked = myWeakRefs[objectIndex].Lock();
		                            THREAD_CHECK(checks, locked != nullptr);
		                            if (locked)
		                            {
		                                THREAD_CHECK(checks, locked->GetId() == objectIndex);
		                                lockedRefs.push_back(std::move(locked));
		                            }
		                            break;
		                        }
		                        case 4:
								{
		                            // Drop half of the locked refs
		                            lockedRefs.resize(lockedRefs.size() / 2);
		                            break;
		                        }
		                    }
		                }

		                lockedRefs.clear();
		            });
		        }

		        start.arrive_and_wait();

		        for (auto& thread : threads) { thread.join(); }

		        REQUIRE_NO_THREAD_FAILURES(checks);

		        // The objects still have their IDs, every locked ref was released, and the WeakRefs are all valid
		        for (int i = 0; i < objectCount; ++i)
				{
		            REQUIRE(objects[i]->GetId() == i);
		            REQUIRE(objects[i].UseCount() == 1);
		            REQUIRE_FALSE(weakRefs[i].Expired());
		            REQUIRE(weakRefs[i].UseCount() == 1);
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
		        REQUIRE(ThreadTestObject::s_InstanceCount == instancesBefore);
		    }
		}

		// Test the control block registry's thread safety
		TEST_CASE("Control block registry thread safety", "[WeakRef][thread][registry]")
		{
		    SECTION("Multiple threads creating and destroying objects with WeakRefs")
			{
		        constexpr int threadCount = 10;
		        constexpr int objectsPerThread = 1000;
		        constexpr int operationsPerObject = 12;

		        const int instancesBefore = ThreadTestObject::s_InstanceCount;
		        ThreadChecks checks;

		        std::latch start(threadCount + 1);

		        // Every thread creates and destroys its own objects with WeakRefs, so they all use the registry at once
		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);

		        for (int t = 0; t < threadCount; ++t)
		        {
		            threads.emplace_back([t, &checks, &start]()
					{
		                start.arrive_and_wait();

		                for (int i = 0; i < objectsPerThread; ++i)
						{
		                    const int id = t * objectsPerThread + i;

		                    // Create an object and a weak reference to it
		                    auto obj = CreateRef<ThreadTestObject>(id);
		                    WeakRef<ThreadTestObject> weak(obj);

		                    for (int op = 0; op < operationsPerObject; ++op)
							{
		                        switch (op % 6)
								{
		                            case 0:
									{
		                                // Lock the weak reference
		                                auto locked = weak.Lock();
		                                THREAD_CHECK(checks, locked != nullptr);
		                                if (locked)
		                                    THREAD_CHECK(checks, locked->GetId() == id);
		                                break;
		                            }
		                            case 1:
									{
		                                // Reset the weak reference and re-assign it
		                                weak.Reset();
		                                THREAD_CHECK(checks, weak.Expired());
		                                weak = obj;
		                                THREAD_CHECK(checks, !weak.Expired());
		                                break;
		                            }
		                            case 2:
									{
		                                // The object is alive, so the weak reference is not expired
		                                THREAD_CHECK(checks, !weak.Expired());
		                                break;
		                            }
		                            case 3:
									{
		                                // Drop the original Ref while a copy keeps the object alive: the weak
		                                // reference must stay valid and see one strong reference
		                                auto keepAlive = obj;
		                                obj = nullptr;
		                                THREAD_CHECK(checks, !weak.Expired());
		                                THREAD_CHECK(checks, weak.UseCount() == 1);
		                                obj = keepAlive;
		                                break;
		                            }
		                            case 4:
									{
		                                // More weak references to the same object share its control block
		                                WeakRef<ThreadTestObject> weak2(obj);
		                                const WeakRef<ThreadTestObject> weak3 = weak2;
		                                THREAD_CHECK(checks, !weak2.Expired());
		                                THREAD_CHECK(checks, !weak3.Expired());
		                                THREAD_CHECK(checks, weak2.GetControlBlock() == weak.GetControlBlock());
		                                THREAD_CHECK(checks, weak3.GetControlBlock() == weak.GetControlBlock());
		                                break;
		                            }
		                            case 5:
									{
		                                // A short-lived object: its weak reference expires when it is destroyed
		                                WeakRef<ThreadTestObject> shortLived;
		                                {
		                                    auto temp = CreateRef<ThreadTestObject>(-1);
		                                    shortLived = temp;
		                                    THREAD_CHECK(checks, !shortLived.Expired());
		                                }
		                                THREAD_CHECK(checks, shortLived.Expired());
		                                THREAD_CHECK(checks, shortLived.Lock() == nullptr);
		                                break;
		                            }
		                        }
		                    }

		                    // Let the object be destroyed
		                    obj = nullptr;
		                    THREAD_CHECK(checks, weak.Expired());
		                    THREAD_CHECK(checks, weak.Lock() == nullptr);
		                }
		            });
		        }

		        start.arrive_and_wait();

		        for (auto& thread : threads) {  thread.join(); }

		        REQUIRE_NO_THREAD_FAILURES(checks);

		        // All objects should be destroyed
		        REQUIRE(ThreadTestObject::s_InstanceCount == instancesBefore);
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
			        std::atomic<int> destructions{0};

			        // Create a shared object
			        auto obj = CreateRef<TrackedObject>(iter, destructions);
			        WeakRef<TrackedObject> weak(obj);

			        // Launch two threads:
			        // 1. One that keeps creating and destroying Refs from the WeakRef (it only reads `weak`)
			        // 2. One that keeps setting the original Ref to nullptr and back (it alone touches `obj` until join)
			        // The reset thread's backup keeps the object alive, so every lock must succeed.

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

			        resetThread.join();
			        lockThread.join();

			        INFO("Iteration " << iter << ": lock successes: " << lockSuccesses << ", lock failures: " << lockFailures);
			        REQUIRE(lockFailures == 0);
			        REQUIRE(destructions == 0);

			        // The reset thread put the object back before it ended
			        REQUIRE(obj != nullptr);
			        REQUIRE_FALSE(weak.Expired());
			        auto locked = weak.Lock();
			        REQUIRE(locked != nullptr);
			        REQUIRE(locked->GetId() == iter);

			        locked = nullptr;
			        obj = nullptr;
			        REQUIRE(destructions == 1);
			        REQUIRE(weak.Expired());
			        REQUIRE(weak.Lock() == nullptr);
			    }
			}

			SECTION("Race between WeakRef operations while the last Ref is released")
			{
		        constexpr int iterations = 200;
		        constexpr int threadCount = 8;
		        constexpr int operationsPerThread = 2000;

			    for (int iter = 0; iter < iterations; ++iter)
				{
			        std::atomic<int> destructions{0};
			        auto obj = CreateRef<TrackedObject>(iter, destructions);
			        const WeakRef<TrackedObject> original = obj; // only read while the threads run

			        // One WeakRef per thread, changed only by that thread while it runs and read here after join()
			        std::vector<WeakRef<TrackedObject>> threadWeakRefs(threadCount, original);

			        ThreadChecks checks;
			        std::atomic<int> operationsDone{0};
			        std::latch start(threadCount + 1);

			        std::vector<std::thread> threads;
			        threads.reserve(threadCount);

			        for (int t = 0; t < threadCount; ++t)
					{
			            threads.emplace_back([t, iter, &threadWeakRefs, &original, &checks, &destructions, &operationsDone, &start]()
						{
			                WeakRef<TrackedObject> &mine = threadWeakRefs[t];
			                WeakRef<TrackedObject> spare = mine; // a second WeakRef only this thread uses

			                start.arrive_and_wait();

			                for (int i = 0; i < operationsPerThread; ++i)
							{
			                    switch ((t + i) % 5)
								{
			                        case 0:
									{
			                            // Lock and use: a Ref from Lock() keeps the object alive
			                            if (const auto locked = mine.Lock())
										{
			                                THREAD_CHECK(checks, destructions == 0);
			                                THREAD_CHECK(checks, locked->GetId() == iter);
			                            }
			                            break;
			                        }
			                        case 1:
									{
			                            // Reset, then point at the object again through a Ref locked from the shared WeakRef
			                            mine.Reset();
			                            if (const auto locked = original.Lock())
			                            {
			                                mine = locked;
			                                THREAD_CHECK(checks, !mine.Expired());
			                            }
			                            break;
			                        }
			                        case 2:
									{
			                            // Not expired, then lock: the object may be destroyed in between, which is fine;
			                            // a Ref that Lock() does return must hold a live object
			                            if (!mine.Expired())
										{
			                                if (const auto locked = mine.Lock())
			                                    THREAD_CHECK(checks, destructions == 0);
			                            }
			                            break;
			                        }
			                        case 3:
									{
			                            // Copy between this thread's WeakRefs and from the shared one
			                            spare = mine;
			                            mine = spare;
			                            spare = original;
			                            break;
			                        }
			                        case 4:
									{
			                            // Move between this thread's WeakRefs
			                            WeakRef<TrackedObject> temp = spare;
			                            mine = std::move(temp);
			                            THREAD_CHECK(checks, temp.GetControlBlock() == nullptr);
			                            break;
			                        }
			                    }
			                    ++operationsDone;
			                }
			            });
			        }

			        start.arrive_and_wait();

			        // Let the threads get going, then release the only strong reference while they lock, copy and
			        // reset WeakRefs to the object
			        while (operationsDone.load() < threadCount * operationsPerThread / 2)
			            std::this_thread::yield();
			        obj = nullptr;

			        for (auto &thread : threads) { thread.join(); }

			        INFO("Iteration " << iter);
			        REQUIRE_NO_THREAD_FAILURES(checks);
			        REQUIRE(destructions == 1);

			        // Every WeakRef is now expired
			        REQUIRE(original.Expired());
			        REQUIRE(original.Lock() == nullptr);
			        for (const auto& weak : threadWeakRefs)
					{
			            REQUIRE(weak.Expired());
			            REQUIRE(weak.Lock() == nullptr);
			        }
			    }
			}

			SECTION("WeakRef::Lock racing with the release of the last Ref")
			{
			    constexpr int iterations = 2000;
			    constexpr int threadCount = 4;

			    for (int iter = 0; iter < iterations; ++iter)
				{
			        std::atomic<int> destructions{0};
			        auto obj = CreateRef<TrackedObject>(iter, destructions);

			        // One WeakRef per thread
			        std::vector<WeakRef<TrackedObject>> weakRefs(threadCount, WeakRef<TrackedObject>(obj));

			        ThreadChecks checks;
			        std::atomic<int> running{0};

			        std::vector<std::thread> threads;
			        threads.reserve(threadCount);

			        for (int t = 0; t < threadCount; ++t)
					{
			            threads.emplace_back([t, iter, &weakRefs, &checks, &destructions, &running]()
						{
			                const WeakRef<TrackedObject> &mine = weakRefs[t];
			                ++running;

			                // Lock until the object is gone. Whichever Ref is released last (the main thread's or a
			                // locked one) destroys it; a lock may never bring it back.
			                while (const auto locked = mine.Lock())
							{
			                    THREAD_CHECK(checks, destructions == 0);
			                    THREAD_CHECK(checks, locked->GetId() == iter);
			                }
			                THREAD_CHECK(checks, mine.Expired());
			            });
			        }

			        // Release the only strong reference while every thread is locking
			        while (running.load() < threadCount)
			            std::this_thread::yield();
			        obj = nullptr;

			        for (auto &thread : threads) { thread.join(); }

			        INFO("Iteration " << iter);
			        REQUIRE_NO_THREAD_FAILURES(checks);
			        REQUIRE(destructions == 1);
			        for (const auto &weak : weakRefs)
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
		        constexpr int objectsPerThread = 100;

		        const int instancesBefore = ComplexThreadTestObject::s_InstanceCount;
		        ThreadChecks checks;

		        // Keeps the phases apart: create, connect, traverse
		        std::barrier sync_point(threadCount + 1);

		        // Shared collection of objects that threads can connect to (guarded by the mutex)
		        std::vector<Ref<ComplexThreadTestObject>> sharedObjects;
		        std::mutex sharedObjectsMutex;

		        std::vector<std::thread> threads;
		        threads.reserve(threadCount);

		        for (int t = 0; t < threadCount; ++t)
		        {
		            threads.emplace_back([t, &sharedObjects, &sharedObjectsMutex, &sync_point, &checks]()
					{
		                std::mt19937 gen(static_cast<unsigned>(t) + 1u);

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
		                    const int localTargetIdx = static_cast<int>(gen() % objectsPerThread);
		                    localObjects[i]->SetStrongNext(localObjects[localTargetIdx]);

		                    // Connect to a shared object (maybe another thread's) with weak reference
		                    {
		                        std::lock_guard<std::mutex> lock(sharedObjectsMutex);
		                        if (!sharedObjects.empty())
								{
		                            const size_t sharedTargetIdx = gen() % sharedObjects.size();
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
		                    // Follow strong references (always to this thread's objects)
		                    if (auto next = localObjects[i]->GetStrongNext())
							{
		                        THREAD_CHECK(checks, next->GetId() >= t * objectsPerThread);
		                        THREAD_CHECK(checks, next->GetId() < (t + 1) * objectsPerThread);
		                    }

		                    // Follow weak references: they can point to objects from any thread, all kept alive by
		                    // the shared collection until the threads are joined
		                    if (auto weakNext = localObjects[i]->GetWeakNext())
							{
		                        THREAD_CHECK(checks, weakNext->GetId() >= 0);
		                        THREAD_CHECK(checks, weakNext->GetId() < threadCount * objectsPerThread);
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

		                localObjects.clear();
		            });
		        }

		        // Start the test, then wait for the connections and the traversal
		        sync_point.arrive_and_wait();
		        sync_point.arrive_and_wait();
		        sync_point.arrive_and_wait();

		        for (auto& thread : threads)
				{
		            thread.join();
		        }

		        REQUIRE_NO_THREAD_FAILURES(checks);

		        sharedObjects.clear();

		        // All objects should be properly cleaned up
		        REQUIRE(ComplexThreadTestObject::s_InstanceCount == instancesBefore);
		    }
		}

		#undef REQUIRE_NO_THREAD_FAILURES
		#undef THREAD_CHECK

	}

    /// -------------------------------------------------------------------

}

/// -------------------------------------------------------------------
