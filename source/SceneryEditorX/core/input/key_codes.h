/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* key_codes.h
* -------------------------------------------------------
* Created: 13/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <ostream>
#include <cstdint>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Space		= 32,
		Apostrophe	= 39, /* ' */
		Comma		= 44, /* , */
		Minus		= 45, /* - */
		Period		= 46, /* . */
		Slash		= 47, /* / */

		D0			= 48, /* 0 */
		D1			= 49, /* 1 */
		D2			= 50, /* 2 */
		D3			= 51, /* 3 */
		D4			= 52, /* 4 */
		D5			= 53, /* 5 */
		D6			= 54, /* 6 */
		D7			= 55, /* 7 */
		D8			= 56, /* 8 */
		D9			= 57, /* 9 */

		Semicolon	= 59, /* ; */
		Equal		= 61, /* = */

		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,

		LeftBracket		= 91,  /* [ */
		Backslash		= 92,  /* \ */
		RightBracket	= 93, /* ] */
		GraveAccent		= 96,  /* ` */

		World1		= 161, /* non-US #1 */
		World2		= 162, /* non-US #2 */

		/* Function keys */
		Escape			= 256,
		Enter			= 257,
		Tab				= 258,
		Backspace		= 259,
		Insert			= 260,
		Delete			= 261,
		Right			= 262,
		Left			= 263,
		Down			= 264,
		Up				= 265,
		PageUp			= 266,
		PageDown		= 267,
		Home			= 268,
		End				= 269,
		CapsLock		= 280,
		ScrollLock		= 281,
		NumLock			= 282,
		PrintScreen		= 283,
		Pause			= 284,
		F1				= 290,
		F2				= 291,
		F3				= 292,
		F4				= 293,
		F5				= 294,
		F6				= 295,
		F7				= 296,
		F8				= 297,
		F9				= 298,
		F10				= 299,
		F11				= 300,
		F12				= 301,
		F13				= 302,
		F14				= 303,
		F15				= 304,
		F16				= 305,
		F17				= 306,
		F18				= 307,
		F19				= 308,
		F20				= 309,
		F21				= 310,
		F22				= 311,
		F23				= 312,
		F24				= 313,
		F25				= 314,

		/* Keypad */
		KP0				= 320,
		KP1				= 321,
		KP2				= 322,
		KP3				= 323,
		KP4				= 324,
		KP5				= 325,
		KP6				= 326,
		KP7				= 327,
		KP8				= 328,
		KP9				= 329,
		KPDecimal		= 330,
		KPDivide		= 331,
		KPMultiply		= 332,
		KPSubtract		= 333,
		KPAdd			= 334,
		KPEnter			= 335,
		KPEqual			= 336,

		LeftShift		= 340,
		LeftControl		= 341,
		LeftAlt			= 342,
		LeftSuper		= 343,
		RightShift		= 344,
		RightControl	= 345,
		RightAlt		= 346,
		RightSuper		= 347,
		Menu			= 348
	} Key;

	enum class KeyState
	{
		None = -1,
		Pressed,
		Held,
		Released
	};

	enum class CursorMode : uint8_t
	{
		Normal = 0,
		Hidden = 1,
		Locked = 2
	};

	typedef enum class MouseButton : uint8_t
	{
		Button0 = 0,
		Button1 = 1,
		Button2 = 2,
		Button3 = 3,
		Button4 = 4,
		Button5 = 5,
		Left = Button0,
		Right = Button1,
		Middle = Button2
	} Button;


	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int32_t>(keyCode);
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, MouseButton button)
	{
		os << static_cast<int32_t>(button);
		return os;
	}

}

/// ------------------------------------------------------------------------///

