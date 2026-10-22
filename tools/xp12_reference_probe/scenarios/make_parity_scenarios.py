"""Writes xp12_parity.json, the scenario set X-Plane 12 (through the reference probe) and SEDX (through
--env-scenario) both render for the parity comparison (plan TASK-004 / TASK-102).

Run with:  python -I tools/xp12_reference_probe/scenarios/make_parity_scenarios.py
Standard library only. Local times for the sun-elevation sweep are solved with the NOAA solar-position
equations (the same ones source/SceneryEditorX/environment/solar_position.cpp uses), so every scenario names
the sun elevation it was made for.
"""
import json
import math
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'xp12_parity.json')

XP_BUILD = '12.4.4-b4'

# KSEA: runway 16L threshold (apt.dat 12.00, Global Airports) and the bearing to the 34R threshold.
KSEA_16L = (47.4638283, -122.3077522)
KSEA_34R = (47.4311753, -122.3080412)
KSEA_AERIAL = (47.4800, -122.3080)      # North of the field, looking south over it
KSEA_UTC_OFFSET_SUMMER = -7.0           # PDT
KSEA_UTC_OFFSET_WINTER = -8.0           # PST
YSSY_AERIAL = (-33.9250, 151.1770)      # North of YSSY, looking south over it
YSSY_UTC_OFFSET_SUMMER = 11.0           # AEDT
# The SEDX_Calibration tiles (test_scenery/make_calibration_pack.py), seen from 22 m north of their center, 8 m up
CALIBRATION_CENTER = (47.4560, -122.31442)
CALIBRATION_CAMERA_NORTH_M = 22.0
CALIBRATION_PROJECT = 'tools/xp12_reference_probe/test_scenery/SEDX_Calibration/SEDX_Calibration.edX'   # What SEDX opens; X-Plane gets the pack
# Art controls a twin shot sets (the probe sets and restores them in X-Plane; SEDX emulates them): the twin's difference
# from its base shot isolates one X-Plane effect
NO_AO = {'ssao/enable': 0}
NO_SSR = {'debug/kill_ssr': 1}


def bearing(a, b):
    """Initial true bearing from a to b (degrees clockwise from north)."""
    lat1, lon1, lat2, lon2 = map(math.radians, (a[0], a[1], b[0], b[1]))
    y = math.sin(lon2 - lon1) * math.cos(lat2)
    x = math.cos(lat1) * math.sin(lat2) - math.sin(lat1) * math.cos(lat2) * math.cos(lon2 - lon1)
    return (math.degrees(math.atan2(y, x)) + 360.0) % 360.0


def julian_day(year, month, day, utc_hours):
    if month <= 2:
        year -= 1
        month += 12
    a = year // 100
    b = 2 - a + a // 4
    return int(365.25 * (year + 4716)) + int(30.6001 * (month + 1)) + day + b - 1524.5 + utc_hours / 24.0


