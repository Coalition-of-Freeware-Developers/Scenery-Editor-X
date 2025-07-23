/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* delegations.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
/**
 *	Based on:
 *	- original work by Sergey Ryazanov (MIT)
 *	"The Impossibly Fast C++ Delegates", 18 Jul 2005
 *	https://www.codeproject.com/articles/11015/the-impossibly-fast-c-delegates
 *
 *	- and the work of Sergey Alexandrovich Kryukov (MIT)
 *	"The Impossibly Fast C++ Delegates, Fixed", 11 Mar 2017
 *	https://www.codeproject.com/Articles/1170503/The-Impossibly-Fast-Cplusplus-Delegates-Fixed
 */
#pragma once

/// --------------------------------------------

namespace SceneryEditorX::Utils
{

    /// --------------------------------------------

	template <class T>
	class Delegate;

	template <class T>
	class MulticastDelegate;

    /// --------------------------------------------

    /** Simple function delegate to bind a callback of some sort without unnecessary allocations. Faster than std::function. */
	template <class TReturn, class... TArgs>
	class Delegate<TReturn(TArgs...)>
	{
	    friend class MulticastDelegate<TReturn(TArgs...)>;

	    using TInstancePtr = void *;
	    using TInternalFunction = TReturn (*)(TInstancePtr, TArgs...);

	    struct InvocationElement
	    {
	        InvocationElement() = default;
	        InvocationElement(TInstancePtr thisPtr, TInternalFunction aStub) : Object(thisPtr), Stub(aStub) {}

	        bool operator==(const InvocationElement &another) const { return another.Stub == Stub && another.Object == Object; }
	        bool operator!=(const InvocationElement &another) const { return another.Stub != Stub || another.Object != Object; }

	        TInstancePtr Object = nullptr;
	        TInternalFunction Stub = nullptr;
	    };

	public:
	    Delegate() = default;
	    Delegate(const Delegate &other) { m_Invocation = other.m_Invocation; }

	    Delegate &operator=(const Delegate &other) = default;
        bool operator==(const Delegate &other) const { return m_Invocation == other.m_Invocation; }
	    bool operator!=(const Delegate &other) const { return m_Invocation != other.m_Invocation; }

	    /// -----------------------------------------------------------------

	    /// Free function binding
	    template <TReturn (*TFunction)(TArgs...)>
	    void Bind()
	    {
	        Assign(nullptr, FreeFunctionStub<TFunction>);
	    }

	    /// Lambda binding. For now this only forks with persistent or captureless
	    /// lambdas. (TODO: store copy of the lambda)
	    template <class TLambda>
	    void BindLambda(const TLambda &lambda)
	    {
	        Assign((TInstancePtr)(&lambda), LambdaStub<TLambda>);
	    }

	    /// Member function binding
	    template <auto TFunction, class TClass>
	    void Bind(TClass *object)
	    {
	        using TMembFunc = TReturn (TClass::*)(TArgs...);
	        using TMembFuncConst = TReturn (TClass::*)(TArgs...) const;

	        if constexpr (std::is_same_v<decltype(TFunction), TMembFuncConst>)
	        {
	            Assign(const_cast<TClass *>(object), ConstMemberFunctionStub<TClass, TFunction>);
	        }
	        else
	        {
	            static_assert(std::is_same_v<decltype(TFunction), TMembFunc>, "Invalid function signature."); ///< TODO: C++20 'requires' would've solved this
	            Assign((TInstancePtr)(object), MemberFunctionStub<TClass, TFunction>);
	        }
	    }

	    /// -----------------------------------------------------------------

	    /// Unbind any binding
	    void Unbind()
	    {
	        m_Invocation = InvocationElement();
	    }

	    /// -----------------------------------------------------------------

        [[nodiscard]] bool IsBound() const
	    {
	        return m_Invocation.Stub;
	    }

        explicit operator bool() const
	    {
	        return IsBound();
	    }

	    TReturn Invoke(TArgs... args) const
	    {
	        SEDX_CORE_ASSERT(IsBound(), "Trying to invoke unbound delegate.");
	        return std::invoke(m_Invocation.Stub, m_Invocation.Object, std::forward<TArgs>(args)...);
	    }

	private:
        void Assign(TInstancePtr anObject, TInternalFunction aStub)
	    {
	        m_Invocation.Object = anObject;
	        m_Invocation.Stub = aStub;
	    }

	    template <class TClass, TReturn (TClass::*TFunction)(TArgs...)>
	    static TReturn MemberFunctionStub(TInstancePtr thisPtr, TArgs... args)
	    {
	        TClass *p = static_cast<TClass *>(thisPtr);
	        return (p->*TFunction)(std::forward<TArgs>(args)...);
	    }

	    template <class TClass, TReturn (TClass::*TFunction)(TArgs...) const>
	    static TReturn ConstMemberFunctionStub(TInstancePtr thisPtr, TArgs... args)
	    {
	        const TClass *p = static_cast<TClass *>(thisPtr);
	        return (p->*TFunction)(std::forward<TArgs>(args)...);
	    }

	    template <TReturn (*TFunction)(TArgs...)>
	    static TReturn FreeFunctionStub(TInstancePtr thisPtr, TArgs... args)
	    {
	        return (TFunction)(std::forward<TArgs>(args)...);
	    }

	    template <typename TLambda>
	    static TReturn LambdaStub(TInstancePtr thisPtr, TArgs... args)
	    {
	        TLambda *p = static_cast<TLambda *>(thisPtr);
	        return (p->operator())(std::forward<TArgs>(args)...);
	    }

	    InvocationElement m_Invocation;
	};

    /// -----------------------------------------------------------------

