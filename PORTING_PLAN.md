# Plan de Port — Immortal Dusk (PS Vita)

> Documento de Arquitectura, Análisis Estático Verificado de `libaist-jni.so` y `decompiled/apk_jadx/`, Mapeo de Símbolos JNI y Hoja de Ruta para el port de **Immortal Dusk** (Android) a **PS Vita**.

---

## 0. Contexto y Estrategia General

* **Juego:** Immortal Dusk (이모탈 더스크)
* **Desarrollador / Publisher:** Gamevil / Knetp (Korean Network Entertainment Platform)
* **Paquete Java:** `com.gamevil.ast.global` (clases de soporte: `com.gamevil.nexus2`, `gxg.android`)
* **Activity Principal:** `com.gamevil.ast.global.SkeletonLauncher`
* **Vista de Render:** `com.gamevil.ast.global.MainView`
* **Binario Nativo:** `libaist-jni.so` (524 KB, arquitectura ARM 32-bit `armeabi`)
* **TITLEID asignado:** `IMMDUSK01`
* **Ruta de Datos en PS Vita:** `ux0:data/immortaldusk/`
* **Ruta de la Librería:** `ux0:data/immortaldusk/libaist-jni.so`
* **Ruta de Guardado:** `ux0:data/immortaldusk/saves/`

### Arquitectura del Motor (Verificada en Decompilación)
1. **Renderizado por Software 2D en C:** `libaist-jni.so` renderiza toda la escena del juego (sprites, mapas, tiles, HUD, efectos de partículas y fuentes bitmap) directamente en un búfer de memoria de píxeles RGB565 / 16-bit (`short m_sBuf[800 * 480]`).
2. **Ciclo de Frame:** En cada tick, `Java_com_gamevil_ast_global_SkeletonLauncher_runC` recibe el búfer de píxeles, invoca `setShortBuf()` para fijar el búfer de destino (`Back_Buffer`), y ejecuta `ONTimerCB()`, retornando el tiempo de delay (en ms).
3. **Pipeline en PS Vita:**
   * Crear una textura 2D en `vitaGL` (`GL_RGB`, `GL_UNSIGNED_SHORT_5_6_5`) de 800x480.
   * En cada frame: invocar `runC(m_sBuf)`, actualizar la textura con `glTexSubImage2D` (o volcado de memoria directo) y renderizar un quad escalado a pantalla completa (**960x544**) con filtrado bilineal por hardware a 60 FPS estables.
   * **Sin sobrecarga de emulación GLES:** `libaist-jni.so` no realiza llamadas a OpenGL ES ni usa shaders GLSL; todo el rasterizado se ejecuta en CPU de manera sumamente eficiente.

---

## 1. Análisis de Arquitectura y Gráficos

* **Arquitectura Binaria:** ARMv5TE / ARMv6 (`armeabi`), código Thumb/ARM. Compatible de forma nativa con el runtime de PS Vita (`armv7k` en modo softfp).
* **Resolución Interna del Motor:** `800 x 480` píxeles (`m_Width = 800`, `m_Height = 480`, `nBpp = 2`, `nBpl = 1600`).
* **Resolución de Pantalla PS Vita:** `960 x 544` píxeles.
* **Formato de Color:** 16-bit RGB565 (`short[]`).

---

## 2. Catálogo Exhaustivo de Símbolos JNI Verificados

### 2.1 Exports JNI (Entradas a `libaist-jni.so`)

| Símbolo Exportado | Firma JNI | Función / Comportamiento |
|---|---|---|
| `Java_com_gamevil_ast_global_SkeletonLauncher_initJni` | `([BII)V` | Inicializa el motor, almacena `andPath`, registra callbacks de Java y configura punteros de la JVM |
| `Java_com_gamevil_ast_global_SkeletonLauncher_startC` | `(II)V` | Arranca el motor Clet invocando `startClet(800, 480)` y cargando las tablas de datos |
| `Java_com_gamevil_ast_global_SkeletonLauncher_runC` | `([S)I` | Asigna el puntero de framebuffer con `setShortBuf()`, ejecuta `ONTimerCB()` y devuelve delay en ms |
| `Java_com_gamevil_ast_global_SkeletonLauncher_eventC` | `(III)V` | Procesa eventos de entrada (`handleCletEvent`): Press=1, Release=2, Move=3 |
| `Java_com_gamevil_ast_global_SkeletonLauncher_resumeC` | `()V` | Reanuda el estado del motor (`resum_chk()`) |
| `Java_com_gamevil_ast_global_SkeletonLauncher_resultlanguage` | `()I` | Consulta el idioma activo (`language_chk`): 0=Coreano, 1=Inglés |
| `Java_com_gamevil_ast_global_SkeletonLauncher_netRecv` | `([B)V` | Stub de recepción de paquetes |
| `Java_com_gamevil_ast_global_SkeletonLauncher_netState` | `(II)V` | Stub de estado de conexión |
| `Java_com_gamevil_ast_global_SkeletonLauncher__netTmpSet` | `(I)V` | Stub de configuración temporal |

