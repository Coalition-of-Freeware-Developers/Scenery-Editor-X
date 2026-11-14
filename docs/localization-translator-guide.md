# Translating Scenery Editor X

Thank you for helping translate Scenery Editor X. The editor's menus, panels and messages are translated by volunteers
on **Hosted Weblate**, a web site where you translate one string at a time with the English text, notes from the
developers and suggestions next to it. You don't need to build the editor or use git.

> The Weblate project is being set up. Until its link is here, translations can be sent as pull requests that edit
> `resources/locale/<language>.po` (see "Without Weblate" below), or as an issue with the file attached.

## How it works

- English is the source language. Developers mark the English text in the code; every pull request into `dev`
  refreshes the template `resources/locale/sedx.pot` automatically.
- Each language has one file, `resources/locale/<tag>.po` (`de.po`, `ja.po`, `pt_BR.po`, `zh_Hans.po`). Weblate keeps
  them in step with the template and sends your translations back as pull requests.
- A string that isn't translated (or is marked "needs editing") shows in English, so a partial translation is fine.
  Preferences lists a language once it is 40% translated; below that, tick "Show incomplete translations".
- German and Japanese were drafted by machine translation to test the system: please review them.

## Rules for translators

- **Placeholders.** `{}`, `{0}`, `{1:.2f}` are replaced by values (a number, a name). Keep each one; you may reorder
  numbered ones (`{1} in {0}`). Weblate warns when one is missing. `%s` and `%d` work the same way. `{{` and `}}` are
  shown as `{` and `}`.
- **Plurals.** Strings with a count have one form per plural category of your language (Weblate shows them: one, few,
  many, other...). Fill every form.
- **Notes.** Read the developer note above a string ("A panel title, shown in capitals", "{0} is a number of
  objects"). Panel titles and section headers are shown in capitals: write them in capitals if your script has them.
- **Keep it short.** Many labels sit in narrow columns and buttons. Prefer the shorter of two good translations.
- **Don't translate** file formats and X-Plane terms of art: DSF, OBJ, apt.dat, file extensions (`.fac`, `.for`,
  `.pol`), dataref names, ICAO codes, X-Plane, WED. Follow the glossary on Weblate for scenery terms (facade, forest,
  ramp start, draped polygon, exclusion zone...).
- **Spacing and punctuation.** Keep spaces at the start or end of a string, line breaks, and a final `...` on menu
  items that open a window.
- **License.** Translations are contributed under the project's MIT license.

## Testing your translation

1. Download your language's file from Weblate (Files › Download translation) or take it from `resources/locale`.
2. Put it in the `locale` folder of the editor's settings folder:
   - Windows: `%APPDATA%\SceneryEditorX\locale\`
   - macOS: `~/Library/Application Support/SceneryEditorX/locale/`
   - Linux: `~/.config/SceneryEditorX/locale/`

   A file there replaces the editor's own file of the same language.
3. In the editor, choose your language in **Edit › Preferences › Interface**, edit the file, and use
   **Help › Reload Translations** to see the changes without restarting.
4. Start the editor with `--i18n-log-missing` to get a list of the strings you saw that had no translation
   (`missing-translations-<tag>.po` in the editor's `logs` folder).

The **pseudo-locale** (Preferences › Interface, with "Show incomplete translations") shows every translatable string
as `[!! Šåvé ··!!]`. It is how developers find text that isn't translatable yet: if you find English text in a
translated editor, tell us where.

## Without Weblate

The `.po` files are plain text (GNU gettext). Edit `msgstr` lines with any text editor or Poedit, keep `msgid` lines as
they are, and check the file before sending it:

```bash
python -I tools/i18n/sedx_i18n.py check resources/locale/de.po
```

## A new language

Ask on Weblate (or in an issue). Languages written right to left (Arabic, Hebrew, Persian) and complex scripts (Thai,
Devanagari) can't be shown correctly yet; Chinese, Japanese and Korean use the operating system's own fonts.
