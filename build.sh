#!/bin/bash
set -e

# ==============================================================================
# Script de Build para Immortal Dusk (PS Vita)
# Maneja staging en /tmp para evitar problemas con rutas con espacios en VitaSDK
# ==============================================================================

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VITASDK="${VITASDK:-$HOME/vitasdk}"
export VITASDK
export PATH="$VITASDK/bin:$PATH"

if [ ! -d "$VITASDK" ]; then
    echo "[-] Error: VITASDK no encontrado en '$VITASDK'. Configura la variable VITASDK."
    exit 1
fi

TMP_SRC="/tmp/immortaldusk-src"
TMP_BUILD="/tmp/immortaldusk-build"
OUT_BUILD_DIR="$PROJECT_DIR/build"

BUILD_TYPE="Release"
EXTRA_CMAKE_FLAGS=()
VPK_SUFFIX=""

# Parsear argumentos
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            echo "[*] Limpiando directorios de build temporales..."
            rm -rf "$TMP_SRC" "$TMP_BUILD"
            shift
            ;;
        *)
            EXTRA_CMAKE_FLAGS+=("$1")
            shift
            ;;
    esac
done

echo "================================================================"
echo "  Compilando Immortal Dusk para PS Vita ($BUILD_TYPE)"
echo "================================================================"

# Preparar symlink sin espacios
rm -f "$TMP_SRC"
ln -s "$PROJECT_DIR" "$TMP_SRC"

mkdir -p "$TMP_BUILD"
mkdir -p "$OUT_BUILD_DIR"

cd "$TMP_BUILD"

echo "[1/3] Configurando con CMake..."
cmake "$TMP_SRC" \
    -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    "${EXTRA_CMAKE_FLAGS[@]}"

echo "[2/3] Compilando..."
NCPU=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
make -j"$NCPU"

echo "[3/3] Copiando artefactos a $OUT_BUILD_DIR..."
TARGET_NAME="immortal_dusk"
if [ -f "$TARGET_NAME.vpk" ]; then
    cp "$TARGET_NAME.vpk" "$OUT_BUILD_DIR/${TARGET_NAME}${VPK_SUFFIX}.vpk"
    echo "[+] VPK generado: $OUT_BUILD_DIR/${TARGET_NAME}${VPK_SUFFIX}.vpk"
fi

if [ -f "eboot.bin" ]; then
    cp "eboot.bin" "$OUT_BUILD_DIR/eboot.bin"
fi

if [ -f "$TARGET_NAME" ]; then
    cp "$TARGET_NAME" "$OUT_BUILD_DIR/${TARGET_NAME}${VPK_SUFFIX}.elf"
    echo "[+] ELF generado: $OUT_BUILD_DIR/${TARGET_NAME}${VPK_SUFFIX}.elf"
fi

echo "[+] Build completado con éxito."
