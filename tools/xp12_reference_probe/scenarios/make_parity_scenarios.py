"""Writes the scenario sets X-Plane 12 (through the reference probe) and SEDX (through --env-scenario) both render:
xp12_parity.json, the KSEA parity set (plan TASK-004 / TASK-102), and xp12_airports.json, the same kinds of views at
the standard test airports used for feature, performance and parity checks: KGON, KDEN, KORD, KBOS, KBTV in the United
States, and EIDW, LEBL, EGLL, LSZH, OMDB, RCTP, RJTT, YSSY, SBGR, CYYZ around the world.

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
OUT_AIRPORTS = os.path.join(HERE, 'xp12_airports.json')

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
    if sun_elevation(lat, lon, year, month, day, noon - utc_offset) < target:
        raise ValueError(f'the sun never reaches {target} deg at ({lat}, {lon}) on {year}-{month:02d}-{day:02d}')
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

# The standard test airports, each from the pack X-Plane uses for it (scenery_packs.ini order, then Global Airports):
# field datum (apt.dat 1302 datum_lat / datum_lon), elevation, UTC offset on the summer date and in winter (standard
# time; the same where there is no daylight saving), a runway threshold and the threshold it looks toward (apt.dat
# 12.00), and for coastal fields a low view over the water inside the terrain area (airport + 1 mile). Southern
# fields take mid-January as their summer date and mid-July for the winter shot (aerial_season07): in July the sun
# never reaches 40 deg at YSSY.
TEST_AIRPORTS = [
    {'icao': 'KGON', 'notes': 'Groton-New London (Global Airports): small coastal field on Long Island Sound',
     'datum': (41.330055556, -72.045138889), 'elevation_ft': 7.0, 'utc_offset': -4.0, 'utc_offset_winter': -5.0,
     'runway': ('05', (41.3250320, -72.0480260), (41.3364130, -72.0378530)),
     'water': {'lat': 41.3225, 'lon': -72.0455, 'alt_m_agl': 60.0, 'heading': 180.0, 'pitch': -8.0}},
    {'icao': 'KDEN', 'notes': "Denver International (X-Coder's custom scenery, its overlay and mesh packs)",
     'datum': (39.861666667, -104.673166667), 'elevation_ft': 5434.0, 'utc_offset': -6.0, 'utc_offset_winter': -7.0,
     'runway': ('34L', (39.8518877, -104.6965861), (39.8957903, -104.6960844))},
    {'icao': 'KORD', 'notes': "Chicago O'Hare (Global Airports): very large default airport",
     'datum': (41.977319444, -87.908005556), 'elevation_ft': 666.0, 'utc_offset': -5.0, 'utc_offset_winter': -6.0,
     'runway': ('28R', (41.9690623, -87.8837878), (41.9689863, -87.9315632))},
    {'icao': 'KBOS', 'notes': "Boston Logan (Zero Dollar Payware's scenery and library): harbour on three sides",
     'datum': (42.362944444, -71.006388889), 'elevation_ft': 19.0, 'utc_offset': -4.0, 'utc_offset_winter': -5.0,
     'runway': ('22L', (42.3769075, -70.9992907), (42.3510634, -71.0117945)),
     'water': {'lat': 42.3530, 'lon': -70.9890, 'alt_m_agl': 60.0, 'heading': 135.0, 'pitch': -8.0}},
    {'icao': 'KBTV', 'notes': "Burlington (Global Airports): Laminar Research's home field, extra care in its default art",
     'datum': (44.471955556, -73.153277778), 'elevation_ft': 333.0, 'utc_offset': -4.0, 'utc_offset_winter': -5.0,
     'runway': ('33', (44.4657555, -73.1417677), (44.4806823, -73.1658848))},
    {'icao': 'EIDW', 'notes': 'Dublin (Global Airports): northern Europe',
     'datum': (53.421388889, -6.27), 'elevation_ft': 242.0, 'utc_offset': 1.0, 'utc_offset_winter': 0.0,
     'runway': ('28R', (53.4348400, -6.2382354), (53.4373999, -6.2848230))},
    {'icao': 'LEBL', 'notes': 'Barcelona-El Prat (Global Airports): Mediterranean coast, runway 06R/24L beside the beach',
     'datum': (41.296944444, 2.078333333), 'elevation_ft': 14.0, 'utc_offset': 2.0, 'utc_offset_winter': 1.0,
     'runway': ('24R', (41.3057260, 2.1037290), (41.2932451, 2.0672749)),
     'water': {'lat': 41.2776, 'lon': 2.0951, 'alt_m_agl': 60.0, 'heading': 154.0, 'pitch': -8.0}},
    {'icao': 'EGLL', 'notes': 'London Heathrow (custom freeware pack "EGLL - Heathrow v0.4.0"): very large European hub',
     'datum': (51.4775, -0.461388889), 'elevation_ft': 83.0, 'utc_offset': 1.0, 'utc_offset_winter': 0.0,
     'runway': ('27R', (51.4776701, -0.4332308), (51.4774794, -0.4894472))},
    {'icao': 'LSZH', 'notes': 'Zurich (Global Airports): Alpine foreland, 432 m up',
     'datum': (47.458055556, 8.548055556), 'elevation_ft': 1416.0, 'utc_offset': 2.0, 'utc_offset_winter': 1.0,
     'runway': ('16', (47.4757789, 8.5358680), (47.4454458, 8.5567363))},
    {'icao': 'OMDB', 'notes': 'Dubai International (Global Airports): desert, high sun, no daylight saving',
     'datum': (25.252777778, 55.364444444), 'elevation_ft': 62.0, 'utc_offset': 4.0, 'utc_offset_winter': 4.0,
     'runway': ('30L', (25.2353015, 55.3960034), (25.2561923, 55.3583180))},
    {'icao': 'RCTP', 'notes': 'Taiwan Taoyuan (Global Airports): subtropical East Asia',
     'datum': (25.080277778, 121.232222222), 'elevation_ft': 106.0, 'utc_offset': 8.0, 'utc_offset_winter': 8.0,
     'runway': ('05R', (25.0608389, 121.2234970), (25.0832667, 121.2519944))},
    {'icao': 'RJTT', 'notes': 'Tokyo Haneda (Global Airports): on Tokyo Bay, runway 16L/34R along the shore',
     'datum': (35.553333333, 139.781111111), 'elevation_ft': 20.0, 'utc_offset': 9.0, 'utc_offset_winter': 9.0,
     'runway': ('34R', (35.5396871, 139.8051580), (35.5658965, 139.7865529)),
     'water': {'lat': 35.5573, 'lon': 139.8054, 'alt_m_agl': 60.0, 'heading': 60.0, 'pitch': -8.0}},
    {'icao': 'YSSY', 'notes': 'Sydney Kingsford Smith (Global Airports): southern hemisphere, runways out into Botany Bay',
     'datum': (-33.946111111, 151.177222222), 'elevation_ft': 20.0, 'utc_offset': 11.0, 'utc_offset_winter': 10.0, 'southern': True,
     'runway': ('16R', (-33.9293426, 151.1715901), (-33.9642672, 151.1806464)),
     'water': {'lat': -33.9751, 'lon': 151.1806, 'alt_m_agl': 60.0, 'heading': 180.0, 'pitch': -8.0}},
    {'icao': 'SBGR', 'notes': 'Sao Paulo/Guarulhos (Global Airports): southern hemisphere, South America, 749 m up',
     'datum': (-23.435555556, -46.473055556), 'elevation_ft': 2458.0, 'utc_offset': -3.0, 'utc_offset_winter': -3.0, 'southern': True,
     'runway': ('10L', (-23.4342870, -46.4833810), (-23.4248980, -46.4486340))},
    {'icao': 'CYYZ', 'notes': 'Toronto Pearson (Global Airports): large Canadian hub',
     'datum': (43.6817, -79.632), 'elevation_ft': 541.0, 'utc_offset': -4.0, 'utc_offset_winter': -5.0,
     'runway': ('23', (43.6951488, -79.6335940), (43.6741405, -79.6640579))},
]
AIRPORT_AERIAL_ALT_M = 600.0
AIRPORT_AERIAL_PITCH = -20.0        # The datum in the middle of the image: the camera stands 600 / tan(20 deg) m north of it
WAVES = {'wave_amplitude_m': 0.6, 'wave_direction_deg': 210.0}


def offset(lat, lon, bearing_deg, meters):
    """The point some distance from (lat, lon) along a true bearing (spherical earth, as the probe's Offset)."""
    r = 6371000.0
    d = meters / r
    b = math.radians(bearing_deg)
    p1, l1 = math.radians(lat), math.radians(lon)
    p2 = math.asin(math.sin(p1) * math.cos(d) + math.cos(p1) * math.sin(d) * math.cos(b))
    l2 = l1 + math.atan2(math.sin(b) * math.sin(d) * math.cos(p1), math.cos(d) - math.sin(p1) * math.sin(p2))
    return math.degrees(p2), math.degrees(l2)


def airport_cameras(airport):
    """The runway view (2 m over the threshold, down the runway), the aerial (north of the field looking south over
    it) and, for coastal fields, the low view over the water."""
    icao = airport['icao']
    _, threshold, far_end = airport['runway']
    cameras = {
        'rwy': {'icao': icao, 'lat': threshold[0], 'lon': threshold[1], 'alt_m_agl': 2.0,
                'heading': round(bearing(threshold, far_end), 2), 'pitch': 0.0},
    }
    lat, lon = offset(airport['datum'][0], airport['datum'][1], 0.0, AIRPORT_AERIAL_ALT_M / math.tan(math.radians(-AIRPORT_AERIAL_PITCH)))
    cameras['aerial'] = {'icao': icao, 'lat': round(lat, 7), 'lon': round(lon, 7), 'alt_m_agl': AIRPORT_AERIAL_ALT_M,
                         'heading': 180.0, 'pitch': AIRPORT_AERIAL_PITCH}
    if 'water' in airport:
        cameras['water'] = dict({'icao': icao}, **airport['water'])
    return cameras


def airport_scenarios():
    """Per airport: sun 40 deg (auto and locked exposure) from the runway and the aerial, low sun, night, haze,
    overcast 600 m above the field, a wet runway, snow, winter noon and the water views (waves 0.6 m)."""
    scenarios = []
    for airport in TEST_AIRPORTS:
        icao = airport['icao'].lower()
        lat, lon = airport['datum']
        utc_offset = airport['utc_offset']
        southern = airport.get('southern', False)
        summer = (2026, 1, 15) if southern else (2026, 7, 15)
        winter = (2026, 7, 15) if southern else (2026, 1, 15)
        cameras = airport_cameras(airport)
        elevation_m = airport['elevation_ft'] * 0.3048
        runway = airport['runway'][0]

        def add(sid, cam, date, local_time, offset_hours, weather, notes, **extra):
            entry = {'id': f'{icao}_{sid}'}
            entry.update(cameras[cam])
            entry['date'] = '%04d-%02d-%02d' % date
            entry['local_time'] = clock(local_time)
            entry['utc_offset'] = offset_hours
            entry.update(weather)
            entry['notes'] = notes
            entry.update(extra)
            scenarios.append(entry)

        day40 = morning_time_for_elevation(lat, lon, summer, utc_offset, 40)
        day08 = morning_time_for_elevation(lat, lon, summer, utc_offset, 8)
        clean = WEATHER['clean']
        rwy_note = f'runway {runway} threshold, 2 m up'
        for cam, where in (('rwy', rwy_note), ('aerial', 'aerial, 600 m up north of the field')):
            add(f'{cam}_sunp40', cam, summer, day40, utc_offset, clean, f'{where}; sun elevation 40 deg')
            add(f'{cam}_sunp40_ev', cam, summer, day40, utc_offset, clean, f'{where}; sun elevation 40 deg, locked exposure',
                ev100_lock=EV100_LOCK[40])
        add('aerial_sunp08', 'aerial', summer, day08, utc_offset, clean, 'aerial; sun elevation 8 deg')
        add('aerial_sunp08_ev', 'aerial', summer, day08, utc_offset, clean, 'aerial; sun elevation 8 deg, locked exposure',
            ev100_lock=EV100_LOCK[8])
        for cam, where in (('rwy', rwy_note), ('aerial', 'aerial')):
            add(f'{cam}_night', cam, summer, 23.5, utc_offset, clean, f'{where}; 23:30 local')
        add('aerial_hazy', 'aerial', summer, day40, utc_offset, WEATHER['hazy'], 'aerial; sun elevation 40 deg, 8 km visibility')
        overcast = {'visibility_m': 30000.0, 'clouds': [{'base_msl_m': round(elevation_m + 600.0, 1),
                                                         'top_msl_m': round(elevation_m + 1268.0, 1), 'coverage': 1.0, 'type': 1}]}
        add('aerial_overcast', 'aerial', summer, day40, utc_offset, overcast, 'aerial; 8/8 stratus 600 m above the field')
        add('rwy_wet5', 'rwy', summer, day40, utc_offset, WEATHER['wet5'], f'{rwy_note}; sun elevation 40 deg, rain, runway friction 5')
        add('aerial_snowy', 'aerial', summer, day40, utc_offset, WEATHER['snowy'], 'aerial; sun elevation 40 deg, snow cover 1')
        month = 'July (southern winter)' if southern else 'January'
        add(f'aerial_season{winter[1]:02d}', 'aerial', winter, 12.0, airport['utc_offset_winter'], clean,
            f'aerial; mid-{month}, local solar noon (12:00 standard time)')
        if 'water' in cameras:
            add('water_sunp40', 'water', summer, day40, utc_offset, clean, 'low over the water; sun elevation 40 deg, waves 0.6 m from 210 deg',
                **WAVES)
    return scenarios


def write(path, scenarios, about):
    data = {
        'about': about + ' Generated by make_parity_scenarios.py; edit that script, not this file.',
        'xp_build': XP_BUILD,
        'defaults': {
            'fov': 60.0, 'width': 1280, 'height': 720,
            'visibility_m': 80000.0, 'clouds': [], 'rain_percent': 0.0, 'runway_friction': 0, 'snow_cover': 0.0,
            'temperature_c': 15.0, 'settle_frames': 600, 'ev100_lock': None, 'ui_mask': [],
        },
        'scenarios': scenarios,
    }
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(data, f, indent='\t')
        f.write('\n')
    print(f'{len(scenarios)} scenarios -> {path}')


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

    write(OUT, scenarios, 'Parity scenarios rendered by X-Plane 12 (tools/xp12_reference_probe) and SEDX (--env-scenario).')
    write(OUT_AIRPORTS, airport_scenarios(),
          'The standard test airports (' + ', '.join(a['icao'] for a in TEST_AIRPORTS) + ') rendered by X-Plane 12 (the '
          'reference probe\'s airport set) and SEDX (--env-scenario; tools/xp12_reference_probe/render_scenarios.py also logs performance).')


if __name__ == '__main__':
    main()
