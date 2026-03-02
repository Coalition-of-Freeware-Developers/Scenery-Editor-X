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
 * application_context.h
 * -------------------------------------------------------
 * Created: 25/12/2025
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @class PlatformContext
     * @brief Application context contains an abstract platform-specific.
     *
     */
    class PlatformContext : public RefCounted
    {
    public:
		virtual ~PlatformContext() = default;

        // TODO: Decide if there really is any need for singleton access
        //static PlatformContext &Get() { return *appCtx; } // Singleton access

		virtual const std::string &GetWorkingDirectory() const { return m_WorkingDirectory; }
		virtual const std::string &GetTempDirectory() const { return m_TempDirectory; }
        virtual const std::vector<std::string> &GetCommandLineArgs() const { return m_CommandLineArgs; }

        //static Ref<PlatformContext> Create(const std::string &name, const std::string &workingDirectory, const std::vector<std::string> &commandLineArgs);

    protected:
        std::string m_WorkingDirectory;
        std::string m_TempDirectory;
        std::vector<std::string> m_CommandLineArgs;
        //static PlatformContext *appCtx;

        PlatformContext() = default;
        
    };

}

// -------------------------------------------------------
