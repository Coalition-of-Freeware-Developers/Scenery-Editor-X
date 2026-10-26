"""
The complete documentation: the user manual (Volume I) and the developer reference (Volume II, the Markdown documents
kept in the repository) printed as one book, docs/manual/site/SceneryEditorX-Documentation.pdf.

build.py calls build() after it has converted the manual. book.json lists the reference's parts and documents, each
with the title it has in the book. The documents are reproduced as they are: their own headings, tables and code,
their links pointed at each other's chapters, and anything else in the repository linked on GitHub.
The covers and volume dividers are the charts in art/ (tools/make_art.py), with the titles set over them.
"""

from __future__ import annotations

import json
import posixpath
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import quote, unquote

import build
import md2html
from build import HERE, REPO, ROMAN, Chapter, Manual, Part, esc

EXTERNAL = re.compile(r"^[a-z][a-z0-9+.-]*:", re.I)


@dataclass
class Volume:
    number: int
    title: str
    description: str
    art: str
    parts: list[Part]

    @property
    def numeral(self) -> str:
        return ROMAN[self.number - 1]

    @property
    def anchor(self) -> str:
        return f"volume-{self.number}"

    @property
    def chapters(self) -> list[Chapter]:
        return [c for p in self.parts for c in p.chapters]


@dataclass
class Book:
    config: dict
    manual: Manual
    volumes: list[Volume]
    reference: list[Chapter]  # Volume II's chapters
    from_file: dict[str, bool]  # Chapter slug -> the document's first # heading is its title (and isn't printed)
    bodies: dict[str, str]  # Chapter slug -> the HTML printed in the book
    notes: list[str]


# --------------------------------------------------------------------------------------------------------------------
# Loading

def load(manual: Manual) -> Book:
    config = json.loads((HERE / "book.json").read_text(encoding="utf-8"))
    volumes: list[Volume] = []
    reference: list[Chapter] = []
    from_file: dict[str, bool] = {}
    number = len(manual.parts)  # Parts are numbered on from the manual's, and so are their numerals and chapters
    numeral = sum(1 for p in manual.parts if not p.appendix)
    chapter_number = sum(1 for c in manual.chapters if not c.part.appendix)
    for v, volume_config in enumerate(config["volumes"], start=1):
        if volume_config.get("manual"):
            parts = manual.parts
        else:
            parts = []
            for part_config in volume_config["parts"]:
                number += 1
                numeral += 1
                part = Part(number, part_config["title"], part_config.get("description", ""), False, numeral=ROMAN[numeral - 1])
                for doc in part_config["documents"]:
                    chapter_number += 1
                    path = doc["path"]
                    source = REPO / path
                    if not source.is_file():
                        raise SystemExit(f"error: {path} is listed in book.json but doesn't exist")
                    stem = re.sub(r"^docs/", "", path[:-3] if path.lower().endswith(".md") else path)
                    slug = "dev-" + re.sub(r"[^a-z0-9]+", "-", stem.lower()).strip("-")
                    chapter = Chapter(slug, source, part, str(chapter_number), origin=path)
                    chapter.title = doc["title"]
                    chapter.title_html = esc(doc["title"])
                    from_file[slug] = doc.get("titleFromFile", True)
                    part.chapters.append(chapter)
                    reference.append(chapter)
                parts.append(part)
        volumes.append(Volume(v, volume_config["title"], volume_config.get("description", ""), volume_config.get("art", ""), parts))
    return Book(config, manual, volumes, reference, from_file, {}, [])


# --------------------------------------------------------------------------------------------------------------------
# Converting the reference

def count_h1(markdown: str) -> int:
    """Level 1 headings outside code blocks."""
    count, fence = 0, None
    for line in markdown.replace("\r\n", "\n").split("\n"):
        if fence:
            if line.strip().startswith(fence):
                fence = None
            continue
        m = md2html._FENCE_RE.match(line)
        if m and m.group(2):
            fence = m.group(2)
            continue
        h = md2html._HEADING_RE.match(line)
        if h and len(h.group(1)) == 1:
            count += 1
    return count


