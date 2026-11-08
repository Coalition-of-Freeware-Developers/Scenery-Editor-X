"""
A small Markdown to HTML converter for the Scenery Editor X manual. Standard library only.

It reads the GitHub-flavoured subset the manual is written in, so every chapter also reads well on GitHub:

- ATX headings (``## Title``), with an optional ``{#custom-id}``; the first ``# Title`` is the chapter title
- paragraphs, ``**strong**``, ``*emphasis*``, ``code spans``, hard breaks (two trailing spaces or a backslash)
- links and images; an image alone in a paragraph becomes a ``<figure>`` whose caption is the image title
- fenced code blocks, GFM tables with column alignment, nested ordered and unordered lists, blockquotes
- GitHub alerts (``> [!NOTE]``, ``[!TIP]``, ``[!IMPORTANT]``, ``[!WARNING]``, ``[!CAUTION]``)
- raw HTML blocks (block-level tags, ended by a blank line) and inline tags
- ``<kbd>Ctrl+S</kbd>``, drawn as one key cap per key
- ``<!-- class: name -->`` on its own line adds a class to the next block (``steps`` lists, ``shortcuts`` tables)
"""

from __future__ import annotations

import html
import re
from dataclasses import dataclass, field
from typing import Callable

from highlight import highlight

LONG_BLOCK = 10  # Code blocks with more lines, and tables with more rows, get class "long"


@dataclass
class Heading:
    level: int
    text: str  # Plain text
    html: str  # Inline HTML
    id: str


@dataclass
class Context:
    """What a document needs from its surroundings: link / image rewriting and unique heading ids."""

    link: Callable[[str], str] = lambda href: href
    image: Callable[[str], str] = lambda src: src
    id_prefix: str = ""
    title: str = ""
    consume_title: bool = True  # The first # heading is the document's title (not drawn in the body)
    heading_shift: int = 0  # Added to the level of every other heading (documents with several # headings)
    headings: list[Heading] = field(default_factory=list)
    used_ids: set[str] = field(default_factory=set)
    links: list[str] = field(default_factory=list)  # Rewritten hrefs, for the link check

    def unique_id(self, base: str) -> str:
        candidate = self.id_prefix + base
        n = 2
        while candidate in self.used_ids:
            candidate = f"{self.id_prefix}{base}-{n}"
            n += 1
        self.used_ids.add(candidate)
        return candidate


# --------------------------------------------------------------------------------------------------------------------
# Inline

_ESCAPABLE = "\\`*_{}[]()#+-.!|<>~"
_TOKEN = "\x00{}\x00"
_TOKEN_RE = re.compile("\x00(\\d+)\x00")

_CODE_RE = re.compile(r"(`+)(.+?)(?<!`)\1(?!`)", re.S)
_KBD_RE = re.compile(r"<kbd>(.*?)</kbd>", re.S)
_AUTOLINK_RE = re.compile(r"<(https?://[^\s>]+)>")
# Inline HTML that passes through. Anything else in angle brackets (C++ templates: <typename T>, <int>) is text.
_INLINE_TAGS = "a|abbr|b|br|code|del|em|i|img|ins|kbd|mark|s|small|span|strong|sub|sup|u|div|p|details|summary|hr|font|center"
_TAG_RE = re.compile(r"<!--.*?-->|</?(?:" + _INLINE_TAGS + r")(?:\s[^<>]*?)?/?>", re.S | re.I)
_IMAGE_RE = re.compile(r'!\[([^\]]*)\]\(\s*<?([^)\s>]+)>?(?:\s+"([^"]*)")?\s*\)')
_LINK_RE = re.compile(r'\[((?:[^\[\]]|\[[^\]]*\])*)\]\(\s*<?([^)\s>]+)>?(?:\s+"([^"]*)")?\s*\)')
_ENTITY_RE = re.compile(r"&(?!(?:#\d+|#x[0-9a-fA-F]+|[A-Za-z][A-Za-z0-9]*);)")
_STRONG_RE = re.compile(r"(\*\*|__)(?=\S)(.+?)(?<=\S)\1", re.S)
_EM_STAR_RE = re.compile(r"(?<![\w*])\*(?=[^\s*])(.+?)(?<=[^\s*])\*(?![\w*])", re.S)
_EM_UNDER_RE = re.compile(r"(?<![\w_])_(?=[^\s_])(.+?)(?<=[^\s_])_(?![\w_])", re.S)


