import copy
import http.client
import json
from pathlib import Path
import tempfile
import threading
import unittest
from unittest.mock import patch

import studio


def animation(name="spooky", part="eyes"):
    width = 13 if part == "eyes" else 26
    return {"name": name, "part": part, "frames": [[0xE3] * (width * 9), [0] * (width * 9)],
            "durations": [125, 125]}


class StudioTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        directory = Path(self.temporary.name)
        self.library = directory / "animations.json"
        self.header = directory / "custom_animations.h"
        self.library.write_text("[]\n", encoding="utf-8")
        for name, path in [("LIBRARY", self.library), ("HEADER", self.header)]:
            patcher = patch.object(studio, name, path)
            patcher.start()
            self.addCleanup(patcher.stop)

    def test_builtin_frames_match_panel_dimensions_and_timing(self):
        builtins = studio.builtin_animations()
        self.assertEqual([item["name"] for item in builtins], ["blink", "look", "talk", "smile"])
        for item in builtins:
            studio.validate_animation(item)
        self.assertEqual(builtins[0]["durations"], [2400, 100, 180, 100])

    def test_save_reload_and_generate_flash_descriptors(self):
        eye = animation()
        mouth = animation("chatter", "mouth")
        studio.save_animation(eye)
        studio.save_animation(mouth)
        self.assertEqual(studio.load_library(), [eye, mouth])
        header = self.header.read_text(encoding="utf-8")
        self.assertIn("{nullptr, 125}", header)
        self.assertIn("custom_0_frames, 2, 13", header)
        self.assertIn("custom_1_frames, 2, 26", header)
        self.assertEqual(header, studio.render_header(studio.load_library()))
        self.assertIn("}, \\\n", header)  # Multi-entry macro must continue onto the next line.

    def test_duplicate_or_builtin_does_not_modify_files(self):
        studio.save_animation(animation())
        before = self.library.read_bytes(), self.header.read_bytes()
        for name in ["spooky", "blink", "off"]:
            with self.assertRaises(FileExistsError):
                studio.save_animation(animation(name))
        self.assertEqual(before, (self.library.read_bytes(), self.header.read_bytes()))

    def test_black_and_repeated_frames_do_not_duplicate_pixel_data(self):
        value = animation()
        value["frames"].append(value["frames"][0][:])
        value["durations"].append(100)
        header = studio.render_header([value])
        self.assertEqual(header.count("const uint8_t"), 1)
        self.assertEqual(header.count("{custom_0_pixels_0,"), 2)

    def test_reject_malformed_or_unsafe_data_without_writing(self):
        cases = [{"name": '../bad'}, {"name": 'bad";'}, {"part": "all"},
                 {"frames": []}, {"frames": [[0] * 116]}, {"frames": [[256] * 117]},
                 {"frames": [[True] * 117]}, {"durations": [0, 100]},
                 {"durations": [65536, 100]}, {"durations": [100]},
                 {"frames": [[0] * 117] * 256, "durations": [1] * 256}]
        for changes in cases:
            with self.subTest(changes=list(changes)):
                value = copy.deepcopy(animation())
                value.update(changes)
                with self.assertRaises(ValueError):
                    studio.save_animation(value)
        self.assertEqual(self.library.read_text(), "[]\n")
        self.assertFalse(self.header.exists())

    def test_library_pixel_budget(self):
        with patch.object(studio, "MAX_PIXELS", 200):
            with self.assertRaises(ValueError):
                studio.save_animation(animation())
        self.assertEqual(self.library.read_text(), "[]\n")

    def test_http_upload_reload_assets_and_cross_origin_rejection(self):
        server = studio.create_server(0)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        port = server.server_address[1]

        def request(method, path, body=None, extra=None):
            headers = {"Content-Type": "application/json", "Origin": f"http://127.0.0.1:{port}"}
            headers.update(extra or {})
            connection = http.client.HTTPConnection("127.0.0.1", port)
            connection.request(method, path, json.dumps(body) if body is not None else None, headers)
            response = connection.getresponse()
            result = response.status, response.read()
            connection.close()
            return result

        for asset in ["/", "/app.js", "/sprites.js", "/style.css"]:
            self.assertEqual(request("GET", asset)[0], 200)
        self.assertEqual(request("GET", "/../README.md")[0], 404)
        self.assertEqual(request("POST", "/api/animations", animation(), {"Origin": "https://elsewhere.example"})[0], 403)
        self.assertEqual(request("GET", "/api/animations", extra={"Host": "elsewhere.example"})[0], 403)
        code, body = request("POST", "/api/animations", animation())
        self.assertEqual(code, 201, body)
        code, body = request("GET", "/api/animations")
        self.assertEqual(code, 200)
        self.assertEqual(json.loads(body)["animations"][-1]["name"], "spooky")
        self.assertEqual(request("POST", "/api/animations", animation())[0], 409)
        self.assertEqual(request("POST", "/api/animations", {"name": "broken"})[0], 400)
        self.assertEqual(request("POST", "/api/animations", animation(), {"Content-Type": "text/plain"})[0], 415)


if __name__ == "__main__":
    unittest.main()
