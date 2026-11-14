#!/usr/bin/env python3
"""
Scenery Editor X translation tooling (Python standard library only).

    python -I tools/i18n/sedx_i18n.py extract [--check] [--verbose]
        Scans source/ for translatable strings and writes resources/locale/sedx.pot.
        --check compares instead of writing (exit 1 when the template is out of date).
    python -I tools/i18n/sedx_i18n.py update [files...]
        Brings catalogs (default: every resources/locale/*.po) in line with the template: new strings are added
        untranslated, translations are kept, strings the code no longer has become obsolete ("#~") entries.
    python -I tools/i18n/sedx_i18n.py new <tag>
        Creates resources/locale/<tag>.po from the template (Weblate does this itself for its translators).
    python -I tools/i18n/sedx_i18n.py check [files...]
        Validates catalogs: syntax, header, plural forms, placeholders, "##" IDs, whitespace, length limits.
    python -I tools/i18n/sedx_i18n.py lint [--update-baseline]
        Finds Dear ImGui calls given English literals that aren't marked for translation. Sites listed in
        tools/i18n/lint_baseline.txt (the ones not converted yet) are allowed; new ones fail.
    python -I tools/i18n/sedx_i18n.py stats [--markdown]
        How much of each catalog is translated.
    python -I tools/i18n/sedx_i18n.py ci
        extract --check, check and lint together.
    python -I tools/i18n/sedx_i18n.py changes [old.pot]
        Markdown list of the messages added and removed since an older template (the localization workflow's summary).

The localization workflow (.github/workflows/localization.yml) runs extract and update on every pull request into
dev and commits the refreshed template and catalogs to the pull request's branch, so they never fall behind the code.

What is extracted (see source/SceneryEditorX/i18n/tr.h):
  * string literals passed to Tr, TrL, TrF, TrN, TrNF, TrNoop and Ctx (context, text);
  * string literals passed to any function declared with a Text parameter (EditorUI::PropertyLabel(Text label), ...),
    found by reading the declarations, so a new helper needs no change here.
The text before the first "##" is the msgid (the rest is Dear ImGui's ID). A "// TRANSLATORS: ..." comment on the
line above a call (or above a declaration with Text parameters, for every call of it) becomes a "#." comment.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass, field

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LOCALE_DIR = os.path.join(ROOT, "resources", "locale")
TEMPLATE = os.path.join(LOCALE_DIR, "sedx.pot")
LANGUAGES_CPP = os.path.join(ROOT, "source", "SceneryEditorX", "i18n", "languages.cpp")
BASELINE = os.path.join(ROOT, "tools", "i18n", "lint_baseline.txt")
SOURCE_DIR = os.path.join(ROOT, "source")
REPOSITORY_URL = "https://github.com/The3dVehicleguy/Scenery-Editor-X"

# Folders under source/ that are never scanned: vendored code and tests (their strings aren't user interface)
EXCLUDED_DIRS = {
    os.path.normcase(os.path.join(SOURCE_DIR, "SceneryEditorX", "renderer", "ui", "source")),
    os.path.normcase(os.path.join(SOURCE_DIR, "Tests")),
    os.path.normcase(os.path.join(SOURCE_DIR, "Fuzz")),
}
SOURCE_EXTENSIONS = (".h", ".hpp", ".cpp", ".inl", ".mm")

# Translation functions: argument positions of messages, and what kind of entry they make
KEYWORDS = {
    "Tr": {"messages": [0]},
    "TrL": {"messages": [0]},
    "TrNoop": {"messages": [0]},
    "Text": {"messages": [0]},      # Text("A") and Msg("A") made explicitly, e.g. cond ? Text("A") : Text("B")
    "Msg": {"messages": [0]},
    "TrF": {"messages": [0], "format": True},
    "TrN": {"messages": [0, 1], "plural": True},
    "TrNF": {"messages": [0, 1], "plural": True, "format": True},
}

HEADER_FIELDS = [
    ("Project-Id-Version", "Scenery Editor X"),
    ("Report-Msgid-Bugs-To", REPOSITORY_URL + "/issues"),
]


class ToolError(Exception):
    pass


# -------------------------------------------------------
# C++ lexer
# -------------------------------------------------------

@dataclass
class Token:
    kind: str       # id, str, num, char, punct, comment
    value: str
    line: int
    end_line: int = 0
    prefix: str = ""


SIMPLE_ESCAPES = {"n": "\n", "t": "\t", "r": "\r", "a": "\a", "b": "\b", "f": "\f", "v": "\v",
                  "\\": "\\", "\"": "\"", "'": "'", "?": "?"}


def decode_cpp_escapes(text: str) -> str:
    """Decodes the escapes of a C++ narrow string literal into text (UTF-8 bytes from \\x / octal are joined)."""
    out = bytearray()
    i = 0
    while i < len(text):
        c = text[i]
        if c != "\\":
            out += c.encode("utf-8")
            i += 1
            continue
        i += 1
        if i >= len(text):
            break
        e = text[i]
        if e in SIMPLE_ESCAPES:
            out += SIMPLE_ESCAPES[e].encode("utf-8")
            i += 1
        elif e == "x":
            j = i + 1
            while j < len(text) and text[j] in "0123456789abcdefABCDEF":
                j += 1
            out.append(int(text[i + 1:j] or "0", 16) & 0xFF)
            i = j
        elif e in "01234567":
            j = i
            while j < len(text) and j < i + 3 and text[j] in "01234567":
                j += 1
            out.append(int(text[i:j], 8) & 0xFF)
            i = j
        elif e in "uU":
            width = 4 if e == "u" else 8
            out += chr(int(text[i + 1:i + 1 + width], 16)).encode("utf-8")
            i += 1 + width
        else:
            out += e.encode("utf-8")
            i += 1
    return out.decode("utf-8", errors="replace")


STRING_PREFIX = re.compile(r'(u8R|uR|UR|LR|R|u8|u|U|L)?"')
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
NUMBER = re.compile(r"\.?[0-9](?:[0-9a-zA-Z_.']|[eEpP][+-])*")
TRANSLATORS = re.compile(r"TRANSLATORS:\s*(.*)", re.S)


def tokenize(source: str) -> list[Token]:
    tokens: list[Token] = []
    i, line, n = 0, 1, len(source)
    at_line_start = True
    while i < n:
        c = source[i]
        if c == "\n":
            line += 1
            i += 1
            at_line_start = True
            continue
        if c in " \t\r\f\v":
            i += 1
            continue
        if c == "\\" and i + 1 < n and source[i + 1] in "\r\n":
            i += 1
            continue
        if c == "#" and at_line_start:
            # Preprocessor directive, continuation lines included
            while i < n and source[i] != "\n":
                if source[i] == "\\" and i + 1 < n and source[i + 1] == "\n":
                    line += 1
                    i += 2
                    continue
                if source.startswith("/*", i):
                    end = source.find("*/", i + 2)
                    end = n if end < 0 else end + 2
                    line += source.count("\n", i, end)
                    i = end
                    continue
                i += 1
            continue
        at_line_start = False
        if source.startswith("//", i):
            end = source.find("\n", i)
            end = n if end < 0 else end
            tokens.append(Token("comment", source[i + 2:end].strip(), line, line))
            i = end
            continue
        if source.startswith("/*", i):
            end = source.find("*/", i + 2)
            end = n if end < 0 else end + 2
            text = source[i + 2:end - 2]
            end_line = line + source.count("\n", i, end)
            cleaned = " ".join(part.strip().lstrip("*").strip() for part in text.splitlines()).strip()
            tokens.append(Token("comment", cleaned, line, end_line))
            line = end_line
            i = end
            continue
        match = STRING_PREFIX.match(source, i)
        if match and (match.start() == i):
            prefix = match.group(1) or ""
            # An identifier ending in u/U/L/R right before a quote is a prefix only at an identifier boundary
            if prefix and i > 0 and (source[i - 1].isalnum() or source[i - 1] == "_"):
                match = None
        if match and match.start() == i:
            prefix = match.group(1) or ""
            start = match.end()
            if "R" in prefix:
                paren = source.find("(", start)
                delimiter = source[start:paren]
                close = source.find(")" + delimiter + "\"", paren)
                if paren < 0 or close < 0:
                    raise ToolError(f"line {line}: unterminated raw string")
                value = source[paren + 1:close]
                end = close + len(delimiter) + 2
            else:
                j = start
                while j < n and source[j] != "\"":
                    if source[j] == "\\":
                        j += 1
                    elif source[j] == "\n":
                        raise ToolError(f"line {line}: unterminated string")
                    j += 1
                value = decode_cpp_escapes(source[start:j])
                end = j + 1
            tokens.append(Token("str", value, line, line + source.count("\n", i, end), prefix))
            line += source.count("\n", i, end)
            i = end
            continue
        if c == "'":
            j = i + 1
            while j < n and source[j] != "'":
                if source[j] == "\\":
                    j += 1
                j += 1
            tokens.append(Token("char", source[i:j + 1], line, line))
            i = j + 1
            continue
        match = IDENTIFIER.match(source, i)
        if match:
            tokens.append(Token("id", match.group(0), line, line))
            i = match.end()
            continue
        match = NUMBER.match(source, i)
        if match:
            tokens.append(Token("num", match.group(0), line, line))
            i = match.end()
            continue
        if source.startswith("::", i) or source.startswith("->", i):
            tokens.append(Token("punct", source[i:i + 2], line, line))
            i += 2
            continue
        tokens.append(Token("punct", c, line, line))
        i += 1
    return tokens


def code_tokens(tokens: list[Token]) -> list[Token]:
    return [t for t in tokens if t.kind != "comment"]


def matching_paren(tokens: list[Token], open_index: int) -> int:
    """Index of the ')' closing tokens[open_index] == '(' (or len(tokens) when unbalanced)."""
    depth = 0
    for k in range(open_index, len(tokens)):
        v = tokens[k].value if tokens[k].kind == "punct" else ""
        if v in ("(", "[", "{"):
            depth += 1
        elif v in (")", "]", "}"):
            depth -= 1
            if depth == 0:
                return k
    return len(tokens)


def split_arguments(tokens: list[Token], open_index: int, close_index: int) -> list[tuple[int, int]]:
    """[start, end) token ranges of the top-level arguments between a '(' and its ')'."""
    args, depth, start = [], 0, open_index + 1
    for k in range(open_index + 1, close_index):
        v = tokens[k].value if tokens[k].kind == "punct" else ""
        if v in ("(", "[", "{"):
            depth += 1
        elif v in (")", "]", "}"):
            depth -= 1
        elif v == "," and depth == 0:
            args.append((start, k))
            start = k + 1
    if close_index > open_index + 1:
        args.append((start, close_index))
    return args


def literal_value(tokens: list[Token], start: int, end: int) -> str | None:
    """The text of an argument made only of adjacent narrow string literals, else None."""
    if start >= end:
        return None
    parts = []
    for k in range(start, end):
        t = tokens[k]
        if t.kind != "str" or t.prefix not in ("", "R"):
            return None
        parts.append(t.value)
    return "".join(parts)


def visible(text: str) -> str:
    marker = text.find("##")
    return text if marker < 0 else text[:marker]


# -------------------------------------------------------
# Extraction
# -------------------------------------------------------

@dataclass
class Entry:
    msgid: str
    context: str | None = None
    plural: str | None = None
    references: set = field(default_factory=set)
    comments: list = field(default_factory=list)
    flags: set = field(default_factory=set)

    def key(self):
        return (self.context, self.msgid)


class Catalog:
    def __init__(self):
        self.entries: dict = {}
        self.errors: list[str] = []

    def add(self, msgid, context, plural, reference, comment, flags, where):
        if not msgid:
            return
        if "##" in msgid:
            self.errors.append(f"{where}: message '{msgid}' contains '##' after removing the ID part")
        key = (context, msgid)
        entry = self.entries.get(key)
        if entry is None:
            entry = Entry(msgid, context, plural)
            self.entries[key] = entry
        elif plural != entry.plural:
            if entry.plural is None or plural is None:
                self.errors.append(f"{where}: '{msgid}' is used both with and without a plural form; give one of them a context")
            else:
                self.errors.append(f"{where}: '{msgid}' has two plural forms ('{entry.plural}', '{plural}')")
        entry.references.add(reference)
        if comment and comment not in entry.comments:
            entry.comments.append(comment)
        entry.flags |= flags


def translator_comment(all_tokens: list[Token], line: int) -> str | None:
    """A TRANSLATORS comment ending on the line above `line`, or on that line itself."""
    found = None
    for t in all_tokens:
        if t.kind != "comment":
            continue
        if t.end_line > line:
            break
        if t.end_line >= line - 1:
            m = TRANSLATORS.search(t.value)
            if m:
                found = " ".join(m.group(1).split())
    return found


def iter_source_files():
    for directory, subdirs, files in os.walk(SOURCE_DIR):
        subdirs[:] = sorted(d for d in subdirs if os.path.normcase(os.path.join(directory, d)) not in EXCLUDED_DIRS)
        for name in sorted(files):
            if name.endswith(SOURCE_EXTENSIONS):
                yield os.path.join(directory, name)


def relative(path: str) -> str:
    try:
        return os.path.relpath(path, ROOT).replace(os.sep, "/")
    except ValueError:   # Another drive (Windows)
        return path.replace(os.sep, "/")


def read_source(path: str) -> str:
    with open(path, encoding="utf-8", errors="replace") as handle:
        return handle.read()


TEXT_PARAMETER_SKIP = {"const", "SceneryEditorX", "::", "&", "&&"}


def find_text_functions(files: dict) -> dict:
    """Functions declared with Text parameters: name -> {"positions": set, "comment": str | None, "where": [...]}"""
    functions: dict = {}
    for path, (all_tokens, tokens) in files.items():
        for k in range(len(tokens) - 1):
            t = tokens[k]
            if t.kind != "id" or tokens[k + 1].value != "(" or t.value in ("Text", "if", "while", "for", "switch", "return", "sizeof"):
                continue
            close = matching_paren(tokens, k + 1)
            if close >= len(tokens):
                continue
            positions = set()
            for index, (start, end) in enumerate(split_arguments(tokens, k + 1, close)):
                words = [x for x in tokens[start:end] if x.value not in TEXT_PARAMETER_SKIP]
                if not words or words[0].value != "Text" or words[0].kind != "id":
                    continue
                rest = words[1:]
                # "Text", "Text label", "Text label = {}" are parameters; "Text(...)" or "Text{...}" are expressions
                if not rest or (rest[0].kind == "id" and (len(rest) == 1 or rest[1].value == "=")) or rest[0].value == "=":
                    positions.add(index)
            if not positions:
                continue
            info = functions.setdefault(t.value, {"positions": set(), "comment": None, "where": []})
            info["positions"] |= positions
            info["where"].append(f"{relative(path)}:{t.line}")
            comment = translator_comment(all_tokens, t.line)
            if comment and not info["comment"]:
                info["comment"] = comment
    return functions


FNV_OFFSET = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3


def fnv1a64(data: bytes) -> int:
    value = FNV_OFFSET
    for byte in data:
        value = ((value ^ byte) * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return value


def message_key(context: str | None, msgid: str) -> int:
    """The 64-bit key the editor looks a message up by (I18n::HashKey in message.h): msgctxt '\\x04' msgid, FNV-1a."""
    data = (context.encode("utf-8") + b"\x04" if context is not None else b"") + msgid.encode("utf-8")
    return fnv1a64(data)


def check_key_collisions(catalog: "Catalog") -> None:
    """Two different messages with the same key would leave the second one English: refuse them at extraction."""
    seen = {}
    for key in sorted(catalog.entries, key=lambda k: (k[0] or "", k[1])):
        value = message_key(*key)
        other = seen.get(value)
        if other is not None and other != key:
            catalog.errors.append(f"'{key[1]}' and '{other[1]}' have the same message key {value:#018x}: reword one of them")
        seen.setdefault(value, key)


def qualifier_of(tokens: list[Token], index: int) -> str | None:
    if index >= 2 and tokens[index - 1].value == "::" and tokens[index - 2].kind == "id":
        return tokens[index - 2].value
    return None


# Calls qualified with these never refer to SEDX's own helpers
FOREIGN_QUALIFIERS = {"ImGui", "std", "SDL", "pfd", "fmt"}


def extract(verbose: bool = False) -> Catalog:
    files = {}
    for path in iter_source_files():
        try:
            all_tokens = tokenize(read_source(path))
        except ToolError as error:
            raise ToolError(f"{relative(path)}: {error}")
        files[path] = (all_tokens, code_tokens(all_tokens))

    text_functions = find_text_functions(files)
    if verbose:
        print("Functions with Text parameters (their literal arguments are extracted):")
        for name in sorted(text_functions):
            info = text_functions[name]
            print(f"  {name} {sorted(info['positions'])}  ({info['where'][0]})")

    catalog = Catalog()
    for path, (all_tokens, tokens) in files.items():
        reference = relative(path)
        consumed = set()     # Token indices of Ctx calls already recorded by an enclosing call

        def ctx_call(start, end):
            """(context, text, ctx token index) when tokens[start:end] is exactly Ctx("c", "t")."""
            if end - start >= 4 and tokens[start].value == "Ctx" and tokens[start + 1].value == "(":
                close = matching_paren(tokens, start + 1)
                if close == end - 1:
                    args = split_arguments(tokens, start + 1, close)
                    if len(args) == 2:
                        context = literal_value(tokens, *args[0])
                        text = literal_value(tokens, *args[1])
                        if context is not None and text is not None:
                            return context, text, start
            return None

        for k in range(len(tokens) - 1):
            t = tokens[k]
            if t.kind != "id" or tokens[k + 1].value != "(":
                continue
            name = t.value
            if qualifier_of(tokens, k) in FOREIGN_QUALIFIERS:
                continue
            spec = KEYWORDS.get(name)
            positions = None
            if spec is None:
                info = text_functions.get(name)
                if info is None:
                    continue
                positions = sorted(info["positions"])
            close = matching_paren(tokens, k + 1)
            if close >= len(tokens):
                continue
            args = split_arguments(tokens, k + 1, close)
            comment = translator_comment(all_tokens, t.line)
            where = f"{reference}:{t.line}"

            if spec is not None:
                indices = spec["messages"]
                if len(args) < len(indices):
                    continue
                flags = {"python-brace-format"} if spec.get("format") else set()
                first = args[indices[0]]
                ctx = ctx_call(*first)
                context = None
                if ctx:
                    context, msgid, ctx_index = ctx
                    consumed.add(ctx_index)
                else:
                    msgid = literal_value(tokens, *first)
                if msgid is None:
                    continue     # A Msg variable: marked where it was made (TrNoop / Ctx)
                plural = None
                if spec.get("plural"):
                    plural = literal_value(tokens, *args[indices[1]])
                    if plural is None:
                        catalog.errors.append(f"{where}: the plural of {name}() must be a string literal")
                        continue
                    plural = visible(plural)
                catalog.add(visible(msgid), context, plural, reference, comment, flags, where)
            else:
                default_comment = text_functions[name]["comment"]
                for position in positions:
                    if position >= len(args):
                        continue
                    if ctx_call(*args[position]):
                        continue     # Recorded as a Ctx call
                    msgid = literal_value(tokens, *args[position])
                    if msgid is None:
                        continue     # Run-time text, shown as it is
                    catalog.add(visible(msgid), None, None, reference, comment or default_comment, set(), where)

        # Ctx calls on their own (tables, Text parameters, Tr(Ctx(...)))
        for k in range(len(tokens) - 1):
            if tokens[k].value != "Ctx" or tokens[k].kind != "id" or tokens[k + 1].value != "(" or k in consumed:
                continue
            if qualifier_of(tokens, k) in FOREIGN_QUALIFIERS:
                continue
            close = matching_paren(tokens, k + 1)
            ctx = ctx_call(k, close + 1)
            if not ctx:
                continue
            context, msgid, _ = ctx
            catalog.add(visible(msgid), context, None, reference, translator_comment(all_tokens, tokens[k].line), set(),
                        f"{reference}:{tokens[k].line}")
    check_key_collisions(catalog)
    return catalog


# -------------------------------------------------------
# PO files
# -------------------------------------------------------

@dataclass
class PoEntry:
    msgid: str = ""
    context: str | None = None
    plural: str | None = None
    msgstr: list = field(default_factory=list)
    translator_comments: list = field(default_factory=list)
    extracted_comments: list = field(default_factory=list)
    references: list = field(default_factory=list)
    flags: list = field(default_factory=list)
    obsolete: bool = False
    line: int = 0

    def key(self):
        return (self.context, self.msgid)

    @property
    def fuzzy(self):
        return "fuzzy" in self.flags

    def translated(self):
        return not self.fuzzy and bool(self.msgstr) and all(self.msgstr)


@dataclass
class PoFile:
    header_comments: list = field(default_factory=list)
    header: list = field(default_factory=list)     # (name, value)
    header_flags: list = field(default_factory=list)
    entries: list = field(default_factory=list)
    errors: list = field(default_factory=list)

    def get_header(self, name):
        for key, value in self.header:
            if key.lower() == name.lower():
                return value
        return None


PO_ESCAPES = {"n": "\n", "t": "\t", "r": "\r", "a": "\a", "b": "\b", "f": "\f", "v": "\v", "\\": "\\", "\"": "\"", "'": "'", "?": "?"}


def po_unescape(text, errors, line):
    out = []
    i = 0
    while i < len(text):
        c = text[i]
        if c != "\\":
            out.append(c)
            i += 1
            continue
        i += 1
        if i >= len(text):
            errors.append(f"line {line}: a string ends with a backslash")
            break
        e = text[i]
        if e in PO_ESCAPES:
            out.append(PO_ESCAPES[e])
            i += 1
        elif e == "x":
            j = i + 1
            while j < len(text) and j < i + 3 and text[j] in "0123456789abcdefABCDEF":
                j += 1
            out.append(chr(int(text[i + 1:j] or "0", 16)))
            i = j
        elif e in "01234567":
            j = i
            while j < len(text) and j < i + 3 and text[j] in "01234567":
                j += 1
            out.append(chr(int(text[i:j], 8)))
            i = j
        else:
            errors.append(f"line {line}: unknown escape \\{e}")
            out.append(e)
            i += 1
    return "".join(out)


def parse_po(text: str) -> PoFile:
    po = PoFile()
    if text.startswith("﻿"):
        text = text[1:]
    entry = PoEntry()
    current = None      # (field, index)
    seen_id = seen_str = has_content = False
    header_seen = []

    def finish():
        nonlocal entry, current, seen_id, seen_str, has_content
        if seen_id:
            if not seen_str:
                po.errors.append(f"line {entry.line}: msgid without msgstr")
            if not entry.msgstr:
                entry.msgstr = [""]
            if entry.context is None and entry.msgid == "" and not entry.obsolete:
                if header_seen:
                    po.errors.append(f"line {entry.line}: a second header entry")
                header_seen.append(True)
                po.header_comments = entry.translator_comments
                po.header_flags = entry.flags
                for header_line in entry.msgstr[0].split("\n"):
                    if ":" in header_line:
                        name, value = header_line.split(":", 1)
                        po.header.append((name.strip(), value.strip()))
            else:
                po.entries.append(entry)
        entry = PoEntry()
        current = None
        seen_id = seen_str = has_content = False

    for number, raw in enumerate(text.split("\n"), start=1):
        line = raw.rstrip("\r").strip()
        obsolete = False
        if line.startswith("#~"):
            obsolete = True
            line = line[2:].strip()
            if not line:
                continue
        if not line:
            finish()
            continue
        if line.startswith("#") and not obsolete:
            if seen_str:
                finish()
            if seen_id:
                po.errors.append(f"line {number}: a comment inside an entry")
                continue
            if line.startswith("#."):
                entry.extracted_comments.append(line[2:].strip())
            elif line.startswith("#:"):
                entry.references.extend(line[2:].split())
            elif line.startswith("#,"):
                entry.flags.extend(f.strip() for f in line[2:].split(",") if f.strip())
            elif line.startswith("#|"):
                pass
            else:
                entry.translator_comments.append(line[1:].strip())
            has_content = True
            continue
        if line.startswith("\""):
            if current is None:
                po.errors.append(f"line {number}: a string outside an entry")
                continue
            value = read_po_string(line, po.errors, number)
            fieldname, index = current
            if fieldname == "msgstr":
                entry.msgstr[index] += value
            else:
                setattr(entry, fieldname, (getattr(entry, fieldname) or "") + value)
            continue
        keyword, _, rest = line.partition(" ")
        if not rest and "\"" in keyword:
            keyword, rest = keyword[:keyword.index("\"")], keyword[keyword.index("\""):]
        rest = rest.strip()
        if keyword in ("msgctxt", "msgid") and seen_str:
            finish()
        if obsolete:
            entry.obsolete = True
        if keyword == "msgctxt":
            entry.context = read_po_string(rest, po.errors, number)
            current = ("context", 0)
        elif keyword == "msgid":
            if seen_id:
                po.errors.append(f"line {number}: a second msgid in one entry")
            seen_id = True
            entry.line = number
            entry.msgid = read_po_string(rest, po.errors, number)
            current = ("msgid", 0)
        elif keyword == "msgid_plural":
            entry.plural = read_po_string(rest, po.errors, number)
            current = ("plural", 0)
        elif keyword == "msgstr" or re.fullmatch(r"msgstr\[\d+\]", keyword):
            if not seen_id:
                po.errors.append(f"line {number}: msgstr without a msgid")
            seen_str = True
            index = 0 if keyword == "msgstr" else int(keyword[7:-1])
            if keyword == "msgstr" and entry.plural is not None:
                po.errors.append(f"line {number}: a plural entry needs msgstr[n]")
            if keyword != "msgstr" and index != len(entry.msgstr):
                po.errors.append(f"line {number}: {keyword} out of order")
            while len(entry.msgstr) <= index:
                entry.msgstr.append("")
            entry.msgstr[index] = read_po_string(rest, po.errors, number)
            current = ("msgstr", index)
        else:
            po.errors.append(f"line {number}: unknown keyword '{keyword[:32]}'")
            current = None
    finish()
    return po


def read_po_string(rest, errors, number):
    if len(rest) < 2 or not rest.startswith("\"") or not rest.endswith("\""):
        errors.append(f"line {number}: expected a quoted string")
        return ""
    inside = rest[1:-1]
    i = 0
    while i < len(inside):
        if inside[i] == "\\":
            i += 2
            continue
        if inside[i] == "\"":
            errors.append(f"line {number}: unescaped quote in a string")
            return ""
        i += 1
    return po_unescape(inside, errors, number)


def po_escape(text: str) -> str:
    out = []
    for c in text:
        if c == "\\":
            out.append("\\\\")
        elif c == "\"":
            out.append("\\\"")
        elif c == "\n":
            out.append("\\n")
        elif c == "\t":
            out.append("\\t")
        elif c == "\r":
            out.append("\\r")
        elif ord(c) < 0x20:
            out.append("\\x%02x" % ord(c))
        else:
            out.append(c)
    return "".join(out)


def po_string(keyword: str, value: str) -> str:
    """keyword "value", split after each line break like msgcat --no-wrap (the format Weblate is set to write)."""
    segments = []
    rest = value
    while "\n" in rest[:-1]:
        cut = rest.index("\n") + 1
        segments.append(rest[:cut])
        rest = rest[cut:]
    if segments:
        if rest:
            segments.append(rest)
        return "\n".join([f'{keyword} ""'] + [f'"{po_escape(s)}"' for s in segments])
    return f'{keyword} "{po_escape(value)}"'


def format_header(fields, comments, flags) -> str:
    lines = [("# " + c) if c else "#" for c in comments]
    if flags:
        lines.append("#, " + ", ".join(flags))
    lines.append('msgid ""')
    lines.append('msgstr ""')
    for name, value in fields:
        lines.append(f'"{po_escape(name)}: {po_escape(value)}\\n"')
    return "\n".join(lines)


def format_entry(entry: PoEntry, nplurals: int = 2) -> str:
    lines = []
    prefix = "#~ " if entry.obsolete else ""
    for comment in entry.translator_comments:
        lines.append(("# " + comment) if comment else "#")
    if not entry.obsolete:
        for comment in entry.extracted_comments:
            lines.append("#. " + comment)
        for reference in entry.references:
            lines.append("#: " + reference)
        if entry.flags:
            lines.append("#, " + ", ".join(entry.flags))
    body = []
    if entry.context is not None:
        body.append(po_string("msgctxt", entry.context))
    body.append(po_string("msgid", entry.msgid))
    if entry.plural is not None:
        body.append(po_string("msgid_plural", entry.plural))
        forms = list(entry.msgstr) or []
        while len(forms) < nplurals:
            forms.append("")
        for index, form in enumerate(forms):
            body.append(po_string(f"msgstr[{index}]", form))
    else:
        body.append(po_string("msgstr", entry.msgstr[0] if entry.msgstr else ""))
    for chunk in body:
        for text_line in chunk.split("\n"):
            lines.append(prefix + text_line)
    return "\n".join(lines)


def read_text(path):
    with open(path, encoding="utf-8", newline="") as handle:
        return handle.read().replace("\r\n", "\n")


def write_text(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)


# -------------------------------------------------------
# Languages (read from languages.cpp, the one copy of the table)
# -------------------------------------------------------

LANGUAGE_ROW = re.compile(r'\{\s*"([A-Za-z_]+)",\s*"([^"]*)",\s*u8"((?:[^"\\]|\\.)*)",\s*Script::(\w+),\s*(\d+),\s*"([^"]*)"')


def load_languages() -> dict:
    languages = {}
    for match in LANGUAGE_ROW.finditer(read_text(LANGUAGES_CPP)):
        tag, english, native, script, nplurals, forms = match.groups()
        languages[tag] = {"english": english, "native": decode_cpp_escapes(native), "script": script,
                          "nplurals": int(nplurals), "plural_forms": forms}
    if len(languages) < 5:
        raise ToolError(f"couldn't read the language table in {relative(LANGUAGES_CPP)}")
    return languages


def normalize_tag(tag: str) -> str:
    parts = re.split(r"[-_]", tag.split(".")[0].split("@")[0])
    out = [parts[0].lower()]
    for part in parts[1:]:
        if len(part) == 4 and part.isalpha():
            out.append(part.title())
        elif len(part) == 2 or (len(part) == 3 and part.isdigit()):
            out.append(part.upper())
        else:
            out.append(part.lower())
    return "_".join(p for p in out if p)


# -------------------------------------------------------
# Template
# -------------------------------------------------------

def template_text(catalog: Catalog) -> str:
    header = format_header(
        HEADER_FIELDS + [("Language", ""), ("MIME-Version", "1.0"), ("Content-Type", "text/plain; charset=UTF-8"),
                         ("Content-Transfer-Encoding", "8bit"), ("Plural-Forms", "nplurals=INTEGER; plural=EXPRESSION;")],
        ["Scenery Editor X user interface strings.", "Copyright (C) 2026 Coalition of Freeware Developers",
         "This file is distributed under the same license as Scenery Editor X (MIT).",
         "Generated by tools/i18n/sedx_i18n.py extract from the source code: don't edit it by hand.", ""],
        ["fuzzy"])
    entries = sorted(catalog.entries.values(), key=lambda e: (min(e.references), e.msgid, e.context or ""))
    blocks = [header]
    for e in entries:
        po_entry = PoEntry(msgid=e.msgid, context=e.context, plural=e.plural, msgstr=["", ""] if e.plural is not None else [""],
                           extracted_comments=list(e.comments), references=sorted(e.references), flags=sorted(e.flags))
        blocks.append(format_entry(po_entry))
    return "\n\n".join(blocks) + "\n"


def command_extract(args) -> int:
    catalog = extract(args.verbose)
    for error in catalog.errors:
        print("error: " + error, file=sys.stderr)
    text = template_text(catalog)
    current = read_text(TEMPLATE) if os.path.exists(TEMPLATE) else None
    if args.check:
        if current != text:
            print(f"{relative(TEMPLATE)} is out of date: run python -I tools/i18n/sedx_i18n.py extract", file=sys.stderr)
            return 1
        print(f"{relative(TEMPLATE)} is up to date ({len(catalog.entries)} messages)")
        return 1 if catalog.errors else 0
    os.makedirs(LOCALE_DIR, exist_ok=True)
    if current != text:
        write_text(TEMPLATE, text)
        print(f"Wrote {relative(TEMPLATE)}: {len(catalog.entries)} messages")
    else:
        print(f"{relative(TEMPLATE)} unchanged ({len(catalog.entries)} messages)")
    return 1 if catalog.errors else 0


# -------------------------------------------------------
# update / new
# -------------------------------------------------------

def catalog_files(paths):
    if paths:
        return [os.path.abspath(p) for p in paths]
    if not os.path.isdir(LOCALE_DIR):
        return []
    return [os.path.join(LOCALE_DIR, n) for n in sorted(os.listdir(LOCALE_DIR)) if n.endswith(".po")]


def merge_catalog(po: PoFile, template: PoFile, nplurals: int) -> PoFile:
    existing = {e.key(): e for e in po.entries if not e.obsolete}
    obsolete = {e.key(): e for e in po.entries if e.obsolete}
    merged = PoFile(header_comments=po.header_comments, header=po.header, header_flags=[f for f in po.header_flags if f != "fuzzy"])
    used = set()
    for t in template.entries:
        old = existing.get(t.key()) or obsolete.get(t.key())
        entry = PoEntry(msgid=t.msgid, context=t.context, plural=t.plural, extracted_comments=list(t.extracted_comments),
                        references=list(t.references), flags=[f for f in t.flags if f != "fuzzy"])
        if old is not None:
            used.add(t.key())
            entry.translator_comments = list(old.translator_comments)
            entry.msgstr = list(old.msgstr)
            if old.fuzzy or (old.plural or None) != (t.plural or None):
                entry.flags = sorted(set(entry.flags) | {"fuzzy"})
        if t.plural is not None:
            entry.msgstr = (entry.msgstr + [""] * nplurals)[:nplurals]
        else:
            entry.msgstr = [entry.msgstr[0] if entry.msgstr else ""]
        merged.entries.append(entry)
    for key, old in list(existing.items()) + list(obsolete.items()):
        if key in used or not any(old.msgstr):
            continue
        old.obsolete = True
        merged.entries.append(old)
    return merged


def format_po(po: PoFile, nplurals: int) -> str:
    blocks = [format_header(po.header, po.header_comments, po.header_flags)]
    blocks += [format_entry(e, nplurals) for e in po.entries]
    return "\n\n".join(blocks) + "\n"


def load_template() -> PoFile:
    if not os.path.exists(TEMPLATE):
        raise ToolError(f"{relative(TEMPLATE)} doesn't exist: run extract first")
    template = parse_po(read_text(TEMPLATE))
    if template.errors:
        raise ToolError(f"{relative(TEMPLATE)}: " + "; ".join(template.errors))
    return template


def command_update(args) -> int:
    template = load_template()
    languages = load_languages()
    for path in catalog_files(args.files):
        po = parse_po(read_text(path))
        if po.errors:
            print(f"{relative(path)}: not updated, fix it first: " + "; ".join(po.errors), file=sys.stderr)
            return 1
        tag = normalize_tag(po.get_header("Language") or os.path.splitext(os.path.basename(path))[0])
        nplurals = languages.get(tag, {}).get("nplurals", 2)
        text = format_po(merge_catalog(po, template, nplurals), nplurals)
        if text != read_text(path):
            write_text(path, text)
            print(f"Updated {relative(path)}")
        else:
            print(f"{relative(path)} already matches the template")
    return 0


def command_new(args) -> int:
    template = load_template()
    languages = load_languages()
    tag = normalize_tag(args.tag)
    if tag not in languages:
        raise ToolError(f"'{tag}' isn't in the language table ({relative(LANGUAGES_CPP)}): add it there first")
    language = languages[tag]
    path = os.path.join(LOCALE_DIR, tag + ".po")
    if os.path.exists(path):
        raise ToolError(f"{relative(path)} exists already")
    po = PoFile(
        header_comments=[f"{language['english']} translation of Scenery Editor X.", "Copyright (C) 2026 Coalition of Freeware Developers",
                         "This file is distributed under the same license as Scenery Editor X (MIT).", ""],
        header=HEADER_FIELDS + [("Language", tag), ("MIME-Version", "1.0"), ("Content-Type", "text/plain; charset=UTF-8"),
                                ("Content-Transfer-Encoding", "8bit"), ("Plural-Forms", language["plural_forms"])])
    merged = merge_catalog(po, template, language["nplurals"])
    write_text(path, format_po(merged, language["nplurals"]))
    print(f"Created {relative(path)} ({len(merged.entries)} messages to translate)")
    return 0


# -------------------------------------------------------
# check
# -------------------------------------------------------

def brace_fields(text: str):
    """[(index, spec)] of std::format replacement fields, or None when the pattern is malformed."""
    fields, automatic, manual, i, next_index = [], False, False, 0, 0
    while i < len(text):
        c = text[i]
        if c == "{":
            if text.startswith("{{", i):
                i += 2
                continue
            close = text.find("}", i)
            if close < 0:
                return None
            inside = text[i + 1:close]
            if "{" in inside:
                return None
            ident, _, spec = inside.partition(":")
            if ident == "":
                automatic = True
                index = next_index
                next_index += 1
            elif ident.isdigit():
                manual = True
                index = int(ident)
            else:
                return None
            if automatic and manual:
                return None
            fields.append((index, spec))
            i = close + 1
            continue
        if c == "}":
            if text.startswith("}}", i):
                i += 2
                continue
            return None
        i += 1
    return fields


def check_catalog(path, template_keys, languages):
    errors, warnings = [], []
    name = relative(path)
    po = parse_po(read_text(path))
    errors += [f"{name}: {e}" for e in po.errors]

    file_tag = normalize_tag(os.path.splitext(os.path.basename(path))[0])
    language_header = po.get_header("Language")
    if not language_header:
        errors.append(f"{name}: the header has no Language")
    elif normalize_tag(language_header) != file_tag:
        errors.append(f"{name}: the header says Language: {language_header}, the file name says {file_tag}")
    language = languages.get(file_tag)
    if language is None:
        errors.append(f"{name}: '{file_tag}' isn't in the language table ({relative(LANGUAGES_CPP)})")
    forms = po.get_header("Plural-Forms")
    nplurals = language["nplurals"] if language else 2
    if not forms:
        errors.append(f"{name}: the header has no Plural-Forms")
    else:
        m = re.search(r"nplurals\s*=\s*(\d+)", forms)
        if not m:
            errors.append(f"{name}: Plural-Forms has no nplurals")
        elif language and int(m.group(1)) != language["nplurals"]:
            errors.append(f"{name}: Plural-Forms has nplurals={m.group(1)}, {language['english']} has {language['nplurals']}")

    seen = set()
    stats = {"total": 0, "translated": 0, "fuzzy": 0}
    for e in po.entries:
        if e.obsolete:
            continue
        where = f"{name}:{e.line}"
        if e.key() in seen:
            errors.append(f"{where}: '{e.msgid}' is in the catalog twice")
        seen.add(e.key())
        stats["total"] += 1
        if e.fuzzy:
            stats["fuzzy"] += 1
        if template_keys is not None and e.key() not in template_keys:
            warnings.append(f"{where}: '{e.msgid}' isn't in the template any more (run update)")
        if e.plural is not None and len(e.msgstr) != nplurals:
            errors.append(f"{where}: '{e.msgid}' has {len(e.msgstr)} plural forms, the language has {nplurals}")
        if not e.translated():
            continue
        stats["translated"] += 1
        english = [e.msgid] + ([e.plural] if e.plural is not None else [])
        for index, translation in enumerate(e.msgstr):
            source = e.msgid if (e.plural is None or index == 0) else e.plural
            if "##" in translation:
                errors.append(f"{where}: the translation of '{e.msgid}' contains '##' (an ImGui ID marker)")
            if translation[:1].isspace() != source[:1].isspace() or translation[-1:].isspace() != source[-1:].isspace():
                errors.append(f"{where}: the translation of '{e.msgid}' starts or ends with different spacing")
            if translation.count("\n") != source.count("\n") and e.plural is None:
                warnings.append(f"{where}: the translation of '{e.msgid}' has {translation.count(chr(10))} line breaks, the English {source.count(chr(10))}")
            if "python-brace-format" in e.flags:
                wanted = {}
                for text in english:
                    parsed = brace_fields(text)
                    for index_, spec in parsed or []:
                        wanted[index_] = spec
                got = brace_fields(translation)
                if got is None:
                    errors.append(f"{where}: the translation of '{e.msgid}' has a malformed {{}} field")
                    continue
                for index_, spec in got:
                    if index_ not in wanted:
                        errors.append(f"{where}: the translation of '{e.msgid}' uses field {{{index_}}}, which the English doesn't have")
                    elif spec != wanted[index_]:
                        errors.append(f"{where}: the translation of '{e.msgid}' formats field {index_} as '{spec}', the English as '{wanted[index_]}'")
                if e.plural is None:
                    missing = set(wanted) - {i for i, _ in got}
                    if missing:
                        errors.append(f"{where}: the translation of '{e.msgid}' leaves out field(s) {sorted(missing)}")
            for flag in e.flags:
                m = re.fullmatch(r"max-length:(\d+)", flag)
                if m and len(translation) > int(m.group(1)):
                    errors.append(f"{where}: the translation of '{e.msgid}' is longer than {m.group(1)} characters")
    return errors, warnings, stats, file_tag


def command_check(args) -> int:
    languages = load_languages()
    template_keys = None
    errors, warnings = [], []
    if os.path.exists(TEMPLATE):
        template = parse_po(read_text(TEMPLATE))
        errors += [f"{relative(TEMPLATE)}: {e}" for e in template.errors]
        template_keys = {e.key() for e in template.entries}
        for e in template.entries:
            if "##" in e.msgid:
                errors.append(f"{relative(TEMPLATE)}:{e.line}: msgid '{e.msgid}' contains '##'")
    rows = []
    for path in catalog_files(args.files):
        e, w, stats, tag = check_catalog(path, template_keys, languages)
        errors += e
        warnings += w
        rows.append((tag, stats))
    for w in warnings:
        print("warning: " + w)
    for e in errors:
        print("error: " + e, file=sys.stderr)
    print_stats(rows, languages, False)
    if errors:
        print(f"{len(errors)} error(s)", file=sys.stderr)
        return 1
    return 0


def print_stats(rows, languages, markdown):
    if not rows:
        print("No catalogs.")
        return
    if markdown:
        print("| Language | Tag | Translated | Fuzzy | Messages | Complete |")
        print("|---|---|---:|---:|---:|---:|")
    for tag, stats in rows:
        info = languages.get(tag, {})
        percent = 100.0 * stats["translated"] / stats["total"] if stats["total"] else 100.0
        name = info.get("english", tag)
        if markdown:
            print(f"| {name} ({info.get('native', tag)}) | `{tag}` | {stats['translated']} | {stats['fuzzy']} | {stats['total']} | {percent:.0f}% |")
        else:
            print(f"  {tag:8} {name:24} {stats['translated']:5} of {stats['total']:5} translated ({percent:5.1f}%), {stats['fuzzy']} fuzzy")


def command_stats(args) -> int:
    languages = load_languages()
    rows = []
    for path in catalog_files([]):
        _, _, stats, tag = check_catalog(path, None, languages)
        rows.append((tag, stats))
    print_stats(rows, languages, args.markdown)
    return 0


# -------------------------------------------------------
# lint
# -------------------------------------------------------

# Dear ImGui functions and the arguments that are shown to the user
LINT_CALLS = {
    "Begin": [0], "BeginMenu": [0], "MenuItem": [0], "Button": [0], "SmallButton": [0], "Checkbox": [0], "RadioButton": [0],
    "Selectable": [0], "TreeNode": [0], "TreeNodeEx": [0], "CollapsingHeader": [0], "BeginTabItem": [0], "TabItemButton": [0],
    "BeginPopupModal": [0], "Text": [0], "TextUnformatted": [0], "TextDisabled": [0], "TextWrapped": [0], "TextColored": [1],
    "LabelText": [0, 1], "BulletText": [0], "SeparatorText": [0], "SetTooltip": [0], "SetItemTooltip": [0],
    "TableSetupColumn": [0], "InputTextWithHint": [1], "BeginCombo": [0, 1], "Combo": [0], "SliderFloat": [0], "SliderInt": [0],
    "DragFloat": [0], "DragInt": [0], "InputText": [0], "InputFloat": [0], "InputInt": [0], "ColorEdit3": [0], "ColorEdit4": [0],
    "TextLink": [0], "ProgressBar": [2],
}
LINT_FILES = (os.path.join("source", "Editor"), os.path.join("source", "SceneryEditorX", "renderer", "ui"))
PRINTF = re.compile(r"%[-+ #0]*(\d+|\*)?(\.(\d+|\*))?[hlLqjzt]*[diouxXeEfFgGaAcsp%]")


def needs_translation(text: str) -> bool:
    shown = visible(text)
    shown = PRINTF.sub("", shown)
    shown = re.sub(r"\{[^{}]*\}", "", shown)
    return sum(1 for c in shown if c.isascii() and c.isalpha()) >= 2


def lint_findings():
    findings = []
    for path in iter_source_files():
        rel = relative(path)
        if not any(rel.startswith(prefix.replace(os.sep, "/")) for prefix in LINT_FILES):
            continue
        tokens = code_tokens(tokenize(read_source(path)))
        for k in range(len(tokens) - 1):
            t = tokens[k]
            if t.kind != "id" or tokens[k + 1].value != "(":
                continue
            positions = None
            if qualifier_of(tokens, k) == "ImGui" and t.value in LINT_CALLS:
                positions = LINT_CALLS[t.value]
            elif t.value == "AddText" and k >= 1 and tokens[k - 1].value in ("->", "."):
                positions = [2]     # ImDrawList::AddText(pos, color, text)
            if positions is None:
                continue
            close = matching_paren(tokens, k + 1)
            args = split_arguments(tokens, k + 1, close)
            for position in positions:
                if position >= len(args):
                    continue
                text = literal_value(tokens, *args[position])
                if text is not None and needs_translation(text):
                    findings.append((rel, t.line, t.value, text))
    return findings


def is_literal_ternary(tokens: list[Token], start: int, end: int) -> bool:
    """True for an argument that is a ?: whose two branches are both plain string literals: cond ? "A" : "B"."""
    depth = 0
    for k in range(start, end):
        v = tokens[k].value if tokens[k].kind == "punct" else ""
        if v in ("(", "[", "{"):
            depth += 1
        elif v in (")", "]", "}"):
            depth -= 1
        elif v == "?" and depth == 0:
            colon = k + 1
            while colon < end and tokens[colon].kind == "str":
                colon += 1
            if colon == k + 1 or colon >= end or tokens[colon].value != ":":
                return False
            rest = range(colon + 1, end)
            return len(rest) > 0 and all(tokens[i].kind == "str" for i in rest)
    return False


def ternary_findings():
    """
    Calls that pass cond ? "A" : "B" to a Text parameter. Two literals of different lengths make a const char*, which
    Text shows as it is (silently English); equal lengths don't compile. Write cond ? Text("A") : Text("B").
    """
    files = {}
    for path in iter_source_files():
        all_tokens = tokenize(read_source(path))
        files[path] = (all_tokens, code_tokens(all_tokens))
    functions = find_text_functions(files)
    findings = []
    for path, (_, tokens) in files.items():
        for k in range(len(tokens) - 1):
            t = tokens[k]
            if t.kind != "id" or t.value not in functions or tokens[k + 1].value != "(":
                continue
            if qualifier_of(tokens, k) in FOREIGN_QUALIFIERS:
                continue
            close = matching_paren(tokens, k + 1)
            args = split_arguments(tokens, k + 1, close)
            for position in sorted(functions[t.value]["positions"]):
                if position < len(args) and is_literal_ternary(tokens, *args[position]):
                    findings.append((relative(path), t.line, t.value))
    return findings


def baseline_key(rel, text):
    return f"{rel}\t{po_escape(text)}"


def command_lint(args) -> int:
    findings = lint_findings()
    if args.update_baseline:
        lines = sorted(baseline_key(rel, text) for rel, _, _, text in findings)
        write_text(BASELINE, "# Dear ImGui calls with English literals not converted yet (tools/i18n/sedx_i18n.py lint).\n"
                             "# Convert them with Tr / TrL (source/SceneryEditorX/i18n/tr.h) and run lint --update-baseline:\n"
                             "# the list only shrinks. path<TAB>text\n" + "\n".join(lines) + "\n")
        print(f"Wrote {relative(BASELINE)}: {len(lines)} sites")
        return 0
    allowed = {}
    if os.path.exists(BASELINE):
        for line in read_text(BASELINE).split("\n"):
            if line and not line.startswith("#"):
                allowed[line] = allowed.get(line, 0) + 1
    new = []
    for rel, line, function, text in findings:
        key = baseline_key(rel, text)
        if allowed.get(key, 0) > 0:
            allowed[key] -= 1
        else:
            new.append((rel, line, function, text))
    fixed = sum(count for count in allowed.values() if count > 0)
    for rel, line, function, text in new:
        print(f"error: {rel}:{line}: ImGui::{function}(\"{po_escape(text)}\") isn't translated: use TrL() for labels, Tr() for "
              f"display text (source/SceneryEditorX/i18n/tr.h)", file=sys.stderr)
    if fixed:
        print(f"note: {fixed} baseline site(s) are translated now: run lint --update-baseline to shrink {relative(BASELINE)}")
    print(f"lint: {len(findings)} untranslated ImGui literal(s), {len(findings) - len(new)} in the baseline, {len(new)} new")

    # Never baselined: there are none, and each one is text that silently stays English
    ternaries = ternary_findings()
    for rel, line, function in ternaries:
        print(f"error: {rel}:{line}: {function}() is given cond ? \"...\" : \"...\", which stays English: write "
              f"cond ? Text(\"...\") : Text(\"...\") (source/SceneryEditorX/i18n/tr.h)", file=sys.stderr)
    print(f"lint: {len(ternaries)} ?: of literals passed to Text parameters")
    return 1 if new or ternaries else 0


def describe_changes(old_text: str, new_text: str, limit: int = 60) -> str:
    """Markdown list of the messages added to and removed from a template (the localization workflow's summary)."""
    old = {e.key(): e for e in parse_po(old_text).entries} if old_text else {}
    new = {e.key(): e for e in parse_po(new_text).entries}
    added = [new[k] for k in new if k not in old]
    removed = [old[k] for k in old if k not in new]
    lines = ["### Translatable text", "",
             f"{len(added)} new and {len(removed)} removed message(s); the template has {len(new)}.", ""]

    def table(title, entries):
        if not entries:
            return
        lines.extend([f"**{title}**", "", "| Text | Context | Where |", "|---|---|---|"])
        for e in sorted(entries, key=lambda x: (x.references[:1], x.msgid))[:limit]:
            text = e.msgid.replace("|", "\\|").replace("\n", "\\n")
            if e.plural is not None:
                text += " / " + e.plural.replace("|", "\\|")
            lines.append(f"| {text} | {e.context or ''} | {', '.join(e.references)} |")
        if len(entries) > limit:
            lines.append(f"| ... and {len(entries) - limit} more | | |")
        lines.append("")

    table("New (translated on Weblate after the merge; English until then)", added)
    table("Removed (their translations become obsolete entries)", removed)
    return "\n".join(lines) + "\n"


def command_changes(args) -> int:
    old_text = read_text(args.old) if args.old and os.path.exists(args.old) else ""
    print(describe_changes(old_text, read_text(TEMPLATE)), end="")
    return 0


def command_ci(args) -> int:
    status = 0
    print("== extract --check")
    status |= command_extract(argparse.Namespace(check=True, verbose=False))
    print("== check")
    status |= command_check(argparse.Namespace(files=[]))
    print("== lint")
    status |= command_lint(argparse.Namespace(update_baseline=False))
    return status


# -------------------------------------------------------

def main(argv=None) -> int:
    # Messages hold any language: UTF-8 even where the console's code page can't show it (Windows cp1252)
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description="Scenery Editor X translation tooling")
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("extract", help="write resources/locale/sedx.pot from the source")
    p.add_argument("--check", action="store_true", help="compare instead of writing")
    p.add_argument("--verbose", action="store_true", help="list the functions whose Text parameters are extracted")
    p = sub.add_parser("update", help="bring catalogs in line with the template")
    p.add_argument("files", nargs="*")
    p = sub.add_parser("new", help="create a catalog for a language")
    p.add_argument("tag")
    p = sub.add_parser("check", help="validate catalogs")
    p.add_argument("files", nargs="*")
    p = sub.add_parser("lint", help="find untranslated ImGui literals")
    p.add_argument("--update-baseline", action="store_true")
    p = sub.add_parser("stats", help="completion per language")
    p.add_argument("--markdown", action="store_true")
    sub.add_parser("ci", help="extract --check, check and lint")
    p = sub.add_parser("changes", help="markdown list of messages added / removed since an older template")
    p.add_argument("old", nargs="?", help="the older sedx.pot (missing or empty: everything is new)")
    args = parser.parse_args(argv)
    commands = {"extract": command_extract, "update": command_update, "new": command_new, "check": command_check,
                "lint": command_lint, "stats": command_stats, "ci": command_ci, "changes": command_changes}
    try:
        return commands[args.command](args)
    except ToolError as error:
        print("error: " + str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
