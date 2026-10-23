"""Renders parity scenarios in Scenery Editor X, one editor run per scenario, and reports how each run performed.

The SEDX half of the X-Plane 12 bridge: the reference probe renders a scenario file inside X-Plane, this renders the
same file in SEDX (--env-scenario), and tools/image_compare/xp_compare.py compares the two folders. Every run also
logs SEDX's frame profiler (--perf-log), so the same captures give the performance numbers per airport:

    python -I tools/xp12_reference_probe/render_scenarios.py                       # the test airports, every view
    python -I tools/xp12_reference_probe/render_scenarios.py "*_aerial_sunp40" "*_rwy_sunp40"
    python -I tools/xp12_reference_probe/render_scenarios.py --scenarios tools/xp12_reference_probe/scenarios/xp12_parity.json ksea_aerial_sunp40
    python -I tools/xp12_reference_probe/render_scenarios.py --summarize-only      # parse the logs of an earlier run again

For each scenario it writes <out>/<id>.png (and the editor's <id>_sky.png), <out>/<id>.log (the editor's log), and
after the run <out>/summary.json and <out>/summary.md: load times (airport index, library, DSF overlay, base terrain,
tile scenery, object placement, the whole airport), scene size, frame rate and the heaviest CPU scopes once the
scene has settled (the last reports before the capture), the GPU passes' milliseconds, peak memory (Windows), errors and warnings. Standard library only; run it
from anywhere (paths are taken from the repository root). One editor at a time: they share one log file.
"""
import argparse
import ctypes
import fnmatch
import json
import os
import re
import shlex
import statistics
import subprocess
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_SCENARIOS = os.path.join('tools', 'xp12_reference_probe', 'scenarios', 'xp12_airports.json')
STEADY_INTERVALS = 3        # The last Perf reports (2 s each) before the capture: objects and textures still stream in after the airport opens


def editor_log_path():
    """The editor's log (VFS::MakeAppLayout: <settings>/logs/SceneryEditorX.log)."""
    if sys.platform == 'win32':
        settings = os.path.join(os.environ.get('APPDATA', ''), 'SceneryEditorX')
    elif sys.platform == 'darwin':
        settings = os.path.expanduser('~/Library/Application Support/SceneryEditorX')
    else:
        settings = os.path.join(os.environ.get('XDG_CONFIG_HOME') or os.path.expanduser('~/.config'), 'SceneryEditorX')
    return os.path.join(settings, 'logs', 'SceneryEditorX.log')


def default_out_folder(scenario_file):
    """Captures are rebuildable: they go to the editor's cache folder (scenario_captures/<scenario file name>)."""
    if sys.platform == 'win32':
        cache = os.path.join(os.environ.get('LOCALAPPDATA', ''), 'SceneryEditorX', 'cache')
    elif sys.platform == 'darwin':
        cache = os.path.expanduser('~/Library/Caches/SceneryEditorX')
    else:
        cache = os.path.join(os.environ.get('XDG_CACHE_HOME') or os.path.expanduser('~/.cache'), 'SceneryEditorX')
    return os.path.join(cache, 'scenario_captures', os.path.splitext(os.path.basename(scenario_file))[0])


def default_exe():
    suffix = '.exe' if sys.platform == 'win32' else ''
    for name in ('SceneryEditorX', 'SceneryEditorX_debug'):
        path = os.path.join(REPO, 'bin', name + suffix)
        if os.path.exists(path):
            return path
    return os.path.join(REPO, 'bin', 'SceneryEditorX' + suffix)


# -------------------------------------------------------
# Peak memory of the editor process (Windows: the peaks only grow, so the last sample before exit is the run's peak)

