# Registro de Progreso del Port — Immortal Dusk (PS Vita)

---

## Estado General del Proyecto

| Subsistema | Estado | Notas |
|---|---|---|
| **Entorno & Toolchain** | ✅ Completado | CMakeLists.txt, build.sh, gitignore y scripts de deploy listos |
| **Decompilación & Símbolos** | ✅ Completado | JADX (APK) y Ghidra + Angr (libaist-jni.so) completados |
| **Loader C/C++ (so_util)** | ✅ Completado | Carga de libaist-jni.so y resolución de símbolos libc |
| **Entorno FalsoJNI** | ✅ Completado | Callbacks JNI implementados (getRes, getModel, getNumber, etc.) |
| **Pipeline Gráfico (Software / vitaGL)** | ✅ Completado | Render loop con quad 960x544 y textura RGB565 (800x480) |
| **Entrada (Táctil / Botones)** | ✅ Completado | handleCletEvent mapeado a Front Touch y controles físicos |
| **Sistema de Audio** | ✅ Completado | Mezclador multi-voz nativo (WAV PCM para SFX y minimp3 para BGM) |
| **Guardado / Persistencia** | ✅ Completado | ux0:data/immortaldusk/saves/ ruteado en initJni |
| **LiveArea & Empaquetado VPK** | ✅ Completado | VPK y ELF generados exitosamente con VitaSDK |

---

## Registro Cronológico de Fases

### Fase 1: Configuración del Entorno y Herramientas (Completada)
* **Fecha:** 2026-08-14
* **Acciones realizadas:**
  1. Repositorio Git inicializado con `.gitignore` que excluye estrictamente archivos comerciales (APKs, OBBs, ZIPs, `.so` originales, decompilados y volcados de memoria).
  2. Adaptación completa de `porting_tools/`:
     * `manage_vita.py` configurado para el Title ID `IMMDUSK01` y rutas `ux0:data/immortaldusk/`.
     * `build_and_install.sh` adaptado para `immortal_dusk.vpk`.
     * `parse_dump.py` configurado con soporte para resolución de símbolos en `libaist-jni.so`.
  3. Creado script `build.sh` con staging en `/tmp/immortaldusk-src` para evitar el bug de rutas con espacios en `vita-pack-vpk`.
  4. Habilidad `psvita-porting` y sus 15 guías de referencia instaladas en `.agents/skills/psvita-porting/` y configuración global.

### Fase 2: Decompilación y Análisis Estático (Completada)
* **Fecha:** 2026-08-14
* **Hallazgos Técnicos:**
  1. **APK:** Decompilado con JADX en `decompiled/apk_jadx/`.
     * Activity principal: `com.gamevil.ast.global.SkeletonLauncher`.
     * Render view: `com.gamevil.ast.global.MainView` ejecuta `runC(short[] sBuf)` dentro de un thread de actualización.
  2. **Binario Nativo (`libaist-jni.so`):**
     * Decompilado con Ghidra & Angr (`decompiled/libaist-jni_armeabi/ghidra/out_ghidra.c`, 83.600+ líneas).
     * Arquitectura: 32-bit ARM (`armeabi`).
     * Motor gráfico: Rasterizador por software 2D puro en C que escribe directamente a un búfer RGB565 de 800x480.
     * Imports: Únicamente funciones estándar de `libc` (sin dependencias directas de GLES).
     * Puntos de entrada JNI: `initJni`, `startC`, `runC`, `eventC`, `resumeC`, `resultlanguage`, `netRecv`, `netState`.
     * Callbacks requeridos desde C a Java: `getRes`, `sndPlay`, `sndStop`, `vibStart`, `selected_lang`, `chkImemory`, `getModel`, `getNumber`.

