#!/bin/bash
# ==============================================================================
# Script para empaquetar y desplegar los archivos de datos de Immortal Dusk
# hacia ux0:data/immortaldusk/ (PS Vita fisica).
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SOURCE_APK_DIR="$PROJECT_ROOT/Immortal Dusk_1.0.2"
OUTPUT_DATA_DIR="$PROJECT_ROOT/ux0_data/immortaldusk"

echo "================================================================"
echo "  Preparando Archivos de Datos para Immortal Dusk (PS Vita)"
echo "================================================================"

mkdir -p "$OUTPUT_DATA_DIR/assets"
mkdir -p "$OUTPUT_DATA_DIR/sound"
mkdir -p "$OUTPUT_DATA_DIR/saves"
mkdir -p "$OUTPUT_DATA_DIR/logs"

# 1. Copiar librería nativa libaist-jni.so
if [ -f "$SOURCE_APK_DIR/lib/armeabi/libaist-jni.so" ]; then
    echo "[1/3] Copiando libaist-jni.so..."
    cp "$SOURCE_APK_DIR/lib/armeabi/libaist-jni.so" "$OUTPUT_DATA_DIR/libaist-jni.so"
else
    echo "[-] Error: no se encontró libaist-jni.so en $SOURCE_APK_DIR/lib/armeabi/"
    exit 1
fi

# 2. Copiar assets del juego
if [ -d "$SOURCE_APK_DIR/assets" ]; then
    echo "[2/3] Copiando assets (ani_data, data, font, img, txt, txte)..."
    cp -R "$SOURCE_APK_DIR/assets/"* "$OUTPUT_DATA_DIR/assets/"
else
    echo "[-] Error: no se encontró la carpeta assets/"
    exit 1
fi

# 3. Copiar y organizar sonidos (WAV SFX y MP3 BGM)
if [ -d "$SOURCE_APK_DIR/res/raw" ]; then
    echo "[3/3] Copiando efectos de sonido y BGM a sound/..."
    cp "$SOURCE_APK_DIR/res/raw/"* "$OUTPUT_DATA_DIR/sound/"
else
    echo "[-] Error: no se encontró res/raw/"
    exit 1
fi

# Limpieza de archivos ocultos de macOS
find "$OUTPUT_DATA_DIR" -name "._*" -delete 2>/dev/null || true
find "$OUTPUT_DATA_DIR" -name ".DS_Store" -delete 2>/dev/null || true

echo ""
echo "[+] Datos preparados exitosamente en: $OUTPUT_DATA_DIR"
du -sh "$OUTPUT_DATA_DIR"

echo ""
echo "Listo. Para transferir a una PS Vita física vía FTP/USB:"
echo "  python3 porting_tools/manage_vita.py"