### 2.2 Callbacks JNI Esperados por `initJni` (Hacia Java / FalsoJNI)

Verificados exactamente contra el pool de literales en `0xc8fc`–`0xc9c8`:

| Método Java | Firma JNI Exacta | Comportamiento Implementado en Vita |
|---|---|---|
| `getRes` | `([B)[B` | **Crítico:** Lee archivos desde `ux0:data/immortaldusk/assets/<path>` y retorna `jbyteArray` |
| `sndPlay` | `(II)V` | Reproduce audio: `type=0` SFX (`res/raw/snd<ID>.wav`), `type=1` BGM (`res/raw/snd<ID>.mp3`) |
| `sndStop` | `(I)V` | Detiene audio (`type=0` para SFX, `type=1` para BGM) |
| `vibStart` | `(I)V` | Activa vibración (`sceCtrlSetActuator`) |
| `selected_lang` | `(I)V` | Notifica idioma seleccionado al host |
| `select_lang` | `()I` | Retorna idioma por defecto (`1` = Inglés) |
| `policy_chk` | `(I)V` | Stub (no-op) |
| `netClose` | `()V` | Stub (no-op) |
| `review_popup` | `()V` | Stub (no-op) |
| `netSend` | `([BI)V` | Stub (no-op) |
| `netBuy` | `([BIII)V` | Stub de compras In-App (desbloqueo offline inmediato) |
| `getNumber` | `()[B` | Retorna `byte[]` con número dummy (`"01012345678"`) |
| `getModel` | `()[B` | Retorna `byte[]` con modelo dummy (`"PlayStation Vita"`) |
| `ipnetSend` | `([BI)V` | Stub (no-op) |
| `hide_new` | `()V` | Stub de banners (no-op) |
| `display_new` | `()V` | Stub de banners (no-op) |
| `chkImemory` | `()I` | Retorna memoria disponible (`0x7FFFFFFF`) |

---

## 3. Subsistemas Clave y Rutas de Datos

### 3.1 Carga de Assets (`getRes` & `MC_knlGetResourceID`)
* Los assets se organizan bajo `ux0:data/immortaldusk/assets/`:
  * `ani_data/`: Animaciones de sprites (`NPC_*.dat`, `game_ui.dat`, `game_eff_*.dat`).
  * `data/`: Tablas de misiones, mapas, items y skills (`m_quest_table.dat`, `mon_data.dat`, `world_map_*.dat`, `math.dat`).
  * `font/`: Fuentes del motor (`inboard.dat`).
  * `img/`: Gráficos empaquetados (`game_ui.dat`, `item_*.dat`, `game_effect.dat`).
  * `txt/`: Texto en Coreano.
  * `txte/`: Texto en Inglés (cuando `language_chk == 1`, el motor traduce automáticamente `/txt` a `/txte`).
* `MC_knlGetResourceID` remueve la barra inicial (`/data/...` → `data/...`). `getRes` concatena `ux0:data/immortaldusk/assets/` y lee el archivo binario completo.

### 3.2 Sistema de Audio
* **Efectos de Sonido (SFX):** `res/raw/snd0.wav` hasta `snd61.wav`. `sndPlay(id, 0)` reproduce el efecto.
* **Música de Fondo (BGM):** `res/raw/snd100.mp3` hasta `snd122.mp3`. `sndPlay(100 + id, 1)` reproduce en bucle continuo.

### 3.3 Sistema de Guardado (`getAndPath`)
* El motor invoca `getAndPath("AIST_S")` y `getAndPath("AIST_OPTION")`, concatenando `andPath` con el nombre del archivo.
* Al pasar `ux0:data/immortaldusk/saves/` en `initJni`, el motor escribe y lee directamente sus archivos de partida con `fopen`/`fwrite`/`fread` sin requerir redirecciones complejas.

---

## 4. Hoja de Ruta de Implementación

- [x] **Fase 1:** Setup del entorno, `.gitignore` anti-DMCA, `build.sh` con staging y adaptación de herramientas.
- [x] **Fase 2:** Decompilación de APK y `libaist-jni.so`, verificación estática de firmas y mapeo completo de literales.
- [x] **Fase 3:** Implementación del loader en C (`source/main.c`, `source/dynlib.c`, `source/java.c`).
- [x] **Fase 4:** Pipeline gráfico `vitaGL` y loop de frame con `runC(sBuf)` (RGB565 800x480 escalado a 960x544).
- [x] **Fase 5:** Subsistema de Audio Nativo (`source/audio.cpp` con `sceAudioOut`, `sceIoRead`, decodificación WAV PCM y MP3 en memoria).
- [x] **Fase 6:** Mapeo de controles (Touch Frontal + Stick Analógico / D-Pad + Botones de acción hacia `eventC`).
- [ ] **Fase 7:** Pruebas y validación en consola PS Vita física vía `manage_vita.py`.
