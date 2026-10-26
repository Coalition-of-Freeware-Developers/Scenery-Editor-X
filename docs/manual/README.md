# Scenery Editor X User Manual

The user manual for Scenery Editor X, the 3D scenery editor for X-Plane 12. Read it here on GitHub, or build the
website and PDF (below).

## Contents

**Part I · Getting Started**

1. [Welcome to Scenery Editor X](chapters/welcome.md)
2. [Installing Scenery Editor X](chapters/installing.md)
3. [First launch](chapters/first-launch.md)
4. [Tutorial: your first airport](chapters/tutorial-first-airport.md)

**Part II · The Workspace**

5. [A tour of the interface](chapters/interface.md)
6. [Navigating the viewport](chapters/viewport-navigation.md)
7. [Selecting, transforming and undo](chapters/selecting-and-editing.md)

**Part III · Building Scenery**

8. [Projects](chapters/projects.md)
9. [Airports](chapters/airports.md)
10. [The X-Plane library](chapters/library.md)
11. [Editing shapes and paths](chapters/node-editing.md)
12. [Drawing facades](chapters/facades.md)
13. [Making your own facades](chapters/facade-editor.md)
14. [Road networks](chapters/roads.md)
15. [Time, weather and the X-Plane 12 look](chapters/environment.md)
16. [Importing and exporting](chapters/import-export.md)

**Part IV · Reference**

17. [Keyboard and mouse reference](chapters/shortcuts.md)
18. [Menu reference](chapters/menus.md)
19. [Panel reference](chapters/panels.md)
20. [Command-line options](chapters/command-line.md)
21. [Project file formats](chapters/file-formats.md)
22. [Where Scenery Editor X keeps files](chapters/file-locations.md)
23. [Troubleshooting](chapters/troubleshooting.md)
24. [Glossary](chapters/glossary.md)

**Appendices**

- A. [Building from source](chapters/building-from-source.md)
- B. [Writing for this manual](chapters/writing-the-manual.md)

## Building the website and PDFs

Python 3.12+ and, for the PDFs, Chrome, Chromium or Microsoft Edge. No packages to install.

```bash
python docs/manual/build.py             # website + both PDFs in docs/manual/site
python docs/manual/build.py --no-book   # website + this manual's PDF
python docs/manual/build.py --no-pdf    # website only
```

The second PDF, `SceneryEditorX-Documentation.pdf`, is the complete documentation: this manual as Volume I, and as
Volume II the developer reference, the repository's own Markdown documents listed in [`book.json`](book.json).

Open `docs/manual/site/index.html`, or serve the folder for search:
`python -m http.server 8000 --directory docs/manual/site`.

[Writing for this manual](chapters/writing-the-manual.md) explains the layout, conventions and options.
