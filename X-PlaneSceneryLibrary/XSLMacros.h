//Module:   XSLMacros.h
//Author:   Connor Russell
//Date:     11/11/2024
//Purpose:  Useful Macros

//Compile once
#pragma once

//Macros
//Min/Max macros
#ifndef MAX
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)
#endif
#define CONSTRAIN(Min, Val, Max) (MIN(MAX(Min, Val), Max))

//Interpolate macro
#define INTERPOLATE(First, Second, Ratio) (First * (1 - Ratio)) + (Second * Ratio)

//Safe div. Division but divide by zero safe
#define SAFE_DIV(a, b) (b == 0 ? 0 : a / b)

//ASSERT macro
#ifdef _DEBUG
//ASSERT with a message. If we don't have MessageBox, we just break
#ifdef MessageBox
#define ASSERTM(cond, msg)\
if (!(cond))\
{\
	int msgBoxID = MessageBoxA(NULL, ("Condition \""s + #cond + "\" was false!\n"s + to_string(__LINE__) + ", "s + __FILE__ + "\n"s + msg + "\n\nDebug?").c_str(), "DEV_ASSERT failed!", MB_YESNO | MB_ICONERROR);\
	if (msgBoxID == IDYES)\
	{\
		__debugbreak();\
	}\
	else\
	{\
		abort();\
	}\
}
#else
#define ASSERTM(cond, msg)\
if (!(cond))\
{\
	__debugbreak();\
}
#endif

//Assert without a message. If we don't have MessageBox, we just break
#ifdef MessageBox
#define ASSERT(cond)\
if (!(cond))\
{\
	int msgBoxID = MessageBoxA(NULL, ("Condition \""s + #cond + "\" was false!\n"s + to_string(__LINE__) + ", "s + __FILE__ + "\n\nDebug?"s).c_str(), "DEV_ASSERT failed!", MB_YESNO | MB_ICONERROR);\
	if (msgBoxID == IDYES)\
	{\
		__debugbreak();\
	}\
	else\
	{\
		abort();\
	}\
}
#else
#define ASSERT(cond, msg)\
if (!(cond))\
{\
	__debugbreak();\
}
#endif

#else
#define ASSERTM(cond, msg)
#define ASSERT(cond)
#endif

