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
 * atomic_flag.h
 * -------------------------------------------------------
 * Created: 16/02/2026
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	
	/**
	 * @struct AtomicFlag
	 * @brief A lightweight wrapper around std::atomic_flag providing intuitive dirty state management.
	 *
	 * The AtomicFlag class encapsulates a std::atomic_flag to provide a thread-safe
	 * mechanism for tracking and resetting a "dirty" state. This is useful for signaling
	 * that data has changed and needs processing, in a way that's safe for concurrent access.
	 *
	 * The implementation uses standard atomic operations to ensure thread safety without
	 * explicit locking. When copied, the new instance starts in a "clean" state regardless
	 * of the source object's state.
	 *
	 * @note - The semantics of atomic_flag in C++ guarantee that operations on the flag
	 *       are atomic and provide synchronization between threads.
	 */
	struct AtomicFlag
	{
	    /**
	     * @brief Sets the flag to dirty state.
	     *
	     * Marks the flag as dirty by clearing the atomic flag. This operation is atomic
	     * and can be safely called from multiple threads.
	     */
	    SEDX_FORCE_INLINE void SetDirty() { m_Flag.clear(); }

	    /**
	     * @brief Checks if the flag is dirty and atomically resets it if it is.
	     *
	     * Atomically tests if the flag is in the dirty state (cleared) and sets it
	     * (marking it as clean) in a single operation.
	     *
	     * @return true if the flag was dirty before this call (indicating data needs processing).
	     * @return false if the flag was already clean.
	     */
	    SEDX_FORCE_INLINE bool Check() { return !m_Flag.test_and_set(); }

	    /**
	     * @brief Constructs an AtomicFlag in clean state.
	     *
	     * The flag is initialized to the "clean" state (set).
	     */
	    explicit AtomicFlag() noexcept { m_Flag.test_and_set(); }


        /**
		 * @brief Default destructor for AtomicFlag.
		 *
		 * The destructor is defaulted and marked noexcept, as there are no resources that require
		 * special handling upon destruction. The atomic_flag will be automatically cleaned up when the
		 * object goes out of scope.
		 */
		~AtomicFlag() noexcept = default;

	    /**
	     * @brief Copy constructor creates a clean flag regardless of source state.
	     *
	     * When copying an AtomicFlag, the new instance is always initialized to
	     * the clean state, regardless of whether the source was dirty or clean.
	     */
	    AtomicFlag(const AtomicFlag&) noexcept {}

	    /**
	     * @brief Copy assignment operator preserves the current instance's state.
	     *
	     * The copy assignment operator doesn't modify the current instance's state.
	     * @return Reference to this instance.
	     */
	    AtomicFlag& operator=(const AtomicFlag&) noexcept { return *this; }

	    /**
	     * @brief Move constructor creates a clean flag regardless of source state.
	     *
	     * When moving an AtomicFlag, the new instance is always initialized to
	     * the clean state, regardless of whether the source was dirty or clean.
	     */
	    AtomicFlag(AtomicFlag&&) noexcept {}

	    /**
	     * @brief Move assignment operator preserves the current instance's state.
	     *
	     * The move assignment operator doesn't modify the current instance's state.
	     * @return Reference to this instance.
	     */
	    AtomicFlag& operator=(AtomicFlag&&) noexcept { return *this; }

	private:
	    std::atomic_flag m_Flag; // The underlying atomic flag that stores the state
	};

}

// -----------------------------------------------------------------