// From glfw3.h
#define SEDX_KEY_SPACE				::SceneryEditorX::Key::Space
#define SEDX_KEY_APOSTROPHE			::SceneryEditorX::Key::Apostrophe   /* ' */
#define SEDX_KEY_COMMA				::SceneryEditorX::Key::Comma        /* , */
#define SEDX_KEY_MINUS				::SceneryEditorX::Key::Minus        /* - */
#define SEDX_KEY_PERIOD				::SceneryEditorX::Key::Period       /* . */
#define SEDX_KEY_SLASH				::SceneryEditorX::Key::Slash        /* / */
#define SEDX_KEY_0					::SceneryEditorX::Key::D0
#define SEDX_KEY_1					::SceneryEditorX::Key::D1
#define SEDX_KEY_2					::SceneryEditorX::Key::D2
#define SEDX_KEY_3					::SceneryEditorX::Key::D3
#define SEDX_KEY_4					::SceneryEditorX::Key::D4
#define SEDX_KEY_5					::SceneryEditorX::Key::D5
#define SEDX_KEY_6					::SceneryEditorX::Key::D6
#define SEDX_KEY_7					::SceneryEditorX::Key::D7
#define SEDX_KEY_8					::SceneryEditorX::Key::D8
#define SEDX_KEY_9					::SceneryEditorX::Key::D9
#define SEDX_KEY_SEMICOLON			::SceneryEditorX::Key::Semicolon	 /* ; */
#define SEDX_KEY_EQUAL				::SceneryEditorX::Key::Equal         /* = */
#define SEDX_KEY_A					::SceneryEditorX::Key::A
#define SEDX_KEY_B					::SceneryEditorX::Key::B
#define SEDX_KEY_C					::SceneryEditorX::Key::C
#define SEDX_KEY_D					::SceneryEditorX::Key::D
#define SEDX_KEY_E					::SceneryEditorX::Key::E
#define SEDX_KEY_F					::SceneryEditorX::Key::F
#define SEDX_KEY_G					::SceneryEditorX::Key::G
#define SEDX_KEY_H					::SceneryEditorX::Key::H
#define SEDX_KEY_I					::SceneryEditorX::Key::I
#define SEDX_KEY_J					::SceneryEditorX::Key::J
#define SEDX_KEY_K					::SceneryEditorX::Key::K
#define SEDX_KEY_L					::SceneryEditorX::Key::L
#define SEDX_KEY_M					::SceneryEditorX::Key::M
#define SEDX_KEY_N					::SceneryEditorX::Key::N
#define SEDX_KEY_O					::SceneryEditorX::Key::O
#define SEDX_KEY_P					::SceneryEditorX::Key::P
#define SEDX_KEY_Q					::SceneryEditorX::Key::Q
#define SEDX_KEY_R					::SceneryEditorX::Key::R
#define SEDX_KEY_S					::SceneryEditorX::Key::S
#define SEDX_KEY_T					::SceneryEditorX::Key::T
#define SEDX_KEY_U					::SceneryEditorX::Key::U
#define SEDX_KEY_V					::SceneryEditorX::Key::V
#define SEDX_KEY_W					::SceneryEditorX::Key::W
#define SEDX_KEY_X					::SceneryEditorX::Key::X
#define SEDX_KEY_Y					::SceneryEditorX::Key::Y
#define SEDX_KEY_Z					::SceneryEditorX::Key::Z
#define SEDX_KEY_LEFT_BRACKET		::SceneryEditorX::Key::LeftBracket		/* [ */
#define SEDX_KEY_BACKSLASH			::SceneryEditorX::Key::Backslash		/* \ */
#define SEDX_KEY_RIGHT_BRACKET		::SceneryEditorX::Key::RightBracket		/* ] */
#define SEDX_KEY_GRAVE_ACCENT		::SceneryEditorX::Key::GraveAccent		/* ` */
#define SEDX_KEY_WORLD_1			::SceneryEditorX::Key::World1           /* non-US #1 */
#define SEDX_KEY_WORLD_2			::SceneryEditorX::Key::World2           /* non-US #2 */

/// ------------------------------------------------------------------------///
///									FUNCTION KEYS							///
/// ------------------------------------------------------------------------///
#define SEDX_KEY_ESCAPE				::SceneryEditorX::Key::Escape
#define SEDX_KEY_ENTER				::SceneryEditorX::Key::Enter
#define SEDX_KEY_TAB				::SceneryEditorX::Key::Tab
#define SEDX_KEY_BACKSPACE			::SceneryEditorX::Key::Backspace
#define SEDX_KEY_INSERT				::SceneryEditorX::Key::Insert
#define SEDX_KEY_DELETE				::SceneryEditorX::Key::Delete
#define SEDX_KEY_RIGHT				::SceneryEditorX::Key::Right
#define SEDX_KEY_LEFT				::SceneryEditorX::Key::Left
#define SEDX_KEY_DOWN				::SceneryEditorX::Key::Down
#define SEDX_KEY_UP					::SceneryEditorX::Key::Up
#define SEDX_KEY_PAGE_UP			::SceneryEditorX::Key::PageUp
#define SEDX_KEY_PAGE_DOWN			::SceneryEditorX::Key::PageDown
#define SEDX_KEY_HOME				::SceneryEditorX::Key::Home
#define SEDX_KEY_END				::SceneryEditorX::Key::End
#define SEDX_KEY_CAPS_LOCK			::SceneryEditorX::Key::CapsLock
#define SEDX_KEY_SCROLL_LOCK		::SceneryEditorX::Key::ScrollLock
#define SEDX_KEY_NUM_LOCK			::SceneryEditorX::Key::NumLock
#define SEDX_KEY_PRINT_SCREEN		::SceneryEditorX::Key::PrintScreen
#define SEDX_KEY_PAUSE				::SceneryEditorX::Key::Pause
#define SEDX_KEY_F1					::SceneryEditorX::Key::F1
#define SEDX_KEY_F2					::SceneryEditorX::Key::F2
#define SEDX_KEY_F3					::SceneryEditorX::Key::F3
#define SEDX_KEY_F4					::SceneryEditorX::Key::F4
#define SEDX_KEY_F5					::SceneryEditorX::Key::F5
#define SEDX_KEY_F6					::SceneryEditorX::Key::F6
#define SEDX_KEY_F7					::SceneryEditorX::Key::F7
#define SEDX_KEY_F8					::SceneryEditorX::Key::F8
#define SEDX_KEY_F9					::SceneryEditorX::Key::F9
#define SEDX_KEY_F10				::SceneryEditorX::Key::F10
#define SEDX_KEY_F11				::SceneryEditorX::Key::F11
#define SEDX_KEY_F12				::SceneryEditorX::Key::F12
#define SEDX_KEY_F13				::SceneryEditorX::Key::F13
#define SEDX_KEY_F14				::SceneryEditorX::Key::F14
#define SEDX_KEY_F15				::SceneryEditorX::Key::F15
#define SEDX_KEY_F16				::SceneryEditorX::Key::F16
#define SEDX_KEY_F17				::SceneryEditorX::Key::F17
#define SEDX_KEY_F18				::SceneryEditorX::Key::F18
#define SEDX_KEY_F19				::SceneryEditorX::Key::F19
#define SEDX_KEY_F20				::SceneryEditorX::Key::F20
#define SEDX_KEY_F21				::SceneryEditorX::Key::F21
#define SEDX_KEY_F22				::SceneryEditorX::Key::F22
#define SEDX_KEY_F23				::SceneryEditorX::Key::F23
#define SEDX_KEY_F24				::SceneryEditorX::Key::F24
#define SEDX_KEY_F25				::SceneryEditorX::Key::F25

