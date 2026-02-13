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
 * command_list.h
 * -------------------------------------------------------
 * Created: 12/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "command_pool.h"
#include "frame_sync.h"
#include "pipeline.h"
#include "queue.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

    class CommandList : public RefCounted
    {
    public:
        CommandList(Ref<Queue>* queue, CommandPool cmdPool, const char* name);
        virtual ~CommandList() override;

        void Begin();
        void Submit(FrameSync *semaphoreWait, const bool isImmediate, FrameSync *semaphoreSignal = nullptr);
        void WaitForExecution(const bool logWaitTime = false);
        //void SetPipelineState(PipelineState &pso);

    private:
        Ref<Queue> *m_Queue;
        Pipeline m_Pipeline;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------
