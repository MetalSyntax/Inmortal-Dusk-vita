#!/bin/bash
# Test suite y verificación previa al build para Immortal Dusk
set -e

TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$TESTS_DIR/../.." && pwd)"

echo "== [1/2] Verificando presencia de librería nativa libaist-jni.so =="
SO_FILE=$(find "$PROJECT_DIR" -name "libaist-jni.so" ! -name "._*" | head -n 1)
if [ -n "$SO_FILE" ] && [ -f "$SO_FILE" ]; then
    echo "  [+] Encontrado: $SO_FILE ($(du -h "$SO_FILE" | cut -f1))"
else
    echo "  [-] Advertencia: libaist-jni.so no encontrado en el proyecto."
fi

echo "== [2/2] Verificando estructura de código fuente =="
if [ -f "$PROJECT_DIR/CMakeLists.txt" ] && [ -f "$PROJECT_DIR/source/main.c" ]; then
    echo "  [+] CMakeLists.txt y source/main.c presentes."
else
    echo "  [-] Error en la estructura del proyecto."
    exit 1
fi

echo "✅ Verificaciones completadas exitosamente."
