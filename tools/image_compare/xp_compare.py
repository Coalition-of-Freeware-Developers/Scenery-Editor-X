"""Parity harness: compares SEDX scenario captures with X-Plane 12 reference screenshots (plan TASK-006).

    python -I tools/image_compare/xp_compare.py --references <probe output> --sedx <SEDX captures> --out <report dir>
        [--scenarios tools/xp12_reference_probe/scenarios/xp12_parity.json]
        [--thresholds tools/image_compare/thresholds.json]

Pairs <id>.png from the reference folder (tools/xp12_reference_probe writes them to X-Plane's Output/sedx_probe/)
with <id>.png from the SEDX folder (--env-scenario ... --capture-png <dir>/<id>.png, which also writes <id>_sky.png).
Per scenario: mean and 95th-percentile CIEDE2000, mean FLIP (NVIDIA FLIP, when the flip-evaluator package is
installed), the Earth Mover's Distance between log2-luminance histograms, and the mean CIEDE2000 of the sky (pixels
where SEDX drew nothing). Writes report.html, summary.json and diff images to --out; exits with 1 when a scenario
breaks its thresholds.

Needs numpy and Pillow (pip install numpy pillow flip-evaluator). Images are compared at the reference's size; the
SEDX image is resized to it (bicubic) and the scenario's ui_mask rectangles are ignored.
"""
import argparse
import html
import json
import math
import os
import re
import sys

try:
    import numpy as np
    from PIL import Image
except ImportError:  # pragma: no cover - reported to the user
    sys.stderr.write('xp_compare.py needs numpy and Pillow: pip install numpy pillow flip-evaluator\n')
    sys.exit(2)

try:
    import flip_evaluator
except ImportError:
    flip_evaluator = None

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..'))

# log2-luminance histogram: display-referred luminance 2^-12 .. 1
HIST_BINS = 96
HIST_RANGE = (-12.0, 0.0)


# -------------------------------------------------------
# Colour

def srgb_to_linear(c):
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def linear_to_lab(rgb):
    """Linear sRGB (D65) -> CIE L*a*b*."""
    m = np.array([[0.4124564, 0.3575761, 0.1804375],
                  [0.2126729, 0.7151522, 0.0721750],
                  [0.0193339, 0.1191920, 0.9503041]])
    xyz = rgb @ m.T
    white = np.array([0.95047, 1.0, 1.08883])
    t = xyz / white
    eps = 216.0 / 24389.0
    kappa = 24389.0 / 27.0
    f = np.where(t > eps, np.cbrt(t), (kappa * t + 16.0) / 116.0)
    lab = np.empty_like(xyz)
    lab[..., 0] = 116.0 * f[..., 1] - 16.0
    lab[..., 1] = 500.0 * (f[..., 0] - f[..., 1])
    lab[..., 2] = 200.0 * (f[..., 1] - f[..., 2])
    return lab


