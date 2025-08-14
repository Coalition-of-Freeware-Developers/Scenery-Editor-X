/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* static_states.h
* -------------------------------------------------------
* Created: 24/4/2025
* -------------------------------------------------------
*
* This header defines semantic macros for static variable declarations
* to improve code readability and maintainability throughout the
* Scenery Editor X codebase.
*
* These macros provide clear intent for different types of static
* variables and help developers understand the scope and purpose
* of static declarations at a glance.
*
* Usage Examples:
* - INTERNAL void ProcessInternalData() - Internal module function
* - LOCAL int callCount = 0; - Local static variable in function
* - GLOBAL const char* APP_NAME = "Scenery Editor X"; - Global static
*
* @note - These macros do not change the behavior of static declarations,
*       they only provide semantic clarity for code maintenance.
* -------------------------------------------------------
*/
#pragma once

/**
 * @def INTERNAL
 * @brief Semantic macro for internal module functions and variables
 *
 * Use this macro for static functions and variables that are internal
 * to a specific module or compilation unit. This indicates that the
 * function or variable is not intended for use outside its defining
 * module.
 *
 * @note - Equivalent to 'static' keyword but provides clearer intent
 */
#define INTERNAL static 	/// For easier readability inside to determine if a static variable is internal to the module

/**
 * @def LOCAL
 * @brief Semantic macro for local static variables within functions
 *
 * Use this macro for static variables that are local to a specific
 * function scope. These variables maintain their value between
 * function calls but are not accessible outside the function.
 *
 * @note - Equivalent to 'static' keyword but provides clearer intent
 */
#define LOCAL static 		/// For easier readability for local variables

/**
 * @def GLOBAL
 * @brief Semantic macro for global static variables and functions
 *
 * Use this macro for static variables and functions that have global
 * scope within their compilation unit. These are accessible throughout
 * the entire source file but not externally linked.
 *
 * @note - Equivalent to 'static' keyword but provides clearer intent
 */
#define GLOBAL static 		/// For easier readability for global variables
