# Localization Instructions for GitHub Copilot

## Overview

Scenery Editor X's user interface is translatable. English text is written in the code and marked with the functions
of `source/SceneryEditorX/i18n/tr.h` (and `TrL` in `renderer/ui/i18n_imgui.h`); `tools/i18n/sedx_i18n.py` extracts it
into `resources/locale/sedx.pot`, and volunteers translate it on Hosted Weblate into `resources/locale/<tag>.po`.
`docs/localization-system-documentation.md` is the full reference.

## Marking text

| Write | For |
|---|---|
| `TrL("Export##pack")` | ImGui labels that are also IDs: `Begin`, `BeginMenu`, `MenuItem`, `Button`, `Checkbox`, `BeginTabItem`, `TreeNodeEx`, `Selectable`, `BeginPopupModal`, slider / combo / input labels |
| `Tr("No airport loaded")` | Display-only text: `Text*`, tooltips, hints, column headers, combo items, status text |
| `TrF("Exported {} tiles to {}", count, folder)` | Text with values (`std::format` fields) |
| `TrNF("{} file", "{} files", n)` / `TrN(...)` | Plurals |
| `Ctx("verb", "Open")` | Short, ambiguous words |
| `static constexpr Msg NAMES[] = { TrNoop("Runway") };` then `Tr(NAMES[i])` | Tables of labels |

- `EditorUI` helpers (`BeginPanel`, `SectionHeader`, `PropertyLabel`, `ItemTooltip`, `EmptyState`, `StripButton`,
  `IconLabel`) take a `Text` and translate literals themselves: don't wrap their literals in `Tr`.
- A new helper that shows text takes a `Text` parameter (a literal is translated, a run-time string shown as it is);
  the extractor finds it from the declaration.
- `TrL` keeps the English ID ("<translation>###<original>"), so layouts and state survive a language change. Keep
  `OpenPopup("Name")` English; `BeginPopupModal(TrL("Name"))` matches it.
- Put `// TRANSLATORS: ...` on the line above anything unclear (placeholders, units, where it appears).

## Don't

- Concatenate translated fragments (`"Undo " + label`): use one message with fields.
- Build plurals with `n == 1 ? "" : "s"`: use `TrN` / `TrNF`.
- Translate logs, file contents, settings keys, serialization tokens or user data (airport names, ICAO codes, paths,
  asset names, datarefs).
- Use a `TR()` macro or `_()`: the API is functions (a `TR` macro breaks Skia's `SkRect::TR()`).
- Call `setlocale(LC_ALL / LC_NUMERIC)` or `std::locale::global`: the file writers need `.` decimals.
- Size widgets for English with fixed pixel widths: measure the text.

## Checks

`python -I tools/i18n/sedx_i18n.py lint` fails on new untranslated ImGui literals (`tools/i18n/lint_baseline.txt` holds
the ones not converted yet; run `lint --update-baseline` after converting some). The Localization workflow refreshes
the template and catalogs on every pull request into `dev`. `I18nTests` covers the core (standard library only).
