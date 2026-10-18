# AI PASSPORT Leogie Translator

Standalone Chinese/English translator board derived from upstream folotoy/ai-passport.
See docs/leogie-design.md and docs/leogie-install.md. Current community release 0.2.1 uses an embedded native-size GB2312/UI font and a safe single BIN. It does not require the old external font partition. Version 0.2.0 was flashed and user-tested; current-version physical acceptance is reported separately in docs/leogie-testing.md.

Version 0.3.0 adds an offline countdown from idle top. Top/middle add/subtract one minute, bottom starts, long bottom cancels. Any key stops the original alert and restores volume. See installation guide for bounds, power behavior and hardware acceptance limits.