def ciede2000(lab1, lab2):
    """CIEDE2000 colour difference per pixel (Sharma, Wu and Dalal 2005), kL = kC = kH = 1."""
    L1, a1, b1 = lab1[..., 0], lab1[..., 1], lab1[..., 2]
    L2, a2, b2 = lab2[..., 0], lab2[..., 1], lab2[..., 2]
    C1 = np.hypot(a1, b1)
    C2 = np.hypot(a2, b2)
    Cbar = 0.5 * (C1 + C2)
    G = 0.5 * (1.0 - np.sqrt(Cbar ** 7 / (Cbar ** 7 + 25.0 ** 7)))
    a1p = (1.0 + G) * a1
    a2p = (1.0 + G) * a2
    C1p = np.hypot(a1p, b1)
    C2p = np.hypot(a2p, b2)
    h1p = np.degrees(np.arctan2(b1, a1p)) % 360.0
    h2p = np.degrees(np.arctan2(b2, a2p)) % 360.0

    dLp = L2 - L1
    dCp = C2p - C1p
    dhp = h2p - h1p
    dhp = np.where(dhp > 180.0, dhp - 360.0, dhp)
    dhp = np.where(dhp < -180.0, dhp + 360.0, dhp)
    dhp = np.where(C1p * C2p == 0.0, 0.0, dhp)
    dHp = 2.0 * np.sqrt(C1p * C2p) * np.sin(np.radians(dhp) / 2.0)

    Lbarp = 0.5 * (L1 + L2)
    Cbarp = 0.5 * (C1p + C2p)
    hsum = h1p + h2p
    hbarp = np.where(np.abs(h1p - h2p) > 180.0, np.where(hsum < 360.0, (hsum + 360.0) / 2.0, (hsum - 360.0) / 2.0), hsum / 2.0)
    hbarp = np.where(C1p * C2p == 0.0, hsum, hbarp)

    T = (1.0 - 0.17 * np.cos(np.radians(hbarp - 30.0)) + 0.24 * np.cos(np.radians(2.0 * hbarp))
         + 0.32 * np.cos(np.radians(3.0 * hbarp + 6.0)) - 0.20 * np.cos(np.radians(4.0 * hbarp - 63.0)))
    dtheta = 30.0 * np.exp(-(((hbarp - 275.0) / 25.0) ** 2))
    Rc = 2.0 * np.sqrt(Cbarp ** 7 / (Cbarp ** 7 + 25.0 ** 7))
    Sl = 1.0 + (0.015 * (Lbarp - 50.0) ** 2) / np.sqrt(20.0 + (Lbarp - 50.0) ** 2)
    Sc = 1.0 + 0.045 * Cbarp
    Sh = 1.0 + 0.015 * Cbarp * T
    Rt = -np.sin(np.radians(2.0 * dtheta)) * Rc
    return np.sqrt((dLp / Sl) ** 2 + (dCp / Sc) ** 2 + (dHp / Sh) ** 2 + Rt * (dCp / Sc) * (dHp / Sh))


def luminance(linear_rgb):
    return linear_rgb @ np.array([0.2126729, 0.7151522, 0.0721750])


def log2_histogram(lum, mask):
    values = np.log2(np.maximum(lum[mask], 2.0 ** HIST_RANGE[0]))
    hist, _ = np.histogram(values, bins=HIST_BINS, range=HIST_RANGE)
    total = hist.sum()
    return hist / total if total > 0 else hist.astype(np.float64)


def emd_1d(p, q):
    """Earth Mover's Distance between two histograms over the same bins, in log2 units (stops)."""
    width = (HIST_RANGE[1] - HIST_RANGE[0]) / HIST_BINS
    return float(np.abs(np.cumsum(p - q)).sum() * width)


# -------------------------------------------------------
# Inputs

def load_rgb(path):
    with Image.open(path) as image:
        return image.convert('RGB')


def load_scenarios(path):
    if not path or not os.path.exists(path):
        return {}
    with open(path, encoding='utf-8') as f:
        data = json.load(f)
    defaults = data.get('defaults', {})
    out = {}
    for entry in data.get('scenarios', []):
        merged = dict(defaults)
        merged.update(entry)
        out[merged['id']] = merged
    return out


def load_thresholds(path):
    with open(path, encoding='utf-8') as f:
        data = json.load(f)
    rules = [(re.compile(rule['match']), rule) for rule in data.get('rules', [])]
    return data.get('default', {}), rules


def limits_for(scenario_id, default, rules):
    for pattern, rule in rules:
        if pattern.search(scenario_id):
            merged = dict(default)
            merged.update(rule.get('limits', {}))
            return rule.get('name', pattern.pattern), merged
    return 'default', dict(default)


def ui_mask(shape, rects):
    """True where pixels count (outside every ui_mask rectangle, given in 0-1 of the image)."""
    h, w = shape
    keep = np.ones((h, w), dtype=bool)
    for x0, y0, x1, y1 in rects or []:
        keep[int(round(y0 * h)):int(round(y1 * h)), int(round(x0 * w)):int(round(x1 * w))] = False
    return keep


# -------------------------------------------------------

