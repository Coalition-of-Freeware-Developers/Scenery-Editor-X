#!/usr/bin/env python3
"""
Builds the Scenery Editor X user manual from the Markdown chapters in docs/manual/chapters:

- an HTML site (one page per chapter, a landing page, client-side search, light and dark themes)
- a print page with a cover, contents and part dividers, and from it a PDF (Chrome, Chromium or Edge, headless)

Standard library only. Run it from anywhere:

    python docs/manual/build.py                  # site + PDF in docs/manual/site
    python docs/manual/build.py --no-pdf         # site only
    python docs/manual/build.py --strict         # fail on broken links or missing images (CI)
    python docs/manual/build.py --browser PATH   # the browser that prints the PDF
"""

from __future__ import annotations

import argparse
import datetime as dt
import html
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE / "tools"))

import md2html  # noqa: E402

ROMAN = ["I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X"]
FONT_FILES = ["OpenSans-Regular.ttf", "OpenSans-Italic.ttf", "OpenSans-Medium.ttf", "OpenSans-SemiBold.ttf",
              "OpenSans-Bold.ttf", "OpenSans-ExtraBold.ttf", "LICENSE.txt"]


# --------------------------------------------------------------------------------------------------------------------
# Model

@dataclass
class Chapter:
    slug: str
    source: Path
    part: "Part"
    label: str  # "7" or "A"
    title: str = ""
    title_html: str = ""
    lead: str = ""
    site_html: str = ""
    print_html: str = ""
    headings: list[md2html.Heading] = field(default_factory=list)
    site_ids: set[str] = field(default_factory=set)
    site_links: list[str] = field(default_factory=list)
    print_links: list[str] = field(default_factory=list)
    images: list[str] = field(default_factory=list)

    @property
    def url(self) -> str:
        return f"{self.slug}.html"

    @property
    def anchor(self) -> str:
        return f"ch-{self.slug}"

    @property
    def kicker(self) -> str:
        return f"Appendix {self.label}" if self.part.appendix else f"Chapter {self.label}"


@dataclass
class Part:
    number: int
    title: str
    description: str
    appendix: bool
    chapters: list[Chapter] = field(default_factory=list)

    @property
    def kicker(self) -> str:
        return "Appendices" if self.appendix else f"Part {ROMAN[self.number - 1]}"

    @property
    def anchor(self) -> str:
        return f"part-{self.number}"


@dataclass
class Manual:
    config: dict
    version: str
    date: str
    parts: list[Part]

    @property
    def chapters(self) -> list[Chapter]:
        return [c for p in self.parts for c in p.chapters]

    def find(self, slug: str) -> Chapter | None:
        return next((c for c in self.chapters if c.slug == slug), None)


# --------------------------------------------------------------------------------------------------------------------
# Loading

def read_version(config: dict) -> str:
    header = REPO / config.get("versionHeader", "source/resource.h")
    try:
        m = re.search(r'#define\s+SEDX_VERSION_STRING\s+"([^"]+)"', header.read_text(encoding="utf-8", errors="replace"))
        if m:
            return m.group(1)
    except OSError:
        pass
    return "unknown"


def load_manual(date: str | None) -> Manual:
    config = json.loads((HERE / "manual.json").read_text(encoding="utf-8"))
    parts: list[Part] = []
    chapter_number, appendix_number = 0, 0
    for p, part_config in enumerate(config["parts"], start=1):
        part = Part(p, part_config["title"], part_config.get("description", ""), bool(part_config.get("appendix")))
        for slug in part_config["chapters"]:
            if part.appendix:
                label = chr(ord("A") + appendix_number)
                appendix_number += 1
            else:
                chapter_number += 1
                label = str(chapter_number)
            part.chapters.append(Chapter(slug, HERE / "chapters" / f"{slug}.md", part, label))
        parts.append(part)
    when = date or dt.date.today().strftime("%B %Y")
    return Manual(config, read_version(config), when, parts)


