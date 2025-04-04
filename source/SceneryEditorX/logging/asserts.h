/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asserts.h
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

#ifdef SEDX_DEBUG
    #define ENABLE_ASSERTS
    #define SEDX_DEBUGBREAK() __debugbreak()
#endif // SEDX_DEBUG

#define SEDX_ENABLE_VERIFY

// -------------------------------------------------------

#ifdef ENABLE_ASSERTS
    #ifdef COMPILER_CLANG
		#define CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Log::PrintAssertMessage(::Log::Type::Editor, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
		#define ASSERT_MESSAGE_INTERNAL(...)       ::Log::PrintAssertMessage(::Log::Type::Launcher, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
	#else
		#define CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Log::PrintAssertMessage(::Log::Type::Editor, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
		#define ASSERT_MESSAGE_INTERNAL(...)       ::Log::PrintAssertMessage(::Log::Type::Launcher, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define SEDX_CORE_ASSERT(condition, ...) do { if(!(condition)) { CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUGBREAK; } } while(0)
	#define SEDX_ASSERT(condition, ...) do { if(!(condition)) { ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUGBREAK; } } while(0)
#else
	#define SEDX_CORE_ASSERT(condition, ...) ((void) (condition))
	#define SEDX_ASSERT(condition, ...) ((void) (condition))
#endif

// -------------------------------------------------------

#ifdef SEDX_ENABLE_VERIFY
	#ifdef COMPILER_CLANG
		#define CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Log::PrintAssertMessage(::Log::Type::Editor, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
		#define VERIFY_MESSAGE_INTERNAL(...)  ::Log::PrintAssertMessage(::Log::Type::Launcher, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
	#else
		#define CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Log::PrintAssertMessage(::Hazel::Log::Type::Editor, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
		#define VERIFY_MESSAGE_INTERNAL(...)  ::Log::PrintAssertMessage(::Hazel::Log::Type::Launcher, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define SEDX_CORE_VERIFY(condition, ...) do { if(!(condition)) { CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUGBREAK; } } while(0)
	#define SEDX_VERIFY(condition, ...) do { if(!(condition)) { VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUGBREAK; } } while(0)
#else
	#define HZ_CORE_VERIFY(condition, ...) ((void) (condition))
	#define HZ_VERIFY(condition, ...) ((void) (condition))
#endif

// -------------------------------------------------------