def compare(scenario_id, ref_path, sedx_path, sky_path, scenario, out_dir):
    ref_image = load_rgb(ref_path)
    sedx_image = load_rgb(sedx_path)
    warnings = []
    ref_aspect = ref_image.width / ref_image.height
    sedx_aspect = sedx_image.width / sedx_image.height
    if abs(ref_aspect - sedx_aspect) / ref_aspect > 0.01:
        warnings.append(f'aspect ratios differ ({ref_image.width}x{ref_image.height} vs {sedx_image.width}x{sedx_image.height}): '
                        'the views do not cover the same field')
    if sedx_image.size != ref_image.size:
        sedx_image = sedx_image.resize(ref_image.size, Image.BICUBIC)

    ref = np.asarray(ref_image, dtype=np.float64) / 255.0
    sedx = np.asarray(sedx_image, dtype=np.float64) / 255.0
    keep = ui_mask(ref.shape[:2], scenario.get('ui_mask', []))

    ref_lin = srgb_to_linear(ref)
    sedx_lin = srgb_to_linear(sedx)
    delta_e = ciede2000(linear_to_lab(ref_lin), linear_to_lab(sedx_lin))
    values = delta_e[keep]
    metrics = {
        'mean_de2000': float(values.mean()),
        'p95_de2000': float(np.percentile(values, 95)),
        'luminance_emd': emd_1d(log2_histogram(luminance(ref_lin), keep), log2_histogram(luminance(sedx_lin), keep)),
        'mean_luminance_ref': float(luminance(ref_lin)[keep].mean()),
        'mean_luminance_sedx': float(luminance(sedx_lin)[keep].mean()),
    }

    if sky_path and os.path.exists(sky_path):
        with Image.open(sky_path) as sky_image:
            sky = np.asarray(sky_image.convert('L').resize(ref_image.size, Image.NEAREST)) > 127
        sky &= keep
        if sky.any():
            metrics['sky_mean_de2000'] = float(delta_e[sky].mean())
            metrics['sky_fraction'] = float(sky.mean())

    if flip_evaluator is not None:
        try:
            _, mean_flip, _ = flip_evaluator.evaluate(ref.astype(np.float32), sedx.astype(np.float32), 'LDR')
            metrics['mean_flip'] = float(mean_flip)
        except Exception as e:  # keep the other metrics when FLIP fails
            warnings.append(f'FLIP failed: {e}')

    # Images for the report: the pair at reduced size and a delta-E heat map (0 black .. 20+ white)
    image_dir = os.path.join(out_dir, 'images')
    os.makedirs(image_dir, exist_ok=True)
    thumb = (min(640, ref_image.width), max(1, round(min(640, ref_image.width) / ref_aspect)))
    ref_image.resize(thumb, Image.BILINEAR).save(os.path.join(image_dir, f'{scenario_id}_ref.png'))
    sedx_image.resize(thumb, Image.BILINEAR).save(os.path.join(image_dir, f'{scenario_id}_sedx.png'))
    heat = np.clip(delta_e / 20.0, 0.0, 1.0)
    heat_rgb = np.stack([heat, np.clip(heat * 2.0 - 0.5, 0, 1), np.clip(heat * 3.0 - 2.0, 0, 1)], axis=-1)
    heat_rgb[~keep] = (0.0, 0.2, 0.4)
    Image.fromarray((heat_rgb * 255.0 + 0.5).astype(np.uint8)).resize(thumb, Image.BILINEAR).save(os.path.join(image_dir, f'{scenario_id}_de.png'))
    return metrics, warnings


def check(metrics, limits):
    failures = []
    for key, limit in limits.items():
        if not key.startswith('max_'):
            continue
        metric = key[4:]
        if metric in metrics and metrics[metric] > limit:
            failures.append(f'{metric} {metrics[metric]:.3f} > {limit}')
    return failures


