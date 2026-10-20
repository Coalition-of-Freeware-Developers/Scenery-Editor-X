"""Writes the SEDX_Calibration scenery pack (plan TASK-101): material calibration tiles at KSEA that X-Plane 12 (as a
Custom Scenery pack) and SEDX (as a project) both draw, so the parity scenarios can compare materials directly.

Run with:  python -I tools/xp12_reference_probe/test_scenery/make_calibration_pack.py
Then export the DSF with SEDX (the pack folder is the project folder):
    bin/SceneryEditorX.exe --project tools/xp12_reference_probe/test_scenery/SEDX_Calibration/SEDX_Calibration.edX --export-scenery

Everything is authored here (no Laminar art): flat 2 m x 2 m tiles 5 cm above the ground in four rows, viewed by the
ksea_calib_* scenarios from the north. Standard library only (PNGs are written with zlib).
"""
import json
import math
import os
import struct
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
PACK = os.path.join(HERE, 'SEDX_Calibration')

# Grid center, in the KSEA infield between runways 16C and 16R; rows run east-west, row 0 nearest the camera (north)
CENTER = (47.4560, -122.31442)
PITCH_M = 2.5            # Tile spacing (2 m tiles, 0.5 m gaps)
ROW_SPACING_M = 3.0

TEXTURE_SIZE = 16


def png(path, rgba, size=TEXTURE_SIZE):
    """A solid RGBA8 PNG."""
    raw = b''.join(b'\x00' + bytes(rgba) * size for _ in range(size))

    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)

    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(data)


