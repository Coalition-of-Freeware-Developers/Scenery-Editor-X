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
 * render_doc.h
 * -------------------------------------------------------
 * Created: 29/03/2026
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @class RenderDoc
     * @brief A static class that manages integration with the RenderDoc graphics debugger. 
     * Provides functions to initialize RenderDoc, capture frames, and launch the RenderDoc UI.
     * This class abstracts away the platform-specific details of loading the RenderDoc API and allows for 
     * easy integration of RenderDoc functionality into the engine's rendering workflow. 
     */
    class RenderDoc
    {
    public:
        /**
         * @brief  
         */
        static void OnPreDeviceCreation();

        /**
         * @brief 
         */
        static void Shutdown();

        /**
         * @brief 
         */
        static void FrameCapture();

        /**
         * @brief 
         */
        static void StartCapture();

        /**
         * @brief 
         */
        static void EndCapture();

        /**
         * @brief  
         */
        static void LaunchRenderDocUi();

    };
}

// -----------------------------------------------------------------
