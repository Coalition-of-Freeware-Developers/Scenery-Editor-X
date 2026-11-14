# Localization system

Scenery Editor X's user interface can be shown in other languages. English is the source language: the English text
is written in the code, marked for translation, extracted into a template, and translated into one catalog per
language. This page is for developers; translators read [the translator guide](localization-translator-guide.md).

| Piece | Where |
|---|---|
| Translation functions (`Tr`, `TrL`, `TrF`, `TrN`, `TrNF`, `Ctx`, `TrNoop`, `Text`) | `source/SceneryEditorX/i18n/tr.h`, `renderer/ui/i18n_imgui.h` |
| Translation core (PO parser, plural rules, language table, formatting, pseudo-locale), standard library only | `source/SceneryEditorX/i18n/` |
| Language choice, catalog discovery, CJK font lookup | `source/SceneryEditorX/core/localization.h` |
| Template (generated) and catalogs | `resources/locale/sedx.pot`, `resources/locale/<tag>.po` |
| Extraction, checks, lint | `tools/i18n/sedx_i18n.py` |
| Tests | `source/Tests/i18n_tests` (`I18nTests`), `tools/i18n/test_sedx_i18n.py` |
| Pull request automation | `.github/workflows/localization.yml` |
| Design and plan | `plan/feature-localization-1.md`, `reports/Localizing Scenery Editor X.md` |

## Marking text

Every string the user reads goes through one of these. Each takes a string literal; the literal's key is hashed at
compile time, so a lookup is one hash-table probe.

```cpp
#include <SceneryEditorX/i18n/tr.h>
#include <SceneryEditorX/renderer/ui/i18n_imgui.h>   // TrL

ImGui::TextUnformatted(Tr("No airport loaded"));                 // Display text
if (ImGui::Button(TrL("Export##pack")))                          // A label that is an ImGui ID
status = TrF("Exported {} tiles to {}", count, folder);          // Formatted (std::format fields)
ImGui::TextUnformatted(TrNF("{} file", "{} files", n).c_str());  // Plural, formatted with n as {0}
ImGui::MenuItem(TrL(Ctx("verb", "Open")));                       // A context for a short, ambiguous word
static constexpr Msg KINDS[] = { TrNoop("Runway"), TrNoop("Taxiway") };   // A table translated later...
ImGui::TextUnformatted(Tr(KINDS[i]));                            // ...here
```

| Use | For |
|---|---|
| `TrL` | Anything whose label is also its ImGui ID: windows, menus, menu items, buttons, checkboxes, tabs, tree nodes, selectables, popups, combo / slider / input labels |
| `Tr` | Display-only text: `Text*`, tooltips, hints, table column headers, combo items, status text, `AddText` |
| `TrF`, `TrNF` | Text with values in it |
| `TrN` | A plural form without formatting |
| `TrNoop` | Marks a literal in a table; translate it where it is shown with `Tr` / `TrL` |
| `Ctx("context", "text")` | Short words that translate differently by use ("Open" the verb, "Open" the state) |

**Helpers translate by themselves.** A function that takes a `Text` parameter translates the literals it is given and
shows run-time strings as they are. The `EditorUI` helpers (`BeginPanel`, `SectionHeader`, `PropertyLabel`,
`ItemTooltip`, `EmptyState`, `StripButton`, `IconLabel`) all do, so `EditorUI::PropertyLabel("Heading")` needs nothing
more. To make your own helper translate, declare the parameter as `Text`; the extractor finds such declarations by
itself. A `?:` of two literals is a `const char *` and is not translated: write `cond ? Text("A") : Text("B")`
(`lint` reports the plain form).

**IDs stay English.** `TrL("Reset Layout")` returns `"Layout zurücksetzen###Reset Layout"`. Since Dear ImGui 1.92.6,
`"a###b"` hashes like `"b"` and only `"b"` goes into `editor.ini`, so translated widgets keep their IDs, and the docking
layout, tree / table state and window positions don't change with the language. A popup opened by name keeps the
English name (`OpenPopup("Delete?")` finds `BeginPopupModal(TrL("Delete?"))`). In a debug run, a translated label whose
ID differs from the English one is logged as an error.

**Explain unclear strings** with a comment on the line above; the extractor passes it on to translators:

```cpp
// TRANSLATORS: {0} is a number of objects, {1} an airport's ICAO code
status = TrF("Placed {0} objects at {1}", count, icao);
```

A `// TRANSLATORS:` comment above a declaration with `Text` parameters applies to every call of it (that is how every
`BeginPanel` title tells translators it is shown in capitals).

## Rules

- **Whole sentences.** Never build a sentence from pieces: `"Undo " + label` can't be translated into languages that
  put the verb last. Use one message with fields: `TrF("Undo {}", label)`.
- **Fields, not concatenation, not printf.** `TrF` uses `std::format`'s replacement fields (`{}`, `{0}`, `{:.2f}`);
  translators may reorder numbered fields. A translation that doesn't format falls back to English for that call and
  is logged once. Arguments are numbers, bools, chars and strings (convert paths with `VFS::ToUtf8`).
- **Plurals with `TrN` / `TrNF`**, never `n == 1 ? "" : "s"`: Russian has three forms, Arabic six, Japanese one.
- **Translate when drawing.** Store a `Msg` (and the values) and translate when the text is shown, so a language change
  reaches it too; that goes for command labels, status messages and validation problems made on worker threads.
