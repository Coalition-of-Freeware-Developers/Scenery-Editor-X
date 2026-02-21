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
 * event_system.h
 * -------------------------------------------------------
 * Created: 14/7/2025
 */
#pragma once
#include <SceneryEditorX/core/base.h>
#include <functional>
#include <string>

// -------------------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @enum EventType
     * @brief Enumerates all discrete event kinds produced by the engine, editor, and input subsystems.
     * @details
     *  These values are used for type-safe static dispatch. Concrete event classes should declare
     *  their static type with `EVENT_CLASS_TYPE(...)` to participate in the dispatcher.
     */
    enum class EventType : uint8_t
    {
        None = 0,                  // No event (sentinel).
        WindowClose,               // Window close requested by the OS or user.
        WindowMinimize,            // Window minimize/restore toggled.
        WindowMaximize,			   // Window maximize/restore toggled.
        WindowResize,              // Window size changed.
        WindowFocus,               // Window gained input focus.
        WindowLostFocus,           // Window lost input focus.
        WindowMoved,               // Window position changed.
        WindowTitleBarHitTest,     // Title bar hit testing (for custom chrome).
        AppTick,                   // Application tick (fixed timestep).
        AppUpdate,                 // Per-frame application update.
        AppRender,                 // Per-frame application render.
        KeyPressed,                // Keyboard key pressed (may repeat).
        KeyReleased,               // Keyboard key released.
        KeyTyped,                  // Character input (text composition).
        MouseButtonPressed,        // Mouse button pressed.
        MouseButtonReleased,       // Mouse button released.
        MouseButtonDown,           // Mouse button is held down (continuous).
        MouseMoved,                // Mouse cursor moved.
        MouseScrolled,             // Mouse wheel scrolled.
        ScenePreStart,             // Scene about to start (before systems initialize).
        ScenePostStart,            // Scene started (after systems initialize).
        ScenePreStop,              // Scene about to stop (before systems teardown).
        ScenePostStop,             // Scene stopped (after systems teardown).
        EditorExitPlayMode,        // Editor transitioned from play mode to edit mode.
        SelectionChanged,          // Editor selection changed.
        AssetReloaded,             // Asset was hot-reloaded.
        AnimationGraphCompiled,    // Animation graph successfully compiled.
        ScreenshotCaptured         // Screenshot captured and stored.
    };

    /**
     * @enum EventCategory
     * @brief Bitmask-based event categories for fast filtering.
     * @details
     *  Use `IsInCategory(...)` on `Event` to test membership. Categories are orthogonal and can be combined.
     */
    enum EventCategory : uint8_t
    {
        None                      = 0,               // No category.
        EventCategoryApplication  = BIT(0),          // Application lifecycle and frame events.
        EventCategoryInput        = BIT(1),          // Generic input events.
        EventCategoryKeyboard     = BIT(2),          // Keyboard-specific events.
        EventCategoryMouse        = BIT(3),          // Mouse movement/scroll events.
        EventCategoryMouseButton  = BIT(4),          // Mouse button events.
        EventCategoryScene        = BIT(5),          // Scene lifecycle events.
        EventCategoryEditor       = BIT(6)           // Editor events (selection, mode, etc.).
    };

    /**
     * @def EVENT_CLASS_TYPE
     * @brief Injects standard static/dynamic type and name boilerplate into a concrete `Event` type.
     * @param type The `EventType` enumerator corresponding to the concrete class.
     * @details
     *  Adds:
     *   - `static EventType GetStaticType()`
     *   - `virtual EventType GetEventType() const override`
     *   - `virtual const char* GetName() const override`
     */
    #define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
                                    virtual EventType GetEventType() const override { return GetStaticType(); }\
                                    virtual const char* GetName() const override { return #type; }

    /**
     * @def EVENT_CLASS_CATEGORY
     * @brief Injects the category flags override into a concrete `Event` type.
     * @param category Bitmask of `EventCategory` values.
     */
    #define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

    // -------------------------------------------------------------------

    /**
     * @class Event
     * @brief Abstract base for all events dispatched through the engine/editor.
     * @details
     *  Concrete events must implement `GetEventType()`, `GetName()`, and `GetCategoryFlags()`. 
     *  Use the helper macros `EVENT_CLASS_TYPE` and `EVENT_CLASS_CATEGORY` to reduce boilerplate.
     */
    class Event
    {
    public:
        /**
         * @brief Indicates whether the event has been fully handled by a listener.
         * @details
         *  Set to `true` by handlers to stop further propagation.
         */
        bool m_Handled = false;

        /**
         * @brief Synchronization flag for queued events.
         * @details
         *  Queued events are only processed when this is `true`. It is set when the asset thread
         *  synchronizes with the main thread to ensure thread-safe delivery.
         */
        bool m_Synced = false;

        virtual ~Event() = default;

        /**
         * @brief Gets the event's concrete type at runtime.
         * @return The `EventType` of this instance.
         */
        [[nodiscard]] virtual EventType GetEventType() const = 0;

        /**
         * @brief Gets a short, stable name for the event (typically the enum label).
         * @return Null-terminated C-string of the event name.
         */
        [[nodiscard]] virtual const char* GetName() const = 0;

        /**
         * @brief Gets the bitmask of categories this event belongs to.
         * @return Integer bitmask composed of `EventCategory` flags.
         */
        [[nodiscard]] virtual int GetCategoryFlags() const = 0;

        /**
         * @brief Converts the event to a human-readable string for logging.
         * @return String containing the event name by default.
         */
        [[nodiscard]] virtual std::string ToString() const { return GetName(); }

        /**
         * @brief Tests whether this event is a member of the specified category mask.
         * @param category The category bit to test (values of `EventCategory`).
         * @return True if the event's category flags intersect with `category`; otherwise false.
         */
        inline bool IsInCategory(const EventCategory category) const
        {
            return GetCategoryFlags() & category;
        }
    };


    // -------------------------------------------------------------------

    /**
     * @class EventDispatcher
     * @brief Type-safe dispatcher that routes a single event to a matching handler.
     * @details
     *  Usage pattern:
     *  @code
     *  void OnEvent(Event& e)
     *  {
     *      EventDispatcher dispatcher(e);
     *      dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& ev)
     *      {
     *          // handle key pressed
     *          return true; // mark handled
     *      });
     *  }
     *  @endcode
     *  The dispatcher invokes a handler only if the event's runtime type matches T's static type,
     *  and if the event has not already been handled. The event's `Handled` flag is set to the
     *  handler's boolean return value.
     */
    class EventDispatcher
    {
        /**
         * @brief Function signature expected by `Dispatch` handlers.
         * @tparam T Concrete event type derived from `Event`.
         */
        template<typename T>
        using EventFn = std::function<bool(T&)>;

    public:
        /**
         * @brief Constructs a dispatcher bound to a specific event instance.
         * @param event Reference to the event to be dispatched (non-owning).
         */
        explicit EventDispatcher(Event& event) : m_Event(event) {}

        /**
         * @brief Attempts to dispatch the bound event to the provided handler if types match.
         * @tparam T Concrete event type to route (must provide `GetStaticType()`).
         * @param func Callback invoked when the bound event is of type `T`.
         * @return True if a type match occurred (handler was considered), false otherwise.
         * @details
         *  - If the bound event type equals `T::GetStaticType()` and `Handled` is false:
         *    - The handler is invoked and its return value is assigned to `Handled`.
         *    - The function returns true to indicate a match.
         *  - If the types don't match or the event was already handled, the function returns false.
         */
        template<typename T>
        bool Dispatch(EventFn<T> func)
        {
            if (m_Event.GetEventType() == T::GetStaticType() && !m_Event.m_Handled)
            {
                m_Event.m_Handled = func(*static_cast<T *>(&m_Event));
                return true;
            }

            return false;
        }

    private:
        /**
         * @brief Reference to the event being dispatched.
         * @note The dispatcher does not own the event; the lifetime must outlive the dispatcher.
         */
        Event& m_Event;
    };

    // -------------------------------------------------------------------

    /**
     * @brief Stream insertion operator for events.
     * @param os Output stream.
     * @param e Event to serialize.
     * @return Reference to the output stream.
     * @details Writes `e.ToString()` into the stream, enabling simple logging like:
     *  `SEDX_CORE_INFO("Event: {}", event.ToString());`
     */
    inline std::ostream& operator<<(std::ostream& os, const Event& e)
    {
        return os << e.ToString();
    }

}

// -------------------------------------------------------------------