	/** Simple multicast function delegate to bind multiple callbacks of some sort without unnecessary allocations. */
	template <class TReturn, class... TArgs>
	class MulticastDelegate<TReturn(TArgs...)>
	{
	    using TDelegate = Delegate<TReturn(TArgs...)>;
	    using TInstancePtr = TDelegate::TInstancePtr;
	    using TInternalFunction = TDelegate::TInternalFunction;
	    using InvocationElement = TDelegate::InvocationElement;

	public:
	    MulticastDelegate() = default;

	    MulticastDelegate(const MulticastDelegate &other) { m_InvocationList = other.m_InvocationList; }
	    MulticastDelegate &operator=(const MulticastDelegate &other) = default;

        bool operator==(const MulticastDelegate &other) const { return m_InvocationList == other.m_InvocationList; }
	    bool operator!=(const MulticastDelegate &other) const { return m_InvocationList != other.m_InvocationList; }

	    /// -----------------------------------------------------------------

	    /// Free function binding
	    template <TReturn (*TFunction)(TArgs...)>
	    void Bind()
	    {
	        Add(nullptr, FreeFunctionStub<TFunction>);
	    }

	    template <class TLambda>
	    void Bind(const TLambda &lambda)
	    {
	        Add((TDelegate::TInstancePtr)(&lambda), LambdaStub<TLambda>);
	    }

	    /// Member function binding
	    template <auto TFunction, class TClass>
	    void Bind(TClass *object)
	    {
	        using TMembFunc = TReturn (TClass::*)(TArgs...);
	        using TMembFuncConst = TReturn (TClass::*)(TArgs...) const;

	        if constexpr (std::is_same_v<decltype(TFunction), TMembFuncConst>)
	        {
	            Add(const_cast<TClass *>(object), ConstMemberFunctionStub<TClass, TFunction>);
	        }
	        else
	        {
	            static_assert(std::is_same_v<decltype(TFunction), TMembFunc>, "Invalid function signature."); ///< TODO: C++20 'requires' would've solved this
	            Add((TInstancePtr)(object), MemberFunctionStub<TClass, TFunction>);
	        }
	    }

	    /// -----------------------------------------------------------------

	    /// Free function unbinding
	    template <TReturn (*TFunction)(TArgs...)>
	    void Unbind()
	    {
	        Remove(nullptr, FreeFunctionStub<TFunction>);
	    }

	    /// Lambda unbinding
	    template <class TLambda>
	    void Unbind(const TLambda &lambda)
	    {
	        Remove((TDelegate::TInstancePtr)(&lambda), LambdaStub<TLambda>);
	    }

	    /// Member function unbinding
	    template <class TClass, TReturn (TClass::*TFunction)(TArgs...)>
	    void Unbind(TClass *object)
	    {
	        Remove((TDelegate::TInstancePtr)(object), MemberFunctionStub<TClass, TFunction>);
	    }

	    /// Const member function unbinding
	    template <class TClass, TReturn (TClass::*TFunction)(TArgs...) const>
	    void Unbind(const TClass *object)
	    {
	        Remove(const_cast<TClass *>(object), ConstMemberFunctionStub<TClass, TFunction>);
	    }

	    /// -----------------------------------------------------------------

        [[nodiscard]] bool IsBound() const { return !m_InvocationList.empty(); }

        explicit operator bool() const { return IsBound(); }

	    /** Multicast Delegate does not support return type handling. */
	    void Invoke(TArgs... args) const
	    {
	       SEDX_CORE_ASSERT(IsBound(), "Trying to invoke unbound delegate.");

	        /// We don't want to Invoke new functions that may be added
	        /// in one of the delegate calls.
	        /// Hopefully none of the elements are removed from the list
	        /// during the iteration.
	        const uint32_t numberOfInvocations = m_InvocationList.size();

	        uint32_t i = 0;

	        for (const auto &element : m_InvocationList)
	        {
	            (*element.Stub)(element.Object, std::forward<TArgs>(args)...);

	            if (++i >= numberOfInvocations)
	                break;
	        }
	    }

	private:
        void Add(TInstancePtr anObject, TInternalFunction aStub)
	    {
	        m_InvocationList.push_back(InvocationElement{anObject, aStub});
	    }

        void Remove(TInstancePtr anObject, TInternalFunction aStub)
	    {
	        m_InvocationList.remove(InvocationElement{anObject, aStub});
	    }

	    template <class TClass, TReturn (TClass::*TFunction)(TArgs...)>
	    static TReturn MemberFunctionStub(TInstancePtr thisPtr, TArgs... args)
	    {
	        TClass *p = static_cast<TClass *>(thisPtr);
	        return (p->*TFunction)(std::forward<TArgs>(args)...);
	    }

	    template <class TClass, TReturn (TClass::*TFunction)(TArgs...) const>
	    static TReturn ConstMemberFunctionStub(TInstancePtr thisPtr, TArgs... args)
	    {
	        const TClass *p = static_cast<TClass *>(thisPtr);
	        return (p->*TFunction)(std::forward<TArgs>(args)...);
	    }

	    template <TReturn (*TFunction)(TArgs...)>
	    static TReturn FreeFunctionStub(TInstancePtr thisPtr, TArgs... args)
	    {
	        return (TFunction)(std::forward<TArgs>(args)...);
	    }

	    template <typename TLambda>
	    static TReturn LambdaStub(TInstancePtr thisPtr, TArgs... args)
	    {
	        TLambda *p = static_cast<TLambda *>(thisPtr);
	        return (p->operator())(std::forward<TArgs>(args)...);
	    }

	    std::list<InvocationElement> m_InvocationList;
	};

}

/// --------------------------------------------