/// ------------------------------------------------------------------------///
///									KEYPAD									///
/// ------------------------------------------------------------------------///
#define SEDX_KEY_KP_0				::SceneryEditorX::Key::KP0
#define SEDX_KEY_KP_1				::SceneryEditorX::Key::KP1
#define SEDX_KEY_KP_2				::SceneryEditorX::Key::KP2
#define SEDX_KEY_KP_3				::SceneryEditorX::Key::KP3
#define SEDX_KEY_KP_4				::SceneryEditorX::Key::KP4
#define SEDX_KEY_KP_5				::SceneryEditorX::Key::KP5
#define SEDX_KEY_KP_6				::SceneryEditorX::Key::KP6
#define SEDX_KEY_KP_7				::SceneryEditorX::Key::KP7
#define SEDX_KEY_KP_8				::SceneryEditorX::Key::KP8
#define SEDX_KEY_KP_9				::SceneryEditorX::Key::KP9
#define SEDX_KEY_KP_DECIMAL			::SceneryEditorX::Key::KPDecimal
#define SEDX_KEY_KP_DIVIDE			::SceneryEditorX::Key::KPDivide
#define SEDX_KEY_KP_MULTIPLY		::SceneryEditorX::Key::KPMultiply
#define SEDX_KEY_KP_SUBTRACT		::SceneryEditorX::Key::KPSubtract
#define SEDX_KEY_KP_ADD				::SceneryEditorX::Key::KPAdd
#define SEDX_KEY_KP_ENTER			::SceneryEditorX::Key::KPEnter
#define SEDX_KEY_KP_EQUAL			::SceneryEditorX::Key::KPEqual

#define SEDX_KEY_LEFT_SHIFT			::SceneryEditorX::Key::LeftShift
#define SEDX_KEY_LEFT_CONTROL		::SceneryEditorX::Key::LeftControl
#define SEDX_KEY_LEFT_ALT			::SceneryEditorX::Key::LeftAlt
#define SEDX_KEY_LEFT_SUPER			::SceneryEditorX::Key::LeftSuper
#define SEDX_KEY_RIGHT_SHIFT		::SceneryEditorX::Key::RightShift
#define SEDX_KEY_RIGHT_CONTROL		::SceneryEditorX::Key::RightControl
#define SEDX_KEY_RIGHT_ALT			::SceneryEditorX::Key::RightAlt
#define SEDX_KEY_RIGHT_SUPER		::SceneryEditorX::Key::RightSuper
#define SEDX_KEY_MENU				::SceneryEditorX::Key::Menu

/// ------------------------------------------------------------------------///
///									MOUSE									///
/// ------------------------------------------------------------------------///
#define SEDX_MOUSE_BUTTON_LEFT		::SceneryEditorX::Button::Left
#define SEDX_MOUSE_BUTTON_RIGHT		::SceneryEditorX::Button::Right
#define SEDX_MOUSE_BUTTON_MIDDLE	::SceneryEditorX::Button::Middle

/// -------------------------------------------------------
