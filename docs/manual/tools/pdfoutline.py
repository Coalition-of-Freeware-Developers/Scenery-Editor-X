"""
Reads and rearranges the bookmarks (the outline) of a PDF that Chrome or Edge printed. Standard library only.

Chrome makes one top-level bookmark per <h1>, in document order, with the lower headings nested under it. The build
uses read() to learn the page each <h1> landed on (for the contents' page numbers), and nest() to hang the top-level
bookmarks under each other (volume > part > chapter). nest() appends an incremental update, so the pages, fonts and
links Chrome wrote stay exactly as they were.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Item:
    num: int  # Object number
    title: str
    page: int | None  # 1-based
    entries: bytes  # The dictionary's own keys (/Title, /Dest, /SE ...) without its links to other bookmarks
    children: list["Item"] = field(default_factory=list)


_LINKS_RE = re.compile(rb"/(?:Parent|First|Last|Next|Prev)\s+\d+\s+\d+\s+R|/Count\s+-?\d+")


# --------------------------------------------------------------------------------------------------------------------
# Lexing

def _string_end(data: bytes, i: int) -> int:
    """data[i] is '(' of a literal string: the index just after its closing ')'."""
    depth, i = 0, i
    while i < len(data):
        c = data[i]
        if c == 0x5C:  # Backslash escapes the next byte
            i += 2
            continue
        if c == 0x28:
            depth += 1
        elif c == 0x29:
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    raise ValueError("unterminated string")


def _dict_end(data: bytes, i: int) -> int:
    """data[i:i+2] is '<<': the index just after the matching '>>'."""
    depth = 0
    while i < len(data):
        if data.startswith(b"<<", i):
            depth += 1
            i += 2
        elif data.startswith(b">>", i):
            depth -= 1
            i += 2
            if depth == 0:
                return i
        elif data[i] == 0x28:
            i = _string_end(data, i)
        elif data[i] == 0x3C:  # Hex string
            i = data.index(b">", i) + 1
        else:
            i += 1
    raise ValueError("unterminated dictionary")


def _decode_string(token: bytes) -> str:
    if token.startswith(b"<"):
        raw = bytes.fromhex(re.sub(rb"\s", b"", token[1:-1]).decode("ascii"))
    else:
        out, i, body = bytearray(), 0, token[1:-1]
        escapes = {ord("n"): 10, ord("r"): 13, ord("t"): 9, ord("b"): 8, ord("f"): 12}
        while i < len(body):
            c = body[i]
            if c != 0x5C:
                out.append(c)
                i += 1
                continue
            i += 1
            if i >= len(body):
                break
            e = body[i]
            if e in escapes:
                out.append(escapes[e])
                i += 1
            elif 0x30 <= e <= 0x37:
                m = re.match(rb"[0-7]{1,3}", body[i:i + 3])
                out.append(int(m.group(0), 8) & 0xFF)
                i += len(m.group(0))
            elif e in (10, 13):  # Line continuation
                i += 2 if body[i:i + 2] == b"\r\n" else 1
            else:
                out.append(e)
                i += 1
        raw = bytes(out)
    if raw.startswith(b"\xfe\xff"):
        return raw[2:].decode("utf-16-be", errors="replace")
    if raw.startswith(b"\xef\xbb\xbf"):
        return raw[3:].decode("utf-8", errors="replace")
    return raw.decode("latin-1")


def _string_after(body: bytes, key: bytes) -> str | None:
    i = body.find(key + b" ")
    if i < 0:
        i = body.find(key + b"(")
        if i < 0:
            i = body.find(key + b"<")
            if i < 0:
                return None
    i += len(key)
    while body[i:i + 1].isspace():
        i += 1
    if body[i:i + 1] == b"(":
        return _decode_string(body[i:_string_end(body, i)])
    if body[i:i + 1] == b"<":
        return _decode_string(body[i:body.index(b">", i) + 1])
    return None


def _ref(body: bytes, key: bytes) -> int | None:
    m = re.search(re.escape(key) + rb"\s+(\d+)\s+0\s+R", body)
    return int(m.group(1)) if m else None


# --------------------------------------------------------------------------------------------------------------------
# The file

class Pdf:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        i = self.data.rfind(b"startxref")
        if i < 0:
            raise ValueError("no startxref")
        self.xref_at = int(self.data[i + 9:].split()[0])
        if not self.data.startswith(b"xref", self.xref_at):
            raise ValueError("cross-reference streams aren't supported")
        self.offsets: dict[int, int] = {}
        self.trailer = b""
        at: int | None = self.xref_at
        while at is not None:  # The newest section first; an incremental update points back at the one before
            self._read_xref(at)
            t = self.data.find(b"trailer", at)
            trailer = self.data[t + 7:_dict_end(self.data, self.data.index(b"<<", t))]
            self.trailer = self.trailer or trailer
            prev = re.search(rb"/Prev\s+(\d+)", trailer)
            at = int(prev.group(1)) if prev else None

    def _read_xref(self, at: int) -> None:
        lines = self.data[at:self.data.find(b"trailer", at)].split(b"\n")
        k = 1
        while k < len(lines):
            head = lines[k].split()
            if len(head) != 2:
                k += 1
                continue
            first, count = int(head[0]), int(head[1])
            for j in range(count):
                entry = lines[k + 1 + j].split()
                if len(entry) >= 3 and entry[2] == b"n":
                    self.offsets.setdefault(first + j, int(entry[0]))
            k += 1 + count

    def object(self, num: int) -> bytes:
        """The object's dictionary (or other value) as source text, without any stream data."""
        at = self.offsets[num]
        m = re.compile(rb"\d+\s+\d+\s+obj\s*").match(self.data, at)
        start = m.end()
        if self.data.startswith(b"<<", start):
            return self.data[start:_dict_end(self.data, start)]
        return self.data[start:self.data.index(b"endobj", start)].strip()

    def catalog(self) -> bytes:
        return self.object(_ref(self.trailer, b"/Root"))

    def pages(self) -> dict[int, int]:
        """Page object number -> 1-based page number."""
        order: list[int] = []

        def walk(num: int) -> None:
            node = self.object(num)
            if re.search(rb"/Type\s*/Pages\b", node):
                kids = re.search(rb"/Kids\s*\[(.*?)\]", node, re.S).group(1)
                for kid in re.findall(rb"(\d+)\s+0\s+R", kids):
                    walk(int(kid))
            else:
                order.append(num)

        walk(_ref(self.catalog(), b"/Pages"))
        return {num: n + 1 for n, num in enumerate(order)}

    def outline(self) -> list[Item]:
        root = _ref(self.catalog(), b"/Outlines")
        if root is None:
            return []
        pages = self.pages()

        def chain(first: int | None) -> list[Item]:
            items = []
            num = first
            while num is not None:
                body = self.object(num)
                dest = re.search(rb"/Dest\s*\[\s*(\d+)\s+0\s+R", body)
                entries = re.sub(rb"\s*\n\s*", b"\n", _LINKS_RE.sub(b"", body[2:-2])).strip()
                item = Item(num, _string_after(body, b"/Title") or "", pages.get(int(dest.group(1))) if dest else None, entries)
                item.children = chain(_ref(body, b"/First"))
                items.append(item)
                num = _ref(body, b"/Next")
            return items

        return chain(_ref(self.object(root), b"/First"))


