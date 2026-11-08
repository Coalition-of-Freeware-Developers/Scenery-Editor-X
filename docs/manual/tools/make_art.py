#!/usr/bin/env python3
"""
Draws the documentation's cover and divider art as SVG, in the "Datum Lines" style (docs/manual/art/philosophy.md).
Standard library only; the output is deterministic, so regenerating it changes nothing unless this file changes.

    python docs/manual/tools/make_art.py      ->  docs/manual/art/{cover,volume-1,volume-2,back}.svg

The art is background only: the build sets the titles, version and date over it.
Every sheet is a Lambert conformal conic chart of the tutorial's field in Kansas (38 deg 30' N, 98 deg 30' W).
"""

from __future__ import annotations

import math
import random
from pathlib import Path

W, H = 2100, 2970  # A4 in tenths of a millimetre
OUT = Path(__file__).resolve().parent.parent / "art"

NAVY_TOP, NAVY_FOOT = "#0d1924", "#0a131b"
INK = "#7fc6f2"  # Instrument cyan for every line
AMBER = "#f0c46a"
LABEL = "#9fb7c8"
FONT = "'Open Sans', 'Segoe UI', Helvetica, Arial, sans-serif"

LAT0, LON0 = 38.5, -98.5  # The tutorial field, and the chart's standard parallel
VARIATION = 3.0  # Magnetic variation there, degrees east


# --------------------------------------------------------------------------------------------------------------------
# Projection

class Chart:
    """Lambert conformal conic around (LAT0, LON0): ``scale`` units per degree of latitude, the datum at ``origin``."""

    def __init__(self, origin: tuple[float, float], scale: float):
        self.ox, self.oy = origin
        self.scale = scale
        self.n = math.sin(math.radians(LAT0))
        earth_deg = 6371.0 * math.pi / 180.0  # km per degree of latitude
        self.rho0 = 6371.0 / math.tan(math.radians(LAT0)) / earth_deg * scale  # Apex distance, in units

    def xy(self, lat: float, lon: float) -> tuple[float, float]:
        rho = self.rho0 - (lat - LAT0) * self.scale
        theta = math.radians(self.n * (lon - LON0))
        return self.ox + rho * math.sin(theta), self.oy - self.rho0 + rho * math.cos(theta)

    def bounds(self) -> tuple[float, float, float, float]:
        """Latitudes and longitudes that cover the sheet (generously)."""
        span_lat = (H / self.scale) * 0.9 + 1
        span_lon = (W / (self.scale * math.cos(math.radians(LAT0)))) * 0.8 + 1
        return LAT0 - span_lat, LAT0 + span_lat, LON0 - span_lon, LON0 + span_lon


def fmt(v: float) -> str:
    return f"{v:.1f}".rstrip("0").rstrip(".")


def polyline(points) -> str:
    pts = list(points)
    return "M" + " L".join(f"{fmt(x)} {fmt(y)}" for x, y in pts)


def frange(a: float, b: float, step: float):
    count = int(round((b - a) / step))
    return [a + i * step for i in range(count + 1)]


def dms(value: float, positive: str, negative: str, seconds: bool = False) -> str:
    hemi = positive if value >= 0 else negative
    value = abs(value)
    d = int(value)
    m = (value - d) * 60
    if seconds:
        s = (m - int(m)) * 60
        return f"{d:02d}°{int(m):02d}′{int(round(s)):02d}″{hemi}"
    return f"{d}°{int(round(m)):02d}′{hemi}"


# --------------------------------------------------------------------------------------------------------------------
# Elements

def graticule(chart: Chart, minor: float, major: float, minor_opacity: float, major_opacity: float) -> str:
    lat_a, lat_b, lon_a, lon_b = chart.bounds()
    lat_a, lat_b = math.floor(lat_a), math.ceil(lat_b)
    lon_a, lon_b = math.floor(lon_a), math.ceil(lon_b)
    minor_paths, major_paths = [], []
    for lat in frange(lat_a, lat_b, minor):
        d = polyline(chart.xy(lat, lon) for lon in frange(lon_a, lon_b, 0.2))
        (major_paths if abs(lat / major - round(lat / major)) < 1e-6 else minor_paths).append(d)
    for lon in frange(lon_a, lon_b, minor):
        d = polyline(chart.xy(lat, lon) for lat in frange(lat_a, lat_b, 0.5))
        (major_paths if abs(lon / major - round(lon / major)) < 1e-6 else minor_paths).append(d)
    return (f'<path d="{" ".join(minor_paths)}" fill="none" stroke="{INK}" stroke-opacity="{minor_opacity}" stroke-width="1"/>\n'
            f'<path d="{" ".join(major_paths)}" fill="none" stroke="{INK}" stroke-opacity="{major_opacity}" stroke-width="1.6"/>\n')