class _MemoryCounters(ctypes.Structure):
    _fields_ = [('cb', ctypes.c_uint32), ('PageFaultCount', ctypes.c_uint32),
                ('PeakWorkingSetSize', ctypes.c_size_t), ('WorkingSetSize', ctypes.c_size_t),
                ('QuotaPeakPagedPoolUsage', ctypes.c_size_t), ('QuotaPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t), ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
                ('PagefileUsage', ctypes.c_size_t), ('PeakPagefileUsage', ctypes.c_size_t)]


def sample_memory(process):
    if sys.platform != 'win32':
        return None
    try:
        counters = _MemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        handle = ctypes.c_void_p(int(process._handle))
        if not ctypes.windll.kernel32.K32GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
            return None
        return {'peak_working_set_mb': counters.PeakWorkingSetSize / 2 ** 20, 'peak_commit_mb': counters.PeakPagefileUsage / 2 ** 20}
    except (AttributeError, OSError, ValueError):
        return None


# -------------------------------------------------------
# Log parsing

LINE = re.compile(r'^\[(\d\d):(\d\d):(\d\d)\] \[(\w+)\] [^:]*: (.*)$')
PATTERNS = {
    'airport_index': re.compile(r'^\[XPlaneAirports\] (?P<airports>\d+) airports .*\((?P<ms>\d+) ms\)$'),
    'library': re.compile(r'^\[XPlaneLibrary\] (?P<assets>\d+) assets .*\((?P<ms>\d+) ms\)$'),
    'dsf_overlay': re.compile(r'^\[XPlaneAirport\] (?P<tile>[+-]\d+[+-]\d+\.dsf): .* in (?P<ms>\d+) ms$'),
    'terrain': re.compile(r'^\[XPlaneTerrain\] \w+: (?P<triangles>\d+) triangles in (?P<parts>\d+) parts .* in (?P<ms>\d+) ms \((?P<reading_ms>\d+) ms reading\)$'),
    'tile_scenery': re.compile(r'^\[XPlaneTerrain\] \w+: tile scenery: (?P<placements>\d+) placements .*, (?P<triangles>\d+) triangles, (?P<objects>\d+) objects in (?P<ms>\d+) ms'),
    'placement': re.compile(r'^\[XPlaneAirport\] Placed (?P<objects>\d+) objects \((?P<lights>\d+) light fixtures\) in (?P<ms>\d+) ms$'),
    'airport': re.compile(r'^\[XPlaneAirport\] \w+ .*: (?P<parts>\d+) parts .*, (?P<triangles>\d+) triangles, (?P<objects>\d+) objects, (?P<entities>\d+) entities, (?P<textures>\d+) textures in (?P<ms>\d+) ms$'),
}
OPENED = re.compile(r'^\[Airports\] Opened (?P<icao>\w+)\s+(?P<name>.*) from (?P<pack>.*?)( \(read-only\))?$')
CAPTURED = re.compile(r'^\[Scenario\] Scenario (?P<id>\S+) captured to ')
EXPOSURE = re.compile(r'^\[Scenario\] Scenario \S+ exposure: EV100 (?P<ev100>[-\d.]+), metered (?P<metered>[-\d.]+)')
PERF = re.compile(r'^\[Perf\] (?P<fps>[\d.]+) fps \((?P<ms>[\d.]+) ms/frame, worst (?P<worst>[\d.]+) ms\) \| (?P<scopes>.*)$')
SCOPE = re.compile(r'^(?P<name>.+) (?P<ms>[\d.]+) ms \(max (?P<max>[\d.]+)\)$')
COUNTERS = re.compile(r'^\[Perf\] Counters: (?P<counters>.*)$')


def parse_log(path):
    result = {'load': {}, 'scene': {}, 'perf': None, 'gpu_ms': {}, 'counters': {}, 'errors': [], 'warnings': 0}
    if not os.path.exists(path):
        result['errors'].append('no log')
        return result
    start = None
    opened = None
    perf_lines, counter_lines = [], []
    with open(path, encoding='utf-8', errors='replace') as f:
        for raw in f:
            match = LINE.match(raw.rstrip('\n'))
            if not match:
                continue
            seconds = int(match.group(1)) * 3600 + int(match.group(2)) * 60 + int(match.group(3))
            if start is None:
                start = seconds
            elapsed = (seconds - start) % 86400
            level, message = match.group(4), match.group(5)
            if level in ('error', 'critical'):
                result['errors'].append(message[:300])
            elif level in ('warning', 'warn'):
                result['warnings'] += 1
            for key, pattern in PATTERNS.items():
                found = pattern.match(message)
                if found:
                    values = {k: (int(v) if v.isdigit() else v) for k, v in found.groupdict().items()}
                    if key == 'dsf_overlay' and key in result['load']:
                        result['load'][key]['ms'] += values['ms']     # Every overlay tile the airport reaches into
                    else:
                        result['load'][key] = values
            if (found := OPENED.match(message)):
                opened = elapsed
                result['scene'].update({'icao': found['icao'], 'name': found['name'].strip(), 'pack': found['pack'], 'ready_s': elapsed})
            elif CAPTURED.match(message):
                result['scene']['captured_s'] = elapsed
            elif (found := EXPOSURE.match(message)):
                result['exposure'] = {'ev100': float(found['ev100']), 'metered': float(found['metered'])}
            elif (found := PERF.match(message)):
                if opened is not None:
                    perf_lines.append(found)
            elif (found := COUNTERS.match(message)):
                if opened is not None:
                    counter_lines.append(found['counters'])

    # Steady state, just before the capture: median frame rate, mean of every scope, median of every counter
    perf_lines = perf_lines[-STEADY_INTERVALS:]
    counter_lines = counter_lines[-STEADY_INTERVALS:]
    if perf_lines:
        scopes = {}
        for line in perf_lines:
            for entry in line['scopes'].split(' | '):
                found = SCOPE.match(entry.strip())
                if found:
                    scopes.setdefault(found['name'], []).append(float(found['ms']))
        result['perf'] = {
            'intervals': len(perf_lines),
            'fps': statistics.median(float(p['fps']) for p in perf_lines),
            'frame_ms': statistics.median(float(p['ms']) for p in perf_lines),
            'worst_ms': max(float(p['worst']) for p in perf_lines),
            'scopes_ms': dict(sorted(((name, round(statistics.fmean(v), 3)) for name, v in scopes.items()), key=lambda kv: -kv[1])),
        }
    values = {}
    for line in counter_lines:
        for entry in line.split(', '):
            name, _, value = entry.rpartition(' ')
            try:
                values.setdefault(name, []).append(float(value))
            except ValueError:
                pass
    for name, samples in values.items():
        median = statistics.median(samples)
        if name.startswith('gpu ms '):
            result['gpu_ms'][name[len('gpu ms '):]] = round(median, 3)
        else:
            result['counters'][name] = median
    return result


# -------------------------------------------------------

def run_scenario(args, scenario_file, scenario_id, out):
    png = os.path.join(out, scenario_id + '.png')
    command = [args.exe, '--env-scenario', scenario_file, '--env-scenario-index', scenario_id, '--lock-camera', '--capture-png', png]
    if args.frames is not None:
        command += ['--capture-after-frames', str(args.frames)]
    if not args.no_perf:
        command.append('--perf-log')
    if not args.vsync:
        command.append('--no-vsync')    # The frame as fast as it renders, not the display's refresh
    command += args.extra
    started = time.monotonic()
    process = subprocess.Popen(command, cwd=REPO, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    memory = None
    timed_out = False
    while process.poll() is None:
        memory = sample_memory(process) or memory
        if time.monotonic() - started > args.timeout:
            timed_out = True
            process.kill()
            process.wait()
            break
        time.sleep(0.5)
    wall = time.monotonic() - started
    log_copy = os.path.join(out, scenario_id + '.log')
    try:
        with open(editor_log_path(), 'rb') as source, open(log_copy, 'wb') as target:
            target.write(source.read())
    except OSError:
        pass
    result = parse_log(log_copy)
    result.update({'id': scenario_id, 'returncode': process.returncode, 'timed_out': timed_out, 'wall_s': round(wall, 1),
                   'png': os.path.exists(png), 'memory': memory})
    return result


def fmt(value, digits=0):
    if value is None:
        return '-'
    return f'{value:,.{digits}f}'


def cpu_ms(result):
    """The CPU's work in a frame: the main thread's update and UI plus the render thread's tick (they run one after
    the other), without the waits; unlike the frame rate it isn't held at the display's refresh."""
    scopes = (result['perf'] or {}).get('scopes_ms', {})
    if 'Render: Tick (CPU)' not in scopes:
        return None
    return scopes.get('Main: update + UI', 0.0) + scopes['Render: Tick (CPU)']


def median_of(values):
    values = [v for v in values if v is not None]
    return statistics.median(values) if values else None


def write_summary(out, scenario_file, results, exe):
    with open(os.path.join(out, 'summary.json'), 'w', encoding='utf-8', newline='\n') as f:
        json.dump({'scenarios': scenario_file, 'exe': exe, 'results': results}, f, indent='\t')
        f.write('\n')

    lines = [f'# SEDX scenario runs: {os.path.basename(scenario_file)}', '', f'Editor: `{os.path.relpath(exe, REPO)}`. '
             'Load times from each run\'s log; frame numbers are the last reports before the capture. CPU ms = main '
             'thread update + UI plus render thread tick per frame.', '']

    # Per airport: medians over its runs (each run loads the airport from nothing)
    airports = {}
    for r in results:
        if r['scene'].get('icao'):
            airports.setdefault(r['scene']['icao'], []).append(r)
    if airports:
        lines += ['## Per airport', '',
                  '| Airport | Pack | Runs | Ready s | Airport ms | DSF overlay ms | Terrain ms | Tile scenery ms | Placement ms | Triangles | Objects | Entities | CPU ms | GPU ms | Peak commit MB |',
                  '|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|']
        for icao, runs in airports.items():
            def load(key, field='ms'):
                return median_of((r['load'].get(key) or {}).get(field) for r in runs)
            peak = [(r['memory'] or {}).get('peak_commit_mb') for r in runs]
            peak = [p for p in peak if p is not None]
            lines.append(f"| {icao} | {runs[0]['scene'].get('pack', '-')} | {len(runs)} | {fmt(median_of(r['scene'].get('ready_s') for r in runs))} | "
                         f"{fmt(load('airport'))} | {fmt(load('dsf_overlay'))} | {fmt(load('terrain'))} | {fmt(load('tile_scenery'))} | "
                         f"{fmt(load('placement'))} | {fmt(load('airport', 'triangles'))} | {fmt(load('airport', 'objects'))} | "
                         f"{fmt(load('airport', 'entities'))} | {fmt(median_of(cpu_ms(r) for r in runs), 1)} | "
                         f"{fmt(median_of(r['gpu_ms'].get('total') for r in runs), 2)} | {fmt(max(peak) if peak else None)} |")
        lines += ['', '## Per scenario', '']

    lines += ['| Scenario | Ready s | Airport ms | Terrain ms | Tile scenery ms | Triangles | Objects | fps | ms/frame | worst ms | CPU ms | GPU ms | Peak commit MB | Errors |',
              '|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|']
    for r in results:
        load, perf, scene = r['load'], r['perf'] or {}, r['scene']
        triangles = (load.get('airport') or {}).get('triangles')
        objects = (load.get('airport') or {}).get('objects')
        status = '' if r['png'] and not r['timed_out'] else (' (timed out)' if r['timed_out'] else ' (no capture)')
        lines.append(f"| {r['id']}{status} | {fmt(scene.get('ready_s'))} | {fmt((load.get('airport') or {}).get('ms'))} | "
                     f"{fmt((load.get('terrain') or {}).get('ms'))} | {fmt((load.get('tile_scenery') or {}).get('ms'))} | "
                     f"{fmt(triangles)} | {fmt(objects)} | {fmt(perf.get('fps'), 1)} | {fmt(perf.get('frame_ms'), 1)} | "
                     f"{fmt(perf.get('worst_ms'), 1)} | {fmt(cpu_ms(r), 1)} | {fmt(r['gpu_ms'].get('total'), 2)} | "
                     f"{fmt((r['memory'] or {}).get('peak_commit_mb'))} | {len(r['errors'])} |")

    # The heaviest CPU scopes and GPU passes, per scenario
    lines += ['', '## Heaviest CPU scopes and GPU passes', '']
    for r in results:
        perf = r['perf'] or {}
        scopes = ', '.join(f'{name} {ms:.1f}' for name, ms in list(perf.get('scopes_ms', {}).items())[:6])
        passes = ', '.join(f'{name} {ms:.2f}' for name, ms in sorted(((n, v) for n, v in r['gpu_ms'].items() if n != 'total'), key=lambda kv: -kv[1])[:6])
        lines.append(f"- **{r['id']}**: CPU ms {scopes or '-'}; GPU ms {passes or '-'}")
    errors = [(r['id'], e) for r in results for e in r['errors']]
    if errors:
        lines += ['', '## Errors', '']
        lines += [f'- {sid}: {message}' for sid, message in errors[:60]]
    with open(os.path.join(out, 'summary.md'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('patterns', nargs='*', help='scenario ids or fnmatch patterns (default: every scenario)')
    parser.add_argument('--scenarios', default=DEFAULT_SCENARIOS, help='scenario file (default: the test airports)')
    parser.add_argument('--exe', default=default_exe(), help='editor executable (default: bin/SceneryEditorX, else the debug build)')
    parser.add_argument('--out', help='output folder (default: <editor cache>/scenario_captures/<scenario file name>)')
    parser.add_argument('--frames', type=int, help='frames to settle before the capture (default: the scenario\'s settle_frames)')
    parser.add_argument('--timeout', type=float, default=1200.0, help='seconds before a run is stopped (default 1200)')
    parser.add_argument('--no-perf', action='store_true', help='without --perf-log')
    parser.add_argument('--vsync', action='store_true', help='keep vsync on (runs pass --no-vsync otherwise)')
    parser.add_argument('--extra', default='', help='more editor arguments, e.g. "--no-water --no-autogen"')
    parser.add_argument('--summarize-only', action='store_true', help='parse the logs already in the output folder again')
    args = parser.parse_args()
    args.extra = shlex.split(args.extra)

    scenario_file = os.path.relpath(os.path.abspath(args.scenarios), REPO) if not os.path.isabs(args.scenarios) else args.scenarios
    with open(os.path.join(REPO, scenario_file), encoding='utf-8') as f:
        ids = [s['id'] for s in json.load(f)['scenarios']]
    if args.patterns:
        ids = [i for i in ids if any(fnmatch.fnmatchcase(i, p) for p in args.patterns)]
    if not ids:
        sys.exit('no scenario matches ' + ' '.join(args.patterns))
    out = os.path.abspath(args.out or default_out_folder(scenario_file))
    os.makedirs(out, exist_ok=True)

    # What only the run itself saw (exit code, time, memory) comes back from the earlier summary
    earlier = {}
    if args.summarize_only and os.path.exists(os.path.join(out, 'summary.json')):
        with open(os.path.join(out, 'summary.json'), encoding='utf-8') as f:
            earlier = {r['id']: r for r in json.load(f).get('results', [])}

    results = []
    for number, scenario_id in enumerate(ids, 1):
        if args.summarize_only:
            result = parse_log(os.path.join(out, scenario_id + '.log'))
            run = earlier.get(scenario_id, {})
            result.update({'id': scenario_id, 'returncode': run.get('returncode'), 'timed_out': run.get('timed_out', False),
                           'wall_s': run.get('wall_s'), 'png': os.path.exists(os.path.join(out, scenario_id + '.png')),
                           'memory': run.get('memory')})
        else:
            result = run_scenario(args, scenario_file, scenario_id, out)
        perf = result['perf'] or {}
        print(f"[{number}/{len(ids)}] {scenario_id}: rc {result['returncode']}, {result['wall_s']} s, ready {result['scene'].get('ready_s')} s, "
              f"{fmt(perf.get('fps'), 1)} fps, GPU {fmt(result['gpu_ms'].get('total'), 2)} ms, {len(result['errors'])} errors", flush=True)
        results.append(result)
    write_summary(out, scenario_file, results, args.exe)
    print('Summary: ' + os.path.join(out, 'summary.md'))
    sys.exit(0 if all(r['png'] and not r['timed_out'] for r in results) else 1)


if __name__ == '__main__':
    main()
