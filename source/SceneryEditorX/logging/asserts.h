/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asserts.h
* -------------------------------------------------------
* Created: 11/4/2025
* -------------------------------------------------------
*/
#pragma once
#include "SceneryEditorX/core/base.hpp"
#if !defined(SEDX_NO_LOGGING)
#include "logging.hpp"
#endif

/// -------------------------------------------------------

#ifdef SEDX_PLATFORM_WINDOWS
    #define SEDX_DEBUG_BREAK __debugbreak()
#elif defined(SEDX_COMPILER_CLANG)
    #define SEDX_DEBUG_BREAK __builtin_debugtrap()
#else
    #define SEDX_DEBUG_BREAK
#endif

/// -------------------------------------------------------

#ifdef SEDX_DEBUG
#define SEDX_ENABLE_ASSERTS
#endif

/// -------------------------------------------------------

#define SEDX_ENABLE_VERIFY

/// -------------------------------------------------------

#ifdef SEDX_ENABLE_ASSERTS
	#ifdef SEDX_COMPILER_CLANG
		#if !defined(SEDX_NO_LOGGING)
			#define SEDX_CORE_ASSERT_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
			#define SEDX_ASSERT_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Editor, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
		#else
			#define SEDX_CORE_ASSERT_MESSAGE_INTERNAL(...) ((void)0)
			#define SEDX_ASSERT_MESSAGE_INTERNAL(...) ((void)0)
		#endif
	#else
		#if !defined(SEDX_NO_LOGGING)
			#define SEDX_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(, ) __VA_ARGS__)
			#define SEDX_ASSERT_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Editor, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(, ) __VA_ARGS__)
		#else
			#define SEDX_CORE_ASSERT_MESSAGE_INTERNAL(...) ((void)0)
			#define SEDX_ASSERT_MESSAGE_INTERNAL(...) ((void)0)
		#endif
	#endif

	#define SEDX_CORE_ASSERT(condition, ...) do { if (!(condition)) { SEDX_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUG_BREAK; } } while (0)
	#define SEDX_ASSERT(condition, ...) do { if (!(condition)) { SEDX_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUG_BREAK; } } while (0)
#else
	#define SEDX_CORE_ASSERT(condition, ...) ((void)(condition))
	#define SEDX_ASSERT(condition, ...) ((void)(condition))
#endif

/// -------------------------------------------------------

#ifdef SEDX_ENABLE_VERIFY
	#if !defined(SEDX_NO_LOGGING)
		#ifdef SEDX_COMPILER_CLANG
			#define SEDX_CORE_VERIFY_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
			#define SEDX_VERIFY_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Editor, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
		#else
			#define SEDX_CORE_VERIFY_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(, ) __VA_ARGS__)
			#define SEDX_VERIFY_MESSAGE_INTERNAL(...)  ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Editor, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(, ) __VA_ARGS__)
		#endif
	#else
		#define SEDX_CORE_VERIFY_MESSAGE_INTERNAL(...) ((void)0)
		#define SEDX_VERIFY_MESSAGE_INTERNAL(...) ((void)0)
	#endif

	#define SEDX_CORE_VERIFY(condition, ...) do { if (!(condition)) { SEDX_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUG_BREAK; } } while (0)
	#define SEDX_VERIFY(condition, ...) do { if (!(condition)) { SEDX_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUG_BREAK; } } while (0)
#else
	#define SEDX_CORE_VERIFY(condition, ...) ((void)(condition))
	#define SEDX_VERIFY(condition, ...) ((void)(condition))
#endif

/// -------------------------------------------------------