def contours(center: tuple[float, float], rings: int, start: float, step: float, seed: int, opacity: float,
             squash: float = 1.0, angle: float = 0.0) -> str:
    """Terrain-like rings: one set of harmonics for the whole hill, growing slowly outward."""
    rnd = random.Random(seed)
    harmonics = [(k, rnd.uniform(0.03, 0.09) / k ** 0.6, rnd.uniform(0, math.tau)) for k in range(2, 7)]
    drift = [(rnd.uniform(-1, 1), rnd.uniform(-1, 1)) for _ in range(rings)]
    cx, cy = center
    ca, sa = math.cos(math.radians(angle)), math.sin(math.radians(angle))
    paths = []
    for r in range(rings):
        base = start + r * step
        grow = 0.6 + 0.4 * r / max(1, rings - 1)
        pts = []
        for i in range(0, 361, 2):
            t = math.radians(i)
            wobble = sum(a * grow * math.sin(k * t + p) for k, a, p in harmonics)
            rr = base * (1 + wobble)
            x, y = rr * math.cos(t), rr * math.sin(t) * squash
            x += drift[r][0] * step * 0.35
            y += drift[r][1] * step * 0.35
            pts.append((cx + x * ca - y * sa, cy + x * sa + y * ca))
        paths.append(polyline(pts) + "Z")
    return f'<path d="{" ".join(paths)}" fill="none" stroke="{INK}" stroke-opacity="{opacity}" stroke-width="1.2"/>\n'


def compass_rose(cx: float, cy: float, radius: float, opacity: float, label_size: float = 24) -> str:
    """A VOR-style rose turned toward magnetic north: ticks every 5 degrees, numbers every 30."""
    out = [f'<g transform="rotate({VARIATION} {fmt(cx)} {fmt(cy)})" stroke="{INK}" fill="none">']
    out.append(f'<circle cx="{fmt(cx)}" cy="{fmt(cy)}" r="{fmt(radius)}" stroke-opacity="{opacity}" stroke-width="1.6"/>')
    out.append(f'<circle cx="{fmt(cx)}" cy="{fmt(cy)}" r="{fmt(radius * 0.72)}" stroke-opacity="{opacity * 0.6}" stroke-width="1" stroke-dasharray="3 9"/>')
    ticks = []
    for d in range(0, 360, 5):
        length = 42 if d % 30 == 0 else 26 if d % 10 == 0 else 13
        a = math.radians(d)
        x0, y0 = cx + radius * math.sin(a), cy - radius * math.cos(a)
        x1, y1 = cx + (radius - length) * math.sin(a), cy - (radius - length) * math.cos(a)
        ticks.append(f"M{fmt(x0)} {fmt(y0)} L{fmt(x1)} {fmt(y1)}")
    out.append(f'<path d="{" ".join(ticks)}" stroke-opacity="{opacity * 1.4}" stroke-width="1.4"/>')
    # North: a fine open arrow above the ring
    out.append(f'<path d="M{fmt(cx)} {fmt(cy - radius - 46)} L{fmt(cx - 11)} {fmt(cy - radius - 14)} M{fmt(cx)} {fmt(cy - radius - 46)} '
               f'L{fmt(cx + 11)} {fmt(cy - radius - 14)}" stroke-opacity="{opacity * 1.6}" stroke-width="1.6"/>')
    out.append("</g>")
    labels = [f'<g transform="rotate({VARIATION} {fmt(cx)} {fmt(cy)})" fill="{LABEL}" fill-opacity="{min(1, opacity * 3.2)}" '
              f'font-family="{FONT}" font-weight="300" font-size="{fmt(label_size)}" text-anchor="middle" letter-spacing="1">']
    for d in range(0, 360, 30):
        r = radius - 42 - label_size * 0.95
        labels.append(f'<text transform="rotate({d} {fmt(cx)} {fmt(cy)})" x="{fmt(cx)}" y="{fmt(cy - r)}">{d // 10}</text>')
    labels.append("</g>")
    return "\n".join(out + labels) + "\n"


