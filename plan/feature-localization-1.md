---
goal: Multi-language user interface for Scenery Editor X on Windows, Linux and macOS - gettext PO catalogs keyed by English, compile-time hashed message literals, ImGui labels that keep every existing ID, lazily merged per-language fonts, a pseudo-locale, extraction and check tooling, and a Weblate translation workflow
version: 1.0
date_created: 2026-10-24
last_updated: 2026-10-24
owner: Scenery Editor X editor and UI (Coalition of Freeware Developers)
status: 'Planned'
tags: [feature, i18n, l10n, localization, imgui, fonts, tooling, weblate, architecture]
---

# Introduction

![Status: Planned](https://img.shields.io/badge/status-Planned-blue)

This plan makes the Scenery Editor X (SEDX) editor translatable, then translated. English stays the source language.

- **Marking and catalogs.** User-facing strings are marked in C++ with a few short functions and extracted into a gettext template, `resources/locale/sedx.pot`. They are translated on Hosted Weblate into one PO file per language, `resources/locale/<language>.po`.
- **Runtime.** A small `i18n` module owned by SEDX reads the catalogs. It uses only the standard library. Godot's editor and Blender both own their catalog loaders the same way.
- **Live switching.** The language can be changed in Preferences without a restart, because ImGui redraws every label each frame.
- **Stable IDs.** Every translated ImGui label is written as `"<translation>###<original label>"`. Since ImGui 1.92.6 that hashes to the same ID as the original label, so these all stay exactly as they are today:
  - the saved docking layout (`editor.ini`);
  - `EditorLayout.json`;
  - tree, table and popup state;
  - the panel IDs.
- **Existing helpers do most of the work.** About 540 call sites go through SEDX's own `EditorUI` helpers. Changing those helpers to accept message literals translates all of them without editing the call sites.
- **CJK fonts.** Chinese, Japanese and Korean get the OS's regional UI font, or a bundled Noto Sans CJK font. It is merged into the UI fonts only while such a language is active.
- **Catching problems early.** A built-in pseudo-locale, a missing-string log and a self-test find untranslated, truncated and concatenated text before translators see it.
- **Deferred.** Right-to-left and complex scripts are a separate, later phase.

The research behind these choices is in [`reports/Localizing Scenery Editor X.md`](../reports/Localizing%20Scenery%20Editor%20X.md). Work happens on a feature branch cut from `dev` (`claude/localization-support-research-9b7c54`, at `1b079ccc`), and pull requests target `dev`.

## 1. Requirements & Constraints

### Baseline facts (SEDX at commit `1b079ccc`, branch `dev`)

- **BAS-001**: There is no translation system. `vcpkg.json` lists only `msdfgen`, `portable-file-dialogs`, `spdlog`, `stb`, `zlib` and `curl`. ICU 78.2, HarfBuzz 13.0.1, FreeType 2.14.3 and fmt are installed only as transitive dependencies (Skia, pulled in by msdfgen), and SEDX links none of them except FreeType (`CMakeLists.txt:346-353`). The only traces of localization are:
  - the comment "Display name for the panel (can be localized)" (`source/SceneryEditorX/renderer/ui/panel_manager.h:48`);
  - an unread `ui.language = "english"` setting (`source/SceneryEditorX/settings/settings.cpp:71-76`, ensured at `:845-859`);
  - a commented-out Language combo (`source/Editor/panels/settings_panel.cpp:120`).
- **BAS-002**: The live UI is `source/Editor/core/*` and `source/Editor/panels/*`.
  - About 870 lines call an ImGui or `EditorUI` widget with an English literal. The files with the most such lines are:

    | File | Lines |
    |---|---|
    | `panels/facade_editor_panel.cpp` | 160 |
    | `panels/airport_panel.cpp` | 104 |
    | `panels/properties_panel.cpp` | 73 |
    | `panels/xplane_library_panel.cpp` | 72 |
    | `core/facade_tool.cpp` | 71 |
    | `core/project_dialogs.cpp` | 57 |
    | `core/editor_layer.cpp` | 54 |
    | `panels/project_panel.cpp` | 47 |
    | `panels/settings_panels.cpp` | 43 |
    | `core/node_tool.cpp` | 42 |

  - Some files are compiled but nothing in the live shell references them: `panels/menu_bar.cpp`, `properties.cpp`, `render_options.cpp`, `texture_viewer.cpp`, `file_dialog.cpp`, `asset_browser.cpp`, and `core/windows/keybind_window.cpp` and `about_window.cpp`. Together they hold about 230 more.
- **BAS-003**: SEDX's own wrappers carry most of the text. In `source/Editor/core/editor_ui.h:47-101` and `editor_ui.cpp:40-168`, the call counts are:

  | Wrapper | Calls |
  |---|---|
  | `EditorUI::ItemTooltip` | 185 |
  | `PropertyLabel` | 161 |
  | `IconLabel` | 110 |
  | `SectionHeader` | 43 (a `TreeNodeEx`, so its label is the ID) |
  | `StripButton` | 29 (tooltip argument) |
  | `EmptyState` | 9 |

  - `SearchFilter` hard-codes the hint `"Search"` (`:155`).
  - `IconLabel` concatenates icon + `"  "` + label, and the result is then used as a `MenuItem`/`Button` ID (`:165-168`).
  - The engine-side `UI::` wrappers are in `source/SceneryEditorX/renderer/ui/ui.cpp:98-492`.
  - Direct ImGui calls with literals: `TextDisabled` 90, `Button` 94, `TableSetupColumn` 88, `MenuItem` 75, `TextUnformatted` 52, `Checkbox` 44, `Selectable` 34.
  - About 264 widget labels are pure `"##id"` placed after a `PropertyLabel`.
- **BAS-004**: Panels already follow the `"LABEL###Id"` convention: `PanelInfo` registry entries such as `{"Properties", "PROPERTIES###Properties", …}` (`source/Editor/core/editor_layer.cpp:91-114`), documented at `editor_ui.h:42`. The same title literal is written in three places:
  - the registry;
  - again in each panel's `EditorUI::BeginPanel` call (12 files, e.g. `properties_panel.cpp:777`, `settings_panels.cpp:90`);
  - a third time in each constructor's `m_Title`, which only the unreferenced `menu_bar.cpp` reads.

  Separate mixed-case display names, passed to `AddPanel` in `RegisterPanels` (`editor_layer.cpp:312-324`), appear in the Window menu (`:642`) and in the strip tooltips (`:814`).

  How layout state is persisted:
  - `EditorLayout.json` stores `{FNV1A(id), Name, IsOpen}` and reads it back by ID only (`source/SceneryEditorX/renderer/ui/panel_manager.cpp:152-211`).
  - `editor.ini` (`renderer/ui/ui_layer.cpp:66-67`) stores only the part after `###`. The user's file has `[Window][Properties]`, `[Window][Viewport]` and so on.
  - The dock builder uses the full window strings (`editor_layer.cpp:420-444`, `:868-880`).

  Modal titles:
  - Some modals use `###` (`editor_project.cpp:71-72`, `editor_airports.cpp:57`, `airport_panel.cpp:57-58`).
  - Others repeat one literal in `OpenPopup` and `BeginPopupModal` (`editor_layer.cpp:888`/`:893`; `project_dialogs.cpp:60-61`).

  Main menu names (`"File" … "Help"`) are both label and ID, and are reused by the hotbox (`editor_layer.cpp:457-471`).
- **BAS-005**: `EditorCommands::Command` stores `label`, `icon`, `shortcut` and `tooltip` as `std::string` at registration (`source/Editor/core/editor_commands.h:51-57`, drawn at `editor_commands.cpp:98-110`).
  - Registrations: `viewport_menus.cpp:698-726`, `facade_tool.cpp` (14), `facade_authoring.cpp:661-686`. The tool name and key tables are at `editor_tools.cpp:384-436`.
  - Sentences are built by concatenation:
    - `GetToolName(tool) + " Tool"` (`viewport_menus.cpp:704`);
    - `"Undo " + label` (`editor_layer.cpp:491-492`);
    - `"Place " + name + " Here"` (`viewport_menus.cpp:198`).
  - About 19 sites build English plurals with `== 1 ? "" : "s"` (e.g. `editor_tools.cpp:772`, `facade_tool.cpp:1277`, `project_dialogs.cpp:463`).
  - Shortcut text is hard-coded English (`"Ctrl+Z"`, `"Del"`; `editor_layer.cpp:493-501`, `viewport_menus.cpp:710-722`).
  - One place upper-cases at runtime with ASCII `toupper`: `Upper()` in `viewport_menus.cpp:171-176`.
- **BAS-006**: Some user-facing text comes from AppCore or from logs:
  - `ParseCoordinate` and `ValidateNewAirportProject` build English messages by concatenation with `std::to_string` (`source/SceneryEditorX/project/scenery_pack.cpp:223-349`). `DrawProblem` displays them (`project_dialogs.cpp:183-196`).
  - `GetItemKindName` (`scenery_project.cpp:177-197`) returns the same tokens that `.edX` serialization writes (`project_edx.cpp:77-78`), so it must not be translated in place.
  - The status bar shows the latest log warning or error (`editor_layer.cpp:676-735`), and the CONSOLE panel shows log lines.
  - There is no toast or notification system. Some log lines are written as user instructions (`settings.cpp:529`).
- **BAS-007**: Fonts are loaded by `UILayer::LoadFonts` (`renderer/ui/ui_layer.cpp:165-186`, via `UI::Fonts::Add` in `renderer/ui/resources/fonts.cpp:42-59`), all at 15 px:
  - OpenSans Regular as Default, with FontAwesome 4 icons merged in (`0xF000-0xF307`);
  - OpenSans Light, Medium, SemiBold and Bold, without icons.

  Glyph coverage and rasterization:
  - The stock Vulkan backend sets `ImGuiBackendFlags_RendererHasTextures` (`backends/imgui_impl_vulkan.cpp:1452`), so glyphs load on demand. The `GetGlyphRangesDefault()` passed in does not limit coverage; the font file does.
  - Open Sans has Latin (including Vietnamese), Greek and Cyrillic, but no CJK.
  - `imgui_freetype.cpp` is compiled, but `IMGUI_ENABLE_FREETYPE` is not defined (`imconfig.h:93`), so ImGui rasterizes with stb_truetype.

  Load order and layout:
  - Fonts load in `UILayer::OnAttach` during the `Application` constructor (`application.cpp:115-116`, `ui_layer.cpp:72`). `UserPreferences` loads later (`main.cpp:212-218`); `settings.cfg` is already available.
  - Fixed widths will clip longer text: label columns of 90-170 px (`editor_ui.h:62`, `project_dialogs.cpp:308`, `facade_editor_panel.cpp:315`) and buttons of 130 px or less (`project_dialogs.cpp:436`, `:557`; `editor_layer.cpp:905`).
- **BAS-008**: The platform layer is SDL3 with `imgui_impl_sdl3`. SDL is the bundled submodule (`CMakeLists.txt:426-434`, release-3.4.0 line). For IME:
  - The backend positions the OS IME with `SDL_SetTextInputArea`/`SDL_StartTextInput` (`imgui_impl_sdl3.cpp:193-233`, `:613`).
  - SDL 3.4's `SDL_HINT_IME_IMPLEMENTED_UI` defaults to `"none"`, meaning native OS composition and candidate UI.
  - The backend header warns "Position somehow broken in SDL3" (`:14`). Its advice to set `SDL_HINT_IME_SHOW_UI` predates SDL3, which no longer has that hint.

  ImGui's own strings (docking and table context menus) go through `ImGuiLocKey_*` and `ImGui::LocalizeRegisterEntries` (`imgui_internal.h:2339-2355`, `:3750`).
- **BAS-009**: Encoding:
  - The project's CMake sets no `/utf-8`. MSVC targets inherit it from `spdlog`'s `INTERFACE_COMPILE_OPTIONS`; test targets that do not link spdlog do not get it.
  - Icon strings are `"\uf0xx"` escapes in narrow literals.
  - `ErrMsg` message boxes widen UTF-8 byte by byte (`source/SceneryEditorX/app_pch.h:125-135`, `source/Editor/xpeditor_pch.h:155-164`, `source/Launcher/startup_pch.h:143-151`), so non-ASCII text is garbled. `source/Editor/entrypoint.cpp:24-33` already converts correctly with `MultiByteToWideChar(CP_UTF8)`.
- **BAS-010**: Locale:
  - The only `setlocale` call is `setlocale(LC_TIME, "")` on Linux and macOS (`settings/user_settings.cpp:129`, `:194`).
  - Several file writers rely on the C locale's `.`: `xp_apt.cpp:123-136` (`vsnprintf "%.8f"`), `project_dsf.cpp:257`, `project_wed.cpp:265`, `xp_facade.cpp:49-53`.
  - Several parsers use `strtod`, `atof` or `sscanf`: `project_edx.cpp:181`, `xp_apt.cpp:469-470`, `xp_dsf.cpp:458-461`.
  - ImGui formats numbers with `vsnprintf` and parses them with `sscanf` (`imgui_widgets.cpp:2401-2416`).
  - UI formats bake units into format strings: `"%.2f m"` ×15, `"%.0f deg"` ×4.
  - Dates:
    - `GetSystemDateTimeFormat` (`user_settings.cpp:69-176`) maps OS formats to `strftime` tokens on Windows and Linux. On macOS it returns an ICU pattern that is then handed to `std::put_time` (`:233-234`).
    - The PROJECT panel shows the raw ISO modified time (`project_panel.cpp:246`).
    - The Gateway list prints `"%Y-%m-%d %H:%M"` or the English word `"never"` (`airport_panel.cpp:75-88`).
- **BAS-011**: Settings:
  - All `Settings` state is static (`settings/settings.h:270-274`), so `settings.cfg` and `user_preferences.cfg` share one in-memory config on `dev`. This is fixed on the unmerged branch `claude/settings-vfs-support-b7085e` (`ConfigFile`).
  - The pattern for a preference is `user_settings.h:124-130` / `user_settings.cpp:388-466`.
  - There is no live Preferences panel (`settings_panel.cpp:40-235` is commented out), and `EditorSettingsManager` is a stub.
- **BAS-012**: Outside the editor window:
  - Native file dialogs use portable-file-dialogs, with English titles and filter names (e.g. `editor_project.cpp:980`).
  - The NSIS installer is English only (`!insertmacro MUI_LANGUAGE "English"`, `installer/windows/SceneryEditorX.nsi:157`; no `LangString`).
  - The `.rc` version resources declare `LANG_ENGLISH` (`source/Editor/SceneryEditorX.rc:18`).
  - There is no `Info.plist` or `.desktop` file.
  - The crash-report dialog (`CrashHandler/dialog.cpp:80-155`) is a non-functional placeholder.
- **BAS-013**: Build and test:
  - The code is C++20.
  - `resources/` is found from the working directory (`core/platform/windows/context.cpp:107-141`). There are no CMake install rules; `installer/windows/Build-Installer.ps1:204` copies `resources\` into the stage folder.
  - Tests are opt-in (`SEDX_ENABLE_TESTS`, `CMakeLists.txt:226`). Each Catch2 v3 suite is its own executable (`source/Tests/CMakeLists.txt`; `ConversionTests` at `:235-258` is the standard-library pattern). The Linux g++ recipes are in `AGENTS.md:24-74`.
  - There is no CI: `.github/workflows` was deleted in `f6e3a53b`.
  - CLI flags are parsed in `editor_layer.cpp:976-1019`, `main.cpp:131-170` and `project_dialogs.cpp:622-653` (`--show-dialog`). There is no `--help`, and no `--focus-panel` on `dev`.
- **BAS-014**: The vendored ImGui is 1.93.0 (`renderer/ui/source/imgui/imgui.h:32-33`). It has three behaviours this plan relies on:
  - **`###` handling.** `"a###b"` hashes like `"b"` (`imgui.cpp:2620-2668`), and the `.ini` stores only `b` (`:2670-2679`, `:16721-16723`).
  - **Runtime font merging.** `ImFontConfig::DstFont` merges into an existing font at runtime (`imgui_draw.cpp:4322`).
  - **Font helpers.** `FontNo` selects a face in a `.ttc` (`:6239`), and `ImFont::IsGlyphInFont` exists (`:6913`).

  It also has ID-conflict highlighting (`io.ConfigDebugHighlightIdConflicts`, `imgui.h:2655`).

### Requirements

- **REQ-001**: English (`en`) is the source language. Every user-facing string in the editor UI is translatable:
  - window and panel titles, menus, buttons, labels, tooltips and hints;
  - status bar text and modal dialogs;
  - form validation messages;
  - context menus, the quad menu and the Space hotbox;
  - native file-dialog titles and filter descriptions;
  - user-facing error summaries.

  Not translated:
  - log messages;
  - CLI flag names;
  - file contents and file-format keywords;
  - settings keys and ImGui IDs;
  - serialization tokens such as `GetItemKindName`'s;
  - user data: airport names, ICAO codes, file and library paths, art asset names, X-Plane directive and dataref names.
- **REQ-002**: Catalogs are GNU gettext PO files in UTF-8: the template `resources/locale/sedx.pot` and translations `resources/locale/<tag>.po`.
  - File tags follow Weblate's gettext naming: `de`, `fr`, `pt_BR`, `zh_Hans`, `zh_Hant`.
  - Supported entry features:
    - `msgctxt`;
    - `msgid_plural` with `msgstr[n]`;
    - `#.` translator comments, `#:` references and `#,` flags (`fuzzy` entries are ignored);
    - obsolete `#~` entries (ignored);
    - the header's `Language` and `Plural-Forms`.
- **REQ-003**: Lookup falls back per string: active catalog → catalog of the base language (`pt_BR` → `pt`) → English source text. A catalog that fails to parse is reported once (log + status-bar message, TASK-034) and English is used.
- **REQ-004**: Plural forms come from the catalog's `Plural-Forms` header.
  - The evaluator supports `n`, integer literals, `! * / % + - < <= > >= == != && || ?:` and parentheses.
  - The checker (TASK-012) rejects a header whose `nplurals` disagrees with the built-in table for that language (TASK-005). At runtime such a catalog falls back to the table's rule.
- **REQ-005**: Formatted messages use `std::format` replacement fields.
  - Translators may reorder positional fields (`{1} … {0}`).
  - A translation that fails to format (bad field, wrong index) is replaced by the English text for that call and logged once per message. Nothing throws out of UI code.
- **REQ-006**: The language setting is `ui.language` in `settings.cfg`: `"system"` (default) or a tag.
  - **Why `settings.cfg`.** The key already exists, and that file is loaded before the fonts are (BAS-007).
  - **Migration.** The never-read legacy value `"english"` is migrated to `"system"` once.
  - **Resolving `"system"`.** It resolves through the OS's preferred UI languages:
    - Windows: `GetUserPreferredUILanguages`;
    - macOS: `CFLocaleCopyPreferredLanguages`;
    - Linux: `LANGUAGE` → `LC_ALL` → `LC_MESSAGES` → `LANG`.
  - **Matching.** Those languages are matched against the installed catalogs with BCP-47 rules: `zh-CN`/`zh-SG`/`zh-Hans-*` → `zh_Hans`; `zh-TW`/`zh-HK`/`zh-MO`/`zh-Hant-*` → `zh_Hant`; then region and script fallback; then `en`.
  - **Override.** `--language <tag>` overrides both for one run.
- **REQ-007**: Changing the language in Preferences takes effect at the start of the next frame, with no restart and no lost state.
- **REQ-008**: These are byte-for-byte identical in every language to what they are today (TASK-022):
  - ImGui IDs;
  - the `editor.ini` docking layout and window keys;
  - `EditorLayout.json`;
  - tree, table, popup and collapsing-header state;
  - dock-builder window names.
- **REQ-009**: Every language of the initial set renders without ImGui fallback glyphs in all five OpenSans UI fonts.
  - Latin-script languages, Greek, Russian and Ukrainian use Open Sans as it is.
  - Simplified Chinese, Traditional Chinese, Japanese and Korean each use a regional CJK font chosen per language and merged lazily (TASK-025).
- **REQ-010**: The process locale is never changed beyond today's `LC_TIME`: no `setlocale(LC_ALL/LC_NUMERIC)`, no `std::locale::global`, no `imbue` on shared streams. The apt.dat, DSF, WED and facade writers depend on it (BAS-010).
  - Numbers in the UI keep `.` decimals and their current formats.
  - Dates and times shown in the UI use a per-language `std::format` chrono pattern supplied by the catalog (context `datetime`), defaulting to ISO 8601.
- **REQ-011**: A pseudo-locale `qps-ploc` is built in, with no file:
  - each translated string becomes `[!! …accented text… ·····!!]`, lengthened by 40% and wrapped in brackets;
  - placeholders, `%` sequences and `###` IDs are left intact.

  It is selectable with `--language qps-ploc` and, in debug builds, in Preferences.
- **REQ-012**: Preferences lists languages by their own names (`Deutsch`, `日本語`) with completion percentages. Languages below 40% are hidden unless "Show incomplete translations" is ticked (ASSUMPTION-001; Godot uses 20% for its editor).
- **REQ-013**: Translators can test without building:
  - a `<tag>.po` in `user:/locale/` overrides the bundled file of the same tag;
  - Help > Reload Translations reloads it;
  - `--watch-translations` reloads it automatically when the file changes.
- **REQ-014**: Performance:
  - `Tr` and `TrN` (with or without `Ctx`) cost one hash-table probe on a hash computed at compile time;
  - `TrL` returns a cached string, so steady-state frames make no heap allocations for translation;
  - parsing a 5,000-entry catalog takes under 20 ms in a release build.
- **REQ-015**: Threading:
  - `Tr`, `TrN`, `TrF` and `TrNF` may be called from any thread;
  - returned `const char*` stay valid for the whole session, because catalogs are never freed before shutdown;
  - `TrL` and language switching are UI-thread only.
- **REQ-016**: Native message boxes show non-ASCII (translated) text correctly on Windows (TASK-019).

### Legal and licensing

- **LEG-001**: The runtime is SEDX's own MIT code. No LGPL or GPL code is linked: not `libintl`, not FriBidi. GNU gettext's tools (GPL-3.0) may be used as optional developer and CI cross-checks, never shipped.
- **LEG-002**: Fonts and their licenses:
  - **Bundled fonts.** Noto Sans SC, TC, JP and KR are SIL Open Font License 1.1. Their license text goes next to the files (`resources/fonts/noto/LICENSE.txt`, like `resources/fonts/opensans/LICENSE.txt`), and the installer lists them.
  - **System fonts.** They are loaded from the user's machine at runtime and are never copied or shipped.
- **LEG-003**: Contributed translations are licensed under the project's MIT license. `docs/localization-translator-guide.md` and the Weblate project's contributor agreement say so.

### Constraints

- **CON-001**: `source/SceneryEditorX/i18n/` is standard-library-only C++20, with no ImGui, SDL, Vulkan, OS or third-party headers, so it builds in the `AGENTS.md` Linux g++ recipe. OS language detection lives in the platform layer, with Windows, macOS and Linux branches all implemented.
- **CON-002**: Catalog files are read through the VFS and the UTF-8 path helpers (`VFS::ToUtf8`/`FromUtf8`). Never call `std::filesystem::path::string()` on paths that can come from the profile or the install folder, because MSVC throws for characters outside the ANSI code page. Font paths built by string concatenation in `UILayer::LoadFonts` must not be copied for catalog paths.
- **CON-003**: The vendored ImGui stays at 1.92.6 or newer, because REQ-008 relies on `"a###b"` hashing like `"b"`. `i18n_imgui.h` carries `static_assert(IMGUI_VERSION_NUM >= 19206)`.
- **CON-004**: The API is plain functions, not macros. A function-like macro named `TR` would rewrite Skia's `SkRect::TR()` member (`skia/core/SkRect.h:838` in the vcpkg tree, a transitive dependency) in any file that includes it after the macro, and `_()` collides with third-party code. The names `Tr`, `TrN`, `TrF`, `TrNF`, `TrL`, `TrNoop` and `Ctx` are unused in `source/` at `1b079ccc`. The `consteval` `Msg` constructor already limits them to string literals, which is all a macro would have added.
- **CON-005**: Logs stay English, so bug reports stay readable by developers. Text the user must act on goes through the user message channel (TASK-034) in addition to the log. A user-facing error shows a translated summary plus the untranslated technical detail (path, error code, exception text).
- **CON-006**: Serialization tokens and the display names shown for them are separate functions. `GetItemKindName` (`.edX` tokens), apt.dat type names and Gateway source names keep returning their tokens; translated names come from new `Get*DisplayName` functions (TASK-035).
- **CON-007**: Every phase builds, and its tests pass, on Windows (MSVC, clang-cl), Linux (GCC, Clang) and macOS (Apple Clang).

### Guidelines and patterns

- **GUD-001**: Translate whole sentences. Never build a sentence from translated fragments; use one message with placeholders.
- **GUD-002**: Give short, ambiguous English strings a context, for example `Tr(Ctx("verb", "Open"))` vs `Tr(Ctx("state", "Open"))`, and `Ctx("panel-title", "PROPERTIES###Properties")`.
- **GUD-003**: Put a `// TRANSLATORS:` comment on the line above anything unclear: what each placeholder holds, units, where the text appears, length limits. The extractor copies it into the template as a `#.` comment.
- **GUD-004**: Size widgets from text (`ImGui::CalcTextSize`, multiples of `ImGui::GetFontSize()`), never from fixed pixel widths sized for English (BAS-007).
- **GUD-005**: Translate at display time. Commands, undo labels, status messages, validation problems and anything built on a worker thread store a message literal plus arguments, and the UI translates when it draws, so a language switch updates them too.
- **GUD-006**: X-Plane terms of art follow the Weblate glossary (TASK-047). File-format keywords, directive names, dataref names and file extensions are never translated.
- **PAT-001**: Which function to use:
  - **`TrL`** for any label that also forms an ImGui ID: windows, menus, buttons, checkboxes, tabs, tree nodes, headers, selectables, combo/slider/input labels.
  - **`Tr`** for display-only text: `Text*`, tooltips, hints, combo items, table column headers, status text, `AddText`.
  - **`TrF` / `TrNF`** for formatted text.
  - **`TrNoop`** for static tables of labels, translated later with `Tr(entry)`.
  - **`EditorUI` helpers** take literals directly (TASK-032).

## 2. Implementation Steps

### Implementation Phase 1 — `i18n` core (standard library only)

- GOAL-001: A tested catalog runtime that parses PO files, evaluates plural rules, matches languages, translates and formats, with no UI code touched.
- CRIT-001: `I18nTests` passes on Windows, Linux and macOS and in the Linux g++ recipe; a generated 5,000-entry catalog parses in under 20 ms (release build).

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-001 | `source/SceneryEditorX/i18n/message.h`: `consteval` FNV-1a 64-bit hashing; `struct Msg` with a `template <std::size_t N> consteval Msg(const char (&)[N])` constructor storing the text, its length, the hash of the whole literal, and the length and hash of the visible part (up to the first `##`), so only string literals convert and nothing is hashed at runtime; `struct CtxMsg { Msg context, text; }` hashing `context + '\x04' + text` (gettext's separator). | | |
| TASK-002 | `i18n/po_parser.h/.cpp`: parse PO text (`std::string_view`) into entries: `msgctxt`, `msgid`, `msgid_plural`, `msgstr`, `msgstr[n]`, multi-line continuation strings, C escapes (`\n \t \" \\ \xHH`, octal), `#.`/`#:`/`#,`/`#\|` comments, obsolete `#~` (skipped), CRLF and BOM tolerance; errors carry line numbers; never throws. | | |
| TASK-003 | `i18n/plural_expr.h/.cpp`: tokenizer, precedence-climbing parser and evaluator for `Plural-Forms` (`nplurals=N; plural=EXPR;`), 64-bit unsigned `n` like gettext, division and modulo by zero evaluate to 0, result clamped to `[0, nplurals-1]`. | | |
| TASK-004 | `i18n/catalog.h/.cpp`: immutable `Catalog` built from parsed entries: `std::unordered_map<std::uint64_t, Entry>` keyed by the same FNV-1a hash as `Msg`, storing the source text for a collision check, the singular and plural translations and the compiled plural rule; `Find(hash, source)`, `FindPlural(hash, source, n)`; statistics (translated, fuzzy, total) for REQ-012; empty and fuzzy translations count as missing. | | |
| TASK-005 | `i18n/languages.h/.cpp`: table of known languages: tag, English name, native name (`Deutsch`, `Português (Brasil)`, `日本語`, `简体中文`, `繁體中文`, `한국어`, `Українська`), script (`Latin`, `Greek`, `Cyrillic`, `HanSimplified`, `HanTraditional`, `Japanese`, `Korean`), a sample string of letters the language needs (for TASK-025's coverage check), CLDR-based `nplurals` and default `Plural-Forms`, direction (LTR only until Phase 7). BCP-47 parsing and `MatchLanguage(preferred list, available tags)` per REQ-006. | | |
| TASK-006 | `i18n/i18n.h/.cpp`: the service: `SetCatalogs(...)`, `SetLanguage(tag)`, `GetLanguage()`, an atomic pointer to the active catalog and its base-language fallback, `Translate(Msg)`, `Translate(CtxMsg)`, `TranslatePlural(Msg, Msg, std::int64_t n)` (+ context overload), `TranslateRuntime(std::string_view)` (slow path for text that is not a literal, hashed at runtime), a generation counter bumped on every change, and a missing-string recorder (hash, source, count) enabled by `--i18n-log-missing`. Catalogs are kept until shutdown (REQ-015). | | |
| TASK-007 | `i18n/format.h`: `Format(Msg fmt, Args&&...)` and `FormatPlural(Msg singular, Msg plural, std::int64_t n, Args&&...)` (with `n` as field `{0}`): translate, then `std::vformat` with `std::make_format_args` on lvalues; catch `std::format_error`, log once per message hash, format the English text instead (REQ-005). `FormatDateTime(std::chrono::system_clock::time_point)` uses the catalog's `datetime` pattern (REQ-010). | | |
| TASK-008 | `i18n/pseudo.h/.cpp`: `qps-ploc` transform (REQ-011): accent map for A-Z/a-z, 40% padding with `·`, `[!!` `!!]` brackets, leaving `{…}` fields, `%` sequences, `##`/`###` suffixes and `\n` untouched; implemented as a synthetic `Catalog` that transforms on first lookup and caches. | | |
| TASK-009 | `i18n/tr.h`: the public API as functions in namespace `SceneryEditorX` (CON-004): `Tr(Msg)`, `Tr(CtxMsg)`, `TrN(singular, plural, n)` (+ `CtxMsg` overload), `TrF(fmt, args...)`, `TrNF(singular, plural, n, args...)`, `consteval CtxMsg Ctx(context, text)` and `consteval Msg TrNoop(text)` (marks a literal for extraction without translating it), each documented with one example. `TrL` is declared in TASK-022 because it needs ImGui. | | |
| TASK-010 | `source/Tests/i18n_tests/I18nTests.cpp` and target `I18nTests` in `source/Tests/CMakeLists.txt` (the `ConversionTests` pattern, `:235-258`, plus `/utf-8` for MSVC-like compilers because the target does not link spdlog, BAS-009): PO edge cases (escapes, continuation lines, CRLF, BOM, obsolete and fuzzy entries, missing header, duplicates, malformed lines with line numbers); plural rules for `en`, `fr` (0 is singular), `de`, `ru`, `uk`, `pl`, `cs`, `ar` (6 forms), `ja`/`zh`/`ko` (1 form) checked against CLDR examples for n = 0..200 and 1,000,000; REQ-006 matching cases; hash equality between `Msg` (compile time) and `Catalog` (load time); format fallback; pseudo-locale keeps fields intact; the CRIT-001 timing check. Add its g++ command to `AGENTS.md`. | | |

### Implementation Phase 2 — Tooling: extraction, checks, lint and CI

- GOAL-002: Developers and CI can regenerate the template, validate every catalog and find untranslated literals, with Python's standard library only.
- CRIT-002: `python -I tools/i18n/sedx_i18n.py extract --check` exits 0 on a clean tree and 1 when the template is stale; `check` rejects each seeded defect in its fixtures; the workflow from TASK-016 runs green on Windows, Linux and macOS.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-011 | `tools/i18n/sedx_i18n.py extract`: scan `source/**/*.{h,hpp,cpp,inl,mm}` (excluding vendored trees: `source/SceneryEditorX/renderer/ui/source/`, `dependency/`) with a small C++ lexer that skips comments, preprocessor lines and character literals, understands ordinary and raw string literals, adjacent-literal concatenation and escapes, and records calls to the TASK-009/TASK-022 functions (`Tr`, `TrN`, `TrF`, `TrNF`, `TrL`, `TrNoop`, `Ctx`) and the TASK-032 `EditorUI` helpers (keyword table with argument positions). For `TrL` and ID-forming helpers the `msgid` is the visible part (before `##`). Writes `resources/locale/sedx.pot` deterministically: sorted by context and msgid; `#:` references as repository-relative paths without line numbers (small diffs); `#.` from `// TRANSLATORS:`; `#, python-brace-format` on entries containing `{`. `--check` compares instead of writing. Non-literal arguments already fail to compile (`consteval`); the extractor reports any call it cannot read. | | |
| TASK-012 | `sedx_i18n.py check [files]`: PO syntax (TASK-002 rules); header `Language` and `Plural-Forms` present; `nplurals` matches the TASK-005 table; `msgstr[n]` count equals `nplurals`; each translation's `{…}` fields are a subset of the English ones and every English field appears (except in the singular form of a plural); no `###`/`##` in any msgid or translation; leading/trailing whitespace and `\n` count match; `max-length:N` flags respected; entries missing from the template reported as stale. Exit 1 on errors; print a per-language completion table. | | |
| TASK-013 | `sedx_i18n.py lint`: report ImGui and `EditorUI`/`UI::` calls whose text argument is a raw literal not passed through `Tr`/`TrL` or a TASK-032 helper (`Begin`, `BeginMenu`, `MenuItem`, `Button`, `SmallButton`, `Checkbox`, `RadioButton`, `Selectable`, `TreeNode*`, `BeginTabItem`, `BeginPopupModal`, `OpenPopup`, `Text*`, `SeparatorText`, `SetTooltip`, `SetItemTooltip`, `TableSetupColumn`, `InputTextWithHint` hints, `Combo`, `AddText`, `pfd::open_file/save_file/select_folder` titles); labels starting with `##` and entries in `tools/i18n/lint_allow.txt` pass. A baseline (`tools/i18n/lint_baseline.txt`) lists today's sites so only new ones fail; Phase 4 empties it. | | |
| TASK-014 | `sedx_i18n.py stats --markdown`: completion per language for `docs/localization-status.md` and the README. | | |
| TASK-015 | Packaging: `resources/locale/` needs no CMake rule in development builds (resources are found from the working directory, BAS-013) and is already staged by `Build-Installer.ps1:204`; add a staging check that the folder and every `.po` are present, and add CTest tests `i18n_template_current` (`extract --check`) and `i18n_catalogs_valid` (`check`) under `SEDX_ENABLE_TESTS` when `Python3_Interpreter` is found. | | |
| TASK-016 | CI (the repository has none since `f6e3a53b`; adding the first workflow is a decision for the owner, DEC-005): `.github/workflows/localization.yml` with a `windows-latest` / `ubuntu-latest` / `macos-latest` matrix that runs `test_sedx_i18n.py`, `extract --check`, `check`, `lint`, and builds and runs `I18nTests` standalone (Catch2 submodule only, no vcpkg); on Ubuntu also `msgfmt --check-format --check-header -o /dev/null` on every PO file as an independent cross-check (LEG-001: tool only). | | |
| TASK-017 | `tools/i18n/tests/` fixtures (one defect per file) and `test_sedx_i18n.py` (`python -I -m unittest`) covering the lexer (raw strings, concatenation, comments containing fake calls, `##` handling), template determinism and every `check` rule. | | |

### Implementation Phase 3 — Editor integration: language service, fonts, labels, preferences

- GOAL-003: The editor picks a language at startup, switches it live, draws every supported script and offers translation-safe label helpers, while the visible text is still English.
- CRIT-003: With `--language qps-ploc` every converted string is bracketed and accented; `editor.ini` and `EditorLayout.json` are byte-identical after a scripted session that switches English → pseudo-locale → Japanese → English (TEST-004); with `--language ja`, `zh_Hans`, `zh_Hant` and `ko`, `--i18n-selftest` reports no missing glyphs on Windows, macOS and Linux.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-018 | Platform language detection (REQ-006): `GetPreferredUiLanguages()` in the platform layer: Windows `GetUserPreferredUILanguages(MUI_LANGUAGE_NAME)`; macOS `CFLocaleCopyPreferredLanguages`; Linux/POSIX the environment chain, with `C`/`POSIX` meaning English and encoding/modifier suffixes stripped (`de_DE.UTF-8@euro` → `de-DE`). | | |
| TASK-019 | Encoding prerequisites (BAS-009): add `/utf-8` for MSVC and clang-cl to `cmake/toolchains/sedx-windows.cmake` so every target gets it, not only spdlog consumers; make `ErrMsg` in `app_pch.h:125-135`, `xpeditor_pch.h:155-164` and `startup_pch.h:143-151` convert with `MultiByteToWideChar(CP_UTF8)` like `entrypoint.cpp:24-33`, with a translated title (REQ-016). | | |
| TASK-020 | Catalog discovery and early load: enumerate `resources/locale/*.po` and `user:/locale/*.po` through the VFS (user files override bundled ones, REQ-013); read headers and statistics lazily for Preferences; resolve and parse the selected catalog in the `Application` constructor before the `UILayer` is pushed (`application.cpp:115-116`), so fonts are set up for the right script on the first frame; keep loaded catalogs for the session; log the chosen language, its file and completion. | | |
| TASK-021 | Settings and flags: read `ui.language` from `settings.cfg` (BAS-001) and migrate `"english"` to `"system"` once; add `ui.show_incomplete_translations` (false). Use the `Settings` API on `dev`, or `ConfigFile` if `claude/settings-vfs-support-b7085e` has merged by then (DEC-006). Add `--language <tag>`, `--i18n-log-missing`, `--watch-translations` and `--i18n-selftest` next to the existing flags (`editor_layer.cpp:976-1019`). | | |
| TASK-022 | `source/SceneryEditorX/renderer/ui/i18n_imgui.h/.cpp` (usable by AppCore's `UI::` and the editor's `EditorUI`): `const char* Label(Msg)` / `Label(CtxMsg)` returns `"<translated visible part>###<id>"`, where `<id>` is the part after `###` when the label has one and the whole original label otherwise, so `ImHashStr` gives exactly today's ID (REQ-008, CON-003); labels whose visible part is empty come back unchanged; results are cached per catalog generation in a map keyed by the literal's hash, so steady-state frames don't allocate. Entry points `TrL(Msg)` and `TrL(CtxMsg)`. A debug-build check compares `ImHashStr(Label(x))` with `ImHashStr(x)` on first use of each label. | | |
| TASK-023 | Panel titles from one place: `PanelInfo::window` changes from `const char*` to a `CtxMsg` built at compile time (`Ctx("panel-title", "PROPERTIES###Properties")`), and `EditorUI::BeginPanel` takes the `PanelInfo` and draws `TrL(info.window)`; the 12 per-panel title literals and the unused `m_Title` copies go away (BAS-004); `PanelData::Name` display names are translated when drawn in the Window menu (`editor_layer.cpp:642`) and the strip tooltips (`:814`), context `panel-name`; the dock builder keeps using the window's English text (same IDs). Uppercase titles stay translator-written (translator note: "shown in capitals; use capitals if your script has them"); `Upper()` in `viewport_menus.cpp:171-176` is replaced by a translated, already-uppercase display name. | | |
| TASK-024 | Live switching: Preferences queues `SetLanguage`; the editor applies it at the start of the next frame (before `ImGui::NewFrame`), bumps the generation, asks the font manager for the new script (TASK-025), refreshes ImGui's own strings (TASK-027) and the SDL window title (built with `std::format` at `editor_layer.cpp:1079`; product and project names stay untranslated). | | |
| TASK-025 | Font fallback chain in `UILayer::LoadFonts` / `UI::Fonts` (BAS-007): keep the five OpenSans fonts and the FontAwesome merge; for CJK languages merge the regional font into each of Default, Light, Medium, SemiBold and Bold with `ImFontConfig::MergeMode = true`, `DstFont` = that font and `FontNo` for `.ttc` faces, at a frame boundary (ImGui 1.92+ allows `AddFont*` at any time). Sources, in order: (1) a user-configured font path; (2) the OS regional UI font from the OS fonts folder (Windows `FOLDERID_Fonts`, not a hard-coded `C:\Windows`): `msyh.ttc` (zh_Hans), `msjh.ttc` (zh_Hant), `YuGothM.ttc` (ja; `meiryo.ttc` is an optional Windows feature and absent on the reference Windows 11 machine), `malgun.ttf` (ko); macOS a CoreText lookup (`CTFontCreateUIFontForLanguage` + `kCTFontURLAttribute`) with `/System/Library/Fonts/PingFang.ttc`, `Hiragino Sans GB.ttc`, `ヒラギノ角ゴシック W3.ttc` and `AppleSDGothicNeo.ttc` as fallbacks; Linux `fc-match -f '%{file}:%{index}' 'sans:lang=<tag>'`, then known Noto Sans CJK paths; (3) a bundled or downloaded `NotoSans{SC,TC,JP,KR}-Regular.otf` (TASK-046). Check coverage with `ImFont::IsGlyphInFont` against the language's sample string (TASK-005). Merged sources stay until restart or are removed on switch (decide in implementation; the Windows CJK fonts measured in the report are 13-21 MB each). Set `TexMaxWidth/TexMaxHeight` from the device limit and call `CompactCache()` after a language change. | | |
| TASK-026 | No CJK font found for the chosen language: keep the language, post a status-bar message (TASK-034) naming the missing font and the documented fix (install the OS language's fonts, or the optional font package), and log it. | | |
| TASK-027 | ImGui's own strings (docking and table context menus, BAS-008): on every language change call `ImGui::LocalizeRegisterEntries` with the `ImGuiLocKey_*` entries translated from the catalog (context `imgui`), and add those English strings to the template through a `TrNoop` table. | | |
| TASK-028 | IME (BAS-008): set `SDL_HINT_IME_IMPLEMENTED_UI` to `"none"` explicitly before SDL is initialized (native OS composition and candidate UI), then test Japanese, Chinese and Korean input on each OS (TEST-006); if the candidate window is misplaced (the backend's "position somehow broken" note), fix the `SDL_SetTextInputArea` rectangle in SEDX's ImGui glue, not in the vendored backend. | | |
| TASK-029 | `--i18n-selftest` (pattern of `--tools-selftest`): for each installed catalog, switch language and merge its fonts, then (a) decode every code point of every translation and check it with `ImFont::IsGlyphInFont` in each UI font, (b) draw one frame of every registered panel and dialog with `io.ConfigDebugHighlightIdConflicts` on and count ID conflicts and TASK-022 label-ID mismatches; print a per-language report; exit non-zero on any missing glyph, conflict or mismatch. | | |
| TASK-030 | Preferences window (none exists, BAS-011): Edit > Preferences… opening a modal in the pattern of the About dialog (`editor_layer.cpp:884-909`), or a registered panel (`PanelInfo` + `RegisterPanels` + `BuildDefaultLayout`), with an Interface > Language combo listing "System default (<resolved name>)" and every catalog at or above the threshold (REQ-012) by native name and completion; "Show incomplete translations"; the pseudo-locale in debug builds; a link to the translator guide. Help > Reload Translations (REQ-013). Native names need their fonts: the combo shows English names for languages whose font isn't loaded. | | |
| TASK-031 | Developer panel "TRANSLATIONS" (debug builds and `--i18n-log-missing`): active language and file, completion, the missing-string log with counts, and a "copy as PO" button for the missing entries. | | |

### Implementation Phase 4 — Convert the user interface

- GOAL-004: Every user-facing string in the live editor is marked; the lint baseline is empty.
- CRIT-004: `sedx_i18n.py lint` reports zero sites with an empty baseline; `--i18n-selftest` passes; pseudo-locale screenshots of every panel and dialog (TEST-005) show no unbracketed text except user data and no clipped control.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-032 | Make the wrappers translate (BAS-003): `EditorUI::ItemTooltip`, `PropertyLabel`, `IconLabel`, `SectionHeader`, `StripButton` (tooltip), `EmptyState`, `SearchFilter` (hint) and the `UI::` helpers get a `template <std::size_t N> f(const char (&text)[N])` overload that builds a `Msg` and translates (ID-forming ones through `Label`), plus a `std::string_view` overload for dynamic text that is shown as given. Exact-match template overloads win over the `string_view` conversion, so the ~540 literal call sites translate without edits; the extractor's keyword table lists these helpers. A `char` buffer filled at runtime would also match the template and then fail to compile (the `Msg` constructor is `consteval`), which is the intended signal to pass `std::string_view(buffer)` instead. `IconLabel` returns `icon + "  " + translation + "###" + icon + "  " + label`, keeping today's ID. | | |
| TASK-033 | Commands and tools (BAS-005): `EditorCommands::Command` stores `Msg`/`CtxMsg` for label and tooltip and translates in `MenuItem`; tool name/key/tooltip tables become `TrNoop` tables; concatenations become messages (`TrF("{0} Tool", toolName)`, `TrF("Undo {0}", label)`, `TrF("Place {0} Here", name)`); the ~19 `== 1 ? "" : "s"` sites become `TrNF`; shortcut text is built by one `FormatShortcut(chord)` with translatable modifier and key names (context `key`; German `Strg`, macOS symbols `⌘⌥⇧`). | | |
| TASK-034 | User message channel (no notification system exists, BAS-006): `EditorStatus::Post(severity, Msg, args...)` storing message + arguments (GUD-005), shown translated in the status bar (falling back to the latest log line as today) and also logged in English; move log lines that are really user instructions (e.g. `settings.cpp:529`) to it. | | |
| TASK-035 | AppCore messages (BAS-006, CON-006): `ValidateNewAirportProject` and `ParseCoordinate` return problems as {field id, `Msg`, arguments} instead of concatenated English, formatted by `DrawProblem`; export warnings likewise; add `GetItemKindDisplayName`, `GetAirportTypeDisplayName`, `GetSurfaceDisplayName`, `GetInstallSourceDisplayName` beside the token functions. | | |
| TASK-036 | Convert the remaining direct ImGui calls, area by area in small pull requests, in order of use: main menu and Window menu, status bar (`"Ready"`, the fps/ms/GPU/Version line), viewport and tool strip, PROPERTIES, OUTLINER, ASSETS shelf, CONSOLE (controls and column headers; log text stays English), X-PLANE LIBRARY, AIRPORTS (+ New Airport and Install Gateway modals), PROJECT, FACADES and FACADE EDITOR, scene/camera/renderer settings, the quad menu and Space hotbox (`viewport_menus.cpp`), panel context menus, New Airport Project and Export dialogs (`project_dialogs.cpp`, including `AIRPORT_TYPE_NAMES` as `TrNoop`), About, unsaved/recovery modals, `AddText` canvas labels (`editor_tools.cpp:261-277`, `facade_editor_panel.cpp`, `asset_shelf_panel.cpp`). Replace fixed widths with text-measured ones (GUD-004). | | |
| TASK-037 | Units and dates (REQ-010): unit suffixes inside `%`-format strings stay as they are (`"%.2f m"`); unit words in prose (`"Wind (m/s)"`, `"deg"`) become translatable; date display goes through `I18n::FormatDateTime` (PROJECT panel modified time, Gateway list, `"never"`), which also removes the macOS ICU-pattern-to-`put_time` mismatch in `GetSystemDateTimeFormat` for UI display. | | |
| TASK-038 | Native dialogs: translate portable-file-dialogs titles and filter descriptions (`"Airport data (apt.dat)"`), keeping the filter patterns (`*.dat`) unchanged (BAS-012). | | |
| TASK-039 | Legacy files (BAS-002): delete the unreferenced, compiled panels and windows (`menu_bar.cpp`, `properties.cpp`, `render_options.cpp`, `texture_viewer.cpp`, `file_dialog.cpp`, `asset_browser.cpp`, `keybind_window.cpp`, `about_window.cpp`) or add them to `lint_allow.txt` (DEC-007). | | |
| TASK-040 | Empty the lint baseline and make `lint` blocking; add the rules (PAT-001, GUD-001..006) to `AGENTS.md`, a new `.github/instructions/localization.instructions.md` (next to the 19 existing instruction files) and `docs/localization-system-documentation.md`. | | |

### Implementation Phase 5 — Translation workflow and first languages

- GOAL-005: Volunteers translate in a web UI; translations arrive as pull requests to `dev`; the first languages ship.
- CRIT-005: A Weblate pull request against `dev` passes the TASK-016 workflow; at least three languages reach 90% and pass `--i18n-selftest`.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-041 | Apply for Hosted Weblate Libre hosting (fallbacks: self-hosted Weblate, Crowdin's open-source license; DEC-001). Component: gettext PO, file mask `resources/locale/*.po`, template `resources/locale/sedx.pot`, new translations created from the template, push as pull requests to `dev`, add-ons "Update PO files to match POT (msgmerge)" and "Customize gettext output" (wrapping matching the extractor). | | |
| TASK-042 | Weblate checks: `python-brace-format` (set by the extractor), plurals, same plurals, punctuation, `max-length` where flagged; relax the capitalization checks only for context `panel-title`. | | |
| TASK-043 | Translator context screenshots: a script in `tools/i18n/` that drives the editor with the existing flags (`--show-dialog new\|export`, `--project`, `--lock-camera`; a `--focus-panel <id>` flag exists on `tester-feedback-round-1` and would be ported) to capture each panel and dialog in English and uploads them to Weblate through its API with the strings they show. | | |
| TASK-044 | Documentation in the `docs/` naming style: `docs/localization-translator-guide.md` (joining, glossary, placeholders, plurals, testing a PO through `user:/locale/` and Reload Translations, the pseudo-locale, license, LEG-003) and `docs/localization-system-documentation.md` (the `Tr*` functions, PAT-001, GUD rules, adding a language, tooling); link both from the README and the user manual's contributor appendix when that lands. | | |
| TASK-045 | Initial language set (DEC-002): X-Plane 12's interface languages — French, German, Italian, Spanish (Spain), Portuguese (Portugal), Russian, Ukrainian, Japanese, Korean, Simplified Chinese — plus Brazilian Portuguese and Polish if volunteers come forward. Create each PO from the template with the TASK-005 `Plural-Forms`. | | |
| TASK-046 | Optional CJK font package for systems without a regional font (minimal Linux installs; DEC-003): download on demand from a pinned release asset with SHA-256 verification into `user:/fonts/`, or an installer component; not bundled in the base installer by default. | | |
| TASK-047 | Weblate glossary: X-Plane and SEDX terms (scenery pack, DSF, OBJ8, facade, forest, autogen, draped polygon, apt.dat, ramp start, taxiway sign, library, exclusion zone, ATC), with "do not translate" on format names, directives and file extensions. | | |

### Implementation Phase 6 — Beyond the editor window

- GOAL-006: Everything the user reads before and around the editor is localized too.
- CRIT-006: The Windows installer shows translated pages for every shipped language; macOS and Linux metadata are localized once those packages exist.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-048 | NSIS installer (BAS-012): add `MUI_LANGUAGE` entries for the shipped languages and `LangString`s for the custom texts (welcome and finish pages, message boxes, section names, `DRIVER_HELP`, file-type names), generated into `installer/windows/lang/*.nsh` from a separate PO component so they are translated on Weblate too; on first run, default `ui.language` to the installer's language when the user picked one. | | |
| TASK-049 | macOS bundle and Linux desktop entry, when those packages are created: `CFBundleDevelopmentRegion` `en`, `CFBundleLocalizations` with the shipped languages; `Name[xx]`/`Comment[xx]` in the `.desktop` file and translated AppStream metadata, generated from PO. | | |
| TASK-050 | Early-startup text that appears before the catalog loads (the Vulkan 1.3 check in `entrypoint.cpp:24-33`, fatal startup errors): load the catalog first where the startup order allows; otherwise keep English and document it. | | |
| TASK-051 | User manual (`docs/manual` on the `user-manual` branch): evaluate po4a or Weblate's Markdown support for per-chapter translation with screenshots captured per language; out of scope until the UI translations are stable. | | |
| TASK-052 | Text domains for plugins, when the plugin system lands on `dev`: `I18n::Domain` with its own catalogs from `<plugin>/locale/*.po`, a `Domain` object per plugin with the same `Tr*` member functions (`domain.Tr("…")`, no per-target macros, which would break the one-definition rule in inline code), and `sedx_i18n.py extract --domain <name> --root <plugin dir>`; the core domain stays `sedx`. | | |

### Implementation Phase 7 — Deferred: right-to-left and complex scripts

- GOAL-007: Arabic, Hebrew and Persian (right to left) and Thai/Devanagari (complex shaping) render correctly.
- CRIT-007: Decided separately; not required for Phases 1-6.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-053 | Prototype a text layer that runs bidi reordering (SheenBidi, Apache-2.0) and HarfBuzz shaping (MIT; already built transitively through Skia, BAS-001) per run and draws shaped glyph IDs through `ImDrawList`, measured against ImGui's own text cost; decide whether mirrored layout (right-aligned panels, flipped tool strip) is in scope. Until then right-to-left languages are not offered. | | |

## 3. Alternatives

- **ALT-001**: GNU `libintl`. Rejected:
  - it is LGPL (LEG-001);
  - its state is global and driven by `setlocale` and the environment;
  - it is awkward to package on Windows;
  - the editor would still need its own fallback and pseudo-locale logic.
- **ALT-002**: Boost.Locale (BSL-1.0). Rejected for now: it brings Boost into the app, plus an MO build step, for what is about a thousand lines of code. Blender moved off it to its own `messages.cc`.
- **ALT-003**: ICU4C 78 with MessageFormat 2. Rejected for strings: the C++ MessageFormat 2 implementation is a technology preview, and linking ICU's data adds about 30 MB. Reconsider if CLDR collation or number/date formatting become requirements; ICU is already built transitively (BAS-001).
- **ALT-004**: Opaque keys in JSON files (ImHex's model). Rejected:
  - no plurals and no fuzzy tracking;
  - every string is edited in two places;
  - the code no longer shows the English text.
- **ALT-005**: Mozilla Fluent. Rejected: there is no maintained C++ implementation outside Firefox.
- **ALT-006**: Compile PO to MO, or embed catalogs in the binary (Godot). Not needed:
  - parsing PO is fast enough (REQ-014);
  - plain files let translators test without a build (REQ-013).

  Embedding stays an option if file loading becomes a problem.
- **ALT-007**: Restart to change language. Rejected: an immediate-mode UI makes live switching nearly free.
- **ALT-008**: `xgettext` for extraction. Kept only as an optional cross-check:
  - it is GPL tooling that Windows contributors would have to install;
  - the project extractor knows the `##` rule and the `EditorUI` helper keywords.
- **ALT-009**: Crowdin (free open-source license on application, proprietary) or Tolgee (Apache-2.0 core, self-hostable) instead of Weblate. Kept as fallbacks if Hosted Weblate is unavailable.
- **ALT-010**: Bundle all four CJK fonts in the installer. Rejected as the default: several tens of megabytes for users who mostly read Latin scripts (the Windows equivalents are 13-21 MB each), and system fonts cover Windows and macOS.
- **ALT-011**: Translate labels without `###` and migrate `editor.ini`. Rejected: ImGui 1.92.6 made the migration unnecessary (REQ-008).
- **ALT-012**: Edit every call site to wrap literals in `Tr` calls. Rejected for the `EditorUI` helpers: the literal-template overloads (TASK-032) do it for ~540 calls with no edits. Direct ImGui calls still need `TrL`/`Tr`.

## 4. Dependencies

- **DEP-001**: Python 3.12+ standard library for `tools/i18n/`, as the manual build already requires.
- **DEP-002**: Hosted Weblate (Libre plan) connected to the GitHub repository.
- **DEP-003**: GNU gettext tools (`msgfmt`) on the Ubuntu CI runner, as a cross-check only.
- **DEP-004**: Noto Sans SC/TC/JP/KR (SIL OFL 1.1) for the optional font package (TASK-046).
- **DEP-005**: Catch2 v3 (existing `dependency/Catch2` submodule) for `I18nTests`.
- **DEP-006**: Dear ImGui 1.92.6 or newer (vendored 1.93.0).
- **DEP-007**: SDL 3.2 or newer for `SDL_HINT_IME_IMPLEMENTED_UI` (bundled 3.4 line).
- **DEP-008**: Phase 7 only: HarfBuzz (MIT) and SheenBidi 3.0 (Apache-2.0) from vcpkg.

## 5. Files

- **FILE-001**: `source/SceneryEditorX/i18n/`: `message.h`, `po_parser.h/.cpp`, `plural_expr.h/.cpp`, `catalog.h/.cpp`, `languages.h/.cpp`, `i18n.h/.cpp`, `format.h`, `pseudo.h/.cpp`, `tr.h` (new).
- **FILE-002**: `source/SceneryEditorX/renderer/ui/i18n_imgui.h/.cpp` (new); `renderer/ui/ui_layer.cpp`, `renderer/ui/resources/fonts.h/.cpp` (font chain); `renderer/ui/ui.cpp` (`UI::` helpers).
- **FILE-003**: Platform language detection and CJK font lookup next to the existing per-OS code (`source/SceneryEditorX/core/platform/{windows,linux,mac}/`).
- **FILE-004**: `source/SceneryEditorX/core/application.cpp` (early catalog load), `settings/settings.cpp` (`ui.language` migration), `app_pch.h` (`ErrMsg`).
- **FILE-005**: `source/Editor/core/editor_ui.h/.cpp`, `editor_commands.h/.cpp`, `editor_layer.cpp` (panels, menus, status bar, Preferences, flags, live switching), `editor_tools.cpp`, `viewport_menus.cpp`, `project_dialogs.cpp`, `editor_project.cpp`, `editor_airports.cpp`, `editor_xplane.cpp`, `facade_tool.cpp`, `facade_authoring.cpp`, `node_tool.cpp`, `transform_tool.cpp`, `xpeditor_pch.h`; new `editor_status.h/.cpp` (TASK-034).
- **FILE-006**: `source/Editor/panels/*.cpp` (all live panels); new `translations_panel.cpp` (TASK-031) and `preferences` dialog.
- **FILE-007**: `source/SceneryEditorX/project/scenery_pack.cpp/.h`, `scenery_project.cpp/.h`, `xplane/xp_apt.cpp/.h`, `xplane/xp_install_finder.h` (problem records, display names).
- **FILE-008**: `source/Launcher/startup_pch.h` (`ErrMsg`); `cmake/toolchains/sedx-windows.cmake` (`/utf-8`).
- **FILE-009**: `resources/locale/sedx.pot`, `resources/locale/*.po` (new); `resources/fonts/noto/` with `LICENSE.txt` (optional package only).
- **FILE-010**: `tools/i18n/sedx_i18n.py`, `lint_allow.txt`, `lint_baseline.txt`, `tests/` (new).
- **FILE-011**: `source/Tests/i18n_tests/I18nTests.cpp` (new), `source/Tests/CMakeLists.txt`, `AGENTS.md`.
- **FILE-012**: `.github/workflows/localization.yml` (new, DEC-005), `.github/instructions/localization.instructions.md` (new).
- **FILE-013**: `docs/localization-system-documentation.md`, `docs/localization-translator-guide.md`, `docs/localization-status.md` (new); `README.md`.
- **FILE-014**: `installer/windows/SceneryEditorX.nsi`, `installer/windows/Build-Installer.ps1`, `installer/windows/lang/*.nsh` (new).

## 6. Testing

- **TEST-001**: `I18nTests` (TASK-010) on Windows, Linux and macOS and in the Linux g++ recipe.
- **TEST-002**: `test_sedx_i18n.py` (TASK-017) and the CTest checks `i18n_template_current` and `i18n_catalogs_valid` (TASK-015), all three OSes in the TASK-016 workflow, plus `msgfmt --check` on Ubuntu.
- **TEST-003**: `--i18n-selftest` (TASK-029) for every shipped language: no missing glyphs, no ID conflicts, no label-ID mismatches.
- **TEST-004**: Layout stability. A scripted session:
  1. docks panels;
  2. opens tree nodes and table sorts;
  3. switches English → `qps-ploc` → `ja` → English;
  4. exits gracefully (`taskkill /PID` without `/F`).

  `editor.ini` and `EditorLayout.json` must be byte-identical to a pure-English run of the same script.
- **TEST-005**: Pseudo-locale screenshots of every panel and dialog (`--language qps-ploc` with the screenshot recipe and a throwaway `APPDATA` profile), reviewed for unbracketed text and clipping; repeated in German for real expansion.
- **TEST-006**: IME. On each OS, type Japanese, Chinese and Korean into the New Airport Project name field and a property field, using the Microsoft IME on Windows, the built-in input sources on macOS, and fcitx5 or IBus on Linux. Check that:
  - committed text appears in the field;
  - the candidate window sits at the text cursor;
  - no keystrokes leak to viewport shortcuts during composition.
- **TEST-007**: Unicode paths: run with a profile under a non-ANSI folder (the existing "Zoë 日本" recipe) and a `user:/locale/` override, to prove catalog loading never goes through ANSI paths.
- **TEST-008**: Performance:
  - `--perf-log` frame times in English vs Japanese on the standard test airports show no measurable translation cost;
  - an allocation counter shows zero steady-state allocations from `TrL` and the `EditorUI` helpers.
- **TEST-009**: File-format regression: `XPlaneFormatTests` and `ProjectFormatTests` pass with the editor's language set to German. This proves REQ-010: no locale change reaches the writers.

## 7. Risks & Assumptions

- **RISK-001**: Converting ~870 live lines (Phase 4) touches most editor files and collides with concurrent feature branches (`tester-feedback-round-1` and the branches on top of it are 31+ commits ahead of `dev`).
  - Mitigation: TASK-032 removes most edits.
  - Mitigation: convert area by area in small pull requests.
  - Mitigation: the lint baseline keeps CI green meanwhile.
- **RISK-002**: Translated labels could collide inside one ID scope. Because `TrL` keeps the English IDs, they collide only where the English labels already collide.
  - Labels built at runtime (formatted with names) must keep a `###` stable part.
  - The TASK-022 debug check and TASK-029 catch mistakes.
- **RISK-003**: CJK fonts are large, and long sessions that draw many distinct Han characters grow the atlas. Mitigation: `TexMaxWidth/TexMaxHeight`, `CompactCache()` on language change, and only the active language's font is merged.
- **RISK-004**: SDL3 IME placement is flagged as unreliable by the ImGui backend, and SEDX has never been tested with an IME. Mitigation: TASK-028 and TEST-006 on all three OSes before a CJK language ships.
- **RISK-005**: Volunteer availability decides which languages reach 90%. Mitigation: recruit through the X-Plane community; the threshold (REQ-012) keeps mostly-English languages out of the list.
- **RISK-006**: Changing English text marks translations fuzzy and hides them until reviewed. Mitigation: freeze wording before release branches; when a pull request changes the meaning of English text, update that text's translations in the same pull request where possible.
- **RISK-007**: Hosted Weblate's free-hosting terms or queue may change. Mitigation: self-hosting and Crowdin as documented fallbacks; the repository's PO files stay the source of truth.
- **RISK-008**: `settings.cfg` and `user_preferences.cfg` share static state on `dev` (BAS-011), so a language setting written to one may appear in the other. Mitigation: read and write `ui.language` only through `settings.cfg`, and re-check after the settings rework merges.
- **ASSUMPTION-001**: The 40% listing threshold (REQ-012) is a proposal.
- **ASSUMPTION-002**: Scenery authors expect `.` decimals, because X-Plane and WED use them; localized number display is not requested.
- **ASSUMPTION-003**: ImGui's `ImGuiTextFilter` folds case for ASCII only. That is acceptable, because search filters mostly run over English asset and library names (user data); translated UI lists can use a simple Unicode-aware filter if needed.

### Decisions for the project owner

- **DEC-001**: Translation platform: Hosted Weblate (recommended), self-hosted Weblate, or Crowdin.
- **DEC-002**: The initial language set (TASK-045).
- **DEC-003**: The CJK font fallback for systems without a regional font: optional download (recommended), an installer component, or none.
- **DEC-004**: Whether translated date patterns are enough, or numbers should also follow the user's locale (recommended: no, ASSUMPTION-002).
- **DEC-005**: Adding the first GitHub Actions workflow (TASK-016), since `.github/workflows` was removed in `f6e3a53b`.
- **DEC-006**: Whether the settings rework (`claude/settings-vfs-support-b7085e`) merges before Phase 3, which changes the API used in TASK-021.
- **DEC-007**: Whether to delete the unreferenced legacy panels (TASK-039).

## 8. Related Specifications / Further Reading

- [Research report: Localizing Scenery Editor X](../reports/Localizing%20Scenery%20Editor%20X.md)
- [Dear ImGui FONTS.md](https://github.com/ocornut/imgui/blob/master/docs/FONTS.md) and [CHANGELOG (1.92.0 dynamic fonts, 1.92.6 `###` hashing)](https://github.com/ocornut/imgui/blob/master/docs/CHANGELOG.txt)
- [GNU gettext manual: PO files](https://www.gnu.org/software/gettext/manual/html_node/PO-Files.html) and [plural forms](https://www.gnu.org/software/gettext/manual/html_node/Plural-forms.html)
- [Unicode CLDR language plural rules](https://www.unicode.org/cldr/charts/latest/supplemental/language_plural_rules.html)
- [Weblate: gettext PO files](https://docs.weblate.org/en/latest/formats/gettext.html), [checks and flags](https://docs.weblate.org/en/latest/user/checks.html)
- [Godot: localization using gettext](https://docs.godotengine.org/en/stable/tutorials/i18n/localization_using_gettext.html); [Blender translation developer guide](https://developer.blender.org/docs/handbook/translating/developer_guide/)
- [Microsoft Learn: pseudolocalization](https://learn.microsoft.com/globalization/methodology/pseudolocalization)
