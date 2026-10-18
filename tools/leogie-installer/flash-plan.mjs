export const EXPECTED = new Map([
  ['bootloader.bin', [0, 0x8000]],
  ['partition-table.bin', [0x8000, 0x1000]],
  ['leogie-translator.bin', [0x10000, 0x340000]],
]);
export async function sha256(bytes) {
  return Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256', bytes)), b => b.toString(16).padStart(2, '0')).join('');
}
export async function prepareFiles(manifest, readFile) {
  if (manifest.chip !== 'ESP32-C3' || manifest.flash_size !== 0x800000 ||
      manifest.erase_all !== false || !Array.isArray(manifest.segments) || manifest.segments.length !== 3) {
    throw new Error('安装包型号或分区不匹配');
  }
  const seen = new Set(), files = [];
  for (const item of manifest.segments) {
    const expected = EXPECTED.get(item.file);
    if (!expected || seen.has(item.file) || item.address !== expected[0] ||
        !Number.isInteger(item.size) || item.size <= 0 || item.size > expected[1]) {
      throw new Error('安装包地址或长度不正确');
    }
    seen.add(item.file);
    const data = await readFile(item.file);
    if (!(data instanceof Uint8Array) || data.length !== item.size || await sha256(data) !== item.sha256) {
      throw new Error('文件校验失败：' + item.file);
    }
    files.push({address: item.address, data});
  }
  return files.sort((a,b) => a.address - b.address);
}
export async function flashVerified(loader, files, calculateMD5Hash, reportProgress) {
  if (loader.chip.CHIP_NAME !== 'ESP32-C3' || await loader.detectFlashSize() !== '8MB') {
    throw new Error('请连接 8 MB Flash 的 ESP32-C3 AI PASSPORT');
  }
  // Read only the protected identity slice, keep it in RAM, never log its value.
  const before = await loader.readFlash(0x356000, 0x4000);
  if (before.length !== 0x4000) throw new Error('无法读取设备身份区域，已停止');
  await loader.writeFlash({fileArray: files, eraseAll: false, compress: true,
    flashMode: 'keep', flashFreq: 'keep', flashSize: 'keep', calculateMD5Hash, reportProgress});
  const after = await loader.readFlash(0x356000, 0x4000);
  if (after.length !== before.length || await sha256(before) !== await sha256(after)) {
    throw new Error('设备身份校验未通过，请停止使用并保留此页面');
  }
}