def runway(cx: float, cy: float, heading: float, length: float, width: float) -> str:
    """The one warm mark: runway 17/35 as a lit outline with its numbers beyond each end."""
    half = length / 2
    out = [f'<g transform="rotate({heading} {fmt(cx)} {fmt(cy)})">',
           f'<rect x="{fmt(cx - width / 2)}" y="{fmt(cy - half)}" width="{fmt(width)}" height="{fmt(length)}" fill="{NAVY_TOP}" '
           f'stroke="{AMBER}" stroke-width="2.2"/>',
           f'<path d="M{fmt(cx)} {fmt(cy - half + 16)} V{fmt(cy + half - 16)}" stroke="{AMBER}" stroke-opacity=".55" stroke-width="1.2" stroke-dasharray="9 7"/>']
    for y, text, flip in ((cy - half - 26, "17", 180), (cy + half + 26, "35", 0)):
        out.append(f'<text transform="rotate({flip} {fmt(cx)} {fmt(y)})" x="{fmt(cx)}" y="{fmt(y + 8)}" fill="{AMBER}" font-family="{FONT}" '
                   f'font-weight="600" font-size="22" text-anchor="middle" letter-spacing="2">{text}</text>')
    out.append("</g>")
    return "\n".join(out) + "\n"


def bezier(nodes, gradient: str, width: float = 2.4) -> str:
    """A curve with its nodes and handles drawn, as the Nodes tool shows them.
    nodes: (x, y, in_dx, in_dy, out_dx, out_dy, kind) with kind 'corner' | 'smooth'."""
    d = f"M{fmt(nodes[0][0])} {fmt(nodes[0][1])}"
    for a, b in zip(nodes, nodes[1:]):
        c1 = (a[0] + a[4], a[1] + a[5])
        c2 = (b[0] + b[2], b[1] + b[3])
        d += f" C{fmt(c1[0])} {fmt(c1[1])} {fmt(c2[0])} {fmt(c2[1])} {fmt(b[0])} {fmt(b[1])}"
    out = [f'<path d="{d}" fill="none" stroke="url(#{gradient})" stroke-width="{width}" stroke-linecap="round"/>']
    for i, (x, y, idx, idy, odx, ody, kind) in enumerate(nodes):
        handles = []
        if i > 0 and (idx or idy):
            handles.append((x + idx, y + idy))
        if i < len(nodes) - 1 and (odx or ody):
            handles.append((x + odx, y + ody))
        for hx, hy in handles:
            out.append(f'<path d="M{fmt(x)} {fmt(y)} L{fmt(hx)} {fmt(hy)}" stroke="{INK}" stroke-opacity=".55" stroke-width="1"/>')
            out.append(f'<circle cx="{fmt(hx)}" cy="{fmt(hy)}" r="5.5" fill="{NAVY_TOP}" stroke="{INK}" stroke-opacity=".8" stroke-width="1.2"/>')
        if kind == "corner":
            out.append(f'<rect x="{fmt(x - 7)}" y="{fmt(y - 7)}" width="14" height="14" fill="{NAVY_TOP}" stroke="#e8f3fb" stroke-width="1.6"/>')
        else:
            out.append(f'<circle cx="{fmt(x)}" cy="{fmt(y)}" r="7.5" fill="{NAVY_TOP}" stroke="#e8f3fb" stroke-width="1.6"/>')
    return "\n".join(out) + "\n"


def tile(chart: Chart, lat: int, lon: int, opacity: float, label: bool = True) -> str:
    """One DSF tile (a 1 x 1 degree cell) outlined, named the way X-Plane names its file."""
    edge = ([chart.xy(lat, x) for x in frange(lon, lon + 1, 0.05)] + [chart.xy(y, lon + 1) for y in frange(lat, lat + 1, 0.05)] +
            [chart.xy(lat + 1, x) for x in frange(lon + 1, lon, -0.05)] + [chart.xy(y, lon) for y in frange(lat + 1, lat, -0.05)])
    out = [f'<path d="{polyline(edge)}Z" fill="{INK}" fill-opacity="{opacity * 0.08}" stroke="{INK}" stroke-opacity="{opacity}" stroke-width="2"/>']
    if label:
        x, y = chart.xy(lat + 1, lon)
        name = f"{lat:+03d}{lon:+04d}.dsf"
        out.append(f'<text x="{fmt(x + 16)}" y="{fmt(y + 34)}" fill="{LABEL}" fill-opacity=".85" font-family="{FONT}" font-weight="400" '
                   f'font-size="21" letter-spacing="2">{name}</text>')
    return "\n".join(out) + "\n"