# --------------------------------------------------------------------------------------------------------------------
# Converting

def split_href(href: str) -> tuple[str, str]:
    path, _, fragment = href.partition("#")
    return path, fragment


def make_link_rewriter(manual: Manual, chapter: Chapter, mode: str):
    repo_url = manual.config.get("repository", "").rstrip("/")
    branch = manual.config.get("branch", "dev")

    def rewrite(href: str) -> str:
        if re.match(r"^[a-z][a-z0-9+.-]*:", href, re.I):
            return href  # External (https:, mailto:)
        path, fragment = split_href(href)
        if not path:
            return f"#{chapter.anchor}--{fragment}" if mode == "print" else f"#{fragment}"
        if path.endswith(".md") and "/" not in path:
            target = manual.find(path[:-3])
            if target:
                if mode == "print":
                    return f"#{target.anchor}--{fragment}" if fragment else f"#{target.anchor}"
                return f"{target.url}#{fragment}" if fragment else target.url
            return href  # Reported by the link check
        if path.startswith("../images/"):
            return path[3:] + (f"#{fragment}" if fragment else "")
        # Anything else in the repository: link to it on GitHub
        resolved = os.path.normpath(os.path.join("docs/manual/chapters", path)).replace("\\", "/")
        if repo_url and not resolved.startswith(".."):
            kind = "tree" if path.endswith("/") else "blob"
            return f"{repo_url}/{kind}/{branch}/{resolved}" + (f"#{fragment}" if fragment else "")
        return href

    return rewrite


def make_image_rewriter(chapter: Chapter):
    def rewrite(src: str) -> str:
        if src.startswith("../images/"):
            chapter.images.append(src[3:])
            return src[3:]
        return src

    return rewrite


def first_sentence(fragment: str) -> str:
    text = md2html.plain_text(fragment).strip()
    m = re.match(r"(.+?[.!?])(\s|$)", text)
    return m.group(1) if m else text


def convert_chapters(manual: Manual) -> None:
    for chapter in manual.chapters:
        if not chapter.source.exists():
            raise SystemExit(f"error: {chapter.source} is listed in manual.json but doesn't exist")
        markdown = chapter.source.read_text(encoding="utf-8")

        site = md2html.Context(link=make_link_rewriter(manual, chapter, "site"), image=make_image_rewriter(chapter))
        chapter.site_html = md2html.convert(markdown, site)
        chapter.title_html = site.title or chapter.slug
        chapter.title = md2html.plain_text(chapter.title_html)
        chapter.headings = site.headings
        chapter.site_ids = site.used_ids
        chapter.site_links = site.links
        lead = re.match(r"\s*<p>(.*?)</p>", chapter.site_html, re.S)
        chapter.lead = first_sentence(lead.group(1)) if lead else ""

        printed = md2html.Context(link=make_link_rewriter(manual, chapter, "print"), image=lambda s: s[3:] if s.startswith("../images/") else s,
                                  id_prefix=f"{chapter.anchor}--")
        chapter.print_html = md2html.convert(markdown, printed)
        chapter.print_links = printed.links


def check_links(manual: Manual, print_ids: set[str]) -> list[str]:
    problems = []
    pages = {c.url: c for c in manual.chapters}
    for chapter in manual.chapters:
        for href in chapter.site_links:
            if re.match(r"^[a-z][a-z0-9+.-]*:", href, re.I):
                continue
            path, fragment = split_href(href)
            if path.endswith(".md"):
                problems.append(f"{chapter.source.name}: link to a chapter that isn't in manual.json: {href}")
                continue
            if path.startswith("images/"):
                if not (HERE / path).exists():
                    problems.append(f"{chapter.source.name}: missing image link target {path}")
                continue
            target = pages.get(path) if path else chapter
            if target is None:
                problems.append(f"{chapter.source.name}: unknown link target {href}")
            elif fragment and fragment not in target.site_ids:
                problems.append(f"{chapter.source.name}: no section #{fragment} in {target.source.name}")
        for href in chapter.print_links:
            if href.startswith("#") and href[1:] not in print_ids:
                problems.append(f"{chapter.source.name}: (print) unknown anchor {href}")
        for image in chapter.images:
            if not (HERE / image).exists():
                problems.append(f"{chapter.source.name}: missing image {image}")
    return sorted(set(problems))


