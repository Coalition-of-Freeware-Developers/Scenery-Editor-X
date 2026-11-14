/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * MacroCollisionTests.cpp
 * -------------------------------------------------------
 * The i18n headers end up in every editor file, after <Windows.h> on Windows and after X11's headers on Linux (Vulkan's
 * Xlib platform pulls them in). Those define plain words as macros, so this file defines the same macros before
 * including the headers: a name that collides stops this suite from compiling on every platform, not only in the
 * one editor build that has the header. Standard headers come first, as they would in the editor.
 * -------------------------------------------------------
 */
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <catch2/catch_all.hpp>

// X11 (X.h, Xlib.h)
#define None 0L
#define Bool int
#define Status int
#define True 1
#define False 0
#define Success 0
#define Complex 0
#define Always 2
#define Above 0
#define Below 1
// Windows (winuser.h, wingdi.h, windef.h)
#define GetMessage GetMessageW
#define ERROR 0
#define near
#define far
#define TRANSPARENT 1
#define OPAQUE 2

#include <SceneryEditorX/i18n/i18n.h>
#include <SceneryEditorX/i18n/tr.h>

TEST_CASE("i18n headers compile after X11 and Windows macros", "[i18n][macros]")
{
	const SceneryEditorX::Text text("Save");
	CHECK(text.IsTranslated());
	CHECK(SceneryEditorX::TrF("{}", true) == "true");
}