def read(path: Path) -> list[Item]:
    """The top-level bookmarks, each with its children."""
    return Pdf(path).outline()


def normalize(title: str) -> str:
    """Without any spaces: Chrome joins the lines of a heading that wraps without one."""
    return re.sub(r"\s+", "", title).casefold()


def _encode(title: str) -> bytes:
    return b"<FEFF" + title.encode("utf-16-be").hex().upper().encode("ascii") + b">"


def _retitle(entries: bytes, title: str) -> bytes:
    i = entries.find(b"/Title")
    if i < 0:
        return entries
    j = i + 6
    while entries[j:j + 1].isspace():
        j += 1
    end = _string_end(entries, j) if entries[j:j + 1] == b"(" else entries.index(b">", j) + 1
    return entries[:i] + b"/Title " + _encode(title) + entries[end:]


def match_pages(targets: list[tuple[str, str]], items: list[Item]) -> dict[str, int]:
    """Pages for (anchor, title) pairs, one per <h1> in document order, from the top-level bookmarks."""
    found, k = {}, 0
    for anchor, title in targets:
        want = normalize(title)
        for j in range(k, len(items)):
            if normalize(items[j].title) == want:
                if items[j].page:
                    found[anchor] = items[j].page
                k = j + 1
                break
    return found


# --------------------------------------------------------------------------------------------------------------------
# Rearranging