# --------------------------------------------------------------------------------------------------------------------
# HTML

def esc(text: str) -> str:
    return html.escape(text, quote=True)


THEME_BOOT = """<script>(function(){try{var t=localStorage.getItem('sedx-manual-theme');if(t)document.documentElement.setAttribute('data-theme',t);}catch(e){}})();</script>"""

ICONS = {
    "search": '<svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="11" cy="11" r="7"/><path d="m20 20-3.6-3.6"/></svg>',
    "theme": '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 3a9 9 0 1 0 9 9 7 7 0 0 1-9-9Z"/></svg>',
    "menu": '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 7h16M4 12h16M4 17h16"/></svg>',
    "pdf": '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8Z"/><path d="M14 3v5h5M12 11v6m-3-3 3 3 3-3"/></svg>',
    "github": '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 19c-4.3 1.4-4.3-2.5-6-3m12 5v-3.5c0-1 .1-1.4-.5-2 2.8-.3 5.5-1.4 5.5-6a4.6 4.6 0 0 0-1.3-3.2 4.2 4.2 0 0 0-.1-3.2s-1.1-.3-3.5 1.3a12.3 12.3 0 0 0-6.2 0C6.5 2.8 5.4 3.1 5.4 3.1a4.2 4.2 0 0 0-.1 3.2A4.6 4.6 0 0 0 4 9.5c0 4.6 2.7 5.7 5.5 6-.6.6-.6 1.2-.5 2V21"/></svg>',
    "arrow-left": '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M15 18l-6-6 6-6"/></svg>',
    "arrow-right": '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 18l6-6-6-6"/></svg>',
}


def head(title: str, description: str, extra: str = "") -> str:
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)}</title>
<meta name="description" content="{esc(description)}">
<meta name="color-scheme" content="light dark">
<link rel="icon" href="assets/icon.png">
<link rel="stylesheet" href="assets/manual.css">
{THEME_BOOT}
{extra}
</head>"""


def topbar(manual: Manual, pdf_available: bool) -> str:
    cfg = manual.config
    pdf = (f'<a class="icon-button" href="{esc(cfg["pdfName"])}" title="Download the PDF manual">{ICONS["pdf"]}<span>PDF</span></a>'
           if pdf_available else "")
    repo = cfg.get("repository", "")
    github = f'<a class="icon-button" href="{esc(repo)}" target="_blank" rel="noopener" title="Source on GitHub">{ICONS["github"]}</a>' if repo else ""
    return f"""<header class="topbar">
  <button class="icon-button menu-toggle" type="button" aria-label="Show the contents" data-menu-toggle>{ICONS["menu"]}</button>
  <a class="brand" href="index.html"><img src="assets/icon.png" alt=""><span class="brand-name">{esc(cfg["title"])}</span><span class="brand-sub">{esc(cfg["subtitle"])}</span></a>
  <span class="version-badge" title="Editor version this manual describes">v{esc(manual.version)}</span>
  <div class="search" role="search">
    {ICONS["search"]}
    <input type="search" placeholder="Search the manual" aria-label="Search the manual" autocomplete="off" spellcheck="false" data-search>
    <kbd class="search-hint">/</kbd>
    <div class="search-results" data-search-results hidden></div>
  </div>
  <div class="topbar-actions">
    {pdf}
    {github}
    <button class="icon-button" type="button" aria-label="Switch between light and dark" title="Light / dark" data-theme-toggle>{ICONS["theme"]}</button>
  </div>
