# Wild Guns — Enhanced Native Port (Windows · Linux · Android)

Port nativo mejorado de **Wild Guns** (SNES, 1994, Natsume) escrito en C + SDL2 sobre `snes9x_libretro`.  
**No incluye ROMs ni assets protegidos**: al abrirlo por primera vez te pide enlazar tu propia ROM dumpeada legalmente (`Wild Guns (USA).sfc` / `.smc`, 1 MB, CRC32 `0x085D8D14`), valida el volcado, extrae al vuelo el arte del título y la tipografía Western original de la ROM, y arranca con menú nativo, apuntado con ratón/pantalla táctil, selector de escenario desbloqueado, arsenal en vivo y soporte cooperativo 2P.

---

## Características principales

- **Apuntado con ratón (PC) y toque directo (Android):**
  - Mueve la mira de `P1` directamente sobre el campo de batalla escribiendo en tiempo real las coordenadas de cámara + retícula en WRAM (`0x0090` / `0x1609` / `0x1709`).
  - **Clic izquierdo:** Disparar / Lazo (`Y`) · **Clic derecho:** Saltar / Rodar (`B`) · **Clic central:** Bomba (`X`).
  - En Android puedes tocar y deslizar directamente sobre el escenario para apuntar, o usar el gamepad virtual multi-touch / mando Bluetooth-USB.
  - Se puede activar o desactivar al vuelo en `OPCIONES -> MIRA RATON/TACTIL`.
- **Inicio Rápido + Modo Clásico:**
  - `INICIO RAPIDO 1P` (Clint o Annie) e `INICIO RAPIDO 2P` (Cooperativo local) saltan directo a Carson City.
  - `MODO CLASICO` hace un arranque limpio desde el logo de Natsume, intro y pantalla de título original, aplicando automáticamente tu arsenal y trucos en cuanto comienza la partida.
  - `ELEGIR ESCENARIO` ejecuta el código oficial de Natsume (`SELECT + AAAA BBBB ABABABAB`) para abrir la pantalla original de **Stage Select** con las 6 zonas.
  - `MODO VERSUS` salta directo a `V.S START`.
- **Arsenal y Trucos en vivo (aplican a 1P y 2P en cooperativo):**
  - Selector de arma en tiempo real: `ORIGINAL`, `S.GUN` (Shotgun), `G.GUN` (Grenade Launcher), `M.GUN` (Machine Gun), `P.GUN` (Peashooter).
  - `MUNICION INFINITA` (99 disparos fijos)
  - `BOMBAS INFINITAS` (5 bombas)
  - `VIDAS INFINITAS` (9 vidas)
  - `MODO DIOS` (invulnerabilidad continua)
  - `RELOJ CONGELADO` (detiene la cuenta regresiva del escenario)
  - `DISPARO AUTO` (auto-fire estilo arcade)
- **Cooperativo local 2P:**
  - 2 mandos USB/Bluetooth simultáneos, o 1P en teclado/ratón + 2P en teclado secundario (`IJKL` mover + `N` disparar, `M` saltar, `H` bomba, `U` lazo/acción, `O` Start).
- **UI auténtica extraída de tu ROM:**
  - Fondo de madera/oro del título `WILD GUNS`, tipografía Western 8×16 original y cursor dorado extraídos de VRAM en tiempo de ejecución (cero gráficos incluidos en el repositorio).
- **Funciones modernas:**
  - 3 ranuras de **Save States** (`F5` guardar / `F7` cargar / `F6` cambiar ranura).
  - **Sin ralentizaciones** (overclock de CPU SNES integrado).
  - Remapeo de teclado, modo pantalla completa (`Alt+Enter` / `F11`), `CRT SCANLINES`, modo **Español / English**.
  - Controles táctiles inteligentes en Android (`AUTO`: se ocultan automáticamente si conectas un mando físico, con botón `MENU` siempre accesible).

---

## Cómo jugar (Usuario final)

### Windows (`.exe`)
1. Descarga `WildGuns-Windows-x64.zip` desde [Releases](../../releases) y descomprímelo.
2. Ejecuta `WildGuns.exe`.
3. En el primer arranque, selecciona tu copia legal `Wild Guns (USA).sfc` (o `.smc`) en el diálogo de Windows (también puedes arrastrar el archivo a la ventana o dejarlo junto al `.exe`).

### Linux (`.AppImage`)
1. Descarga `WildGuns-x86_64.AppImage` desde [Releases](../../releases).
2. Dale permisos de ejecución y ábrelo:
   ```bash
   chmod +x WildGuns-x86_64.AppImage
   ./WildGuns-x86_64.AppImage
   ```
3. Selecciona tu ROM de `Wild Guns (USA)` mediante el diálogo (`zenity`/`kdialog`) o arrastrándola a la ventana.

### Android (`.apk`)
1. Descarga e instala `WildGuns-Android.apk` desde [Releases](../../releases).
2. Al abrir la app por primera vez, elige tu archivo `Wild Guns (USA).sfc` / `.smc` con el selector de Android (SAF).
3. Juega con toque directo en pantalla + botones táctiles o conecta un mando Bluetooth/USB (Xbox, PlayStation, 8BitDo, Backbone, etc.).

---

## Controles por defecto

| Acción | Jugador 1 (Teclado) | Jugador 1 (Ratón) | Jugador 2 (Teclado) | Mando |
|---|---|---|---|---|
| Mover / Mira | Flechas o `WASD` | Mover cursor | `I J K L` | Cruceta / Stick izq. |
| Disparar (`Y`) | `Z` | Clic izquierdo | `N` | `X` / Cuadrado |
| Saltar / Rodar (`B`) | `X` | Clic derecho | `M` | `A` / Cruz |
| Bomba (`X`) | `C` | Clic central | `H` | `Y` / Triángulo |
| Acción (`A`) | `A` | — | `U` | `B` / Círculo |
| Pausa SNES (`Start`) | `Enter` | — | `O` | `Start` |
| Menú del Port | `ESC` / `F1` | — | — | `Guide` / `Back+Start` / `L1+R1+Start` |
| Save / Load / Slot | `F5` / `F7` / `F6` | — | — | Desde el menú de pausa |
| Pantalla completa | `F11` / `Alt+Enter` | — | — | Desde `OPCIONES` |

---

## Compilación desde código fuente

### Windows (MSVC x64)
Coloca `SDL2.dll` y `snes9x_libretro.dll` (x64) en la raíz y ejecuta:
```bat
build_win.bat
```

### Linux (GCC + SDL2)
```bash
sudo apt install libsdl2-dev
make
# Opcional: empaquetar AppImage (coloca snes9x_libretro.so en la raíz)
make appimage
```

### Android (Gradle + NDK) & CI automático
El workflow [`.github/workflows/build.yml`](.github/workflows/build.yml) compila automáticamente los 3 objetivos (**Windows x64 ZIP**, **Linux AppImage** y **Android APK arm64-v8a + armeabi-v7a**) en cada `push` y publica una Release de GitHub automáticamente al subir un tag `v*` (ej. `git tag v1.0.0 && git push --tags`).

---

## Aviso legal

Este proyecto contiene exclusivamente código fuente original del frontend y scripts de empaquetado. **No contiene ni distribuye ROMs, BIOS, gráficos ni código propietario de Nintendo o Natsume.** Necesitas proporcionar tu propia copia legal de *Wild Guns* (SNES) para ejecutarlo.