def nest(path: Path, depths: list[int], open_depth: int = 0, titles: list[tuple[str, list[str]]] | None = None) -> bool:
    """Hangs the top-level bookmarks under each other: depths[k] is the level the k-th one belongs at (0 = top).
    Bookmarks deeper than open_depth start closed. titles[k], when given, is the k-th one's title and its sections'
    titles in document order: they replace Chrome's, which lose the space where a heading wraps (sections only when
    the counts agree). Returns False (and leaves the file alone) when the bookmarks don't line up with depths."""
    pdf = Pdf(path)
    root_num = _ref(pdf.catalog(), b"/Outlines")
    items = pdf.outline()
    if root_num is None or len(items) != len(depths):
        return False

    renamed: dict[int, str] = {}
    for item, (title, sections) in zip(items, titles or []):
        renamed[item.num] = title
        below: list[Item] = []
        stack = list(reversed(item.children))
        while stack:
            child = stack.pop()
            below.append(child)
            stack += reversed(child.children)
        if len(below) == len(sections):
            renamed |= {child.num: text for child, text in zip(below, sections)}

    # Build the tree: each bookmark goes under the nearest earlier one that sits higher, after that one's own sections
    top: list[Item] = []
    stack: list[tuple[int, Item]] = []
    for item, depth in zip(items, depths):
        while stack and stack[-1][0] >= depth:
            stack.pop()
        (stack[-1][1].children if stack else top).append(item)
        stack.append((depth, item))

    # Every bookmark is written again, with its links to its parent, siblings and children
    written: dict[int, bytes] = {}

    def visible(item: Item, depth: int) -> int:
        """Descendants shown while this bookmark (at depth) is open; children deeper than open_depth stay closed."""
        return sum(1 + (visible(c, depth + 1) if depth + 1 < open_depth else 0) for c in item.children)

    def write(siblings: list[Item], parent: int, depth: int) -> None:
        for k, item in enumerate(siblings):
            entries = item.entries
            if item.num in renamed and renamed[item.num] != item.title:
                entries = _retitle(entries, renamed[item.num])
            parts = [b"<<", entries, b"\n/Parent %d 0 R" % parent]
            if k > 0:
                parts.append(b"\n/Prev %d 0 R" % siblings[k - 1].num)
            if k + 1 < len(siblings):
                parts.append(b"\n/Next %d 0 R" % siblings[k + 1].num)
            if item.children:
                count = visible(item, depth)  # Negative: closed, and how many would show if it were opened
                parts.append(b"\n/First %d 0 R\n/Last %d 0 R\n/Count %d" % (item.children[0].num, item.children[-1].num,
                                                                         count if depth < open_depth else -count))
                write(item.children, item.num, depth + 1)
            parts.append(b">>")
            written[item.num] = b"".join(parts)

    write(top, root_num, 0)
    total = len(top) + (sum(visible(i, 0) for i in top) if open_depth > 0 else 0)
    written[root_num] = b"<</Type /Outlines\n/First %d 0 R\n/Last %d 0 R\n/Count %d>>" % (top[0].num, top[-1].num, total)

    # Incremental update: the new objects, a cross-reference section for them, and a trailer pointing at the old one
    out = bytearray(pdf.data)
    if not out.endswith(b"\n"):
        out += b"\n"
    offsets = {}
    for num in sorted(written):
        offsets[num] = len(out)
        out += b"%d 0 obj\n" % num + written[num] + b"\nendobj\n"
    xref_at = len(out)
    out += b"xref\n"
    nums = sorted(offsets)
    run: list[int] = []
    for num in nums + [-1]:
        if run and num != run[-1] + 1:
            out += b"%d %d\n" % (run[0], len(run))
            for r in run:
                out += b"%010d 00000 n \n" % offsets[r]
            run = []
        if num >= 0:
            run.append(num)
    size = re.search(rb"/Size\s+(\d+)", pdf.trailer).group(1)
    trailer = b"<</Size " + size + b"\n/Root %d 0 R" % _ref(pdf.trailer, b"/Root")
    info = _ref(pdf.trailer, b"/Info")
    if info is not None:
        trailer += b"\n/Info %d 0 R" % info
    ids = re.search(rb"/ID\s*\[[^\]]*\]", pdf.trailer)
    if ids:
        trailer += b"\n" + ids.group(0)
    trailer += b"\n/Prev %d>>" % pdf.xref_at
    out += b"trailer\n" + trailer + b"\nstartxref\n%d\n%%%%EOF\n" % xref_at
    path.write_bytes(bytes(out))
    return True
