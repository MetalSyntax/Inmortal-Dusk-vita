# Herramientas de Porting Android → PS Vita (Immortal Dusk)

Herramientas adaptadas para el proyecto so-loader (VitaSDK + FalsoJNI) de **Immortal Dusk** en PS Vita.

## Build y despliegue

- **`build/build_and_install.sh`** — build completo vía staging en `/tmp` (workaround para rutas con espacios) y generación de VPK y ELF.
- **`manage_vita.py`** — herramienta interactiva de despliegue y debug por FTP/WiFi (VitaShell):
  - Subida de `.vpk` o únicamente `eboot.bin` a `ux0:app/IMMDUSK01/`.
  - Descarga del `.psp2dmp` (core dump) y logs más recientes de `ux0:data/immortaldusk/logs/`.
  - Análisis automático de crash dumps con `parse_dump.py`.
  - Sincronización y verificación de assets (`ux0:data/immortaldusk/`).
- **`prepare_data_files.sh`** — empaqueta y prepara todos los archivos de datos (164 MB) para la consola PS Vita física.
- **`misc/get_dump.sh`** — descarga el `.psp2dmp` más reciente de la consola por FTP. Uso: `./get_dump.sh <IP-de-la-vita>`.

## Análisis y Debugging de Crashes

- **`parse_dump.py`** — analizador avanzado de volcados de memoria `.psp2dmp` integrando `vita-parse-core`, símbolos ELF y símbolos exportados del `.so` (Ghidra/objdump).
- **`ai_bash_commands.sh`** — scripts y comandos bash para desensamblado con `arm-vita-eabi-objdump` y búsqueda de símbolos en `libaist-jni.so`.

## Decompilación

- **`build/decompile_all.sh`** — decompila el APK con JADX (`classes.dex` → Java) y las librerías dinámicas (`libaist-jni.so` → pseudo-C) usando `devrvk/so-decompiler` (Ghidra & Angr).

## Utilidades varias

- **`build/clean_macos.sh`** — elimina archivos residuales `._*` de macOS.
- **`misc/translate_docs.py`** — traducción de documentación Markdown por lotes.