def write_report(out_dir, results, flip_available):
    rows = []
    for r in results:
        m = r.get('metrics', {})
        status = 'missing' if r['status'] == 'missing' else ('PASS' if not r['failures'] else 'FAIL')
        colour = {'PASS': '#2e7d32', 'FAIL': '#c62828'}.get(status, '#757575')
        cells = ''.join(f'<td>{m[k]:.3f}</td>' if k in m else '<td>-</td>'
                        for k in ('mean_de2000', 'p95_de2000', 'mean_flip', 'luminance_emd', 'sky_mean_de2000'))
        notes = html.escape('; '.join(r.get('failures', []) + r.get('warnings', [])))
        images = ''
        if r['status'] != 'missing':
            sid = html.escape(r['id'])
            images = (f'<tr><td colspan="9"><img src="images/{sid}_ref.png" title="X-Plane 12"> '
                      f'<img src="images/{sid}_sedx.png" title="SEDX"> <img src="images/{sid}_de.png" title="CIEDE2000 (0-20)"></td></tr>')
        rows.append(f'<tr><td>{html.escape(r["id"])}</td><td>{html.escape(r.get("category", ""))}</td>'
                    f'<td style="color:{colour};font-weight:bold">{status}</td>{cells}<td>{notes}</td></tr>{images}')
    flip_note = '' if flip_available else '<p>FLIP not computed: install the flip-evaluator package.</p>'
    page = f'''<!doctype html>
<html><head><meta charset="utf-8"><title>XP12 parity report</title>
<style>
body {{ font-family: sans-serif; background: #1e1e1e; color: #ddd; }}
table {{ border-collapse: collapse; }} td, th {{ border-bottom: 1px solid #444; padding: 4px 8px; vertical-align: top; }}
img {{ max-width: 32%; }}
</style></head><body>
<h1>X-Plane 12 parity</h1>
<p>Reference vs SEDX per scenario. Left image X-Plane 12, middle SEDX, right CIEDE2000 (black 0, white 20+).</p>
{flip_note}
<table><tr><th>Scenario</th><th>Category</th><th>Result</th><th>mean ΔE00</th><th>p95 ΔE00</th><th>FLIP</th><th>lum EMD (stops)</th><th>sky ΔE00</th><th>Notes</th></tr>
{''.join(rows)}
</table></body></html>
'''
    with open(os.path.join(out_dir, 'report.html'), 'w', encoding='utf-8') as f:
        f.write(page)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--references', required=True, help='X-Plane reference folder (<id>.png, <id>.json)')
    parser.add_argument('--sedx', required=True, help='SEDX capture folder (<id>.png, <id>_sky.png)')
    parser.add_argument('--out', required=True, help='Report folder')
    parser.add_argument('--scenarios', default=os.path.join(REPO, 'tools', 'xp12_reference_probe', 'scenarios', 'xp12_parity.json'))
    parser.add_argument('--thresholds', default=os.path.join(HERE, 'thresholds.json'))
    parser.add_argument('--only', default=None, help='Regular expression: compare matching scenario ids only')
    args = parser.parse_args(argv)

    scenarios = load_scenarios(args.scenarios)
    default_limits, rules = load_thresholds(args.thresholds)
    os.makedirs(args.out, exist_ok=True)

    ids = sorted({os.path.splitext(name)[0] for name in os.listdir(args.references)
                  if name.endswith('.png') and not name.endswith('_sky.png')})
    if args.only:
        ids = [i for i in ids if re.search(args.only, i)]

    results = []
    failed = False
    for scenario_id in ids:
        category, limits = limits_for(scenario_id, default_limits, rules)
        sedx_path = os.path.join(args.sedx, scenario_id + '.png')
        result = {'id': scenario_id, 'category': category, 'limits': limits, 'failures': [], 'warnings': []}
        if not os.path.exists(sedx_path):
            result['status'] = 'missing'
            result['warnings'].append('no SEDX capture')
            results.append(result)
            continue
        metrics, warnings = compare(scenario_id, os.path.join(args.references, scenario_id + '.png'), sedx_path,
                                    os.path.join(args.sedx, scenario_id + '_sky.png'), scenarios.get(scenario_id, {}), args.out)
        result['status'] = 'compared'
        result['metrics'] = metrics
        result['warnings'] = warnings
        result['failures'] = check(metrics, limits)
        failed = failed or bool(result['failures'])
        results.append(result)
        print(f'{scenario_id:32s} {"FAIL" if result["failures"] else "pass"}  mean dE {metrics["mean_de2000"]:.2f}  '
              f'p95 {metrics["p95_de2000"]:.2f}  EMD {metrics["luminance_emd"]:.3f}' + (f'  FLIP {metrics["mean_flip"]:.3f}' if 'mean_flip' in metrics else ''))

    summary = {
        'references': os.path.abspath(args.references),
        'sedx': os.path.abspath(args.sedx),
        'flip_available': flip_evaluator is not None,
        'compared': sum(1 for r in results if r['status'] == 'compared'),
        'missing': sum(1 for r in results if r['status'] == 'missing'),
        'failed': sum(1 for r in results if r['failures']),
        'scenarios': results,
    }
    with open(os.path.join(args.out, 'summary.json'), 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=1)
    write_report(args.out, results, flip_evaluator is not None)
    print(f'{summary["compared"]} compared, {summary["failed"]} failed, {summary["missing"]} without a SEDX capture -> {args.out}')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
