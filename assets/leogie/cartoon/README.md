# Leogie cartoon sprite pack

Generated with Codex built-in imagegen from the user's Leogie image and approved cartoon UI concept. `source.png` is the transparent 2×2 four-pose result. Existing photo assets are preserved one directory above.

Art brief: same friendly brown-haired human Leogie with dark round glasses and blue shirt with yellow trim; child-friendly rounded cartoon style; four consistent full-body poses: waving, hand near ear listening, hand on chin thinking, open palms speaking. Transparent background, no letters or words, no scenery, no text baked into images.

Rebuild with `.venv/bin/python scripts/leogie_cartoon_assets.py`. This writes native-size indexed images and generated C/H assets. Do not hand-edit generated C. The separate older `leogie_assets.py` still produces the historical photo atlas and font; running it overwrites the generated character C/H and requires rerunning the cartoon generator.

Text is rendered using the existing real 20px/4bpp UI font; see ../FONT-NOTICES.txt and ../OFL.txt. Scene shapes and listening bars use LVGL objects. Bars indicate listening activity, not measured microphone level.