def network(points, edges, seed: int) -> str:
    """Road-network-like graph: junctions joined by gently curved segments."""
    rnd = random.Random(seed)
    out = []
    for a, b in edges:
        (x0, y0), (x1, y1) = points[a], points[b]
        mx, my = (x0 + x1) / 2, (y0 + y1) / 2
        nx, ny = -(y1 - y0), x1 - x0
        k = rnd.uniform(-0.18, 0.18)
        out.append(f"M{fmt(x0)} {fmt(y0)} Q{fmt(mx + nx * k)} {fmt(my + ny * k)} {fmt(x1)} {fmt(y1)}")
    s = f'<path d="{" ".join(out)}" fill="none" stroke="{INK}" stroke-opacity=".5" stroke-width="1.6"/>\n'
    for x, y in points:
        s += f'<circle cx="{fmt(x)}" cy="{fmt(y)}" r="6.5" fill="{NAVY_TOP}" stroke="#e8f3fb" stroke-opacity=".9" stroke-width="1.4"/>\n'
    return s


def neatline(chart: Chart, inset: float, step: float) -> str:
    """The sheet's frame, with a tick and a label where each whole-degree line meets it."""
    x0, y0, x1, y1 = inset, inset, W - inset, H - inset
    out = [f'<rect x="{fmt(x0)}" y="{fmt(y0)}" width="{fmt(x1 - x0)}" height="{fmt(y1 - y0)}" fill="none" stroke="{INK}" stroke-opacity=".45" stroke-width="1.4"/>',
           f'<rect x="{fmt(x0 - 10)}" y="{fmt(y0 - 10)}" width="{fmt(x1 - x0 + 20)}" height="{fmt(y1 - y0 + 20)}" fill="none" stroke="{INK}" stroke-opacity=".18" stroke-width="1"/>']
    lat_a, lat_b, lon_a, lon_b = chart.bounds()
    ticks, labels = [], []

    def crossings(points):
        hits = []
        for (ax, ay), (bx, by) in zip(points, points[1:]):
            for edge, horizontal, value in (("top", True, y0), ("bottom", True, y1), ("left", False, x0), ("right", False, x1)):
                if horizontal and (ay - value) * (by - value) < 0:
                    t = (value - ay) / (by - ay)
                    x = ax + t * (bx - ax)
                    if x0 < x < x1:
                        hits.append((edge, x, value))
                elif not horizontal and (ax - value) * (bx - value) < 0:
                    t = (value - ax) / (bx - ax)
                    y = ay + t * (by - ay)
                    if y0 < y < y1:
                        hits.append((edge, value, y))
        return hits

    for lat in frange(math.floor(lat_a), math.ceil(lat_b), step):
        for edge, x, y in crossings([chart.xy(lat, lon) for lon in frange(math.floor(lon_a), math.ceil(lon_b), 0.05)]):
            ticks.append((edge, x, y))
            labels.append((edge, x, y, dms(lat, "N", "S")))
    for lon in frange(math.floor(lon_a), math.ceil(lon_b), step):
        for edge, x, y in crossings([chart.xy(lat, lon) for lat in frange(math.floor(lat_a), math.ceil(lat_b), 0.1)]):
            ticks.append((edge, x, y))
            labels.append((edge, x, y, dms(lon, "E", "W")))
    # Nothing crowds a corner, and no two labels on one edge come close
    placed: list[tuple[str, float, float]] = []
    kept = []
    for edge, x, y, text in labels:
        along = x if edge in ("top", "bottom") else y
        lo, hi = (x0, x1) if edge in ("top", "bottom") else (y0, y1)
        if along - lo < 150 or hi - along < 150:
            continue
        if any(e == edge and abs((px if e in ("top", "bottom") else py) - along) < 190 for e, px, py in placed):
            continue
        placed.append((edge, x, y))
        kept.append((edge, x, y, text))
    labels = kept
    ticks = [(edge, x, y) for edge, x, y, _ in kept]
    d = []
    for edge, x, y in ticks:
        dx, dy = {"top": (0, -10), "bottom": (0, 10), "left": (-10, 0), "right": (10, 0)}[edge]
        d.append(f"M{fmt(x)} {fmt(y)} L{fmt(x + dx * 2.2)} {fmt(y + dy * 2.2)}")
    out.append(f'<path d="{" ".join(d)}" stroke="{INK}" stroke-opacity=".6" stroke-width="1.4"/>')
    for edge, x, y, text in labels:
        if edge in ("top", "bottom"):
            ty = y - 34 if edge == "top" else y + 50
            out.append(f'<text x="{fmt(x)}" y="{fmt(ty)}" text-anchor="middle" {LABEL_ATTRS}>{text}</text>')
        else:
            tx = x - 34 if edge == "left" else x + 34
            out.append(f'<text transform="rotate(-90 {fmt(tx)} {fmt(y)})" x="{fmt(tx)}" y="{fmt(y + 7)}" text-anchor="middle" {LABEL_ATTRS}>{text}</text>')
    return "\n".join(out) + "\n"