def repo_url(book: Book, path: str, fragment: str = "") -> str:
    cfg = book.manual.config
    base = cfg.get("repository", "").rstrip("/")
    kind = "tree" if (REPO / path).is_dir() else "blob"
    return f"{base}/{kind}/{cfg.get('branch', 'dev')}/{quote(path)}" + (f"#{fragment}" if fragment else "")


def convert_reference(book: Book, out: Path) -> None:
    # Every chapter of the book by its repository path, for links between documents
    index = {c.origin.lower(): c for c in book.reference}
    index |= {c.source.relative_to(REPO).as_posix().lower(): c for c in book.manual.chapters}

    for chapter in book.reference:
        base = posixpath.dirname(chapter.origin)

        def link(href: str, chapter=chapter, base=base) -> str:
            if EXTERNAL.match(href):
                return href
            path, fragment = build.split_href(href)
            if not path:
                return f"#{chapter.anchor}--{fragment}" if fragment else f"#{chapter.anchor}"
            resolved = posixpath.normpath(posixpath.join(base, unquote(path)))
            target = index.get(resolved.lower())
            if target:
                return f"#{target.anchor}--{fragment}" if fragment else f"#{target.anchor}"
            if not resolved.startswith("..") and (REPO / resolved).exists():
                return repo_url(book, resolved, fragment)
            book.notes.append(f"{chapter.origin}: link to {href}, which isn't in the repository")
            return repo_url(book, resolved, fragment)

        def image(src: str, chapter=chapter, base=base) -> str:
            if EXTERNAL.match(src):
                book.notes.append(f"{chapter.origin}: left out the remote image {src}")
                return "missing:"
            resolved = posixpath.normpath(posixpath.join(base, unquote(src)))
            if (REPO / resolved).is_file():
                target = out / "repo" / resolved
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(REPO / resolved, target)
                return "repo/" + quote(resolved)
            book.notes.append(f"{chapter.origin}: left out the missing image {src}")
            return "missing:"

        markdown = chapter.source.read_text(encoding="utf-8", errors="replace")
        from_file = book.from_file[chapter.slug]
        h1s = count_h1(markdown)
        # A document with several # headings: they become its sections, and everything below moves down a level
        shift = 1 if h1s > 1 or (h1s and not from_file) else 0
        ctx = md2html.Context(link=link, image=image, id_prefix=f"{chapter.anchor}--", consume_title=from_file, heading_shift=shift)
        body = md2html.convert(markdown, ctx)
        body = re.sub(r"<(/?)h1\b", r"<\1h2", body)  # Raw HTML headings too: only the book's own <h1>s make bookmarks
        body = re.sub(r'<figure><img src="missing:"[^>]*>(?:<figcaption>.*?</figcaption>)?</figure>|<img src="missing:"[^>]*>', "", body, flags=re.S)
        remote = re.findall(r"<img\b[^>]*\bsrc=[\"']?https?://[^>]*>", body)
        if remote:
            book.notes.append(f"{chapter.origin}: left out {len(remote)} remote image(s)")
            body = re.sub(r"<img\b[^>]*\bsrc=[\"']?https?://[^>]*>", "", body)
        chapter.print_html = body
        chapter.headings = ctx.headings
        chapter.site_ids = ctx.used_ids
        chapter.print_links = ctx.links