</header>"""


def sidebar(manual: Manual, current: Chapter | None) -> str:
    out = ['<nav class="sidebar" aria-label="Contents" data-sidebar><div class="sidebar-inner">',
           f'<a class="sidebar-home{" current" if current is None else ""}" href="index.html">Overview</a>']
    for part in manual.parts:
        out.append(f'<div class="nav-part"><p class="nav-part-title"><span>{esc(part.kicker)}</span>{esc(part.title)}</p><ol>')
        for chapter in part.chapters:
            is_current = chapter is current
            li_attrs = ' class="current"' if is_current else ""
            a_attrs = ' aria-current="page"' if is_current else ""
            out.append(f'<li{li_attrs}><a href="{chapter.url}"{a_attrs}>'
                       f'<span class="nav-num">{esc(chapter.label)}</span>{chapter.title_html}</a>')
            if is_current:
                sections = [h for h in chapter.headings if h.level == 2]
                if sections:
                    out.append('<ul class="nav-sections">')
                    out += [f'<li><a href="#{h.id}">{h.html}</a></li>' for h in sections]
                    out.append("</ul>")
            out.append("</li>")
        out.append("</ol></div>")
    out.append("</div></nav>")
    return "".join(out)


def page_toc(chapter: Chapter) -> str:
    items = [h for h in chapter.headings if h.level in (2, 3)]
    if len(items) < 2:
        return '<aside class="page-toc"></aside>'
    out = ['<aside class="page-toc" aria-label="On this page"><p class="page-toc-title">On this page</p><ul>']
    out += [f'<li class="toc-l{h.level}"><a href="#{h.id}" data-toc-link>{h.html}</a></li>' for h in items]
    out.append("</ul></aside>")
    return "".join(out)


def pager(manual: Manual, chapter: Chapter) -> str:
    chapters = manual.chapters
    i = chapters.index(chapter)
    prev_link = next_link = "<span></span>"
    if i > 0:
        p = chapters[i - 1]
        prev_link = f'<a class="pager-prev" href="{p.url}">{ICONS["arrow-left"]}<span><small>Previous</small>{p.title_html}</span></a>'
    if i + 1 < len(chapters):
        n = chapters[i + 1]
        next_link = f'<a class="pager-next" href="{n.url}"><span><small>Next</small>{n.title_html}</span>{ICONS["arrow-right"]}</a>'
    return f'<nav class="pager" aria-label="Previous and next chapter">{prev_link}{next_link}</nav>'


def footer(manual: Manual) -> str:
    cfg = manual.config
    return (f'<footer class="site-footer"><p>{esc(cfg["title"])} {esc(cfg["subtitle"])} · editor version {esc(manual.version)} · {esc(manual.date)}</p>'
            f'<p>{esc(cfg.get("copyright", ""))}</p></footer>')


def scripts() -> str:
    return '<script src="assets/search-index.js"></script>\n<script src="assets/manual.js"></script>'


def chapter_page(manual: Manual, chapter: Chapter, pdf_available: bool) -> str:
    cfg = manual.config
    source = chapter.source.relative_to(REPO).as_posix()
    repo = cfg.get("repository", "").rstrip("/")
    edit = (f'<a class="edit-link" href="{esc(repo)}/edit/{esc(cfg.get("branch", "dev"))}/{esc(source)}" target="_blank" rel="noopener">'
            f'Suggest a change to this page</a>') if repo else ""
    return f"""{head(f"{chapter.title} · {cfg['title']} {cfg['subtitle']}", chapter.lead)}
<body class="chapter-page">
{topbar(manual, pdf_available)}
<div class="layout">
{sidebar(manual, chapter)}
<main class="content" id="main">
  <article class="chapter">
    <header class="chapter-header">
      <p class="kicker"><span>{esc(chapter.part.kicker)}</span><span class="dot">·</span><span>{esc(chapter.kicker)}</span></p>
      <h1>{chapter.title_html}</h1>
    </header>
    <div class="chapter-body">
{chapter.site_html}
    </div>
    <div class="chapter-meta">{edit}</div>
  </article>
  {pager(manual, chapter)}
  {footer(manual)}
