# Plan de Port — Immortal Dusk (PS Vita)

> Documento de Arquitectura, Análisis Estático Verificado de `libaist-jni.so` y `decompiled/apk_jadx/`, Mapeo de Símbolos JNI, Correcciones del Motor y Hoja de Ruta para el port de **Immortal Dusk** (Android) a **PlayStation Vita**.

---

## 0. Contexto y Estrategia General

* **Juego:** Immortal Dusk (이모탈 더스크)
* **Desarrollador / Publisher:** Gamevil / Knetp (Korean Network Entertainment Platform)
* **Paquete Java:** `com.gamevil.ast.global` (clases de soporte: `com.gamevil.nexus2`, `gxg.android`, `com.feelingk.iap`)
* **Activity Principal:** `com.gamevil.ast.global.SkeletonLauncher`
* **Vista de Render:** `com.gamevil.ast.global.MainView`
* **Binario Nativo:** `libaist-jni.so` (524 KB, arquitectura ARM 32-bit `armeabi`)
* **TITLEID Asignado:** `IMMDUSK01`
* **Ruta de Datos en PS Vita:** `ux0:data/immortaldusk/`
* **Ruta de la Librería:** `ux0:data/immortaldusk/libaist-jni.so`
* **Ruta de Guardado:** `ux0:data/immortaldusk/saves/`
* **Ruta de Logs:** `ux0:data/immortaldusk/logs/`

---

## 1. Arquitectura del Motor y Pipeline Gráfico

### 1.1 Renderizado por Software 2D en C
1. `libaist-jni.so` renderiza toda la escena del juego (sprites, mapas, capas de tiles, HUD, efectos de partículas y fuentes bitmap) directamente en un búfer de píxeles RGB565 / 16-bit (`short m_sBuf[800 * 480]`).
2. En cada tick, `Java_com_gamevil_ast_global_SkeletonLauncher_runC` recibe el array de framebuffer, invoca `setShortBuf()` para fijar el destino (`Back_Buffer`), y ejecuta `ONTimerCB()`, retornando el tiempo de sleep/delay (en ms).

### 1.2 Pipeline en PS Vita (vitaGL)
* Se crea una textura 2D en `vitaGL` (`GL_RGB`, `GL_UNSIGNED_SHORT_5_6_5`) de 800x480 con filtrado `GL_LINEAR` y `GL_CLAMP_TO_EDGE`.
* En cada frame: se ejecuta `runC(sBuf)`, se sube la memoria con `glTexSubImage2D` y se dibuja un quad a pantalla completa (**960x544**) con filtrado bilineal a 60 FPS estables.
* **Cero sobrecarga de emulación GLES:** `libaist-jni.so` no realiza llamadas a OpenGL ES ni usa shaders GLSL; todo el rasterizado se ejecuta en CPU de forma extremadamente eficiente en el procesador ARM Cortex-A9 de la Vita.

---

## 2. Catálogo Exhaustivo de Símbolos JNI

### 2.1 Exports JNI (Entradas a `libaist-jni.so`)

| Símbolo Exportado | Firma JNI | Función / Comportamiento en Vita |
|---|---|---|
| `Java_com_gamevil_ast_global_SkeletonLauncher_initJni` | `([BII)V` | Inicializa el motor, almacena `andPath`, registra callbacks de Java y configura punteros de la JVM |
| `Java_com_gamevil_ast_global_SkeletonLauncher_startC` | `(II)V` | Arranca el motor Clet invocando `startClet(800, 480)` y cargando las tablas de datos |
| `Java_com_gamevil_ast_global_SkeletonLauncher_runC` | `([S)I` | Asigna el puntero de framebuffer con `setShortBuf()`, ejecuta `ONTimerCB()` y devuelve delay en ms |
| `Java_com_gamevil_ast_global_SkeletonLauncher_eventC` | `(III)V` | Procesa eventos de entrada (`handleCletEvent`): Press=1, Release=2, Move=3 |
| `Java_com_gamevil_ast_global_SkeletonLauncher_resumeC` | `()V` | Reanuda el estado del motor (`resum_chk()`) |
| `Java_com_gamevil_ast_global_SkeletonLauncher_resultlanguage` | `()I` | Consulta el idioma activo (`language_chk`): 0=Coreano, 1=Inglés |
| `Java_com_gamevil_ast_global_SkeletonLauncher_netState` | `(II)V` | **Resuelto y conectado:** Notifica cambios de estado de red (`NetState(state, err)`) |
| `Java_com_gamevil_ast_global_SkeletonLauncher_netRecv` | `([B)V` | **Resuelto y conectado:** Inyecta paquetes de red emulados (`Net_Data(buf)`) |
| `Java_com_gamevil_ast_global_SkeletonLauncher__netTmpSet` | `(I)V` | Configuración temporal de socket |