def sun_elevation(lat, lon, year, month, day, utc_hours):
    """NOAA solar calculator: apparent elevation in degrees (with refraction)."""
    jd = julian_day(year, month, day, utc_hours)
    t = (jd - 2451545.0) / 36525.0
    l0 = (280.46646 + t * (36000.76983 + t * 0.0003032)) % 360.0
    m = 357.52911 + t * (35999.05029 - 0.0001537 * t)
    e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t)
    mr = math.radians(m)
    c = math.sin(mr) * (1.914602 - t * (0.004817 + 0.000014 * t)) + math.sin(2 * mr) * (0.019993 - 0.000101 * t) + math.sin(3 * mr) * 0.000289
    true_long = l0 + c
    omega = 125.04 - 1934.136 * t
    app_long = true_long - 0.00569 - 0.00478 * math.sin(math.radians(omega))
    mean_obliq = 23.0 + (26.0 + (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0
    obliq = mean_obliq + 0.00256 * math.cos(math.radians(omega))
    decl = math.asin(math.sin(math.radians(obliq)) * math.sin(math.radians(app_long)))
    y = math.tan(math.radians(obliq / 2.0)) ** 2
    l0r = math.radians(l0)
    eq_time = 4.0 * math.degrees(y * math.sin(2 * l0r) - 2 * e * math.sin(mr) + 4 * e * y * math.sin(mr) * math.cos(2 * l0r)
                                 - 0.5 * y * y * math.sin(4 * l0r) - 1.25 * e * e * math.sin(2 * mr))
    true_solar_min = (utc_hours * 60.0 + eq_time + 4.0 * lon) % 1440.0
    hour_angle = true_solar_min / 4.0 + 180.0 if true_solar_min < 0 else true_solar_min / 4.0 - 180.0
    latr = math.radians(lat)
    cos_zenith = math.sin(latr) * math.sin(decl) + math.cos(latr) * math.cos(decl) * math.cos(math.radians(hour_angle))
    zenith = math.degrees(math.acos(max(-1.0, min(1.0, cos_zenith))))
    elev = 90.0 - zenith
    # Atmospheric refraction (NOAA)
    if elev > 85.0:
        refraction = 0.0
    else:
        te = math.tan(math.radians(elev))
        if elev > 5.0:
            refraction = 58.1 / te - 0.07 / te ** 3 + 0.000086 / te ** 5
        elif elev > -0.575:
            refraction = 1735.0 + elev * (-518.2 + elev * (103.4 + elev * (-12.79 + elev * 0.711)))
        else:
            refraction = -20.772 / te
        refraction /= 3600.0
    return elev + refraction


def morning_time_for_elevation(lat, lon, date, utc_offset, target):
    """Local time (hours) before solar noon when the sun reaches an elevation, by bisection."""
    year, month, day = date
    # Solar noon: the maximum elevation, searched on a 1-minute grid
    noon = max((h / 60.0 for h in range(0, 24 * 60)),
               key=lambda local: sun_elevation(lat, lon, year, month, day, local - utc_offset))
    lo, hi = noon - 12.0, noon
    for _ in range(60):
        mid = 0.5 * (lo + hi)
        if sun_elevation(lat, lon, year, month, day, mid - utc_offset) < target:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi) % 24.0


def clock(hours):
    total = int(round(hours * 3600.0))
    return f'{total // 3600 % 24:02d}:{total // 60 % 60:02d}:{total % 60:02d}'


def camera(name):
    if name == 'rwy16l':
        lat, lon = KSEA_16L
        return {'icao': 'KSEA', 'lat': lat, 'lon': lon, 'alt_m_agl': 2.0, 'heading': round(bearing(KSEA_16L, KSEA_34R), 2), 'pitch': 0.0}
    if name == 'aerial':
        lat, lon = KSEA_AERIAL
        return {'icao': 'KSEA', 'lat': lat, 'lon': lon, 'alt_m_agl': 600.0, 'heading': 180.0, 'pitch': -20.0}
    if name == 'calib':
        phi = math.radians(CALIBRATION_CENTER[0])
        m_lat = 111132.954 - 559.822 * math.cos(2 * phi) + 1.175 * math.cos(4 * phi)
        lat = CALIBRATION_CENTER[0] + CALIBRATION_CAMERA_NORTH_M / m_lat
        return {'icao': 'KSEA', 'project': CALIBRATION_PROJECT, 'lat': round(lat, 8), 'lon': CALIBRATION_CENTER[1],
                'alt_m_agl': 8.0, 'heading': 180.0, 'pitch': -21.0}
    if name == 'yssy_aerial':
        lat, lon = YSSY_AERIAL
        return {'icao': 'YSSY', 'lat': lat, 'lon': lon, 'alt_m_agl': 600.0, 'heading': 180.0, 'pitch': -20.0}
    raise ValueError(name)


# Locked EV100 per sun elevation: near what a camera meters there, so locked images aren't blown out or black
EV100_LOCK = {-12: 5.0, -6: 7.0, -2: 9.0, 2: 11.0, 8: 12.5, 20: 13.5, 40: 14.0, 58: 14.0}

WEATHER = {
    'clean': {'visibility_m': 80000.0},
    'hazy': {'visibility_m': 8000.0},
    'foggy': {'visibility_m': 800.0},
    # 8/8 stratus, base 600 m above KSEA (elevation 132 m)
    'overcast': {'visibility_m': 30000.0, 'clouds': [{'base_msl_m': 732.0, 'top_msl_m': 1400.0, 'coverage': 1.0, 'type': 1}]},
    'snowy': {'visibility_m': 30000.0, 'snow_cover': 1.0, 'runway_friction': 9, 'temperature_c': -5.0},
    'wet2': {'visibility_m': 20000.0, 'runway_friction': 2, 'rain_percent': 0.6, 'temperature_c': 12.0},
    'wet5': {'visibility_m': 20000.0, 'runway_friction': 5, 'rain_percent': 0.6, 'temperature_c': 12.0},
}


def main():
    summer = (2026, 7, 15)
    scenarios = []

    def add(sid, cam, date, local_time, utc_offset, weather='clean', **extra):
        entry = {'id': sid}
        entry.update(camera(cam))
        entry['date'] = '%04d-%02d-%02d' % date
        entry['local_time'] = clock(local_time)
        entry['utc_offset'] = utc_offset
        entry.update(WEATHER[weather])
        entry.update(extra)
        scenarios.append(entry)

    # Sun-elevation sweep (clean air), morning side, from both cameras; each with auto and locked exposure
    for elevation in (-12, -6, -2, 2, 8, 20, 40, 58):
        local = morning_time_for_elevation(KSEA_16L[0], KSEA_16L[1], summer, KSEA_UTC_OFFSET_SUMMER, elevation)
        tag = f'm{-elevation:02d}' if elevation < 0 else f'p{elevation:02d}'
        for cam in ('rwy16l', 'aerial'):
            note = f'sun elevation {elevation} deg'
            add(f'ksea_{cam}_sun{tag}', cam, summer, local, KSEA_UTC_OFFSET_SUMMER, notes=note)
            if cam == 'aerial' and elevation == 40:
                add(f'ksea_{cam}_sun{tag}_noao', cam, summer, local, KSEA_UTC_OFFSET_SUMMER,
                    notes=note + f'; ambient occlusion off: ksea_{cam}_sun{tag} minus this is the AO (plan CRIT-008)',
                    art_controls=NO_AO)
            add(f'ksea_{cam}_sun{tag}_ev', cam, summer, local, KSEA_UTC_OFFSET_SUMMER, ev100_lock=EV100_LOCK[elevation], notes=note + ', locked exposure')

    # Weather at the 40 degree time
    day40 = morning_time_for_elevation(KSEA_16L[0], KSEA_16L[1], summer, KSEA_UTC_OFFSET_SUMMER, 40)
    for weather, cams in (('hazy', ('rwy16l', 'aerial')), ('foggy', ('rwy16l', 'aerial')), ('overcast', ('rwy16l', 'aerial')),
                          ('snowy', ('rwy16l', 'aerial')), ('wet2', ('rwy16l',)), ('wet5', ('rwy16l',))):
        for cam in cams:
            add(f'ksea_{cam}_{weather}', cam, summer, day40, KSEA_UTC_OFFSET_SUMMER, weather, notes='sun elevation 40 deg')
    add('ksea_rwy16l_wet5_nossr', 'rwy16l', summer, day40, KSEA_UTC_OFFSET_SUMMER, 'wet5',
        notes='sun elevation 40 deg; screen-space reflections off: ksea_rwy16l_wet5 minus this is the SSR on the wet runway (plan CRIT-009)',
        art_controls=NO_SSR)

    # Calibration tiles: materials in sun (auto and locked exposure), LIT ramp at night, weather masks wet and snowy
    day20 = morning_time_for_elevation(KSEA_16L[0], KSEA_16L[1], summer, KSEA_UTC_OFFSET_SUMMER, 20)
    for elevation, local in ((40, day40), (20, day20)):
        add(f'ksea_calib_sunp{elevation:02d}', 'calib', summer, local, KSEA_UTC_OFFSET_SUMMER, notes=f'calibration tiles, sun elevation {elevation} deg')
        if elevation == 40:
            add(f'ksea_calib_sunp{elevation:02d}_noao', 'calib', summer, local, KSEA_UTC_OFFSET_SUMMER,
                notes=f'calibration tiles, sun elevation {elevation} deg; ambient occlusion off: ksea_calib_sunp{elevation:02d} minus this is the AO (plan CRIT-008)',
                art_controls=NO_AO)
            add(f'ksea_calib_sunp{elevation:02d}_nossr', 'calib', summer, local, KSEA_UTC_OFFSET_SUMMER,
                notes=f'calibration tiles, sun elevation {elevation} deg; screen-space reflections off: ksea_calib_sunp{elevation:02d} minus this is the SSR on the gloss and metal ramps (plan CRIT-009)',
                art_controls=NO_SSR)
        add(f'ksea_calib_sunp{elevation:02d}_ev', 'calib', summer, local, KSEA_UTC_OFFSET_SUMMER, ev100_lock=EV100_LOCK[elevation],
            notes=f'calibration tiles, sun elevation {elevation} deg, locked exposure')
    add('ksea_calib_night', 'calib', summer, 23.5, KSEA_UTC_OFFSET_SUMMER, notes='calibration tiles: LIT luminance ramp')
    add('ksea_calib_night_ev', 'calib', summer, 23.5, KSEA_UTC_OFFSET_SUMMER, ev100_lock=5.0, notes='calibration tiles: LIT luminance ramp, locked exposure')
    for weather in ('wet5', 'snowy'):
        add(f'ksea_calib_{weather}', 'calib', summer, day40, KSEA_UTC_OFFSET_SUMMER, weather, notes='calibration tiles: WEATHER masks')

    # Seasons at noon
    for month, offset in ((1, KSEA_UTC_OFFSET_WINTER), (4, KSEA_UTC_OFFSET_SUMMER), (7, KSEA_UTC_OFFSET_SUMMER), (10, KSEA_UTC_OFFSET_SUMMER)):
        add(f'ksea_aerial_season{month:02d}', 'aerial', (2026, month, 15), 12.0 if offset == KSEA_UTC_OFFSET_WINTER else 13.0, offset,
            notes='local solar noon (about 12:00 standard time)')
    add('yssy_aerial_season01', 'yssy_aerial', (2026, 1, 15), 13.0, YSSY_UTC_OFFSET_SUMMER, notes='southern summer, local solar noon')

    # Night
    for cam in ('rwy16l', 'aerial'):
        add(f'ksea_{cam}_night', cam, summer, 23.5, KSEA_UTC_OFFSET_SUMMER, notes='23:30 local')

    data = {
        'about': 'Parity scenarios rendered by X-Plane 12 (tools/xp12_reference_probe) and SEDX (--env-scenario). '
                 'Generated by make_parity_scenarios.py; edit that script, not this file.',
        'xp_build': XP_BUILD,
        'defaults': {
            'fov': 60.0, 'width': 1280, 'height': 720,
            'visibility_m': 80000.0, 'clouds': [], 'rain_percent': 0.0, 'runway_friction': 0, 'snow_cover': 0.0,
            'temperature_c': 15.0, 'settle_frames': 600, 'ev100_lock': None, 'ui_mask': [],
        },
        'scenarios': scenarios,
    }
    with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(data, f, indent='\t')
        f.write('\n')
    print(f'{len(scenarios)} scenarios -> {OUT}')


if __name__ == '__main__':
    main()