</main>
{page_toc(chapter)}
</div>
<div class="scrim" data-scrim></div>
{scripts()}
</body>
</html>
"""


def index_page(manual: Manual, pdf_available: bool) -> str:
    cfg = manual.config
    first = manual.find(cfg.get("startHere", "")) or manual.chapters[0]
    cards = []
    for part in manual.parts:
        items = "".join(
            f'<li><a href="{c.url}"><span class="nav-num">{esc(c.label)}</span><span><strong>{c.title_html}</strong>'
            f'<small>{esc(c.lead)}</small></span></a></li>' for c in part.chapters)
        cards.append(f'<section class="part-card"><p class="kicker">{esc(part.kicker)}</p><h2>{esc(part.title)}</h2>'
                     f'<p class="part-description">{esc(part.description)}</p><ol>{items}</ol></section>')
    quick = "".join(
        f'<a class="quick-link" href="{esc(manual.find(q["chapter"]).url if manual.find(q["chapter"]) else "#")}"><strong>{esc(q["title"])}</strong>'
        f'<span>{esc(q["text"])}</span></a>' for q in cfg.get("quickLinks", []))
    pdf_button = (f'<a class="button button-secondary" href="{esc(cfg["pdfName"])}">{ICONS["pdf"]}Download the PDF</a>'
                  if pdf_available else "")
    title = esc(cfg["title"])
    if title.endswith(" X"):
        title = title[:-1] + '<span class="x">X</span>'
    return f"""{head(f"{cfg['title']} {cfg['subtitle']}", cfg.get("description", ""))}
<body class="index-page">
{topbar(manual, pdf_available)}
<div class="layout">
{sidebar(manual, None)}
<main class="content content-wide" id="main">
  <section class="hero">
    <div class="hero-art" style="background-image:url('assets/splash.png')"></div>
    <div class="hero-text">
      <p class="kicker">{esc(cfg["subtitle"])} · version {esc(manual.version)}</p>
      <h1>{title}</h1>
      <p class="hero-lead">{esc(cfg.get("tagline", ""))}</p>
      <div class="hero-actions">
        <a class="button" href="{first.url}">Start here: {first.title_html}</a>
        {pdf_button}
      </div>
    </div>
  </section>
  <section class="quick-links">{quick}</section>
  <div class="part-grid">{"".join(cards)}</div>
  {footer(manual)}
</main>
</div>
<div class="scrim" data-scrim></div>
{scripts()}
</body>
</html>
"""


def print_page(manual: Manual) -> str:
    cfg = manual.config
    toc = []
    for part in manual.parts:
        toc.append(f'<li class="toc-part"><a href="#{part.anchor}"><span class="toc-label">{esc(part.kicker)}</span>{esc(part.title)}</a><ol>')
        toc += [f'<li><a href="#{c.anchor}"><span class="toc-label">{esc(c.label)}</span>{c.title_html}</a></li>' for c in part.chapters]
        toc.append("</ol></li>")

    body = []
    for part in manual.parts:
        chapters_list = "".join(f'<li><span>{esc(c.label)}</span>{c.title_html}</li>' for c in part.chapters)
        numeral = "A–" + part.chapters[-1].label if part.appendix and part.chapters else ROMAN[part.number - 1]
        body.append(f'<section class="part-divider" id="{part.anchor}"><p class="part-numeral">{esc(numeral)}</p>'
                    f'<p class="kicker">{esc(part.kicker)}</p><h1>{esc(part.title)}</h1><p class="part-description">{esc(part.description)}</p>'
                    f'<ol class="part-chapters">{chapters_list}</ol></section>')
        for c in part.chapters:
            body.append(f'<article class="chapter print-chapter" id="{c.anchor}"><header class="chapter-header">'
                        f'<p class="kicker"><span>{esc(c.part.kicker)}</span><span class="dot">·</span><span>{esc(c.kicker)}</span></p>'
                        f'<h1>{c.title_html}</h1></header><div class="chapter-body">{c.print_html}</div></article>')

    colophon = "".join(f"<p>{esc(line)}</p>" for line in cfg.get("colophon", []))
    return f"""<!doctype html>