def _escape_text(text: str) -> str:
    text = _ENTITY_RE.sub("&amp;", text)
    return text.replace("<", "&lt;").replace(">", "&gt;")


def _escape_attr(text: str) -> str:
    return html.escape(text, quote=True)


def _kbd(inner: str) -> str:
    keys = [k.strip() for k in re.split(r"(?<=\S)\s*\+\s*(?=\S)", inner.strip())]
    caps = [f"<kbd>{html.escape(k, quote=False)}</kbd>" for k in keys if k]
    return '<span class="keys">' + "<span>+</span>".join(caps) + "</span>"


def plain_text(fragment: str) -> str:
    """Inline HTML -> text (for heading ids, the search index and titles)."""
    text = re.sub(r"<[^>]+>", "", fragment)
    return html.unescape(text)


def slugify(text: str) -> str:
    """Heading text -> id, the way GitHub makes them, so a link written for GitHub works here too."""
    text = plain_text(text).strip().lower()
    text = re.sub(r"[^\w\- ]", "", text)
    return text.replace(" ", "-") or "section"


def render_inline(text: str, ctx: Context) -> str:
    tokens: list[str] = []

    def keep(fragment: str) -> str:
        tokens.append(fragment)
        return _TOKEN.format(len(tokens) - 1)

    # Hard breaks: a backslash or two spaces before a newline
    text = re.sub(r"(\\| {2,})\n", lambda m: keep("<br>\n"), text)
    # Backslash escapes
    text = re.sub(r"\\([" + re.escape(_ESCAPABLE) + "])", lambda m: keep(_escape_text(m.group(1))), text)
    # Code spans, before anything can look inside them
    text = _CODE_RE.sub(lambda m: keep("<code>" + _escape_text(m.group(2).strip() if m.group(2).strip() else m.group(2)) + "</code>"), text)
    text = _KBD_RE.sub(lambda m: keep(_kbd(m.group(1))), text)
    text = _AUTOLINK_RE.sub(lambda m: keep(f'<a href="{_escape_attr(m.group(1))}">{_escape_text(m.group(1))}</a>'), text)
    text = _TAG_RE.sub(lambda m: keep(m.group(0)), text)

    def image(m: re.Match) -> str:
        alt, src, title = m.group(1), ctx.image(m.group(2)), m.group(3)
        title_attr = f' title="{_escape_attr(title)}"' if title else ""
        return keep(f'<img src="{_escape_attr(src)}" alt="{_escape_attr(alt)}"{title_attr} loading="lazy">')

    text = _IMAGE_RE.sub(image, text)

    def link(m: re.Match) -> str:
        label, href, title = m.group(1), m.group(2), m.group(3)
        href = ctx.link(href)
        ctx.links.append(href)
        title_attr = f' title="{_escape_attr(title)}"' if title else ""
        external = ' class="external" target="_blank" rel="noopener"' if re.match(r"^[a-z]+://", href) else ""
        return keep(f'<a href="{_escape_attr(href)}"{title_attr}{external}>{_emphasis(_escape_text(label))}</a>')

    text = _LINK_RE.sub(link, text)
    text = _emphasis(_escape_text(text))

    # Tokens can hold tokens (a link label with code in it): restore until none are left
    for _ in range(8):
        restored = _TOKEN_RE.sub(lambda m: tokens[int(m.group(1))], text)
        if restored == text:
            break
        text = restored
    return text


def _emphasis(text: str) -> str:
    text = _STRONG_RE.sub(r"<strong>\2</strong>", text)
    text = _EM_STAR_RE.sub(r"<em>\1</em>", text)
    return _EM_UNDER_RE.sub(r"<em>\1</em>", text)


