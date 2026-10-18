# Leogie Translator v0.2

User-approved scope (2026-10-17): AI PASSPORT standalone bilingual translator,
based on XiaoZhi, using the user's existing Leogie character. One confirm press
starts a continuous session; the next press ends it in every active state.
Chinese speech becomes simple, faithful English; English becomes natural Chinese.
No chat, explanations, unsolicited correction, or answering the spoken question.
The user explicitly authorized implementing and flashing the cartoon UI on 2026-10-18.
Do not commit, push or publish without separate approval.

## Architecture

- Fork upstream e0d2390a1ca415473508bbc0c66eaf1d8dc04e37 in its own checkout and
  codex/leogie-translator branch. Build with independent ESP-IDF 6.1.
- New `ai-passport-leogie` board: ESP32-C3, 8 MB Flash, no PSRAM, ST7789 240x320,
  ES8311, ADC three-button input, CW2017 battery. Pin mapping comes from local
  AI PASSPORT board_config.h and firmware skill, never another board's defaults.
- Reuse XiaoZhi Wi-Fi provisioning, Opus streaming and audio pipeline. Add a
  bounded translator session controller. Stop invalidates pending work, aborts
  playback and closes the socket; stale events cannot resume a session.
- Direct XiaoZhi official cloud connection. Use the upstream activation/discovery
  flow and provisioned MQTT/UDP or WebSocket endpoints and credentials. No Mac
  runtime, self-hosted service, local model, or model API key is required.
- Translation is supplied by a dedicated XiaoZhi console role. Every utterance is
  translated to the opposite language, with A1/A2 English, preserving quantities,
  questions and negation. The role text ships with the firmware. Device binding
  and role setup are required before the first live translation test.
- Half duplex: capture while listening, cloud VAD decides sentence completion;
  resume capture after actual playback drain. The session lasts until next press.
  Do not require a nonstandard playback ACK from the official server.
- Cloud provisioning is retained, automatic generic firmware upgrade is disabled
  to protect the custom factory/font layout and Leogie experience. MCP tools are
  not advertised in translation mode, and spoken instructions are translated.
- UI: original Leogie character at top, readable source/translation below;
  listening/thinking/speaking/stopped/error states, direction indicator and
  network/battery status. Upper/lower buttons change volume or scroll long text;
  confirm remains start/stop; long confirm while idle enters setup.
- No full-screen animation transformations. Small Flash-resident I8 sprites at two native sizes.
  Four cartoon poses generated from the approved Leogie reference; preserve original assets.
- Protect [0x356000,0x35A000). Safe web BIN ends before 0x356000. If assets cannot
  fit, use a verified split package and an accompanying multi-segment web flasher;
  never merge over the protected gap. Report NVS reset behavior accurately.

## Acceptance

Host tests: session toggles, stale/cancelled work, capture cancellation, transport
selection, display direction, font coverage, release validation and board settings.
Build exact ESP32-C3 binary; inspect idf size and native UI; verify partition ranges,
hashes and ZIP contents. Preserve the upstream protocol contracts and build both
transport implementations. Test host receive ordering without a custom ACK.

Live official-cloud translation and timing require the user's flashed device,
account binding and role configuration. They are PENDING_DEVICE_ACTIVATION until
actually exercised. Retired local-service tests are not evidence of cloud success.

Physical boot, microphone gain, speaker volume, ADC calibration, Wi-Fi robustness,
animation performance and battery behavior remain user acceptance, explicitly
NOT_RUN. This is an authorized handoff boundary, not an assertion of hardware
verification. No OTA upgrade of unrelated firmware is allowed.

## 0.2.1 community packaging

Font embedded directly in the application: 7172 glyphs, including every GB2312 Han, native 20px/4bpp. No external resource install. Rare characters outside this set can be missing on screen. Full font licenses embedded and extractable. Safe web image at address 0 ends before factory end 0x350000; merged gap resets NVS, identity remains untouched. Cloud/character/button semantics remain unchanged.

## 0.3.0 countdown

Idle top opens 1–120 minutes (default 10); top/middle adjust by one minute; bottom starts; long bottom cancels and returns, with no background timing. A monotonic microsecond deadline drives a ceil-to-minutes “倒计时 XX 分” display. Repeated confirm cannot reset it. Ringing repeats original six-note Opus pulses at transient gain 100 until any key stops it and restores volume, without persisting the boost. Timer is exclusive with translation and disables idle power transitions until exit; queued sleep events recheck the active flag. Timer callbacks coalesce to one pending main-loop job. Nonblocking audio admission and each 600ms burst fits the decode queue. Existing character plus native 48px/4bpp WenKai digits; no scale layers or partition/protocol change. Reboot cancels timer.