LABEL_ATTRS = f'fill="{LABEL}" fill-opacity=".8" font-family="{FONT}" font-weight="400" font-size="19" letter-spacing="1.5"'


def margin_text(x: float, y: float, text: str, anchor: str = "start", size: float = 19, opacity: float = .75, spacing: float = 3) -> str:
    return (f'<text x="{fmt(x)}" y="{fmt(y)}" text-anchor="{anchor}" fill="{LABEL}" fill-opacity="{opacity}" font-family="{FONT}" '
            f'font-weight="400" font-size="{fmt(size)}" letter-spacing="{fmt(spacing)}">{text}</text>\n')


def calm(y_from: float, y_to: float, strength: float = .92) -> str:
    """A quiet field for the title: the ground color rising over the lines."""
    return (f'<linearGradient id="calm{int(y_from)}" x1="0" y1="{fmt(y_from)}" x2="0" y2="{fmt(y_to)}" gradientUnits="userSpaceOnUse">'
            f'<stop offset="0" stop-color="{NAVY_FOOT}" stop-opacity="0"/><stop offset=".55" stop-color="{NAVY_FOOT}" stop-opacity="{strength * .8}"/>'
            f'<stop offset="1" stop-color="{NAVY_FOOT}" stop-opacity="{strength}"/></linearGradient>'
            f'<rect x="0" y="{fmt(y_from)}" width="{W}" height="{fmt(y_to - y_from)}" fill="url(#calm{int(y_from)})"/>\n')


def sheet(body: str, title: str) -> str:
    return f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="210mm" height="297mm" role="img" aria-label="{title}">
