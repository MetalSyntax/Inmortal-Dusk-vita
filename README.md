# Immortal Dusk — PS Vita Port

<p align="center">
  <img src="extras/livearea/pic0.png" alt="Immortal Dusk PS Vita" width="700"/>
</p>

<p align="center">
  <b>Port nativo del clásico RPG de acción de Gamevil para PlayStation Vita.</b><br/>
  Basado en la versión de Android mediante carga dinámica de librerías (.so) y FalsoJNI.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-PS%20Vita-blue.svg" alt="Platform"/>
  <img src="https://img.shields.io/badge/Status-Playable-brightgreen.svg" alt="Status"/>
  <img src="https://img.shields.io/badge/Target%20FPS-60%20FPS-orange.svg" alt="FPS"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License"/>
</p>

---

## 🌟 Características del Port

- **Rendimiento Nativo a 60 FPS:** Renderizado por software RGB565 escalado por hardware a pantalla completa (**960x544**) con filtrado bilineal en `vitaGL`.
- **Controles Físicos Completos:** Mapeo de botones físicos (Ataque, Habilidades 1-4, Pociones HP/MP, Menú, D-Pad y Stick Analógico) junto a soporte completo para la pantalla táctil frontal.
- **Audio Nativo de Baja Latencia:** Reproducción de efectos de sonido WAV (PCM) y música de fondo MP3 en bucle mediante `sceAudio`.
- **Tienda y Tienda de Puntos K Funcional Offline:** Emulación local del protocolo de red de Gamevil/KNETP, eliminando cuelgues infinitos y permitiendo comprar pociones, pergaminos de teletransporte, piedras de resurrección, expansiones de inventario/almacén y orbes sin conexión a servidores.
- **Recursos de LiveArea en Alta Definición:** Ícono, fondo, splash gate y arte principal remasterizados y adaptados al formato de color indexado de 8 bits exigido por el sistema PlayStation Vita.

---

## 🎮 Controles

| Botón PS Vita | Acción en el Juego |
| :--- | :--- |
| **Stick Izquierdo / D-Pad** | Movimiento del personaje en 8 direcciones |
| **Cruz ($\times$)** | Ataque Principal / Aceptar |
| **Cuadrado ($\square$)** | Habilidad 1 |
| **Triángulo ($\triangle$)** | Habilidad 2 |
| **Círculo ($\bigcirc$)** | Habilidad 3 / Cancelar / Salir |
| **R1** | Habilidad 4 |
| **L1** | Poción de Vida (HP) |
| **SELECT** | Poción de Maná (MP) |
| **START** | Menú Principal / Pausa |
| **Pantalla Táctil** | Interacción táctil directa en menús, inventario y HUD |

---

## 📦 Instrucciones de Instalación

> **Aviso Legal (DMCA):** Este repositorio no contiene archivos de juego protegidos por derechos de autor. Para jugar en tu consola debes extraer los recursos desde un APK legítimo de *Immortal Dusk v1.0.2*.

### Requisitos Previos en la Consola
1. Consola PS Vita / PS TV con Custom Firmware (HENkaku / Enso).
2. Plugin `kubridge.skprx` instalado en tu `config.txt` de taiHEN.
3. Plugin `fd_fix.skprx` (o rePatch).
4. `libshacccg.suprx` extraído en `ur0:data/` (mediante *ShaRKBR3ED* / *CrystalPSM*).

### Extracción de Archivos del Juego
1. Descarga e instala `immortal_dusk.vpk` en tu consola mediante **VitaShell**.
2. Obtén el archivo `Immortal Dusk_1.0.2.apk` y renómbralo a `.zip` (o extráelo con 7-Zip/WinRAR).
3. Extrae la librería nativa:
   - `lib/armeabi/libaist-jni.so` $\to$ `ux0:data/immortaldusk/libaist-jni.so`
4. Extrae los recursos del juego:
   - La carpeta `assets/` del APK $\to$ `ux0:data/immortaldusk/assets/`
   - La carpeta `res/raw/` del APK $\to$ `ux0:data/immortaldusk/res/raw/`
5. La estructura final en tu tarjeta de memoria debe quedar así:
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

---

## 🛠️ Compilación desde el Código Fuente

El proyecto utiliza **VitaSDK** y **CMake**.

```bash
# 1. Configurar variables de entorno
export VITASDK=/ruta/a/tu/vitasdk
export PATH=$VITASDK/bin:$PATH

# 2. Clonar el repositorio
git clone https://github.com/tu-usuario/Inmortal-Dusk-vita.git
cd Inmortal-Dusk-vita

# 3. Compilar usando el script automatizado
chmod +x build.sh
./build.sh
```

El archivo `.vpk` y el `.elf` se generarán dentro del directorio `build/`.

---

## 📜 Créditos y Agradecimientos

- **Gamevil / KNETP** por desarrollar el juego original.
- **TheFloW** por la arquitectura base de carga dinámica de librerías (.so loader).
- **Rinnegatamante** por `vitaGL`, soporte técnico y herramientas del ecosistema Homebrew de Vita.
- **Volodymyr Atamanenko** por las librerías `so_util` y plantillas de porteo.

---

## 📄 Licencia

El código de este port se distribuye bajo la licencia **MIT**. Consulta [`LICENSE`](LICENSE) para más detalles.
Los activos y el código binario original del juego pertenecen a sus respectivos autores.
