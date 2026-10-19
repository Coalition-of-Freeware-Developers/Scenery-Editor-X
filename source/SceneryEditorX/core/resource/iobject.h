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
 * iobject.h
 * -------------------------------------------------------
 * Created: 16/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	class IObject
	{
	public:
	    // IDs come from a process-wide counter, so no two objects share one and they can key caches (bound buffers,
	    // pipeline hashes, watched shaders). They used to be derived from the clock and repeated for objects created
	    // within one clock tick.
	    IObject() : m_ObjectId(NextObjectId()) {}

	    // A copy is another object: it gets its own ID. Assignment keeps the target's.
	    IObject(const IObject &other) : m_ObjectName(other.m_ObjectName), m_ObjectId(NextObjectId()), m_ObjectSize(other.m_ObjectSize) {}
	    IObject(IObject &&other) noexcept : m_ObjectName(std::move(other.m_ObjectName)), m_ObjectId(NextObjectId()), m_ObjectSize(other.m_ObjectSize) {}

	    IObject &operator=(const IObject &other)
	    {
	        m_ObjectName = other.m_ObjectName;
	        m_ObjectSize = other.m_ObjectSize;
	        return *this;
	    }

	    IObject &operator=(IObject &&other) noexcept
	    {
	        m_ObjectName = std::move(other.m_ObjectName);
	        m_ObjectSize = other.m_ObjectSize;
	        return *this;
	    }

	    ~IObject() = default;

	    // Object name
	    const std::string &GetObjectName() const { return m_ObjectName; }
	    void SetObjectName(const std::string &name) { m_ObjectName = name; }
	
	    // Object ID
	    const uint64_t GetObjectId() const { return m_ObjectId; }
	    void SetObjectId(const uint64_t id) { m_ObjectId = id; }
	
	    // Object size
	    const uint64_t GetObjectSize() const { return m_ObjectSize; }
	
	protected:
	    std::string m_ObjectName;
	    uint64_t m_ObjectId = 0;
	    uint64_t m_ObjectSize = 0;
	
	private:
	    static uint64_t NextObjectId()
	    {
	        static std::atomic<uint64_t> s_NextObjectId{1}; // 0 stays free for "no object"
	        return s_NextObjectId.fetch_add(1, std::memory_order_relaxed);
	    }
	};
	
}

// -----------------------------------------------------------------