# --------------------------------------------------------------------------------------------------------------------
# Blocks

_FENCE_RE = re.compile(r"^(\s*)(`{3,}|~{3,})\s*([\w+#.-]*)\s*(.*)$")
_HEADING_RE = re.compile(r"^\s{0,3}(#{1,6})\s+(.*?)(?:\s+\{#([A-Za-z0-9_-]+)\})?\s*#*\s*$")
_HR_RE = re.compile(r"^\s{0,3}([-*_])(\s*\1){2,}\s*$")
_LIST_RE = re.compile(r"^(\s*)([-*+]|\d{1,9}[.)])(?:(\s+)(.*))?$")
_TABLE_SEP_RE = re.compile(r"^\s*\|?\s*:?-{2,}:?\s*(\|\s*:?-{2,}:?\s*)*\|?\s*$")
_QUOTE_RE = re.compile(r"^\s{0,3}>\s?(.*)$")
_ALERT_RE = re.compile(r"^\s*\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\]\s*(.*)$", re.I)
_HTML_BLOCK_RE = re.compile(
    r"^\s{0,3}<(?:/?(?:div|figure|svg|table|section|details|summary|aside|nav|picture|video|iframe|style|script|dl|hr|header|footer|ol|ul)\b|!--)",
    re.I,
)
_CLASS_MARK_RE = re.compile(r"^\s*<!--\s*class:\s*([\w -]+?)\s*-->\s*$")

_ALERT_TITLES = {"NOTE": "Note", "TIP": "Tip", "IMPORTANT": "Important", "WARNING": "Warning", "CAUTION": "Caution"}