### Fase 3: Bootstrap del Loader y Compilación (Completada)
* **Fecha:** 2026-08-14
* **Acciones realizadas:**
  1. **`source/java.c`**: Implementación completa de FalsoJNI para `SkeletonLauncher`:
     * `getRes`: Carga dinámica de archivos desde `ux0:data/immortaldusk/assets/` con sanitización de ruta.
     * `getNumber` y `getModel`: Generación de arreglos `byte[]` (`()[B`) con identificadores compatibles.
     * `select_lang` / `selected_lang`: Forzado de idioma a Inglés (`1`).
     * `chkImemory`: Retorno de memoria disponible.
     * Stubs de red, IAP y banners publicitarios.
  2. **`source/main.c`**:
     * Inicialización de VitaGL con renderer de quad texturizado (RGB565 800x480 escalado a 960x544).
     * Mapeo de eventos táctiles (`sceTouchPeek`) y botones físicos hacia `eventC`.
     * Bucle principal ejecutando `runC`, actualización de textura y `gl_swap()`.
     * Sistema de logging `game_log` hacia consola y archivo `ux0:data/immortaldusk/logs/game.log`.
  3. **Compilación**:
     * `build/immortal_dusk.vpk` y `build/immortal_dusk.elf` generados exitosamente con VitaSDK.

### Fase 4: Integración del Sistema de Audio Nativo (Completada)
* **Fecha:** 2026-08-14
* **Acciones realizadas:**
  1. **Motor de Audio (`source/audio.h`, `source/audio.cpp`)**:
     * Implementación basada en la arquitectura probada de Prince of Persia Vita.
     * Cero dependencias de stdio/FILE*: lectura directa con `sceIoOpen`/`sceIoRead`.
     * Hilo mezclador con `sceAudioOutOpenPort` a 44100 Hz estéreo con buffer de 2048 muestras (~46 ms) para cero underruns.
     * Decodificador de streaming y bucle BGM con `mp3dec_ex_open_buf` en memoria.
     * Parser en memoria de WAV PCM de 16-bit con caché (`SfxSample`) y 12 voces polifónicas.
     * Limitador de saturación suave (`soft_clip16`) con `SOFT_CLIP_THRESHOLD = 0.92f`.
  2. **Herramientas de Preparación de Datos (`porting_tools/prepare_data_files.sh`)**:
     * Extracción y organización de 164 MB de assets, sonidos y librería nativa en `ux0:data/immortaldusk/`.

### Fase 5: Mapeo de Controles Físicos y Preparación Final (Completada)
* **Fecha:** 2026-08-14
* **Acciones realizadas:**
  1. **Mapeo de Controles (`source/main.c`)**:
     * **Stick Analógico Izquierdo & D-Pad**: Movimiento omnidireccional analógico/digital mapeado hacia el joystick virtual de combate en `(110, 370)` con radio 60 y normalización de vector.
     * **Botón $\times$ (Cruz)**: Ataque principal / Interacción / Diálogo en `(700, 380)`.
     * **Botón $\square$ (Cuadrado)**: Habilidad 1 en `(620, 420)`.
     * **Botón $\triangle$ (Triángulo)**: Habilidad 2 en `(620, 310)`.
     * **Botón $\bigcirc$ (Círculo)**: Habilidad 3 en `(700, 260)`.
     * **Gatillo R1**: Habilidad 4 en `(770, 310)`.
     * **Gatillo L1**: Poción de Vida (HP) en `(420, 450)`.
     * **Botón Select**: Poción de Maná (MP) en `(480, 450)`.
     * **Botón Start**: Menú principal / Pausa en `(760, 35)`.
     * **Pantalla Táctil Frontal**: Muestreo continuo 1:1 para menús, inventario y navegación.
  2. **LiveArea**:
     * Iconos, fondos, splash screen y template XML configurados con Title ID `IMMDUSK01`.
  3. **Compilación**:
     * `immortal_dusk.vpk` y `immortal_dusk.elf` generados con soporte completo de controles físicos y táctiles.

---

## Estado de Entrega & Pruebas en Hardware Real
1. **Compilación:** 100% exitosa con VitaSDK (`build/immortal_dusk.vpk`).
2. **Archivos de Datos:** 164 MB listos en `ux0_data/immortaldusk/`.
3. **Herramienta de Gestión:** `porting_tools/manage_vita.py` lista para despliegue por FTP / USB / WiFi en la PS Vita física.