### 2.2 Callbacks JNI Esperados por `initJni` (Hacia FalsoJNI)

| Método Java | Firma JNI Exacta | Comportamiento Implementado en Vita |
|---|---|---|
| `getRes` | `([B)[B` | Lee archivos de recursos desde `ux0:data/immortaldusk/assets/<path>` y retorna `jbyteArray` |
| `sndPlay` | `(II)V` | Reproduce audio: `type=0` SFX (`res/raw/snd<ID>.wav`), `type=1` BGM (`res/raw/snd<ID>.mp3`) |
| `sndStop` | `(I)V` | Detiene audio (`type=0` para SFX, `type=1` para BGM) |
| `vibStart` | `(I)V` | Activa vibración háptica en la consola (`sceCtrlSetActuator`) |
| `selected_lang` | `(I)V` | Notifica idioma seleccionado al host |
| `select_lang` | `()I` | Retorna idioma por defecto (`1` = Inglés) |
| `netConn` | `(I)V` | **Emulado:** Responde `netState(1, 0)` confirmando conexión para evitar bloqueo del motor |
| `netSend` | `([BI)V` | **Emulado:** Despacha respuestas de paquetes locales (`0x36b5` para compras, `5, 0` para tienda HUD) |
| `netBuy` | `([BIII)V` | **Emulado:** Concede 999,999 Puntos K inmediatamente y confirma la compra (`0x36b3`) |
| `netClose` | `()V` | Cierre limpio de sesión de red |
| `ipnetSend` | `([BI)V` | Desconexión segura (`netState(0, 100)`) |
| `policy_chk` | `(I)V` | No-op seguro |
| `review_popup` | `()V` | No-op seguro |
| `getNumber` | `()[B` | Retorna `byte[]` con identificador dummy (`"01012345678"`) |
| `getModel` | `()[B` | Retorna `byte[]` con modelo (`"PlayStation Vita"`) |
| `hide_new` | `()V` | No-op seguro (banners desactivados) |
| `display_new` | `()V` | No-op seguro (banners desactivados) |
| `chkImemory` | `()I` | Retorna memoria disponible (`0x7FFFFFFF`) |

---

## 3. Diagnóstico y Corrección de la Tienda (Shop Infinite Hang Fix)

### 3.1 Causa Raíz Diagnosticada
* Al pulsar el botón de Shop (en el HUD o menú), el motor nativo fija `connect_chk = 1` y llama a `netConn`.
* Si `METHOD_NET_CONN` es un stub vacío (`jni_noop_void`), el motor se queda esperando perpetuamente que la capa Java llame a `netState`.
* Mientras `connect_chk != 0`, las funciones `touch_process` y `play_press_key` descartan **incondicionalmente** todas las pulsaciones de botones físicos y toques táctiles (`if (connect_chk != 0) return;`), congelando la entrada en un bucle infinito.

