"""
Syntax highlighting for the manual's code blocks. Standard library only.

highlight(code, lang) returns escaped HTML with <span class="tok-..."> around comments, strings, numbers, keywords,
types, function names and preprocessor directives. It is a tokenizer, not a parser: good enough to make C++, Slang /
HLSL, JSON, shell and CMake readable, and it never changes the text itself.
"""

from __future__ import annotations

import html
import re

_C_KEYWORDS = (
    "alignas alignof break case catch class concept const consteval constexpr constinit const_cast continue co_await "
    "co_return co_yield decltype default delete do dynamic_cast else enum explicit export extern false final for "
    "friend goto if inline mutable namespace new noexcept nullptr operator override private protected public register "
    "reinterpret_cast requires return sizeof static static_assert static_cast struct switch template this thread_local "
    "throw true try typedef typeid typename union using virtual volatile while "
    # Slang / HLSL
    "cbuffer in out inout uniform groupshared interface extension import module __generic associatedtype where"
).split()
_C_TYPES = (
    "auto bool char char8_t char16_t char32_t double float int long short signed unsigned void wchar_t size_t "
    "ptrdiff_t intptr_t uintptr_t int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t "
    "float2 float3 float4 float3x3 float4x4 int2 int3 int4 uint uint2 uint3 uint4 half half2 half3 half4"
).split()

_C_RE = re.compile(
    r"(?P<com>//[^\n]*|/\*.*?\*/)"
    r'|(?P<str>R"(?P<delim>[^(\s]*)\(.*?\)(?P=delim)"|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\')'
    r"|(?P<pp>^[ \t]*#[ \t]*[A-Za-z_]+)"
    r"|(?P<num>\b(?:0[xX][0-9a-fA-F']+|0[bB][01']+|\d[\d']*(?:\.\d*)?(?:[eE][+-]?\d+)?)[uUlLfF]*\b)"
    r"|(?P<word>[A-Za-z_]\w*)",
    re.S | re.M,
)

_JSON_RE = re.compile(
    r'(?P<key>"(?:\\.|[^"\\\n])*"(?=\s*:))'
    r'|(?P<str>"(?:\\.|[^"\\\n])*")'
    r"|(?P<num>-?\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?\b)"
    r"|(?P<kw>\b(?:true|false|null)\b)"
    r"|(?P<com>//[^\n]*|/\*.*?\*/)",
    re.S,
)

_SHELL_RE = re.compile(
    r"(?P<com>(?:^|(?<=\s))(?:#|::|REM\b|rem\b)[^\n]*)"
    r'|(?P<str>"(?:\\.|[^"\\])*"|\'[^\']*\')'
    r"|(?P<var>\$\{?[A-Za-z_][A-Za-z0-9_]*\}?|%[A-Za-z_][A-Za-z0-9_]*%|\$env:[A-Za-z_]+)"
    r"|(?P<flag>(?<=\s)--?[A-Za-z][\w-]*)"
    r"|(?P<cmd>^[ \t]*[A-Za-z_.][\w.-]*)",
    re.M,
)

_CMAKE_RE = re.compile(
    r"(?P<com>#[^\n]*)"
    r'|(?P<str>"(?:\\.|[^"\\])*")'
    r"|(?P<var>\$\{[^}]+\})"
    r"|(?P<fn>\b[A-Za-z_]\w*(?=\s*\())"
    r"|(?P<kw>\b[A-Z][A-Z0-9_]{2,}\b)",
)

_C_LANGS = {"cpp", "c++", "cxx", "c", "h", "hpp", "hlsl", "slang", "glsl", "cs", "java", "js", "javascript", "ts"}
_SHELL_LANGS = {"bash", "sh", "shell", "bat", "cmd", "powershell", "ps1", "pwsh", "console"}


def _span(kind: str, text: str) -> str:
    return f'<span class="tok-{kind}">{html.escape(text, quote=False)}</span>'


def _highlight_c(code: str) -> str:
    keywords, types = set(_C_KEYWORDS), set(_C_TYPES)
    out, pos = [], 0
    for m in _C_RE.finditer(code):
        out.append(html.escape(code[pos:m.start()], quote=False))
        pos = m.end()
        kind = m.lastgroup if m.lastgroup != "delim" else "str"
        text = m.group(0)
        if kind == "word":
            follows = code[m.end():m.end() + 1]
            if text in keywords:
                out.append(_span("kw", text))
            elif text in types or text.endswith("_t"):
                out.append(_span("ty", text))
            elif follows == "(":
                out.append(_span("fn", text))
            elif text[0].isupper() and not text.isupper():
                out.append(_span("ty", text))
            else:
                out.append(html.escape(text, quote=False))
        elif m.group("str") is not None:
            out.append(_span("str", text))
        else:
            out.append(_span(kind, text))
    out.append(html.escape(code[pos:], quote=False))
    return "".join(out)


def _highlight_regex(code: str, pattern: re.Pattern) -> str:
    out, pos = [], 0
    for m in pattern.finditer(code):
        if not m.group(0):
            continue
        out.append(html.escape(code[pos:m.start()], quote=False))
        out.append(_span(m.lastgroup, m.group(0)))
        pos = m.end()
    out.append(html.escape(code[pos:], quote=False))
    return "".join(out)


def _looks_like_c(code: str) -> bool:
    return bool(re.search(r"#include|::|\bstd::|;\s*$", code, re.M)) and ("{" in code or ";" in code)


def highlight(code: str, lang: str) -> str:
    """Escaped, highlighted HTML for a code block. Unlabelled blocks are highlighted when they look like C++."""
    lang = (lang or "").lower()
    if lang in _C_LANGS or (not lang and _looks_like_c(code)):
        return _highlight_c(code)
    if lang == "json":
        return _highlight_regex(code, _JSON_RE)
    if lang in _SHELL_LANGS:
        return _highlight_regex(code, _SHELL_RE)
    if lang == "cmake":
        return _highlight_regex(code, _CMAKE_RE)
    return html.escape(code, quote=False)
