/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* event_templates.h
* -------------------------------------------------------
* Created: 7/4/2025
* -------------------------------------------------------
*/
#pragma once

// ---------------------------------------------------------

namespace SceneryEditorX
{
	
	template <typename Func>
	void QueueEvent(Func &&func)
	{
	    std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
	    m_EventQueue.emplace_back(true, func);
	}
	
	// ---------------------------------------------------------
	
	template <typename TEvent, bool DispatchImmediately = false, typename... TEventArgs>
	void DispatchEvent(TEventArgs &&...args)
	{
	
	    static_assert(std::is_assignable_v<Event, TEvent>);
	
	    std::shared_ptr<TEvent> event = std::make_shared<TEvent>(std::forward<TEventArgs>(args)...);
	    if constexpr (DispatchImmediately)
	    {
	        OnEvent(*event);
	    }
	    else
	    {
	        std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
	        m_EventQueue.emplace_back(false, [event]() { Application::Get().OnEvent(*event); });
	    }
	}
	
	// ---------------------------------------------------------

}; // namespace SceneryEditorX
