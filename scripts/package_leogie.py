#!/usr/bin/env python3
"""Verify and package explicit flash segments. Never opens a serial device."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
PROTECTED = (0x356000, 0x35A000)
VERSION = '0.3.0'


def verify_ranges(ranges):
    last_end = 0
    if not ranges: raise ValueError('No flash segments')
    for address, size in sorted(ranges):
        if address < 0 or address % 4096 or size <= 0:
            raise ValueError('Invalid flash segment or alignment')
        erased_end = (address + size + 4095) // 4096 * 4096
        if erased_end > 0x800000 or address < last_end:
            raise ValueError('Overlapping segments or outside 8 MB Flash')
        if address < PROTECTED[1] and erased_end > PROTECTED[0]:
            raise ValueError('Segment erases protected identity sectors')
        last_end = erased_end


def verify_esp_image(data):
    if len(data) < 24 or data[0] != 0xE9 or not 1 <= data[1] <= 16:
        raise ValueError('Invalid ESP image header')
    if struct.unpack_from('<H', data, 12)[0] != 5:
        raise ValueError('Firmware must target ESP32-C3')
    if data[2:4] != b'\x02\x3f': raise ValueError('Expected DIO / 8MB / 80MHz flash header')
    if data[23] != 1: raise ValueError('Image SHA-256 is required')
    pos, checksum = 24, 0xEF
    for _ in range(data[1]):
        if pos + 8 > len(data): raise ValueError('Truncated segment header')
        _, size = struct.unpack_from('<II', data, pos)
        pos += 8
        if pos + size > len(data): raise ValueError('Truncated segment data')
        for byte in data[pos:pos+size]: checksum ^= byte
        pos += size
    pos += 15 - pos % 16
    if pos >= len(data) or data[pos] != checksum: raise ValueError('Image checksum mismatch')
    end = pos + 1
    if data[end:end+32] != hashlib.sha256(data[:end]).digest():
        raise ValueError('Image SHA-256 mismatch')
    if len(data) != end + 32: raise ValueError('Unexpected bytes after ESP image')


def verify_font(data, manifest):
    if not data or len(data) > 0x4A0000 or len(data) != manifest['bitmap_bytes']:
        raise ValueError('Font length mismatch')
    if hashlib.sha256(data).hexdigest() != manifest['bitmap_sha256']:
        raise ValueError('Font SHA-256 mismatch')
    if zlib.crc32(data) != manifest['bitmap_crc32']:
        raise ValueError('Font CRC mismatch')


def verify_partitions(data):
    expected = {
        'nvs': (1, 2, 0x9000, 0x6000), 'phy_init': (1, 1, 0xF000, 0x1000),
        'factory': (0, 0, 0x10000, 0x340000),
        'legacy_cardid': (1, 2, 0x356000, 0x4000),
        'leogie_font': (1, 0x40, 0x360000, 0x4A0000),
    }
    actual, md5_seen = {}, False
    for pos in range(0, len(data), 32):
        row = data[pos:pos+32]
        if len(row) != 32: raise ValueError('Truncated partition table')
        if row[:2] == b'\xeb\xeb':
            if row[16:] != hashlib.md5(data[:pos]).digest():
                raise ValueError('Partition table MD5 mismatch')
            if any(byte != 0xff for byte in data[pos+32:]):
                raise ValueError('Unexpected partition table padding')
            md5_seen = True
            break
        if row[:2] != b'\xaa\x50': raise ValueError('Missing partition table entry')
        _, kind, subtype, offset, size, name, flags = struct.unpack('<2sBBII16sI', row)
        if flags: raise ValueError('Unexpected partition flags')
        name = name.rstrip(b'\0').decode('ascii')
        if name in actual: raise ValueError('Duplicate partition')
        actual[name] = (kind, subtype, offset, size)
    if not md5_seen or actual != expected: raise ValueError('Unexpected partition layout')


def esp_image_size(data):
    if len(data) < 24 or data[0] != 0xe9 or not 1 <= data[1] <= 16:
        raise ValueError('Invalid image header')
    pos = 24
    for _ in range(data[1]):
        if pos + 8 > len(data): raise ValueError('Truncated image')
        size = struct.unpack_from('<I', data, pos+4)[0]
        pos += 8 + size
        if pos > len(data): raise ValueError('Truncated image segment')
    end = (pos // 16 + 1) * 16 + 32
    if end > len(data): raise ValueError('Truncated checksum or digest')
    return end


def verify_web_image(data):
    # Check erase sectors, not just the final payload byte.
    if not 0x10000 < len(data) <= 0x350000:
        raise ValueError('Web image is incomplete or exceeds factory partition')
    verify_ranges([(0, len(data))])
    boot_end = esp_image_size(data)
    if boot_end > 0x8000: raise ValueError('Bootloader overlaps partition table')
    verify_esp_image(data[:boot_end])
    verify_partitions(data[0x8000:0x8c00])
    verify_esp_image(data[0x10000:])
    for padding in (data[boot_end:0x8000], data[0x8c00:0x10000]):
        if any(byte != 0xff for byte in padding):
            raise ValueError('Unexpected merged padding or embedded private settings')


def merge_web_image(boot, partitions, app):
    verify_esp_image(boot)
    verify_partitions(partitions)
    verify_esp_image(app)
    if len(boot) > 0x8000 or len(partitions) != 3072 or len(app) > 0x340000:
        raise ValueError('Image exceeds its fixed slot')
    data = boot.ljust(0x8000, b'\xff') + partitions
    data = data.ljust(0x10000, b'\xff') + app
    verify_web_image(data)
    return data


def verify_release(folder):
    manifest = json.loads((folder/'manifest.json').read_text())
    expected = {'bootloader.bin':0, 'partition-table.bin':0x8000,
                'leogie-translator.bin':0x10000}
    if manifest.get('erase_all') is not False or manifest.get('chip') != 'ESP32-C3':
        raise ValueError('Unexpected release hardware or erase policy')
    role = (folder/'小智角色配置.txt').read_text()
    if len(role.strip()) < 100: raise ValueError('Translator role is missing or empty')
    entries = manifest['segments']
    if len(entries) != 3 or {x['file']:x['address'] for x in entries} != expected:
        raise ValueError('Unexpected release files/addresses')
    verify_ranges([(x['address'],x['size']) for x in entries])
    for item in entries:
        data = (folder/item['file']).read_bytes()
        if len(data) != item['size'] or hashlib.sha256(data).hexdigest() != item['sha256']:
            raise ValueError('Release file checksum mismatch: '+item['file'])
    verify_esp_image((folder/'bootloader.bin').read_bytes())
    verify_esp_image((folder/'leogie-translator.bin').read_bytes())
    verify_partitions((folder/'partition-table.bin').read_bytes())
    web = (folder/manifest['web_image']['file']).read_bytes()
    verify_web_image(web)
    if hashlib.sha256(web).hexdigest() != manifest['web_image']['sha256']:
        raise ValueError('Web image digest mismatch')
    if web != merge_web_image(*[(folder/name).read_bytes() for name in expected]):
        raise ValueError('Web image differs from split segments')


def package(output, build):
    if output.exists(): raise ValueError('Output exists; choose a new output directory')
    description = json.loads((build/'project_description.json').read_text())
    configuration = json.loads((build/'config/sdkconfig.json').read_text())
    expected_config = {'IDF_TARGET':'esp32c3', 'LEOGIE_TRANSLATOR':True,
        'BOARD_TYPE_AI_PASSPORT_LEOGIE':True, 'ESPTOOLPY_FLASHSIZE':'8MB',
        'ESPTOOLPY_FLASHMODE':'dio', 'ESPTOOLPY_FLASHFREQ':'80m',
        'APP_PROJECT_VER':'leogie-'+VERSION,
        'OTA_URL':'https://api.tenclass.net/xiaozhi/ota/'}
    if any(configuration.get(k) != v for k,v in expected_config.items()) or configuration.get('SPIRAM'):
        raise ValueError('Build configuration does not match the release board')
    app_path = build / description['app_bin']
    images = [
        ('bootloader.bin', 0, (build/'bootloader/bootloader.bin').read_bytes()),
        ('partition-table.bin', 0x8000, (build/'partition_table/partition-table.bin').read_bytes()),
        ('leogie-translator.bin', 0x10000, app_path.read_bytes()),
    ]
    verify_ranges([(address, len(data)) for _, address, data in images])
    verify_esp_image(images[0][2]); verify_esp_image(images[2][2])
    if len(images[0][2]) > 0x8000 or len(images[2][2]) > 0x340000:
        raise ValueError('Bootloader or app exceeds its partition')
    verify_partitions(images[1][2])
    font_manifest = json.loads((ROOT/'assets/leogie/font-manifest.json').read_text())
    bitmap = (ROOT/'assets/leogie/font-bitmap.bin').read_bytes()
    verify_font(bitmap, font_manifest)
    if font_manifest.get('storage') != 'embedded-app' or bitmap not in images[2][2]:
        raise ValueError('Current font is not embedded in this application')
    from show_font_license import extract_license
    if extract_license(images[2][2]) != (ROOT/'assets/leogie/FONT-NOTICES.txt').read_text():
        raise ValueError('Standalone firmware is missing the exact full font notices')
    web = merge_web_image(*[data for _, _, data in images])
    web_name = f'leogie-translator-v{VERSION}-web.bin'
    manifest = {
        'product': 'AI PASSPORT Leogie Translator', 'version': VERSION,
        'chip': 'ESP32-C3', 'flash_size': 0x800000, 'flash_mode': 'dio', 'flash_frequency': '80m',
        'protected_range': list(PROTECTED), 'erase_all': False,
        'device_flashing_performed': False, 'physical_acceptance': 'NOT_RUN',
        'requires': 'Wi-Fi Internet access and device binding to a XiaoZhi cloud account with the bundled translator role',
        'cloud_acceptance': 'PENDING_CURRENT_VERSION_DEVICE_TEST',
        'upstream_commit': subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        'idf_version': description['git_revision'], 'font_codepoints': len(font_manifest['codepoints']),
        'firmware_version': description['project_version'],
        'font_storage': 'embedded-app',
        'web_image': {'file': web_name, 'address': 0, 'size': len(web),
                      'sha256': hashlib.sha256(web).hexdigest(),
                      'clears_nvs': True, 'preserves_device_identity': True},
        'segments': [{'file': name, 'address': address, 'size': len(data),
                      'sha256': hashlib.sha256(data).hexdigest()} for name, address, data in images],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='leogie-package-', dir=output.parent) as temporary:
        folder = Path(temporary)/output.name
        folder.mkdir()
        for name, _, data in images: (folder/name).write_bytes(data)
        (folder/web_name).write_bytes(web)
        (folder/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
        shutil.copy2(ROOT/'docs/leogie-install.md', folder/'使用说明.md')
        shutil.copy2(ROOT/'docs/leogie-testing.md', folder/'验证记录.md')
        if (ROOT/'docs/evidence').is_dir(): shutil.copytree(ROOT/'docs/evidence', folder/'evidence')
        shutil.copy2(ROOT/'LICENSE', folder/'UPSTREAM-LICENSE')
        shutil.copy2(ROOT/'assets/leogie/OFL.txt', folder/'FONT-LICENSE.txt')
        shutil.copy2(ROOT/'assets/leogie/FONT-NOTICES.txt', folder/'FONT-NOTICES.txt')
        shutil.copy2(ROOT/'docs/leogie-role.txt', folder/'小智角色配置.txt')
        shutil.copytree(ROOT/'tools/leogie-installer', folder/'installer')
        shutil.copy2(Path(__file__), folder/'verify_release.py')
        shutil.copy2(ROOT/'scripts/verify_web_image.py', folder/'verify_web_image.py')
        shutil.copy2(ROOT/'scripts/show_font_license.py', folder/'show_font_license.py')
        (folder/'source.patch').write_bytes(subprocess.check_output(['git','diff','--binary'],cwd=ROOT))
        with zipfile.ZipFile(folder/'source.zip','w',zipfile.ZIP_DEFLATED) as archive:
            files = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).decode().split('\0')
            for relative in sorted(set(files)):
                path = ROOT/relative
                if relative and path.is_file() and not relative.endswith('.ppm'):
                    archive.write(path, relative)
            if (build.parent/'dependencies.lock').exists(): archive.write(build.parent/'dependencies.lock','dependencies.lock')
        shutil.copytree(folder, output)
    verify_release(output)
    archive_path = output.parent / (output.name + '.zip')
    with zipfile.ZipFile(archive_path,'w',zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(output.rglob('*')):
            if path.is_file(): archive.write(path, str(Path(output.name)/path.relative_to(output)))
    with zipfile.ZipFile(archive_path) as archive:
        for item in manifest['segments']:
            data = archive.read(f"{output.name}/{item['file']}")
            if hashlib.sha256(data).hexdigest() != item['sha256']: raise ValueError('ZIP differs from manifest')
        if archive.read(f'{output.name}/{web_name}') != web:
            raise ValueError('ZIP web image differs')
    print(json.dumps({'directory': str(output), 'zip': str(archive_path),
                     'zip_sha256': hashlib.sha256(archive_path.read_bytes()).hexdigest(),
                     'manifest': manifest}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/f'releases/leogie-translator-v{VERSION}')
    parser.add_argument('--build', type=Path, default=ROOT/'build')
    parser.add_argument('--verify', type=Path)
    args = parser.parse_args()
    if args.verify:
        verify_release(args.verify.resolve())
        print('Release images, partitions, hashes and protected ranges verified.')
    else:
        package(args.output.resolve(), args.build.resolve())