def finish_links(book: Book) -> None:
    """The bodies the book prints. Links to sections that don't exist become plain text; links from the manual to a
    document that's in Volume II go to its chapter instead of GitHub."""
    ids = {"cover"} | {v.anchor for v in book.volumes}
    ids |= {p.anchor for v in book.volumes for p in v.parts}
    ids |= {c.anchor for v in book.volumes for c in v.chapters}
    ids |= {f"{c.anchor}--{i}" for c in book.manual.chapters for i in c.site_ids}
    ids |= {i for c in book.reference for i in c.site_ids}

    cfg = book.manual.config
    blob = re.escape(f'{cfg.get("repository", "").rstrip("/")}/blob/{cfg.get("branch", "dev")}/')
    by_path = {c.origin.lower(): c for c in book.reference}

    def to_reference(m: re.Match) -> str:
        target = by_path.get(unquote(m.group(1)).lower())
        if not target:
            return m.group(0)
        return f'<a href="#{target.anchor}--{m.group(2)}">' if m.group(2) else f'<a href="#{target.anchor}">'

    for chapter in book.manual.chapters:
        book.bodies[chapter.slug] = re.sub(r'<a href="' + blob + r'([^"#]+)(?:#([^"]*))?"[^>]*>', to_reference, chapter.print_html)

    for chapter in book.reference:
        broken = 0

        def unlink(m: re.Match) -> str:
            nonlocal broken
            if m.group(1) in ids:
                return m.group(0)
            broken += 1
            return f'<span class="dead-link">{m.group(2)}</span>'

        book.bodies[chapter.slug] = re.sub(r'<a href="#([^"]+)"[^>]*>(.*?)</a>', unlink, chapter.print_html, flags=re.S)
        if broken:
            book.notes.append(f"{chapter.origin}: {broken} link(s) to sections that don't exist, printed as text")


# --------------------------------------------------------------------------------------------------------------------
# Pages

def art(name: str) -> str:
    """One of the art/ charts, inline (so its labels use the book's fonts), its ids made unique to it."""
    svg = (HERE / "art" / f"{name}.svg").read_text(encoding="utf-8")
    for ident in set(re.findall(r'\bid="([^"]+)"', svg)):
        svg = svg.replace(f'id="{ident}"', f'id="{name}-{ident}"').replace(f"url(#{ident})", f"url(#{name}-{ident})")
    return svg.replace('width="210mm" height="297mm"', 'class="sheet-art" preserveAspectRatio="xMidYMid slice"', 1)


def span(first: str, last: str) -> str:
    return first if first == last else f"{first}–{last}"


def volume_meta(book: Book, volume: Volume, pages: dict[str, int]) -> str:
    numbered = [p for p in volume.parts if not p.appendix]
    appendices = [c for p in volume.parts if p.appendix for c in p.chapters]
    chapters = [c for p in numbered for c in p.chapters]
    bits = [f"Parts {span(numbered[0].roman, numbered[-1].roman)}" if len(numbered) > 1 else f"Part {numbered[0].roman}"]
    if appendices:
        bits[0] += f" and appendices {span(appendices[0].label, appendices[-1].label)}"
    bits.append(f"Chapters {span(chapters[0].label, chapters[-1].label)}")
    k = book.volumes.index(volume)
    first = pages.get(volume.anchor)
    last = pages.get(book.volumes[k + 1].anchor, 0) - 1 if k + 1 < len(book.volumes) else pages.get("@pages", 0) - 1
    if first and last > first:
        bits.append(f"Pages {first}–{last}")
    return "  ·  ".join(bits)


def cover(book: Book) -> str:
    cfg, manual = book.manual.config, book.manual
    volumes = "".join(f"<li><span>Volume {v.numeral}</span>{esc(v.title)}</li>" for v in book.volumes)
    return f"""<section class="sheet sheet-cover" id="cover">
{art("cover")}
<div class="sheet-text">
  <p class="sheet-eyebrow">{esc(cfg.get("organization", ""))}</p>
  <h1>{esc(cfg["title"])}</h1>
  <p class="sheet-subtitle">{esc(book.config["subtitle"])}</p>
  <div class="sheet-rule"></div>
  <ol class="sheet-volumes">{volumes}</ol>
  <p class="sheet-meta">Editor version {esc(manual.version)}  ·  {esc(manual.date)}</p>
</div>
</section>"""


def colophon(book: Book) -> str:
    cfg, manual = book.manual.config, book.manual
    lines = "".join(f"<p>{esc(line)}</p>" for line in book.config.get("colophon", []))
    return f"""<section class="colophon">
  <h2>About this book</h2>
  {lines}
  <p class="colophon-note">The cover and the volume pages are charts of the tutorial's field in Kansas, 38°30′N 98°30′W, drawn on a
  Lambert conformal conic projection; the one warm mark is its runway, 17/35. The book is set in Open Sans.</p>
  <p class="colophon-meta">Written for Scenery Editor X {esc(manual.version)} · Published {esc(manual.date)}<br>{esc(cfg.get("copyright", ""))}</p>
</section>"""


