# ⚔️ Immortal Dusk — PS Vita Port v1.0

First official release of **Immortal Dusk** for the PlayStation Vita and PlayStation TV!

This port brings Gamevil's classic action RPG to the PS Vita natively via dynamic ARM shared library loading (`.so loader`), `FalsoJNI`, and `vitaGL` hardware scaling.

---

## ✨ Features & Highlights

- **🚀 Native 60 FPS Performance:** Internal 800x480 RGB565 software rasterizer hardware-scaled to full screen (**960x544**) with bilinear filtering via `vitaGL`.
- **🎮 Full Physical & Touch Controls:** Complete dual-input support:
  - 8-direction movement via **Left Analog Stick** and **D-Pad**.
  - Face buttons and triggers mapped for fast combat, skills, and item shortcuts.
  - Full **Front Touchscreen** support for menus, inventory, and HUD navigation.
- **🔊 Low-Latency Multi-Voice Audio Engine:** Powered by `sceAudioOut`, supporting 12-voice polyphonic WAV PCM sound effects and memory-streamed looping MP3 background music (`minimp3`).
- **💎 Offline Cash Shop & K-Point Emulation:** Built-in local simulation of Gamevil's legacy server protocol, preventing infinite loading hangs and allowing purchases of potions, resurrection stones, teleport scrolls, and bag/storage expansions completely offline.
- **🖼️ High-Quality LiveArea:** Custom icon, startup gate, and background remastered with 8-bit indexed palettes matching PS Vita OS standards.

---

## 🕹️ Controls

| PS Vita Button | In-Game Action |
| :--- | :--- |
| **Left Stick / D-Pad** | 8-Direction Character Movement |
| **Cross ($\times$)** | Primary Attack / Talk / Confirm |
| **Square ($\square$)** | Skill 1 |
| **Triangle ($\triangle$)** | Skill 2 |
| **Circle ($\bigcirc$)** | Skill 3 / Cancel / Back |
| **R1 Trigger** | Skill 4 |
| **L1 Trigger** | Quick HP Potion |
| **SELECT** | Quick MP Potion |
| **START** | In-Game Menu / Pause |
| **Front Touchscreen** | Direct touch interaction across menus, inventory, and HUD |

---

## 📦 Requirements & Installation Guide

> **Legal Disclaimer:** This release only provides the loader executable (`.vpk`). No copyrighted assets or proprietary binaries are included. You must supply files from a legally acquired copy of **Immortal Dusk v1.0.2** for Android.

### 1. Prerequisites on your PS Vita
Ensure you have the following plugins installed:
- [kubridge.skprx](https://github.com/TheOfficialFloW/kubridge/releases)
- [fd_fix.skprx](https://github.com/TheOfficialFloW/FdFix/releases) (or rePatch)
- `libshacccg.suprx` (installed in `ur0:data/` via *ShaRKBR3ED* or *CrystalPSM*)

---

### 2. Setup Steps
1. Download and install **`immortal_dusk.vpk`** on your PS Vita using **VitaShell**.
2. Obtain your legal **`Immortal Dusk_1.0.2.apk`** file and rename/extract it as a `.zip` archive.
3. Extract and copy the following files to `ux0:data/immortaldusk/`:
   - Copy `lib/armeabi/libaist-jni.so` $\to$ `ux0:data/immortaldusk/libaist-jni.so`
   - Copy the APK's `assets/` folder $\to$ `ux0:data/immortaldusk/assets/`
   - Copy the APK's `res/raw/` folder $\to$ `ux0:data/immortaldusk/res/raw/`

#### 📂 Final Folder Structure on `ux0:`
```text
ux0:data/immortaldusk/
├── libaist-jni.so
├── assets/
│   ├── ani_data/
│   ├── data/
│   ├── font/
│   ├── img/
│   ├── txt/
│   └── txte/
├── res/
│   └── raw/
│       ├── snd0.wav ... snd61.wav
│       └── snd100.mp3 ... snd122.mp3
└── saves/
```

4. Launch the game from your LiveArea and enjoy!

---

## 👏 Credits & Acknowledgements

- **Gamevil / KNETP** for developing the original game.
- **TheFloW** for the `.so` loader architecture and `kubridge`.
- **Rinnegatamante** for `vitaGL` and invaluable contributions to the PS Vita scene.
- **Volodymyr Atamanenko** for `so_util` and soloader boilerplate.
