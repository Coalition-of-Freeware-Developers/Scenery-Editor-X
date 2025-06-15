/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* system_detection.h
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/
#pragma once
//////////////////////////////////////////////////////
///			   COMPILER TYPE DETECTION			   ///
//////////////////////////////////////////////////////
#if defined(__GNUC__)
	#if defined(__clang__)
		#define SEDX_COMPILER_CLANG
	#else
		#define SEDX_COMPILER_GCC
	#endif
#elif defined(_MSC_VER)
	#define SEDX_COMPILER_MSVC
#endif
//////////////////////////////////////////////////////
#ifdef SEDX_COMPILER_MSVC
	#define SEDX_FORCE_INLINE __forceinline
	#define SEDX_EXPLICIT_STATIC static
#elif defined(__GNUC__)
	#define SEDX_FORCE_INLINE __attribute__((always_inline)) inline
	#define SEDX_EXPLICIT_STATIC
#else
	#define SEDX_FORCE_INLINE inline
	#define SEDX_EXPLICIT_STATIC
#endif
//////////////////////////////////////////////////////
///				BUILD TYPE DETECTION			   ///
//////////////////////////////////////////////////////
#ifdef SEDX_DEBUG
    #define SEDX_BUILD_TYPE "Debug"
#elif defined(SEDX_RELEASE)
    #define SEDX_BUILD_TYPE "Release"
#endif

//////////////////////////////////////////////////////
///           WINDOWS PLATFORM DETECTION		   ///
//////////////////////////////////////////////////////
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(WINDOWS) || defined(WIN64) || defined(_MSC_VER)
    #ifdef _WIN64
	/** Windows x64  **/
        #define SEDX_PLATFORM_WINDOWS
        #define SEDX_PLATFORM_NAME "Windowsx64"
		#include <Windows.h>
		#define VK_USE_PLATFORM_WIN32_KHR
		#include <vulkan/vulkan.h>
        constexpr char dirSeparator = '\\';
		#if defined(_DEBUG) || defined(DEBUG)
			#ifndef SEDX_DEBUG
			    #define SEDX_DEBUG
			#endif
			#define SEDX_DEBUGBREAK() __debugbreak()
			#define APP_USE_VULKAN_DEBUG_REPORT
		#endif
		#if defined(_RELEASE) || defined(NDEBUG) || defined(RELEASE)
		    #ifndef SEDX_RELEASE
		    #define SEDX_RELEASE
		    #endif
		#endif
    #elif
	/** Windows x86 **/
    #error "x86 Builds are not supported!"
    #endif
//////////////////////////////////////////////////////
///         APPLE/MAC PLATFORM DETECTION		   ///
//////////////////////////////////////////////////////
#elif defined(__APPLE__) || defined(__MACH__) || (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
    #define SEDX_PLATFORM_APPLE
    #define SEDX_PLATFORM_NAME "MacOS"
	#include <TargetConditionals.h>
	/**
	 * TARGET_OS_MAC exists on all the platforms
	 * so we must check all of them (in this order)
	 * to ensure that we're running on MAC
	 * and not some other Apple platform
	 */
	#if TARGET_IPHONE_SIMULATOR == 1
	    #error "IOS simulator is not supported!"
	#elif TARGET_OS_IPHONE == 1
	    #define SEDX_PLATFORM_IOS
	    #error "IOS is not supported!"
    #elif TARGET_OS_MAC == 1
	    #define SEDX_PLATFORM_MACOS
		#include <unistd.h>
		#include <sys/types.h>
		#include <pwd.h>
		#define VK_USE_PLATFORM_METAL_EXT
		#include <vulkan/vulkan.h>
    #else
	    #error "Unknown Apple platform!"
	#endif
//////////////////////////////////////////////////////
///			  ANDROID PLATFORM DETECTION		   ///
//////////////////////////////////////////////////////
/**
 * We also have to check __ANDROID__ before __linux__
 * since android is based on the linux kernel
 * it has __linux__ defined
 */
#elif defined(__ANDROID__) || defined(ANDROID)
	#define SEDX_PLATFORM_ANDROID
	#error "Android is not supported!"

//////////////////////////////////////////////////////
///			   LINUX PLATFORM DETECTION			   ///
//////////////////////////////////////////////////////
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    #define SEDX_PLATFORM_LINUX
    #define SEDX_PLATFORM_NAME "Linux"
	#include <unistd.h>
	#include <csignal>
	#include <sys/types.h>
	#include <pwd.h>
	#define VK_USE_PLATFORM_XLIB_KHR
	#include <vulkan/vulkan.h>
	constexpr char dirSeparator = '/';
	#if defined(_DEBUG) || defined(DEBUG)
		#ifndef SEDX_DEBUG
		    #define SEDX_DEBUG
		#endif
		#define SEDX_DEBUGBREAK() raise(SIGTRAP)
	#endif
#else
	/* Unknown compiler/platform */
    #define SEDX_PLATFORM_NAME "Unknown"
    #error "Unknown platform!"
#endif 

//////////////////////////////////////////////////////

