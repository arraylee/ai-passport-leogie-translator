#!/usr/bin/env python3
"""Isolated reproducible build; never flashes, commits or changes the old IDF."""
import argparse
import fcntl
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--idf', type=Path, default=Path.home()/'esp/esp-idf-v6.1')
    parser.add_argument('--mirror', type=Path, default=Path.home()/'.cache/leogie-translator-build')
    args = parser.parse_args()
    mirror, sdk = args.mirror.resolve(), args.idf.resolve()
    if ' ' in str(mirror) or mirror == ROOT or mirror in ROOT.parents or ROOT in mirror.parents:
        raise SystemExit('Use a separate build directory without spaces.')
    if not (sdk/'export.sh').is_file(): raise SystemExit('ESP-IDF 6.1 is not installed at --idf.')
    mirror.mkdir(parents=True, exist_ok=True)
    with (mirror/'.leogie-build.lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        exclusions = ['/.git/','/.venv/','/.local/','/build/','/releases/',
                      '/server/runtime/','/server/.venv/','/.leogie-build.lock']
        subprocess.run(['rsync','-a','--delete',*[f'--exclude={x}' for x in exclusions],str(ROOT)+'/',str(mirror)+'/'],check=True)
        command = ('source ' + shlex.quote(str(sdk/'export.sh')) +
            ' && python scripts/build.py ai-passport-leogie --name ai-passport-leogie --language zh-CN && idf.py size')
        subprocess.run(['/bin/bash','-c',command],cwd=mirror,check=True)
        print('Verified build directory:', mirror/'build')


if __name__ == '__main__': main()
