# X-Plane Scenery Gateway integration

This page covers three things:

- how the [X-Plane Scenery Gateway](https://gateway.x-plane.com) works for a third-party editor;
- what Scenery Editor X (SEDX) does with it now;
- what is still needed before SEDX can create and update airports on it.

The facts come from WorldEditor's source (X-Plane/xptools at `0137e80`, WED 2.7.3), the Gateway's
[API docs](https://gateway.x-plane.com/api) and Laminar's articles. Anything not confirmed by a primary source is
marked *unconfirmed*. None of this research sent requests with credentials, uploads or other changes to the Gateway.

## How the Gateway works

### API

- **Base URL.** `https://gateway.x-plane.com/apiv1/`. This is the only API version.
- **No staging server.** There is no public staging or sandbox server. WED's only alternative is a debugging flag,
  `--gateway_api_url=<url>`, which replaces the API base.
- **Read calls.** Each is a plain GET:
  - `/airports` returns every airport, about 20 MB.
  - `/airport/{id}` returns one airport and its submissions.
  - `/scenery/{id}` returns one submission with its files.
  - `/stats`, `/releases`, `/release/{version}` and `/metadata` also exist.
- **Static files.** These sit outside `apiv1`: `airport_metadata.csv` (metadata defaults) and `runway_coordinates.txt`
  (CIFP runway ends). WED fetches both before an upload.
- **Rate limits.** Nothing is metered, but the docs ask clients to go easy on the server. A client that pulls the full
  airport list too often can be cut off. WED caches answers: the airport list for 1 h, one airport for 10 min, a
  submission for 4 weeks, the CSV for 1 day.

### Accounts and sign-in

- **There is no sign-in, token or session API.** Uploads are the only protected call. The artist's user name and
  plaintext password travel inside each upload body, as `scenery.userId` and `scenery.password`, protected only by
  TLS.
- **WED never stores credentials on disk.** It keeps them in memory for the session and leaves them out of its log.
- **Bad credentials are not documented.** The status code and body for a wrong password are *unconfirmed*. The live
  server does answer `401` with a short `text/html` line for an unsupported X-Plane build ("Invalid X-Plane Version.
  Please check the NOTAMS Gateway page for supported versions."), so a 401 doesn't always mean a bad password.

### Uploading (new airports and updates)

WED sends `PUT https://gateway.x-plane.com/apiv1/scenery` with `Content-Type: application/json` and this body:

```json
{ "scenery": {
    "userId": "<artist user name>", "password": "<password>",
    "icao": "<apt.dat airport ID>", "aptName": "...", "artistComments": "...",
    "parentId": "97429" | null, "type": "2D" | "3D", "features": "1,2",
    "validatedAgainst": "12.1.3-r2-2f236", "clientVersion": 20730,
    "latitude": 0.0, "longitude": 0.0,
    "additionalMetadata": { "<1302 key>": "<value>", ... },
    "masterZipBlob": "<base64 zip>" } }
```

**What each field means:**

- `type` is `3D` when the airport has any objects, facades, forests or object strings.
- `features` lists Gateway feature ids: `1` ATC flow, `2` taxi routes, `8` truck routes, `86` roads.
- `validatedAgainst` is the installed X-Plane's build, read from its `Log.txt`. The server only takes builds on its
  [supported list](https://gateway.x-plane.com/xplaneversions/supported).
- `clientVersion` is WED's own version number. SEDX shouldn't send WED's number. Whether the server accepts a client
  that isn't WED is *unconfirmed*.
- `parentId` is the submission this one replaces, or `null` for a first upload. A new airport and an update use the
  same call and differ only here. WED sends it as a string; the docs show a number.

**The master zip** (`masterZipBlob`) holds:

- `<ICAO>.dat`: the apt.dat.
- `<ICAO>.txt`: the DSF as text. This file is only there when the airport has DSF items.
- `<ICAO>_Scenery_Pack.zip`: a ready-to-install pack. It contains `Earth nav data/apt.dat`, the binary DSF, `doc.kml`,
  `doc.osm`, a README (airport, uploader, comments, GPL v2 notice) and `COPYING`, all under one
  `<ICAO>_Scenery_Pack/` folder.

**Answers.** Only `HTTP 200` with an integer `sceneryId` means the upload arrived. WED stores that id as the
airport's scenery id, so the next upload uses it as its `parentId`. Any other status is a failure. An unknown airport
code gets `404` (*body unconfirmed*). Server-side size limits and locking rules are *unconfirmed*.

**After upload.** A moderator moves each submission through Uploaded → Accepted → Approved (or Declined). An approved
submission can then become Recommended, which is the one that ships with X-Plane. Airport locks
(`checkedOutBy` / `checkOutEndDate`) are set on the website only; the API shows them read-only.

**Validation WED runs first.** The upload is blocked by any error from WED's "Airport Scenery Gateway" export
target. That target checks:

- Name and ID rules.
- A boundary with no beziers, and every runway inside it.
- The airport's extent: at most 0.6 nm past the boundary, 7 nm across.
- Runway headings, and no transparent runways.
- Art: only default-library resources, no custom orthophotos, no `set_MSL`, no custom trucks.
- ATC frequency rules.
- Roads: only `roads.net` / `roads_EU.net`.
- CIFP runway ends within 10 m of `runway_coordinates.txt` (up to 50 m for wide runways).
- 1302 metadata: ISO-3166 country, datum pairs, and no URLs in values.

Before validating, WED fills missing metadata from `airport_metadata.csv`.

> **Laminar's warning:** the API docs say the upload API is not meant to be used directly, because WED's uploader
> validates packs. Accounts found bypassing that validation may be banned. SEDX therefore keeps uploads to the public
> Gateway switched off until its validation matches WED's and Laminar has agreed (contact: `gateway@x-plane.com`).

## What SEDX does now

**Before this branch,** `xplane/xp_gateway.*` was a read-only client. It could download the airport list, one
airport, a submission, stats and releases, and install a submission as a scenery pack. Its URL was hard-coded and it
had no account support, no uploads and no zip writer.

**This branch adds the following.**

### Secure credentials

`core/platform/credential_store.*` saves the password in the operating system's credential store:

- Windows Credential Manager, encrypted per user by DPAPI.
- The macOS login keychain.
- The freedesktop Secret Service on Linux (GNOME Keyring, KWallet), loaded with `dlopen` so libsecret isn't a build
  dependency.

If there is no store, the password is kept for the session only. It is never written to a file.

### Account settings

`xplane/xp_gateway_account.*` keeps the user name, the server and the "remember password" choice in
`gateway_account.json` in the settings folder. The password is never in that file. Each test server gets its own
credential entry (`name@host:port`), so a test password never replaces the real one.

### Preferences

**Edit > Preferences > Scenery Gateway** (also **X-Plane > Gateway Account...**, and the AIRPORTS panel's Gateway
tab) has:

- a server choice: the public Gateway or a test server URL;
- user name and password fields;
- "save the password in <store>";
- **Save** and **Sign Out** (removes the password from memory and from the store);
- a link to the Gateway website for registering and resetting a password.

Because the Gateway has no sign-in call, the page says the password is checked when you upload. For test servers it
adds **Send Test Upload**, which uploads a made-up airport (XSDX) to check the account and the upload path.

### Server switching and safety

`xplane/xp_gateway_server.*` decides which addresses are accepted:

- `https://` only, except `http://` on the loopback interface (127.x, localhost, ::1).
- No credentials or queries in the URL.

The client keeps these protections:

- Requests take their server when they're queued, so switching servers never redirects a queued password.
- Calls that carry credentials don't follow redirects.
- Test servers get their own download cache (`<cache>/gateway-servers/<host_port>`).
- Every account screen shows **TEST SERVER** while the server isn't the public Gateway.

### Uploads

`xplane/xp_gateway_submission.*` handles the upload itself:

- It builds WED's PUT body and reads the answer (uploaded / not authorized / unknown airport / rejected /
  unconfirmed / no answer).
- It writes WED's master zip layout. The zip writer itself is in `xplane/xp_zip.*` (stored or deflated with zlib):
  the made-up test airport stays stored, real packages are deflated.
- `GatewayClient::UploadScenery` runs the upload on the Gateway worker. It wipes the password once sent and never
  logs it.
- `CanUploadTo` refuses the public Gateway unless `GatewayClient::SetLiveUploadsAllowed(true)`. Nothing calls that
  yet.

### Pluggable transport

The HTTP layer is a `GatewayTransport` function. libcurl is the default; tests use a fake in-process transport, and
`--gateway-offline` uses one that never connects.

### Validation

`xplane/xp_gateway_validation.*` checks an airport the way WED's "Airport Scenery Gateway" export target does
(`WED_Validate.cpp`). Errors block the upload; warnings need the artist's tick. It takes the airport's apt.dat rows plus
its DSF content (a `DsfFile` of this airport's placements), so a project airport and a pack airport are checked alike.

| Rule | Severity | Notes |
| --- | --- | --- |
| Name empty, over 30 UTF-8 characters, leading / trailing space, all upper case, `(X)` / `{X}` instead of `[X]` | Error | WED's `ValidateAptName` |
| Name contains "Airport", the full words International / Regional / Municipal, or the airport ID | Warning | |
| Airport ID empty or not ASCII letters / digits | Error | |
| Land airport with helipads but no runway | Error | Gateway target only |
| Runways but no boundary; boundary with bezier curves; boundary sides under 30 m | Error | |
| Runway corners, taxiway (pavement) vertices, ramp starts outside every boundary | Error | Curves ignored, like WED |
| Wider than 7 nm (KSEA / KEDW: 21 nm) | Error | Over the rows and the DSF content |
| Pavement, point features, objects, polygons (not .lin / .str) more than 0.6 nm past the boundary box | Error | |
| Art not exported by `Resources/default scenery`, or only privately / deprecated there (SEMI_DEPRECATED passes) | Error | `DescribeGatewayResource` on the LibraryManager index; `::FLATTEN::.pol` is default; no scanned library = error |
| Orthophotos (UV-mapped .pol) without `#subtex` automatic subtextures | Error | `PolygonAsset::subtextures` |
| `set_MSL` objects; `set_AGL` beyond +-100 m; MSL outside -1000..10000 m | Error | |
| `set_AGL` objects | Warning | WED skips this for .agp with scrapers; SEDX always warns |
| Custom truck objects (1402), transparent runways | Error | |
| Roads other than `lib/g10/roads.net` / `roads_EU.net`, layers outside 0..5, loops, roads past the boundary box + 0.6 nm, nodes a few meters off a tile edge | Error | |
| 1302: country must start with an upper case ISO-3166 alpha-3 code (WED's 249) and the name can't be / start with a number; datum lat / lon both numbers, as a pair, within 1 nm; FAA / IATA / ICAO / local code / local authority formats (ICAO not starting with X); FAA and local code not both; region code in WED's 251; state, city not numbers; transition altitude / level; `allows_circuits` 0/1; `tower_service_type` atc/fiso; `gui_label` 2D/3D; altimeter QNH/QFE; no "http" in any value | Error | WED's `ValidateAirportMetadata` |
| Everything `ValidateAptAirport` finds | Error | Structural apt.dat problems |

Deliberate differences from WED: a country's second code is only flagged when the word after the prefix is itself an
ISO code (WED flags any three-letter first word, so "NZL New Zealand" would fail there); the Nan, Thailand exception also
takes "THA Thailand"; name-style words are found anywhere in the name, not only at their first occurrence.

**Not ported yet:** ATC flow / frequency / runway-use rules, runway heading vs. name (needs a magnetic declination
model), the CIFP runway-end check against `runway_coordinates.txt`, filling missing metadata from
`airport_metadata.csv`, sign syntax, ramp start and jetway rules, and WED's generic per-feature checks beyond what
`ValidateAptAirport` covers. Laminar's ban warning is about uploads that skip WED's validation, so these gaps must close
before live uploads are switched on.

### Packaging

`xplane/xp_gateway_package.*` builds what `GatewayClient::UploadScenery` sends, following `WED_GatewayExport.cpp`:

- `type` is `3D` when the airport places objects (.obj / .agp), facades, forests, object strings or apt.dat jetways;
  `features` lists `1` (ATC flows), `2` (aircraft taxi routes, 1202), `8` (truck routes, 1206), `86` (roads).
  1302 `gui_label` is set to match, as WED does before validating.
- `<ICAO>.dat`: the apt.dat 12.00 file of the airport (`xp_apt` writer).
- `<ICAO>.txt`: the DSF content as DSF2Text (`WriteDsfText` in `xplane/xp_dsf.*`), with WED's whole-world header,
  `sim/filter/aptid <ICAO>` and every placement in `FILTER 0`. DSFTool's format: OBJECT_MSL / OBJECT_AGL give the
  elevation before the heading (its README has them the other way round; its code and WED's importer agree).
  Only there when the airport has DSF content.
- `<ICAO>_Scenery_Pack.zip`: `Earth nav data/apt.dat`, one binary DSF per 1x1 degree tile (`WriteDsf`, bounds first,
  the airport filter, its exclusions), `README.txt` (airport, uploader, comments, install notes, GPL v2 notice) and
  `COPYING` (the GPL v2 text, embedded in `xp_gateway_license.inl`), all under `<ICAO>_Scenery_Pack/`. WED's `doc.kml`
  / `doc.osm` are only written for KML / OSM shapes, which SEDX doesn't have.
- `additionalMetadata` carries only the 1302 keys WED uploads (`GetGatewayMetadataKeys`); `latitude` / `longitude` is
  the middle of everything the airport places.
- `validatedAgainst` is read from `<X-Plane>/Log.txt` the way WED does (`ReadXPlaneBuild`: "12.4.4-b5-3a0f7e10").
- A pack airport's DSF content comes from its tiles: placements filtered to its `sim/filter/aptid`, or in tiles
  without filters those near it (`LoadPackOverlay` / `SelectAirportPlacements`).

### Scenery ids and Gateway imports

- **In the project.** `ProjectAirport::sceneryId` (`.edX` `scenery-id`, WED's `scenery_id`) is the public Gateway's
  submission the airport descends from. Submissions on test servers are kept apart in `testSceneryIds`
  (`.edX` `test-scenery-ids`: server key + id, appended to the archive's field list), so a mock upload can never become
  a real upload's parent. Projects without test ids save byte-for-byte as before.
- **Import.** AIRPORTS > Gateway > **Import into Project** (or `--gateway-import <id>`) downloads a submission and adds
  it to the open project (a new, unsaved one when none is open): the airport from `<ICAO>.dat`, replacing one with the
  same ID, and the pack's DSF placements as items in the airport's hierarchy (`ImportDsfItems` in
  `project/project_dsf.*`, the inverse of the project -> DSF conversion). The submission's id is recorded for the server
  it came from.
- **Upload.** The id recorded for the current server is sent as `parentId`; an accepted upload's `sceneryId`
  replaces it (the project is marked changed: save it).

### Submit dialog

**X-Plane > Submit to Scenery Gateway...** works on the airport the airport editor shows: a project airport (its rows
and the visible items in its hierarchy; items outside it are listed as a warning) or an airport opened from a scenery
pack (its rows and the pack's tiles; no project remembers its scenery id). It shows the server (with the TEST SERVER
banner), the account, 2D / 3D, the features, a content summary, the parent submission, the X-Plane build, the
validation results and a comments box. **Upload...** packs the airport on a worker thread, then a confirmation names
the server, the account, the package size and the parent. The upload's progress and the server's answer follow; on
success the scenery id goes into the project, and on the public Gateway the dialog links to the submission's page. It
uses `EditorGateway::GetCredentials` and `GatewayClient::UploadScenery`, so `CanUploadTo` still refuses the public
Gateway.

## Testing without the public Gateway

1. Start the test server on this computer:

   ```bash
   python scripts/gateway_mock/gateway_mock.py
   ```

   It listens on `127.0.0.1:8780` only and serves made-up airports (KGON, KDEN, KORD, KBOS, KBTV, XSDX, all named
   "... (MOCK)"). It checks uploads against test accounts (default `Test Artist` / `test-password`) the way the real
   server is known to, and writes what it receives to a temp folder without the password. Options simulate an
   unsupported X-Plane build (`--supported-version`) and a locked Gateway (`--maintenance`). See
   [its README](../scripts/gateway_mock/README.md).

2. Point the editor at it. Use either Preferences > Scenery Gateway > Test server, or the command line:

   ```bash
   bin/SceneryEditorX_debug.exe --gateway-url http://127.0.0.1:8780 --gateway-credentials memory --gateway-preferences
   ```

   | Flag | Meaning |
   | --- | --- |
   | `--gateway-url <url>` / `SEDX_GATEWAY_URL` | Use this server this session (not saved) |
   | `--gateway-credentials memory` | Never touch the system keychain |
   | `--gateway-offline` | Block all Gateway traffic |
   | `--gateway-preferences` | Open the Scenery Gateway page at startup |
   | `--gateway-test-upload <name>` | Upload the test airport at startup with the password in `SEDX_GATEWAY_PASSWORD`; refused for the public Gateway |
   | `--gateway-import <id>` | Download a submission and add it to the project (a new one when none is open) |
   | `--gateway-import-save <file.edX>` | Save the project there after the import |
   | `--gateway-submit <airport id>` | Open the Submit dialog for a project airport (after any import) |
   | `--gateway-submit-auto <name>` | With `--gateway-submit`: validate, pack and upload without the dialog, as this account with the password in `SEDX_GATEWAY_PASSWORD`; test servers only; the project is saved afterwards |
   | `--gateway-comments <text>` | The artist's comments for `--gateway-submit` |

   An end-to-end run against the test server (the seed KBTV is submission 900001 on a fresh server):

   ```bash
   bin/SceneryEditorX_debug.exe --gateway-url http://127.0.0.1:8780 --gateway-credentials memory --gateway-import 900001 --gateway-import-save KBTV.edX
   SEDX_GATEWAY_PASSWORD=test-password bin/SceneryEditorX_debug.exe --project KBTV.edX --gateway-url http://127.0.0.1:8780 --gateway-credentials memory --gateway-submit KBTV --gateway-submit-auto "Test Artist"
   ```

   The editor's log (`Gateway` tag) shows the validation, the package and the answer; `GET /apiv1/mock/log` shows what
   arrived (parent, type, features, the master zip's and the pack's files) and the server's state folder keeps each
   `masterZip.zip`.

3. Run the automated tests. Neither suite opens a connection to anything but the loopback interface.
   - **`XPlaneFormatTests`.** `GatewayAccountTests.cpp` covers server URLs, the settings file, the upload body and
     answers, zips, and the live-upload guard, all through a fake transport. `GatewayValidationTests.cpp` covers the
     validation rules, `GatewayPackageTests.cpp` the packaging, deflated zips, DSF2Text and the X-Plane build.
     `CredentialStoreTests.cpp` covers the store. Its hidden `[.keychain]` case writes and deletes one entry in the
     real system store; run it explicitly.
   - **`ProjectFormatTests`.** `GatewayProjectTests.cpp` covers scenery ids in `.edX` / `.sex`, what an airport uploads
     and importing a submission; `DsfImportTests.cpp` the DSF -> project items conversion.
   - **The test server's own tests.** Run `python scripts/gateway_mock/test_gateway_mock.py`.

## What's still needed for the creation / update pipeline

Validation (most of WED's Gateway rules), packaging, scenery id tracking, Gateway imports and the Submit dialog are in
place and tested against the test server. Before live uploads, roughly in order:

1. **Laminar's agreement.** Before switching on live uploads, ask `gateway@x-plane.com` whether a non-WED uploader is
   acceptable. Ask what `clientVersion` and User-Agent it should send (SEDX sends `SceneryEditorX/1.0` and no
   `clientVersion`). Also ask which fields are required, what a wrong password returns, and the size limits.
2. **The rest of WED's validation.** The rules listed under "Not ported yet" above: ATC flows and frequencies, runway
   headings (magnetic model), CIFP runway ends from `runway_coordinates.txt`, metadata defaults from
   `airport_metadata.csv` (cached for a day), and WED's generic feature checks.
3. **Supported X-Plane builds.** Warn before uploading when `validatedAgainst` isn't on `/xplaneversions/supported`.
4. **The airport's lock.** Show `checkedOutBy` / `checkOutEndDate` in the Submit dialog before uploading.
5. **Turning on live uploads.** Call `GatewayClient::SetLiveUploadsAllowed(true)` only after steps 1-4. Even then,
   keep test servers the default in development builds.

**Notes for merging and other platforms:**

- **Merging with `tester-feedback-round-1`.** That branch has its own Preferences window (`EditorWelcome`, also titled
  `Preferences###EditorPreferences`). When the branches meet, move the "Scenery Gateway" tab into that window
  (`EditorGateway::DrawPreferencesTab()`) and drop `EditorPreferencesWindow`.
- **macOS and Linux credential stores.** These are written to the documented APIs but haven't been compiled or run
  yet; only the Windows store has been tested. Build and run the `[.keychain]` test on each OS.
