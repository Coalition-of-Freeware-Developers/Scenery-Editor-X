# Scenery Gateway test server

`gateway_mock.py` stands in for the [X-Plane Scenery Gateway](https://gateway.x-plane.com) API on your own
computer, so the editor's Gateway account, downloads and uploads can be tried without sending anything to the
public Gateway. It listens on the loopback interface only, serves made-up airports, and keeps whatever you upload
in a local folder.

Python 3.9 or newer, standard library only. Works the same on Windows, macOS and Linux.

## Run it

```bash
python scripts/gateway_mock/gateway_mock.py
```

It prints its address (`http://127.0.0.1:8780/apiv1` by default), the test accounts and the folder uploads go to.
Stop it with Ctrl+C.

| Option | Default | What it does |
| --- | --- | --- |
| `--port N` | `8780` | Port to listen on |
| `--host H` | `127.0.0.1` | `127.0.0.1`, `::1` or `localhost`; anything else is refused |
| `--account NAME:PASSWORD` | `Test Artist:test-password` | A test artist account; repeat for more |
| `--state DIR` | a new temp folder | Where received uploads are written |
| `--supported-version BUILD` | any | Only take uploads validated against this X-Plane build (repeatable); others get the live Gateway's `401 Invalid X-Plane Version` |
| `--maintenance` | off | Refuse every upload with `503`, like a Gateway locked for maintenance |
| `--quiet` | off | Don't log each request |

## Point the editor at it

```bash
SceneryEditorX --gateway-url http://127.0.0.1:8780 --gateway-credentials memory
```

- `--gateway-url` uses the test server for this session only; it isn't saved. The `SEDX_GATEWAY_URL` environment
  variable does the same. A test server can also be chosen in **Edit > Preferences > Scenery Gateway > Server**.
- `--gateway-credentials memory` keeps the test password out of your system keychain. Without it, a remembered test
  password is stored under its own entry (`Test Artist@127.0.0.1:8780`), never over your real one.
- `--gateway-offline` blocks every Gateway request instead, if you only want to work from the cache.
- To check an account and the upload path, use **Send Test Upload** on the Preferences page. It uploads a made-up
  airport (XSDX) and is only offered for test servers. Unattended runs can use
  `SEDX_GATEWAY_PASSWORD=test-password SceneryEditorX --gateway-url http://127.0.0.1:8780 --gateway-test-upload "Test Artist"`,
  which the editor refuses for the public Gateway.
- Every screen that shows the account says **TEST SERVER** while it isn't the public Gateway, and test servers get
  their own download cache (`<cache>/gateway-servers/127.0.0.1_8780`), separate from the real one.

## What it serves

| Request | Answer |
| --- | --- |
| `GET /apiv1/airports` | KGON, KDEN, KORD, KBOS, KBTV and XSDX, with names ending in "(MOCK)" |
| `GET /apiv1/airport/{icao}` | The airport and its submissions |
| `GET /apiv1/scenery/{id}` | A submission with its `masterZipBlob` (zip of `<ICAO>.dat`, `<ICAO>.txt`, `<ICAO>_Scenery_Pack.zip`) |
| `GET /apiv1/stats`, `/apiv1/releases` | Small made-up answers |
| `PUT /apiv1/scenery` | An upload, checked against the test accounts (see below). `POST` gets `405`: the real Gateway takes uploads as `PUT` |
| `GET /apiv1/mock/log` | Test hook: the sign-in checks and uploads received (never passwords): parent, type, features, the master zip's files and the scenery pack's files |

KBTV and XSDX start with one approved 2D submission each (ids from 900001, well away from real Gateway ids). Their
metadata passes WorldEditor's Gateway checks (an ISO-3166 country prefix, `gui_label` matching the content), so a
downloaded seed can be imported into a project, validated and uploaded back unchanged.
Uploads get the next id, appear under their airport as `Uploaded`, and are written to the state folder as
`<id>/request.json` (password removed) plus the decoded `masterZip.zip`.

An upload is answered the way the real Gateway is known to answer (the Gateway has no sign-in call, so the upload is
where the user name and password are checked):

| Case | Answer |
| --- | --- |
| Accepted | `200 {"sceneryId": N}` |
| Wrong user name or password | `401`, a line of `text/html` (the real body for this case isn't documented) |
| Build not in `--supported-version` | `401 Invalid X-Plane Version. Please check the NOTAMS Gateway page for supported versions.` |
| Airport code the server doesn't have | `404 {"message": "Unknown airport code X"}` |
| Bad `type`, `parentId` or `masterZipBlob` | `400 {"message": ...}` |
| `--maintenance` | `503` |

All responses carry an `X-Gateway-Mock: 1` header.