- **What is not translated:** log messages (bug reports stay readable), CLI flags, file contents and format keywords,
  settings keys, serialization tokens (`GetItemKindName` writes `.edX` files: add a separate display-name function),
  and user data (airport names, ICAO codes, paths, art asset names, datarefs).
- **Size widgets from their text** (`ImGui::CalcTextSize`, multiples of `ImGui::GetFontSize()`), not fixed pixel
  widths sized for English: German is about a third longer.
- **Numbers keep `.` decimals.** The process locale is never changed (`setlocale` beyond `LC_TIME`,
  `std::locale::global`): the apt.dat, DSF, WED and facade writers depend on the C locale.

## Tools

```bash
python -I tools/i18n/sedx_i18n.py extract         # resources/locale/sedx.pot from the source (--check: compare only)
python -I tools/i18n/sedx_i18n.py update          # catalogs in line with the template
python -I tools/i18n/sedx_i18n.py new pt_BR       # a catalog for a language of the table in i18n/languages.cpp
python -I tools/i18n/sedx_i18n.py check           # validate catalogs (placeholders, plurals, ## IDs, spacing, length)
python -I tools/i18n/sedx_i18n.py lint            # untranslated ImGui literals not in tools/i18n/lint_baseline.txt
python -I tools/i18n/sedx_i18n.py stats --markdown
python -I -m unittest discover -s tools/i18n -p "test_*.py"
```

`lint` knows the Dear ImGui calls that show text. `tools/i18n/lint_baseline.txt` lists the sites not converted yet;
new ones fail. After converting some, run `lint --update-baseline` so the list only shrinks. `lint` also fails on a
`cond ? "A" : "B"` passed to a `Text` parameter (no baseline: that text would silently stay English).

### On pull requests

The **Localization** workflow runs on every pull request into `dev` that touches `source/`, `resources/locale/` or
`tools/i18n/`. It runs the tool's tests on Windows, macOS and Linux, then extracts the strings, updates the catalogs,
lists the text the pull request adds and removes in the run summary, runs `check` and `lint`, and commits the refreshed
`resources/locale` files to the pull request's branch ("Update the translation template and catalogs"). Pull before
pushing again to that branch. For a pull request from a fork, which the workflow can't push to, it fails with the
commands to run and uploads the changes as a `localization-patch` artifact. The unit-tests workflow builds and runs
`I18nTests` on the three platforms.

## Languages

`settings.cfg` holds the choice: `ui.language = "system"` (default: the operating system's preferred languages, read
through `SDL_GetPreferredLocales`) or a catalog tag. `--language <tag>` overrides it for one run, and
**Edit › Preferences › Interface** (also **Help › Language...**) changes it at once: the switch happens between two
frames, so nothing is drawn half in one language. Languages less than 40% translated are listed only with **Show
incomplete translations**, which also offers the pseudo-locale.

Lookup falls back per string: the language's catalog, then its base language's (`pt_BR` → `pt`), then English.

A language chosen in Preferences is saved at once. If its catalog later goes away (a translator removes their own
`user:/locale` file, say), the editor starts in English and logs "Language '...' isn't available" at each start until
another language is chosen; Preferences shows the saved tag until then. A catalog file over 32 MB isn't read.

### Adding a language

1. If it isn't in `source/SceneryEditorX/i18n/languages.cpp`, add a row (tag in Weblate's form, names, script, CLDR
   plural rule; non-ASCII as `\u` escapes). `I18nTests` checks every row's plural rule.
2. `python -I tools/i18n/sedx_i18n.py new <tag>` (or let Weblate create it).
3. Right-to-left languages (Arabic, Hebrew, Persian) and complex scripts (Thai, Devanagari) need shaping the UI
   doesn't have yet (plan phase 7); they aren't offered.

### Fonts

The UI fonts (Open Sans) cover Latin, Greek and Cyrillic. For Chinese, Japanese and Korean the editor merges the
operating system's regional font into all five UI fonts when that language is chosen: Windows `msyh.ttc`, `msjh.ttc`,
`YuGothM.ttc`, `malgun.ttf`, then Noto Sans CJK / Source Han Sans installed for everyone or just this user; macOS PingFang / Hiragino / Apple SD Gothic Neo; Linux fontconfig's choice or Noto Sans
CJK. Each candidate's character map is checked against a sample of the language's characters
(`i18n/font_coverage.h`) and the first that has them all is used, so a font that only looks right (a Chinese font for
Japanese, whatever fontconfig answered) is passed over; the log names each one skipped. `ui.language_font` (and
`ui.language_font_face` for a `.ttc`) in `settings.cfg` picks another file, used whatever it covers. A merged font
stays until the editor closes, so after switching between two CJK languages, shared Han characters keep the first one's
shapes until a restart. Without a font the text shows as `?` and the log says what to install.

## Testing

- `--language qps-ploc` (or the pseudo-locale in Preferences) shows every translated string as `[!! Šåvé ··!!]`:
  plain English is text that isn't marked, a missing `!!]` is text cut off, and brackets in the middle of a sentence
  are fragments glued together.
- `--i18n-log-missing` writes the strings that had no translation in the active language to
  `logs:/missing-translations-<tag>.po` on exit, ready to paste into a catalog.
- A translator's copy in `user:/locale/<tag>.po` (the `locale` folder in the editor's settings folder) replaces the
  bundled one; **Help › Reload Translations** reads the files again without a restart.
- `I18nTests` builds with the standard library only; see `AGENTS.md` for the g++ command.