<defs>
<linearGradient id="ground" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{NAVY_TOP}"/><stop offset="1" stop-color="{NAVY_FOOT}"/></linearGradient>
<linearGradient id="lit" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="{INK}" stop-opacity=".35"/><stop offset=".55" stop-color="{INK}"/><stop offset="1" stop-color="{AMBER}"/></linearGradient>
<linearGradient id="lit2" x1="0" y1="1" x2="1" y2="0"><stop offset="0" stop-color="{AMBER}" stop-opacity=".6"/><stop offset=".5" stop-color="{INK}"/><stop offset="1" stop-color="{INK}" stop-opacity=".3"/></linearGradient>
<clipPath id="sheet"><rect x="0" y="0" width="{W}" height="{H}"/></clipPath>
</defs>
<rect width="{W}" height="{H}" fill="url(#ground)"/>
<g clip-path="url(#sheet)">
{body}</g>
</svg>
"""


# --------------------------------------------------------------------------------------------------------------------
# Sheets

def cover() -> str:
    chart = Chart((1050, 1180), 330)
    cx, cy = chart.xy(LAT0, LON0)
    body = graticule(chart, 1 / 6, 1, .07, .22)
    body += tile(chart, 38, -99, .55)
    body += contours((1640, 560), 15, 40, 26, 7, .16, .82, -18)
    body += contours((430, 1690), 11, 34, 24, 11, .13, .9, 24)
    body += compass_rose(cx, cy, 380, .32)
    body += runway(cx, cy, 350, 150, 13)
    # The curve sweeps around the rose, never across its marks: in from the left, up past it, out at the top right
    body += bezier([
        (-60, 1730, 0, 0, 180, -60, "smooth"),
        (420, 1260, -70, 180, 70, -180, "smooth"),
        (900, 470, -260, 120, 260, -120, "smooth"),
        (1640, 300, -240, 0, 240, 0, "corner"),
        (2180, 640, -170, -150, 0, 0, "smooth"),
    ], "lit")
    body += calm(1700, 2970, .94)
    body += neatline(chart, 92, 1)
    body += margin_text(140, 182, "SCENERY EDITOR X  ·  DOCUMENTATION")
    body += margin_text(W - 140, 182, "SHEET 01", "end")
    body += margin_text(140, H - 128, "DATUM  " + dms(LAT0, "N", "S", True) + "  " + dms(LON0, "E", "W", True), size=18)
    body += margin_text(W - 140, H - 128, "LAMBERT CONFORMAL CONIC  ·  1 : 3 400 000", "end", size=18)
    return sheet(body, "Chart of the tutorial field: graticule, compass rose and runway 17/35")


def volume_one() -> str:
    chart = Chart((1050, 1010), 900)
    cx, cy = chart.xy(LAT0, LON0)
    body = graticule(chart, 1 / 12, 1 / 2, .06, .2)
    body += contours((1580, 520), 18, 50, 30, 23, .14, .78, -10)
    body += contours((330, 1500), 12, 44, 28, 29, .11, .92, 30)
    body += compass_rose(cx, cy, 560, .34, 30)
    body += runway(cx, cy, 350, 330, 26)
    # Final approach to runway 35: a gentle turn onto the extended centreline, then straight in from the south,
    # stopping short of the threshold's number
    ax, ay = math.sin(math.radians(10)), math.cos(math.radians(10))  # Unit vector from the runway's centre to its south end
    far, near = 720, 222
    body += bezier([
        (-60, cy + 900, 0, 0, 420, 0, "smooth"),
        (cx + ax * far, cy + ay * far, ax * 300, ay * 300, -ax * 170, -ay * 170, "smooth"),
        (cx + ax * near, cy + ay * near, ax * 170, ay * 170, 0, 0, "corner"),
    ], "lit2", 2.6)
    body += calm(1560, 2970, .95)
    body += neatline(chart, 92, 1 / 2)
    body += margin_text(140, 182, "SCENERY EDITOR X  ·  VOLUME I")
    body += margin_text(W - 140, 182, "SHEET 02", "end")
    body += margin_text(140, H - 128, "FIELD XTUT  ·  RWY 17 / 35", size=18)
    body += margin_text(W - 140, H - 128, "1 : 1 250 000", "end", size=18)
    return sheet(body, "The tutorial field at a larger scale, with the runway and its approach drawn as a bezier")


def volume_two() -> str:
    chart = Chart((1050, 1060), 420)
    body = graticule(chart, 1 / 6, 1, .07, .24)
    for lat in (37, 38, 39):
        for lon in (-100, -99, -98, -97):
            body += tile(chart, lat, lon, .2 if (lat, lon) != (38, -99) else .55, label=(lat, lon) == (38, -99))
    rnd = random.Random(41)
    pts = []
    for row in range(5):
        for col in range(5):
            if rnd.random() < 0.82:
                pts.append((300 + col * 375 + rnd.uniform(-90, 90), 330 + row * 300 + rnd.uniform(-70, 70)))
    edges = []
    for i, (x, y) in enumerate(pts):
        near = sorted(range(len(pts)), key=lambda j: (pts[j][0] - x) ** 2 + (pts[j][1] - y) ** 2)[1:3]
        for j in near:
            if (j, i) not in edges:
                edges.append((i, j))
    body += network(pts, edges, 43)
    body += calm(1620, 2970, .95)
    body += neatline(chart, 92, 1)
    body += margin_text(140, 182, "SCENERY EDITOR X  ·  VOLUME II")
    body += margin_text(W - 140, 182, "SHEET 03", "end")
    body += margin_text(140, H - 128, "TILES 37–40 N  ·  100–96 W", size=18)
    body += margin_text(W - 140, H - 128, "LAMBERT CONFORMAL CONIC  ·  1 : 2 650 000", "end", size=18)
    return sheet(body, "DSF tiles around the field with a network of junctions and roads")


def back() -> str:
    chart = Chart((1050, 1480), 300)
    cx, cy = chart.xy(LAT0, LON0)
    body = graticule(chart, 1 / 6, 1, .05, .16)
    body += compass_rose(cx, cy, 210, .22, 18)
    body += runway(cx, cy, 350, 80, 8)
    body += neatline(chart, 92, 1)
    return sheet(body, "The field's compass rose, small, on the graticule")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for name, make in (("cover", cover), ("volume-1", volume_one), ("volume-2", volume_two), ("back", back)):
        (OUT / f"{name}.svg").write_text(make(), encoding="utf-8", newline="\n")
        print(f"wrote {OUT / name}.svg")


if __name__ == "__main__":
    main()
