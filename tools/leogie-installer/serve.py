#!/usr/bin/env python3
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import webbrowser

root = Path(__file__).resolve().parents[1]
if not (root/'manifest.json').is_file():
    raise SystemExit('Please run the installer from the extracted release ZIP.')
server = ThreadingHTTPServer(('127.0.0.1', 0), partial(SimpleHTTPRequestHandler, directory=str(root)))
url = f'http://127.0.0.1:{server.server_port}/installer/'
print('安装页面：' + url, flush=True)
webbrowser.open(url)
try: server.serve_forever()
except KeyboardInterrupt: pass
finally: server.server_close()