### 3.2 Solución Implementada
1. **Resolución de símbolos exportados:** Se resolvieron `Java_com_gamevil_ast_global_SkeletonLauncher_netState` y `Java_com_gamevil_ast_global_SkeletonLauncher_netRecv` en `source/main.c`.
2. **Emulación del protocolo de paquetes en `source/java.c`:**
   - **`jni_netConn`:** Notifica `bridge_netState(1, 0)` confirmando la apertura del socket.
   - **`jni_netSend` (Comando `1000` - Apertura de tienda):** Responde con `bridge_netState(5, 0)` para limpiar `connect_chk = 0` manteniendo abierta la capa de la tienda (`gameflag = 5`, `now_layer = 0xd`) de forma totalmente interactiva.
   - **`jni_netSend` (Comando `0x36b4` - Compra de ítems):** Emula la respuesta `0x36b5` con saldo de puntos K, añade el ítem al inventario y muestra la confirmación de compra sin colgarse.
   - **`jni_netBuy` & Comando `0x36b2` (Recarga de K):** Emula la respuesta `0x36b3` concediendo **999,999 puntos K** al jugador.
   - **Comandos no soportados:** Notifican `bridge_netState(0, 100)` para desconectar limpiamente y liberar el control de entrada.

---

## 4. Mapeo de Controles (PS Vita)

| Botón PS Vita | Acción en el Juego | Coordenadas Táctiles Simuladas |
|---|---|---|
| **Cruz ($\times$)** | Ataque Principal / Confirmar | `(700, 380)` |
| **Cuadrado ($\square$)** | Habilidad 1 | `(620, 420)` |
| **Triángulo ($\triangle$)** | Habilidad 2 | `(620, 310)` |
| **Círculo ($\bigcirc$)** | Habilidad 3 / Cancelar / Salir | `(700, 260)` |
| **R1** | Habilidad 4 | `(770, 310)` |
| **L1** | Poción de Vida (HP) | `(420, 450)` |
| **SELECT** | Poción de Maná (MP) | `(480, 450)` |
| **START** | Menú Principal / Pausa | `(760, 35)` |
| **D-Pad / Stick Izquierdo** | Movimiento en 8 direcciones | Centro `(110, 370)` con radio 60px |
| **Pantalla Táctil Frontal** | Control táctil nativo completo | Mapeo directo `960x544` $\to$ `800x480` |

---

## 5. Recursos de LiveArea (Especificación Oficial)

Todos los recursos se ubicaron en `extras/livearea/` en formato **PNG 8-bits Indexado (256 colores)**:

* `icon0.png`: `128 x 128` — Ícono principal de la burbuja.
* `bg0.png`: `840 x 500` — Fondo remasterizado en alta definición del LiveArea.
* `pic0.png`: `960 x 544` — Pantalla completa HD sin distorsión.
* `startup.png`: `280 x 158` — Emblema oficial *"Immortal Dusk"* aislado sin personajes en el fondo.

---

## 6. Estado del Proyecto y Verificación

- [x] **Fase 1:** Configuración del entorno, toolchain de VitaSDK, `build.sh` y `.gitignore` anti-DMCA.
- [x] **Fase 2:** Decompilación estática de `libaist-jni.so` y `apk_jadx`, mapeo de firmas y literales JNI.
- [x] **Fase 3:** Implementación del loader en C (`source/main.c`, `source/dynlib.c`, `source/java.c`).
- [x] **Fase 4:** Pipeline gráfico `vitaGL` y loop de frame con `runC(sBuf)` (RGB565 800x480 $\to$ 960x544).
- [x] **Fase 5:** Subsistema de Audio Nativo (`source/audio.cpp` con `sceAudioOut`, decodificación WAV PCM y MP3 en memoria).
- [x] **Fase 6:** Mapeo de controles físicos y pantalla táctil frontal.
- [x] **Fase 7:** Corrección completa del bucle infinito de la tienda y emulación offline del sistema de compras.
- [x] **Fase 8:** Remasterización de recursos de LiveArea cumpliendo los estándares de PS Vita.
- [x] **Fase 9:** Compilación y empaquetado del VPK final (`build/immortal_dusk.vpk`).