def dds_dxt5(path, rgba, size=TEXTURE_SIZE):
    """A solid DXT5 (BC3) DDS with its full mip chain. Next to a PNG of the same name both X-Plane and SEDX load this
    one, so the metalness row exercises block-compressed NORMAL_METALNESS maps (565 colour: B, the F0 value, is kept to
    1/31)."""
    r, g, b, a = rgba
    color = ((r * 31 + 127) // 255) << 11 | ((g * 63 + 127) // 255) << 5 | ((b * 31 + 127) // 255)
    block = struct.pack('<BB6xHHI', a, a, color, color, 0)    # Both endpoints equal, every index 0
    mips, blocks, side = [], 0, size
    while True:
        count = max(1, side // 4) ** 2
        mips.append(block * count)
        if side == 1:
            break
        side //= 2
    header = struct.pack('<4sIIIIIII44x', b'DDS ', 124, 0x000A1007, size, size, len(mips[0]), 0, len(mips))
    pixel_format = struct.pack('<II4sIIIII', 32, 0x4, b'DXT5', 0, 0, 0, 0, 0)
    caps = struct.pack('<IIIII', 0x00401008, 0, 0, 0, 0)
    with open(path, 'wb') as f:
        f.write(header + pixel_format + caps + b''.join(mips))


def srgb_byte(linear):
    c = linear * 12.92 if linear <= 0.0031308 else 1.055 * linear ** (1.0 / 2.4) - 0.055
    return max(0, min(255, round(c * 255.0)))


def to_byte(value):
    return max(0, min(255, round(value * 255.0)))


# -------------------------------------------------------
# Tiles: name, albedo (sRGB bytes), normal-map B (metal / F0 value v) and A (gloss), extra OBJ directives

def tiles():
    grey18 = (srgb_byte(0.18),) * 3
    light = (200, 200, 200)
    concrete = (150, 150, 150)
    rows = []

    row0 = [('grey18', grey18, 0.04, 0.0, [])]
    for v in (0.0, 0.04, 0.08, 0.25, 0.5, 0.75, 1.0):
        row0.append((f'metal_{round(v * 100):03d}', light, v, 0.8, []))
    rows.append(row0)

    rows.append([(f'gloss_{i}', light, 0.04, i / 7.0, []) for i in range(8)])

    row2 = [('superrough_050', light, 0.04, 1.0, ['SUPER_ROUGHNESS 0.5']),
            ('superrough_100', light, 0.04, 1.0, ['SUPER_ROUGHNESS 1.0'])]
    for nits in (10, 100, 1000, 10000):
        row2.append((f'lit_{nits:05d}', (40, 40, 40), 0.04, 0.0, ['TEXTURE_LIT ../textures/lit_white.png', f'GLOBAL_luminance {nits}']))
    rows.append(row2)

    rows.append([
        ('weather_r', concrete, 0.04, 0.3, ['WEATHER ../textures/weather_r_wm.png']),
        ('weather_g', concrete, 0.04, 0.3, ['WEATHER ../textures/weather_g_wm.png']),
        ('weather_b', concrete, 0.04, 0.3, ['WEATHER ../textures/weather_b_wm.png']),
        ('snowluma_a', concrete, 0.04, 0.3, ['SNOW_ALBEDO_LUMA 0.4 0.2 0.0 0.0']),
        ('snowluma_b', concrete, 0.04, 0.3, ['SNOW_ALBEDO_LUMA 1.5 2.0 0.0 0.0']),
        ('weather_transparent', concrete, 0.04, 0.3, ['WEATHER_TRANSPARENT']),
    ])
    return rows


def obj_text(name, extra):
    # A 2 m square facing up, 5 cm above the ground. X-Plane OBJ8: x east, y up, z south; triangles clockwise seen
    # from the front (above); s runs east, t north.
    lines = ['I', '800', 'OBJ', '',
             f'TEXTURE ../textures/{name}_alb.png',
             f'TEXTURE_NORMAL ../textures/{name}_nml.png',
             'NORMAL_METALNESS',
             'GLOBAL_specular 1.0']
    lines += extra
    lines += ['', 'POINT_COUNTS 4 0 0 6', '',
              'VT -1.0 0.05 -1.0 0 1 0 0 1',
              'VT 1.0 0.05 -1.0 0 1 0 1 1',
              'VT 1.0 0.05 1.0 0 1 0 1 0',
              'VT -1.0 0.05 1.0 0 1 0 0 0',
              '',
              'IDX 0', 'IDX 1', 'IDX 2', 'IDX 0', 'IDX 2', 'IDX 3',
              '',
              'TRIS 0 6', '']
    return '\n'.join(lines)


def offset(lat, lon, east_m, north_m):
    phi = math.radians(lat)
    m_lat = 111132.954 - 559.822 * math.cos(2 * phi) + 1.175 * math.cos(4 * phi)
    m_lon = 111412.84 * math.cos(phi) - 93.5 * math.cos(3 * phi) + 0.118 * math.cos(5 * phi)
    return lat + north_m / m_lat, lon + east_m / m_lon


def main():
    os.makedirs(os.path.join(PACK, 'objects'), exist_ok=True)
    os.makedirs(os.path.join(PACK, 'textures'), exist_ok=True)

    png(os.path.join(PACK, 'textures', 'lit_white.png'), (255, 255, 255, 255))
    png(os.path.join(PACK, 'textures', 'weather_r_wm.png'), (255, 0, 0, 255))
    png(os.path.join(PACK, 'textures', 'weather_g_wm.png'), (0, 255, 0, 255))
    png(os.path.join(PACK, 'textures', 'weather_b_wm.png'), (0, 0, 255, 255))

    assets = []
    rows = tiles()
    for r, row in enumerate(rows):
        north = (len(rows) - 1) * ROW_SPACING_M * 0.5 - r * ROW_SPACING_M
        for c, (name, albedo, v, gloss, extra) in enumerate(row):
            png(os.path.join(PACK, 'textures', f'{name}_alb.png'), (*albedo, 255))
            png(os.path.join(PACK, 'textures', f'{name}_nml.png'), (128, 128, to_byte(v), to_byte(gloss)))
            if name.startswith('metal_'):
                dds_dxt5(os.path.join(PACK, 'textures', f'{name}_nml.dds'), (128, 128, to_byte(v), to_byte(gloss)))
            with open(os.path.join(PACK, 'objects', f'{name}.obj'), 'w', encoding='ascii', newline='\n') as f:
                f.write(obj_text(name, extra))
            east = (c - (len(row) - 1) * 0.5) * PITCH_M
            lat, lon = offset(CENTER[0], CENTER[1], east, north)
            assets.append({'type': 'object', 'id': f'cal{r}{c:02d}', 'name': name, 'resource': f'objects/{name}.obj',
                           'latitude': round(lat, 8), 'longitude': round(lon, 8), 'altitude': 0, 'heading': 0,
                           'elevation-mode': 'ground', 'show-level': 1})

    project = {
        'Project': {
            'name': 'SEDX_Calibration',
            'uuid': '6d1b4c1e-7f0a-4b8e-9a3e-5c2f0e1d9a77',
            'kind': 'scenery',
            'format-version': 1,
            'editor-version': '0.2.28',
            'XPlaneVersion': '12.4.4-b4',
            'created': '2026-10-20T00:00:00Z',
            'editdate': '2026-10-20T00:00:00Z',
            'author': 'Scenery Editor X contributors',
            'description': 'Material calibration tiles for the XP12 parity harness (plan TASK-101). Generated by make_calibration_pack.py.',
            'package': 'SEDX_Calibration',
        },
        'Datum': {'latitude': CENTER[0], 'longitude': CENTER[1], 'elevation': 132},
        'Assets': [{'type': 'group', 'id': 'calibration', 'name': 'Calibration tiles', 'children': assets}],
    }
    with open(os.path.join(PACK, 'SEDX_Calibration.edX'), 'w', encoding='utf-8', newline='\n') as f:
        json.dump(project, f, indent='\t')
        f.write('\n')
    print(f'{len(assets)} tiles -> {PACK}')


if __name__ == '__main__':
    main()
