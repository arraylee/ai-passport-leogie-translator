# Leogie 中英翻译器 · AI PASSPORT

让 Leogie 陪你练口语，也帮你看时间。按一下开始，中英文自动互译并朗读，再按一下结束；待机按上键，还能开启会响铃的分钟倒计时。

[下载 v0.3.0](https://github.com/arraylee/ai-passport-leogie-translator/releases/tag/v0.3.0) · [完整使用说明](docs/leogie-install.md) · [小智翻译角色](docs/leogie-role.txt) · [验证记录](docs/leogie-testing.md)

![倒计时实际界面渲染](docs/evidence/countdown-native-lvgl-screens.png)

## 怎么用

- **翻译**：连接 Wi-Fi 并在小智控制台绑定专用翻译角色。按确定开始，看到“我在听”后说话。中文译成简单英文，英文译成中文，播报结束继续听；再按确定结束。
- **音量**：翻译过程中，上下键调音量；长按上键最大音量，长按下键静音。
- **倒计时**：待机按上键进入，默认 10 分钟。上键加 1 分钟，中键减 1 分钟，最下面的确定键开始。范围 1–120 分钟，屏幕显示“倒计时 XX 分”，剩余分钟向上取整。
- **取消与响铃**：长按确定约 2 秒取消并返回翻译器；到点循环响铃，按任意键停止并恢复原音量。倒计时不需要网络，退出或重启后不会继续计时。

翻译由设备直接接入小智服务，不需要电脑提供翻译。第一次配网、绑定和角色配置见[使用说明](docs/leogie-install.md)。模型可能误译；屏幕覆盖常用简体汉字，少量生僻字或繁体字可能显示不全。

## 安装

优先下载完整 ZIP，解压后使用其中的分段安装器；分段安装保留 NVS 间隙并检查设备身份。社区单 BIN 是地址 `0x0` 的合并镜像，会重置本地网络设置。不要全片擦除，也不要把应用文件当作地址 0 的镜像。

本版已经刷入设备；用户确认倒计时显示、响铃、停止与退出正常。145 项主机测试完成（1 项跳过）；本版完整云端翻译回归、长时间计时和续航仍待验证。安装包内为打包时的验证快照，最新验收见[设备记录](docs/evidence/device-install-v0.3.0.json)。

## 从源码构建

此仓库以小智 `e0d2390a1ca415473508bbc0c66eaf1d8dc04e37` 为基础，加入独立 `ai-passport-leogie` 板型。使用 ESP-IDF 6.1；依赖与板型配置由构建脚本管理。

```sh
source /path/to/esp-idf-v6.1/export.sh
python scripts/build.py ai-passport-leogie --name ai-passport-leogie --language zh-CN
idf.py size
python3 -m unittest discover -s scripts/tests -q
python3 scripts/package_leogie.py --build build
```

源码目录包含空格时，可用 `python3 scripts/build_leogie.py --idf /path/to/esp-idf-v6.1 --mirror /path/without/spaces` 在独立目录构建，再把该目录的 `build` 传给打包脚本。

字体和卡通素材已生成并随源码提供，普通构建无需生图。重新生成素材时，字体脚本需要 fontTools 与 Node.js，倒计时资源脚本还需要 FFmpeg；完整许可证位于 `assets/leogie/FONT-NOTICES.txt`。硬件输入、输出共享 24 kHz 时钟；不要改为不同采样率。分区与安装约束见[设计说明](docs/leogie-design.md)。

## English

Leogie is a Chinese–English translator with a friendly cartoon interface and a minute countdown. Press Confirm once to translate, wait until listening resumes before speaking again, and press Confirm to stop. Chinese-to-English translations favor simple everyday words. Translation requires Wi-Fi and a XiaoZhi account with the supplied translator role; the computer is not the translation server.

While idle, press Up to open a 1–120 minute timer. Up adds one minute, the middle button subtracts one, and Confirm starts. Remaining minutes are rounded up; hold Confirm to cancel and return. A brisk repeating alarm sounds at the deadline; any key stops it and restores the previous volume. The timer works offline after initial setup, keeps the screen awake, and is cancelled by exit or reboot.

v0.3.0 was flashed and the countdown display, alarm, dismissal and exit were user-tested. Long-duration operation and full translation regression on this version remain unverified. Download the release ZIP for a segmented installer; the merged BIN at address zero resets local network settings while preserving the protected device identity range.

## 来源与许可

基于 [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)，保留 [MIT LICENSE](LICENSE) 与[上游说明](README.upstream.md)。文字使用 LXGW WenKai Medium（SIL OFL 1.1）；第三方安装器依赖各自的许可证随文件提供。界面图片为实际显示代码的渲染预览，不是设备照片。