def contents(book: Book, pages: dict[str, int]) -> str:
    volumes = "".join(
        f'<li class="toc-volume">{build.toc_line(v.anchor, f"Volume {v.numeral}", esc(v.title), pages)}'
        f'<ol>{build.toc_parts(v.parts, pages)}</ol></li>' for v in book.volumes)
    return f'<nav class="print-toc book-toc"><h2>Contents</h2><ol>{volumes}</ol></nav>'


def volume_sheet(book: Book, volume: Volume, pages: dict[str, int]) -> str:
    return f"""<section class="sheet sheet-volume" id="{volume.anchor}">
{art(volume.art) if volume.art else ""}
<div class="sheet-text">
  <p class="sheet-eyebrow">Volume {volume.numeral}</p>
  <h1>{esc(volume.title)}</h1>
  <div class="sheet-rule"></div>
  <p class="sheet-description">{esc(volume.description)}</p>
  <p class="sheet-meta">{esc(volume_meta(book, volume, pages))}</p>
</div>
</section>"""


def back(book: Book) -> str:
    cfg = book.manual.config
    repository = re.sub(r"^https?://", "", cfg.get("repository", ""))
    return f"""<section class="sheet sheet-back">
{art("back")}
<div class="sheet-text">
  <p class="sheet-eyebrow">{esc(cfg["title"])}</p>
  <p class="sheet-tagline">{esc(cfg.get("tagline", ""))}</p>
  <div class="sheet-rule"></div>
  <p class="sheet-meta">{esc(repository)}<br>{esc(cfg.get("copyright", ""))}</p>
</div>
</section>"""


def render(book: Book, pages: dict[str, int]) -> tuple[str, build.Targets]:
    cfg = book.manual.config
    targets: build.Targets = [("cover", cfg["title"], 0, [])]
    body, footers = [], []
    for volume in book.volumes:
        targets.append((volume.anchor, volume.title, 0, []))
        body.append(volume_sheet(book, volume, pages))
        for part in volume.parts:
            targets.append((part.anchor, part.title, 1, []))
            targets += [build.chapter_target(c, 2) for c in part.chapters]
            body.append(build.part_pages(part, pages, lambda c: book.bodies[c.slug]))
            footers.append((part.anchor, f"{volume.title}  ·  {part.title}"))

    title = f'{cfg["title"]}: {book.config["subtitle"]}'
    return f"""<!doctype html>
<html lang="en" data-theme="light">
<head>
<meta charset="utf-8">
<title>{esc(title)}</title>
<meta name="description" content="{esc(book.config.get("tagline", ""))}">
<link rel="stylesheet" href="assets/manual.css">
<link rel="stylesheet" href="assets/print.css">
<style>
{build.footer_css(footers)}
</style>
</head>
<body class="print-page book">
{cover(book)}
{colophon(book)}
{contents(book, pages)}
{"".join(body)}
{back(book)}
{build.PRINT_SCRIPT}
</body>
</html>
""", targets


# --------------------------------------------------------------------------------------------------------------------

def make(manual: Manual, out: Path, browser: str) -> Path | None:
    """Prints the book into out (beside the site, whose assets it shares). Returns the PDF, or None if printing failed."""
    book = load(manual)
    convert_reference(book, out)
    finish_links(book)
    pdf = out / book.config["pdfName"]
    print(f"printing the complete documentation ({len(book.reference)} reference documents)")
    if not build.print_numbered(browser, out / "book.html", pdf, lambda pages: render(book, pages)):
        return None
    if book.notes:
        print(f"note: the developer reference prints as written; {len(book.notes)} things were left out or unlinked:")
        for note in book.notes:
            print(f"  {note}")
    return pdf
