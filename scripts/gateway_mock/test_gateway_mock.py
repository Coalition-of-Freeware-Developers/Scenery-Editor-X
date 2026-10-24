#!/usr/bin/env python3
# -------------------------------------------------------
# Scenery Editor X
# -------------------------------------------------------
# test_gateway_mock.py
# -------------------------------------------------------
# Tests for the Scenery Gateway test server: python scripts/gateway_mock/test_gateway_mock.py
# Runs the server in this process on a free loopback port; nothing leaves the computer.
# -------------------------------------------------------

import base64
import io
import json
import os
import shutil
import sys
import tempfile
import threading
import unittest
import urllib.error
import urllib.request
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gateway_mock  # noqa: E402


def master_zip(icao):
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w") as archive:
        archive.writestr(f"{icao}.dat", "I\n1200 test\n\n1 0 0 0 %s Test\n99\n" % icao)
        archive.writestr(f"{icao}_Scenery_Pack.zip", b"")
    return base64.b64encode(buffer.getvalue()).decode("ascii")


class MockGatewayTests(unittest.TestCase):
    def setUp(self):
        self.state = tempfile.mkdtemp(prefix="sedx-gateway-mock-test-")
        self.data = gateway_mock.GatewayData({"Test Artist": "test-password"}, self.state)
        self.server = gateway_mock.make_server("127.0.0.1", 0, self.data, quiet=True)
        self.base = "http://127.0.0.1:%d/apiv1" % self.server.server_address[1]
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        shutil.rmtree(self.state, ignore_errors=True)

    # --- helpers ---

    def request(self, method, path, body=None):
        data = json.dumps(body).encode("utf-8") if body is not None else None
        req = urllib.request.Request(self.base + path, data=data, method=method)
        if data is not None:
            req.add_header("Content-Type", "application/json")
        try:
            with urllib.request.urlopen(req, timeout=10) as reply:
                return reply.status, reply.headers, reply.read().decode("utf-8")
        except urllib.error.HTTPError as e:
            with e:
                return e.code, e.headers, e.read().decode("utf-8")

    def upload(self, **fields):
        scenery = {"userId": "Test Artist", "password": "test-password", "icao": "XSDX", "aptName": "Test",
                   "artistComments": "", "parentId": None, "type": "2D", "features": "",
                   "validatedAgainst": "12.1.3-r2-2f236", "additionalMetadata": {"city": "Mockville"},
                   "masterZipBlob": master_zip(fields.get("icao", "XSDX"))}
        scenery.update(fields)
        return self.request("PUT", "/scenery", {"scenery": scenery})

    # --- reads ---

    def test_reads_like_the_gateway(self):
        status, headers, body = self.request("GET", "/airports")
        self.assertEqual(status, 200)
        self.assertEqual(headers.get("X-Gateway-Mock"), "1")
        airports = json.loads(body)
        codes = {a["AirportCode"] for a in airports["airports"]}
        self.assertTrue({"KGON", "KDEN", "KORD", "KBOS", "KBTV", "XSDX"} <= codes)
        self.assertTrue(all(a["AirportName"].endswith("(MOCK)") for a in airports["airports"]))

        status, _, body = self.request("GET", "/airport/kbtv")
        self.assertEqual(status, 200)
        details = json.loads(body)["airport"]
        self.assertEqual(details["icao"], "KBTV")
        self.assertEqual(len(details["scenery"]), 1)
        scenery_id = details["recommendedSceneryId"]
        self.assertGreaterEqual(scenery_id, gateway_mock.FIRST_SCENERY_ID)

        status, _, body = self.request("GET", "/scenery/%d" % scenery_id)
        self.assertEqual(status, 200)
        pack = json.loads(body)["scenery"]
        with zipfile.ZipFile(io.BytesIO(base64.b64decode(pack["masterZipBlob"]))) as archive:
            names = archive.namelist()
            self.assertIn("KBTV.dat", names)
            self.assertIn("KBTV_Scenery_Pack.zip", names)
            with zipfile.ZipFile(io.BytesIO(archive.read("KBTV_Scenery_Pack.zip"))) as inner:
                self.assertIn("KBTV_Scenery_Pack/Earth nav data/apt.dat", inner.namelist())
            # Gateway-valid metadata: an ISO-3166 alpha-3 country prefix, gui_label matching the (2D) content
            apt = archive.read("KBTV.dat").decode("utf-8")
            self.assertIn("\n1302 country USA United States\n", apt)
            self.assertIn("\n1302 gui_label 2D\n", apt)
        self.assertEqual(pack["type"], "2D")
        self.assertEqual(pack["additionalMetadata"]["country"], "USA United States")

        self.assertEqual(self.request("GET", "/airport/NOPE")[0], 404)
        self.assertEqual(self.request("GET", "/scenery/1")[0], 404)
        self.assertEqual(self.request("GET", "/stats")[0], 200)
        self.assertEqual(self.request("GET", "/releases")[0], 200)

    # --- uploads ---

    def test_upload_with_good_credentials(self):
        status, _, body = self.upload()
        self.assertEqual(status, 200)
        scenery_id = json.loads(body)["sceneryId"]

        # Listed under its airport as Uploaded, downloadable, saved without the password
        details = json.loads(self.request("GET", "/airport/XSDX")[2])["airport"]
        entry = [s for s in details["scenery"] if s["sceneryId"] == scenery_id][0]
        self.assertEqual(entry["Status"], "Uploaded")
        self.assertEqual(entry["userName"], "Test Artist")
        self.assertEqual(self.request("GET", "/scenery/%d" % scenery_id)[0], 200)

        saved = os.path.join(self.state, str(scenery_id))
        with open(os.path.join(saved, "request.json"), encoding="utf-8") as f:
            text = f.read()
        self.assertNotIn("test-password", text)
        self.assertNotIn('"password"', text)
        self.assertTrue(os.path.isfile(os.path.join(saved, "masterZip.zip")))

        log = json.loads(self.request("GET", "/mock/log")[2])
        self.assertEqual(log["signIns"][-1], {"time": log["signIns"][-1]["time"], "userId": "Test Artist", "accepted": True})
        self.assertEqual(log["uploads"][-1]["files"], ["XSDX.dat", "XSDX_Scenery_Pack.zip"])
        self.assertIsNone(log["uploads"][-1]["packFiles"])      # The test's pack zip is empty: listed as unreadable
        self.assertNotIn("test-password", json.dumps(log))

    def test_upload_updates_chain_on_parent(self):
        first = json.loads(self.upload()[2])["sceneryId"]
        status, _, body = self.upload(parentId=str(first))      # WED sends the parent as a string
        self.assertEqual(status, 200)
        second = json.loads(body)["sceneryId"]
        entry = [s for s in json.loads(self.request("GET", "/airport/XSDX")[2])["airport"]["scenery"] if s["sceneryId"] == second][0]
        self.assertEqual(entry["parentId"], first)
        self.assertEqual(self.upload(parentId=12345)[0], 400)

    def test_upload_refusals(self):
        status, headers, body = self.upload(password="wrong")
        self.assertEqual(status, 401)
        self.assertIn("text/html", headers.get("Content-Type"))
        self.assertIn("Invalid username or password", body)
        self.assertEqual(self.upload(userId="Nobody")[0], 401)
        self.assertEqual(self.upload(userId=None)[0], 401)

        self.assertEqual(self.upload(icao="ENTY")[0], 404)
        self.assertEqual(self.upload(type="4D")[0], 400)
        self.assertEqual(self.upload(masterZipBlob="not base64!")[0], 400)
        self.assertEqual(self.request("PUT", "/scenery", {"nope": 1})[0], 400)
        self.assertEqual(self.request("POST", "/scenery", {"scenery": {}})[0], 405)

        log = json.loads(self.request("GET", "/mock/log")[2])
        self.assertEqual([s["accepted"] for s in log["signIns"]][:3], [False, False, False])

    def test_supported_versions_and_maintenance(self):
        self.data.supported_versions = ["12.1.3-r2-2f236"]
        self.assertEqual(self.upload()[0], 200)
        status, _, body = self.upload(validatedAgainst="12.0.9")
        self.assertEqual(status, 401)
        self.assertIn("Invalid X-Plane Version", body)

        self.data.maintenance = True
        self.assertEqual(self.upload()[0], 503)


class LoopbackOnlyTests(unittest.TestCase):
    def test_refuses_other_interfaces(self):
        old_argv = sys.argv
        sys.argv = ["gateway_mock.py", "--host", "0.0.0.0"]
        try:
            with self.assertRaises(SystemExit):
                gateway_mock.main()
        finally:
            sys.argv = old_argv


if __name__ == "__main__":
    unittest.main(verbosity=2)
