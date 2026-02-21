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

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	class IObject
	{
	public:
	    IObject()
	    {
	        // stack-only, deterministic pseudo-random ID
	        auto timeNow = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	
	        // simple stack-safe thread-unique value
	        uint64_t threadUnique = reinterpret_cast<uint64_t>(GetThreadUniqueAddress());
	
	        uint64_t randomValue = (timeNow ^ threadUnique) * 2654435761u;
	        randomValue ^= (randomValue >> 16);
	
	        m_ObjectId = randomValue;
	    }
	
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
	    static void *GetThreadUniqueAddress()
	    {
	        thread_local int dummy;
	        return &dummy;
	    }
	};
	
}

// -----------------------------------------------------------------
