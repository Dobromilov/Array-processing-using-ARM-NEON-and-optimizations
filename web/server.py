#!/usr/bin/env python3
import argparse
import json
import subprocess
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
REPORT_PATH = PROJECT_ROOT / "web" / "benchmark_results.json"
GO_BINARY = PROJECT_ROOT / "go"


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(PROJECT_ROOT), **kwargs)

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        super().end_headers()

    def _write_json(self, code: int, payload: dict) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        if self.path != "/api/run":
            self._write_json(HTTPStatus.NOT_FOUND, {"ok": False, "error": "Not found"})
            return

        if not GO_BINARY.exists():
            self._write_json(
                HTTPStatus.INTERNAL_SERVER_ERROR,
                {"ok": False, "error": "Binary ./go not found. Build project first."},
            )
            return

        cmd = [str(GO_BINARY), "--json", str(REPORT_PATH), "--no-terminal"]
        try:
            run = subprocess.run(
                cmd,
                cwd=str(PROJECT_ROOT),
                capture_output=True,
                text=True,
                timeout=180,
                check=False,
            )
        except subprocess.TimeoutExpired:
            self._write_json(
                HTTPStatus.GATEWAY_TIMEOUT,
                {"ok": False, "error": "Benchmark timeout after 180s"},
            )
            return
        except Exception as exc:
            self._write_json(
                HTTPStatus.INTERNAL_SERVER_ERROR,
                {"ok": False, "error": f"Failed to run benchmark: {exc}"},
            )
            return

        if run.returncode != 0:
            self._write_json(
                HTTPStatus.INTERNAL_SERVER_ERROR,
                {
                    "ok": False,
                    "error": "Benchmark failed",
                    "stdout": run.stdout[-1200:],
                    "stderr": run.stderr[-1200:],
                },
            )
            return

        self._write_json(
            HTTPStatus.OK,
            {"ok": True, "message": "Benchmark completed", "report": "/web/benchmark_results.json"},
        )


def main() -> None:
    parser = argparse.ArgumentParser(description="Serve web dashboard and run benchmark API")
    parser.add_argument("--port", type=int, default=8080, help="HTTP port")
    parser.add_argument("--host", default="0.0.0.0", help="Bind host")
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"Serving dashboard on http://{args.host}:{args.port}/web/")
    print("POST /api/run will execute ./go and refresh web/benchmark_results.json")
    server.serve_forever()


if __name__ == "__main__":
    main()
