#!/usr/bin/env python3
"""
Libris Diagnostic Log Receiver Server
Listens on 0.0.0.0:8080 and records diagnostic runtime logs sent from BlackBerry 10 devices.
Logs are saved in real-time to: logs/device_runtime.log
"""

import os
import sys
from datetime import datetime
from http.server import HTTPServer, ThreadingHTTPServer, BaseHTTPRequestHandler

HOST = "0.0.0.0"
PORT = 8080
LOG_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "logs")
LOG_FILE = os.path.join(LOG_DIR, "device_runtime.log")

class LogRequestHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        # Override to keep server stdout clean and formatted
        sys.stdout.write(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] HTTP: {format % args}\n")
        sys.stdout.flush()

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        response = f'{{"status": "ready", "server": "Libris Log Server", "time": "{datetime.now().isoformat()}"}}\n'
        self.wfile.write(response.encode("utf-8"))

    def do_POST(self):
        try:
            content_length = int(self.headers.get("Content-Length", 0))
            if content_length > 0:
                post_data = self.rfile.read(content_length).decode("utf-8", errors="replace")
            else:
                post_data = "(empty payload)"

            client_ip = self.client_address[0]
            now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            os.makedirs(LOG_DIR, exist_ok=True)
            with open(LOG_FILE, "a", encoding="utf-8") as f:
                f.write(f"\n{'=' * 80}\n")
                f.write(f"=== LIBRIS RUNTIME LOG RECEIVED AT {now_str} ===\n")
                f.write(f"From Client: {client_ip}:{self.client_address[1]}\n")
                f.write(f"User-Agent: {self.headers.get('User-Agent', 'Unknown')}\n")
                f.write(f"Content-Length: {content_length} bytes\n")
                f.write(f"{'-' * 80}\n")
                f.write(post_data)
                if not post_data.endswith("\n"):
                    f.write("\n")
                f.write(f"{'=' * 80}\n")

            sys.stdout.write(f"\n[LOG SERVER] >>> SUCCESS: Received {content_length} bytes from {client_ip} at {now_str}! <<<\n")
            sys.stdout.write(f"[LOG SERVER] Saved to {LOG_FILE}\n\n")
            sys.stdout.flush()

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            resp = '{"status": "ok", "message": "Log received and saved to logs/device_runtime.log"}\n'
            self.wfile.write(resp.encode("utf-8"))

        except Exception as e:
            sys.stderr.write(f"[LOG SERVER] Error handling POST: {e}\n")
            sys.stderr.flush()
            self.send_response(500)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(f'{{"status": "error", "message": "{str(e)}"}}\n'.encode("utf-8"))

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

def run():
    os.makedirs(LOG_DIR, exist_ok=True)
    ThreadingHTTPServer.allow_reuse_address = True
    server = ThreadingHTTPServer((HOST, PORT), LogRequestHandler)
    print(f"[LOG SERVER] Libris Log Server started on {HOST}:{PORT}")
    print(f"[LOG SERVER] Target log file: {LOG_FILE}")
    sys.stdout.flush()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("[LOG SERVER] Server stopped.")

if __name__ == "__main__":
    run()