<html lang="en" data-theme="light">
<head>
<meta charset="utf-8">
<title>{esc(cfg["title"])} {esc(cfg["subtitle"])} {esc(manual.version)}</title>
<link rel="stylesheet" href="assets/manual.css">
<link rel="stylesheet" href="assets/print.css">
</head>
<body class="print-page">
<section class="cover">
  <div class="cover-art" style="background-image:url('assets/splash.png')"></div>
  <div class="cover-text">
    <img class="cover-icon" src="assets/icon.png" alt="">
    <p class="kicker">{esc(cfg.get("organization", ""))}</p>
    <h1>{esc(cfg["title"])}</h1>
    <p class="cover-subtitle">{esc(cfg["subtitle"])}</p>
    <p class="cover-tagline">{esc(cfg.get("tagline", ""))}</p>
  </div>
  <div class="cover-footer"><span>Editor version {esc(manual.version)}</span><span>{esc(manual.date)}</span></div>
</section>
<section class="colophon">
  <h2>About this manual</h2>
  {colophon}
  <p class="colophon-meta">Written for Scenery Editor X {esc(manual.version)} · Published {esc(manual.date)}<br>{esc(cfg.get("copyright", ""))}</p>
</section>
<nav class="print-toc"><h2>Contents</h2><ol>{"".join(toc)}</ol></nav>
{"".join(body)}
</body>
</html>
"""


def search_index(manual: Manual) -> str:
    entries = []
    for chapter in manual.chapters:
        # One entry for the chapter's opening, one per level 2 section
        parts = re.split(r'(?=<h2 id=")', chapter.site_html)
        for i, fragment in enumerate(parts):
            m = re.match(r'<h2 id="([^"]+)">(.*?)</h2>', fragment, re.S)
            heading = md2html.plain_text(re.sub(r'<a class="anchor".*?</a>', "", m.group(2))) if m else ""
            url = f"{chapter.url}#{m.group(1)}" if m else chapter.url
            body = re.sub(r"<h2.*?</h2>", "", fragment, count=1, flags=re.S)
            body = re.sub(r"</(p|li|td|th|h[1-6]|tr|div|figcaption|pre|aside)>|<br>", " ", body)  # Blocks don't run together
            text = md2html.plain_text(body)
            text = re.sub(r"\s+", " ", text).strip()
            if not text and not heading:
                continue
            entries.append({"c": f"{chapter.label}. {chapter.title}", "h": heading, "u": url, "t": text[:1600]})
    return "window.SEDX_SEARCH = " + json.dumps(entries, ensure_ascii=False, separators=(",", ":")) + ";\n"


# --------------------------------------------------------------------------------------------------------------------
# Output

def copy_assets(out: Path) -> None:
    assets = out / "assets"
    assets.mkdir(parents=True, exist_ok=True)
    for name in ("manual.css", "print.css", "manual.js"):
        shutil.copy2(HERE / "theme" / name, assets / name)
    shutil.copy2(REPO / "resources" / "icon.png", assets / "icon.png")
    shutil.copy2(REPO / "resources" / "splash_screen.png", assets / "splash.png")
    fonts = assets / "fonts"
    fonts.mkdir(exist_ok=True)
    for name in FONT_FILES:
        source = REPO / "resources" / "fonts" / "opensans" / name
        if source.exists():
            shutil.copy2(source, fonts / name)
    images = HERE / "images"
    if images.exists():
        shutil.copytree(images, out / "images", dirs_exist_ok=True)


def find_browser(explicit: str | None) -> str | None:
    if explicit:
        return explicit
    for env in ("SEDX_MANUAL_BROWSER", "CHROME_PATH"):
        if os.environ.get(env):
            return os.environ[env]
    candidates = [
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
        "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
        "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge",
        "/Applications/Chromium.app/Contents/MacOS/Chromium",
    ]
    for candidate in candidates:
        if Path(candidate).exists():
            return candidate
    for name in ("google-chrome", "google-chrome-stable", "chromium", "chromium-browser", "chrome", "msedge", "microsoft-edge"):
        found = shutil.which(name)
        if found:
            return found
    return None


def print_pdf(browser: str, page: Path, pdf: Path) -> bool:
    with tempfile.TemporaryDirectory(prefix="sedx-manual-") as profile:
        command = [browser, "--headless=new", "--disable-gpu", "--no-first-run", "--no-default-browser-check",
                   f"--user-data-dir={profile}", "--no-pdf-header-footer", "--generate-pdf-document-outline",
                   "--run-all-compositor-stages-before-draw", "--virtual-time-budget=20000",
                   f"--print-to-pdf={pdf}", page.resolve().as_uri()]
        try:
            subprocess.run(command, check=True, timeout=300, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except (subprocess.SubprocessError, OSError) as error:
            print(f"warning: printing the PDF failed: {error}")
            return False
    return pdf.exists() and pdf.stat().st_size > 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Build the Scenery Editor X user manual (HTML site and PDF).")
    parser.add_argument("--out", type=Path, default=HERE / "site", help="output folder (default: docs/manual/site)")
    parser.add_argument("--no-pdf", action="store_true", help="skip the PDF")
    parser.add_argument("--browser", help="Chrome, Chromium or Edge executable for the PDF")
    parser.add_argument("--strict", action="store_true", help="exit with an error on broken links or missing images")
    parser.add_argument("--date", help='publication date printed in the manual (default: this month, e.g. "October 2026")')
    args = parser.parse_args()

    manual = load_manual(args.date)
    convert_chapters(manual)
    print_ids = {c.anchor for c in manual.chapters} | {p.anchor for p in manual.parts}
    print_ids |= {h.id for c in manual.chapters for h in c.headings}
    # Print ids are the site ids with the chapter prefix
    print_ids |= {f"{c.anchor}--{i}" for c in manual.chapters for i in c.site_ids}

    problems = check_links(manual, print_ids)
    for problem in problems:
        print(f"warning: {problem}")

    out: Path = args.out
    out.mkdir(parents=True, exist_ok=True)
    copy_assets(out)
    (out / "assets" / "search-index.js").write_text(search_index(manual), encoding="utf-8")

    pdf = out / manual.config["pdfName"]
    browser = None if args.no_pdf else find_browser(args.browser)
    pdf_available = pdf.exists()
    print_html = out / "print.html"
    print_html.write_text(print_page(manual), encoding="utf-8")
    if browser:
        print(f"printing the PDF with {browser}")
        pdf_available = print_pdf(browser, print_html, pdf)
    elif not args.no_pdf:
        print("warning: no Chrome, Chromium or Edge found for the PDF (use --browser PATH)")

    (out / "index.html").write_text(index_page(manual, pdf_available), encoding="utf-8")
    for chapter in manual.chapters:
        (out / chapter.url).write_text(chapter_page(manual, chapter, pdf_available), encoding="utf-8")

    words = sum(len(md2html.plain_text(c.site_html).split()) for c in manual.chapters)
    print(f"built {len(manual.chapters)} chapters ({words:,} words) into {out}" + (f", PDF {pdf.name}" if pdf_available else ""))
    return 1 if problems and args.strict else 0


if __name__ == "__main__":
    sys.exit(main())
