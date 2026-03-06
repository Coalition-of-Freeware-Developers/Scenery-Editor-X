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
 * flag.h
 * -------------------------------------------------------
 * Created: 16/02/2026
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	
	/**
	 * @struct Flag
	 * @brief A lightweight boolean flag class that tracks and resets dirty state.
	 *
	 * The Flag class provides a simple mechanism to track whether something has been
	 * marked as "dirty" (needing attention) and to atomically check and reset this state.
	 * Unlike AtomicFlag, this implementation uses a regular bool and is not thread-safe.
	 */
	struct Flag
	{
		/**
		 * @brief Sets the flag to dirty state.
		 *
		 * Marks the flag as dirty, indicating that some action or update is needed.
		 */
		SEDX_FORCE_INLINE void SetDirty() noexcept { m_Flag = true; }

		/**
		 * @brief Checks if the flag is dirty and atomically resets it if it is.
		 *
		 * @return true if the flag was dirty before the reset operation.
		 * @return false if the flag was not dirty.
		 */
		SEDX_FORCE_INLINE bool Check() noexcept
		{
			if (m_Flag)
			{
			    return !((m_Flag = !m_Flag));
			}

		    return false;
        }

		/**
		 * @brief Checks if the flag is currently in a dirty state.
		 *
		 * Unlike Check(), this method does not modify the flag's state.
		 *
		 * @return true if the flag is dirty.
		 * @return false if the flag is not dirty.
		 */
		SEDX_FORCE_INLINE bool IsDirty() const noexcept { return m_Flag; }

	private:
		bool m_Flag = false; // Internal boolean that stores the dirty state, initially not dirty
	};


}

// -----------------------------------------------------------------