def _indent(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def _is_blank(line: str) -> bool:
    return not line.strip()


def _starts_block(line: str) -> bool:
    return bool(
        _FENCE_RE.match(line) and _FENCE_RE.match(line).group(2)
        or _HEADING_RE.match(line)
        or _HR_RE.match(line)
        or _QUOTE_RE.match(line)
        or _LIST_RE.match(line) and _LIST_RE.match(line).group(4)
        or _HTML_BLOCK_RE.match(line)
    )


def _add_class(fragment: str, css_class: str | None) -> str:
    if not css_class:
        return fragment
    m = re.match(r"(\s*<[a-zA-Z0-9]+)([^>]*)>", fragment)
    if not m:
        return fragment
    attrs = m.group(2)
    if 'class="' in attrs:
        attrs = attrs.replace('class="', f'class="{css_class} ', 1)
    else:
        attrs += f' class="{css_class}"'
    return m.group(1) + attrs + ">" + fragment[m.end():]


def _split_row(line: str) -> list[str]:
    line = line.strip()
    if line.startswith("|"):
        line = line[1:]
    if line.endswith("|") and not line.endswith("\\|"):
        line = line[:-1]
    # Pipes inside code spans don't split cells
    cells, current, in_code = [], "", False
    i = 0
    while i < len(line):
        c = line[i]
        if c == "\\" and i + 1 < len(line) and line[i + 1] == "|":
            current += "\\|"
            i += 2
            continue
        if c == "`":
            in_code = not in_code
        if c == "|" and not in_code:
            cells.append(current.strip())
            current = ""
        else:
            current += c
        i += 1
    cells.append(current.strip())
    return cells


def _render_table(lines: list[str], ctx: Context) -> str:
    header = _split_row(lines[0])
    aligns = []
    for cell in _split_row(lines[1]):
        left, right = cell.startswith(":"), cell.endswith(":")
        aligns.append("center" if left and right else "right" if right else "left" if left else None)

    def cell(tag: str, text: str, col: int) -> str:
        align = aligns[col] if col < len(aligns) else None
        style = f' style="text-align:{align}"' if align else ""
        return f"<{tag}{style}>{render_inline(text.replace(chr(92) + '|', '|'), ctx)}</{tag}>"

    long = " long" if len(lines) - 2 > LONG_BLOCK else ""
    out = [f'<div class="table-wrap{long}"><table>', "<thead><tr>"]
    out += [cell("th", text, c) for c, text in enumerate(header)]
    out.append("</tr></thead><tbody>")
    for row in lines[2:]:
        cells = _split_row(row)
        cells += [""] * (len(header) - len(cells))
        out.append("<tr>" + "".join(cell("td", text, c) for c, text in enumerate(cells[: len(header)])) + "</tr>")
    out.append("</tbody></table></div>")
    return "".join(out)


def _render_heading(level: int, text: str, custom_id: str | None, ctx: Context) -> str:
    inline = render_inline(text, ctx)
    if level == 1 and ctx.consume_title and not ctx.title:
        ctx.title = inline
        return ""
    # A body never draws an h1: the page template owns those (and the PDF's bookmarks are built from them)
    level = min(6, max(2, level + ctx.heading_shift))
    hid = ctx.unique_id(custom_id or slugify(inline))
    ctx.headings.append(Heading(level, plain_text(inline), inline, hid))
    anchor = f'<a class="anchor" href="#{hid}" aria-label="Link to this section">#</a>' if level in (2, 3) else ""
    return f'<h{level} id="{hid}">{inline}{anchor}</h{level}>'


def _parse_list(lines: list[str], i: int, ctx: Context) -> tuple[str, int]:
    first = _LIST_RE.match(lines[i])
    base = len(first.group(1))
    ordered = first.group(2)[0].isdigit()
    start = int(first.group(2)[:-1]) if ordered else 1
    items: list[list[str]] = []
    loose = False
    n = len(lines)

    while i < n:
        m = _LIST_RE.match(lines[i])
        if not m or m.group(2)[0].isdigit() != ordered or not (base <= len(m.group(1)) < base + 2):
            break
        spacing = len(m.group(3) or " ")
        content = len(m.group(1)) + len(m.group(2)) + (spacing if spacing <= 4 else 1)
        item = [m.group(4) or ""]
        i += 1
        while i < n:
            line = lines[i]
            if _is_blank(line):
                j = i
                while j < n and _is_blank(lines[j]):
                    j += 1
                if j < n and _indent(lines[j]) >= content:
                    item += [""] * (j - i)
                    i = j
                    loose = True
                    continue
                break
            if _indent(line) >= content:
                item.append(line[content:])
                i += 1
                continue
            sub = _LIST_RE.match(line)
            if sub and sub.group(4) is not None:
                break
            if _starts_block(line):
                break
            item.append(line.strip())  # Lazy continuation of the item's paragraph
            i += 1
        items.append(item)

        # Blank lines between items make the list loose, if it goes on
        j = i
        while j < n and _is_blank(lines[j]):
            j += 1
        nxt = _LIST_RE.match(lines[j]) if j < n else None
        if nxt and nxt.group(2)[0].isdigit() == ordered and base <= len(nxt.group(1)) < base + 2:
            if j > i:
                loose = True
            i = j
            continue
        break

    tag = "ol" if ordered else "ul"
    start_attr = f' start="{start}"' if ordered and start != 1 else ""
    out = [f"<{tag}{start_attr}>"]
    for item in items:
        body = parse_blocks(item, ctx)
        if not loose:
            body = re.sub(r"^<p>(.*?)</p>", r"\1", body, count=1, flags=re.S)
        out.append(f"<li>{body}</li>")
    out.append(f"</{tag}>")
    return "".join(out), i


def parse_blocks(lines: list[str], ctx: Context) -> str:
    out: list[str] = []
    pending_class: str | None = None
    i, n = 0, len(lines)

    def emit(fragment: str) -> None:
        nonlocal pending_class
        if fragment:
            out.append(_add_class(fragment, pending_class))
            pending_class = None

    while i < n:
        line = lines[i]
        if _is_blank(line):
            i += 1
            continue

        mark = _CLASS_MARK_RE.match(line)
        if mark:
            pending_class = mark.group(1)
            i += 1
            continue

        fence = _FENCE_RE.match(line)
        if fence and fence.group(2):
            marker, lang = fence.group(2), fence.group(3)
            indent = len(fence.group(1))
            body = []
            i += 1
            while i < n and not lines[i].strip().startswith(marker):
                body.append(lines[i][indent:] if lines[i][:indent].strip() == "" else lines[i])
                i += 1
            i += 1
            code = highlight("\n".join(body), lang)
            lang_attr = f' class="language-{lang}"' if lang else ""
            label = f' data-lang="{lang}"' if lang else ""
            long = " long" if len(body) > LONG_BLOCK else ""  # Long blocks may break across printed pages
            emit(f'<pre class="code{long}"{label}><code{lang_attr}>{code}</code></pre>')
            continue

        heading = _HEADING_RE.match(line)
        if heading:
            emit(_render_heading(len(heading.group(1)), heading.group(2), heading.group(3), ctx))
            i += 1
            continue

        if _HR_RE.match(line):
            emit("<hr>")
            i += 1
            continue

        if "|" in line and i + 1 < n and _TABLE_SEP_RE.match(lines[i + 1]):
            rows = [line, lines[i + 1]]
            i += 2
            while i < n and "|" in lines[i] and not _is_blank(lines[i]):
                rows.append(lines[i])
                i += 1
            emit(_render_table(rows, ctx))
            continue

        if _QUOTE_RE.match(line):
            inner = []
            while i < n:
                q = _QUOTE_RE.match(lines[i])
                if q:
                    inner.append(q.group(1))
                elif not _is_blank(lines[i]) and not _starts_block(lines[i]) and inner and not _is_blank(inner[-1]):
                    inner.append(lines[i])  # Lazy continuation
                else:
                    break
                i += 1
            alert = _ALERT_RE.match(inner[0]) if inner else None
            if alert:
                kind = alert.group(1).upper()
                title = alert.group(2).strip() or _ALERT_TITLES[kind]
                body = parse_blocks(inner[1:], ctx)
                emit(f'<aside class="callout callout-{kind.lower()}"><p class="callout-title">{render_inline(title, ctx)}</p>{body}</aside>')
            else:
                emit(f"<blockquote>{parse_blocks(inner, ctx)}</blockquote>")
            continue

        item = _LIST_RE.match(line)
        if item and item.group(4) is not None:
            fragment, i = _parse_list(lines, i, ctx)
            emit(fragment)
            continue

        if _HTML_BLOCK_RE.match(line):
            if line.lstrip().startswith("<!--"):
                while i < n and "-->" not in lines[i]:
                    i += 1
                i += 1
                continue
            block = []
            while i < n and not _is_blank(lines[i]):
                block.append(lines[i])
                i += 1
            emit("\n".join(block))
            continue

        # Paragraph
        para = [line.strip()]
        i += 1
        while i < n and not _is_blank(lines[i]) and not _starts_block(lines[i]) and not _CLASS_MARK_RE.match(lines[i]):
            if "|" in lines[i] and i + 1 < n and _TABLE_SEP_RE.match(lines[i + 1]):
                break
            para.append(lines[i] if lines[i].endswith("  ") else lines[i].strip())
            i += 1
        text = "\n".join(para)
        lone_image = _IMAGE_RE.fullmatch(text.strip())
        if lone_image:
            img = render_inline(text.strip(), ctx)
            caption = lone_image.group(3)
            figcaption = f"<figcaption>{render_inline(caption, ctx)}</figcaption>" if caption else ""
            emit(f"<figure>{img}{figcaption}</figure>")
        else:
            emit(f"<p>{render_inline(text, ctx)}</p>")

    return "\n".join(out)


def convert(markdown: str, ctx: Context) -> str:
    """Markdown -> HTML body. The first level 1 heading goes to ctx.title instead of the body."""
    text = markdown.replace("\r\n", "\n").replace("\t", "    ")
    return parse_blocks(text.split("\n"), ctx)
