# Writing for this manual

This manual lives in the repository, next to the code it describes, so a change to the editor and the change to its documentation can go in the same pull request. This appendix explains how it's organized, the conventions it follows, and how to build the website and the PDF.

## Where things are

| Path | What it is |
|---|---|
| `docs/manual/manual.json` | The book: its title, parts, the order of the chapters, the quick links on the overview page |
| `docs/manual/chapters/*.md` | One Markdown file per chapter |
| `docs/manual/images/` | Screenshots (`.png`) and diagrams (`.svg`) |
| `docs/manual/theme/` | The website's and the PDF's styles (`manual.css`, `print.css`) and script (`manual.js`) |
| `docs/manual/build.py` | Builds the website and the PDF |
| `docs/manual/tools/md2html.py` | The Markdown converter the build uses |
| `docs/manual/site/` | The built website and PDF (not committed) |

## Building

You need Python 3.12 or newer, and Chrome, Chromium or Microsoft Edge for the PDF. Nothing else: the build uses only Python's standard library.

```bash
python docs/manual/build.py
```

This writes the website to `docs/manual/site/` (open `index.html`) and prints the PDF, `SceneryEditorX-Manual.pdf`, beside it. Options:

| Option | Effect |
|---|---|
| `--no-pdf` | Skip the PDF (much faster while writing) |
| `--browser <path>` | The browser that prints the PDF, when it isn't found on its own |
| `--strict` | Exit with an error on a broken link or missing image (for CI) |
| `--date "October 2026"` | The publication date printed in the manual |

The build checks every link between chapters, every `#section` link, and every image, and warns about any that don't resolve.

To preview the website with search working, serve the folder:

```bash
python -m http.server 8000 --directory docs/manual/site
```

## Organization

The manual follows the [Diátaxis](https://diataxis.fr/) framework: each chapter is mainly one of four kinds, and keeps to it.

| Kind | Purpose | Here |
|---|---|---|
| **Tutorial** | A lesson: takes a newcomer through a complete task | *Tutorial: your first airport* |
| **How-to** | A recipe for one problem | Most of Part III, troubleshooting |
| **Reference** | Facts to look up | Part IV |
| **Explanation** | Background and reasons | *Welcome*, the *How … work* sections |

To add a chapter, write `chapters/<slug>.md`, add the slug to a part in `manual.json`, and add it to the contents in `docs/manual/README.md`.

## Writing style

- **Write for scenery authors**, not programmers. Explain every X-Plane term the first time it matters, or link the [glossary](glossary.md).
- **Describe what the editor does today.** Check behavior in the code or the editor, not from memory. Say plainly what isn't there yet.
- **Lead with what the reader can do**, then how. Short sentences, active voice, second person.
- **Name the interface exactly** as the editor shows it: menu paths in bold with › (**File › New Project…**), panel names in capitals (**PROPERTIES**), buttons and fields in bold.
- **One idea per paragraph**; use a table when you'd otherwise write a list of "X does Y".

## Markdown conventions

Chapters are GitHub-flavored Markdown and read well on GitHub too. The build understands:

| Write | For |
|---|---|
| `# Title` | The chapter title (the first line) |
| The first paragraph | The chapter's lead, shown larger, and its summary on the overview page |
| `## Section {#custom-id}` | A section with a fixed link id. Avoid it when you can: GitHub prints it as text. Without it, the id comes from the heading the way GitHub makes it (`## The project cache` is `#the-project-cache`), so links work in both places |
| `[text](other-chapter.md#section)` | A link to another chapter |
| `![Alt text](../images/file.png "Caption")` | A figure: an image alone in a paragraph, numbered, with the title as its caption |
| `<kbd>Ctrl+S</kbd>` | Keys, drawn as one key cap per key |
| `> [!NOTE]`, `[!TIP]`, `[!IMPORTANT]`, `[!WARNING]`, `[!CAUTION]` | Callouts |
| `<!-- class: steps -->` before a numbered list | Numbered steps with large step markers |
| `<!-- class: shortcuts -->` before a table | A shortcut table (the first column doesn't wrap) |
| `<!-- class: cards -->` before a bullet list | A grid of cards |

## Images

- **Screenshots** are PNGs of the editor at 100% display scale, with the default dark theme. Crop to what the text talks about, and keep the window size consistent (1600 × 900 for whole-window shots).
- **Diagrams** are hand-written SVGs in the blueprint style of the existing ones: a dark navy background, light lines, the accent blue `#1F8AD6`. They read well on both the light and the dark website and in print.
- Give every image alt text that describes what it shows, and a caption that says why it's there.
