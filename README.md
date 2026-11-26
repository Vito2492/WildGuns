# Wild Guns — Enhanced Native Port (Windows · Linux · Android)

Port nativo mejorado de **Wild Guns** (SNES, 1994, Natsume) escrito en C + SDL2 sobre `snes9x_libretro`.  
**No incluye ROMs ni assets protegidos**: al abrirlo por primera vez te pide enlazar tu propia ROM dumpeada legalmente (`Wild Guns (USA).sfc` / `.smc`, 1 MB, CRC32 `0x085D8D14`), valida el volcado, extrae al vuelo el arte del título y la tipografía Western original de la ROM, y arranca con menú nativo, apuntado Twin-Stick / Ratón / Pantalla Táctil, Bullet Time, Rebobinado, Boss Rush, Vulcan Infinita, Hardcore y Cooperativo 2P.

---

## Características principales

### Mecánicas modernas exclusivas del port
- **Modo Twin-Stick (moverte mientras apuntas y disparas):**
  - En la SNES original debes quedarte quieto al disparar. Con `MODO TWIN-STICK` activado, puedes desplazarte con **WASD / Stick izquierdo / D-Pad táctil** mientras apuntas de forma independiente con el **Ratón / Stick derecho / Toque directo** (inyección 16-bit directa en WRAM de personaje y retícula).
- **Apuntado con ratón (PC) y toque directo (Android):**
  - **Clic izquierdo:** Disparar (`Y`) · **Clic derecho:** Saltar/Rodar (`B`) · **Clic central:** Bomba (`X`) · **Botón lateral 1 (`X1`) / `V` / `R1`:** Macro de **Lazo instantáneo** · **Botón lateral 2 (`X2`) / `Espacio`:** Macro de **Esquive rápido (Roll)**.
  - En Android puedes tocar y deslizar directamente sobre el escenario para apuntar y disparar, o usar el mando táctil multi-touch (con botones dedicados `LASO`, `ROLL`, `SLOW` y `REW`) o mando físico Bluetooth/USB.
- **Bullet Time (Cámara lenta 50% tipo *Max Payne*):**
  - Mantén **`Shift`** (teclado), **`L2`** (mando) o **`SLOW`** (Android) para bajar la velocidad al 50% con estiramiento de audio en tiempo real y marco cian cinematográfico.
- **Rebobinado rápido (`Rewind` ~4 segundos):**
  - Mantén **`Backspace`** (teclado), **`L1 + Select`** (mando) o **`REW`** (Android) para retroceder los últimos 4 segundos en memoria si te sorprenden en un jefe.
- **Mira personalizada de alta visibilidad (`MIRA VISUAL`):**
  - Elige entre `ORIGINAL`, `LASER` (mira láser roja brillante con punto blanco central) y `ARCADE` (cruceta táctica estilo *Time Crisis* en cian y oro) dibujada en vivo sobre la posición exacta de 1P y 2P.

### Modos de juego y trucos en vivo
- **Modos desde el menú de inicio:**
  - `INICIO RAPIDO 1P` (Clint o Annie directo a Carson City) e `INICIO RAPIDO 2P` (Cooperativo local).
  - `MODO CLASICO`: arranque limpio desde el logo de Natsume, intro y pantalla de título original, activando tus trucos en cuanto empieza la acción.
  - `MODO BOSS RUSH`: salta automáticamente el reloj de cada fase para enfrentarte directo a los jefes (también puedes pulsar **`F8`** o `SALTAR AL JEFE` en pausa en cualquier momento).
  - `ELEGIR ESCENARIO`: ejecuta el código oficial de Natsume (`SELECT + AAAA BBBB ABABABAB`) para abrir el **Stage Select** de las 6 zonas.
  - `MODO VERSUS`: salta directo a `V.S START`.
  - `COLOR TRAJE`: elige cualquiera de los **4 trajes oficiales** de Clint y Annie (`CLASICO`, `VERDE`, `MORADO`, `ROJO`).
- **Arsenal y Trucos en vivo (aplican a 1P y 2P en cooperativo):**
  - `ARMA ACTIVA`: `ORIGINAL`, `S.GUN` (Shotgun), `G.GUN` (Grenade), `M.GUN` (Machine Gun), `P.GUN` (Peashooter), `V.GUN` (Vulcan Gatling).
  - `VULCAN INFINITA` (Furia Vulcan permanente: `V.GUN` + 99 balas + invulnerabilidad).
  - `MUNICION INFINITA`, `BOMBAS INFINITAS` (5), `VIDAS INFINITAS` (9), `MODO DIOS`.
  - `FICHAS ARCADE` (Continues infinitos en sitio: pierdes vidas con normalidad y al llegar a 0 se inserta ficha automática con +2 vidas y 3 bombas sin perder la fase).
  - `MODO HARDCORE` (Duelo del Oeste 1-Hit Kill: 0 vidas extra, 0 bombas y sin continues — un solo impacto termina la partida).
  - `RELOJ CONGELADO` (congela el temporizador BCD en `80`) y `DISPARO AUTO`.
- **Extras de calidad de vida:**
  - 3 ranuras de **Save States** (`F5` guardar / `F7` cargar / `F6` cambiar slot), **Sin Slowdown** (overclock CPU SNES), remapeo completo de teclado y mando, pantalla completa (`F11` / `Alt+Enter`) e interfaz **Español / English** extraída 100% de tu ROM en tiempo de ejecución.

---

## Cómo jugar (Usuario final)

### Windows
1. Descarga [WildGuns-Windows.zip](https://github.com/Vito2492/WildGuns/releases/latest/download/WildGuns-Windows.zip) y descomprímelo.
2. Abre `WildGuns.exe`. La primera vez elige tu ROM `Wild Guns (USA).sfc` (`.sfc` o `.smc`), o déjala junto al `.exe`.

### Linux
1. Descarga [WildGuns-Linux.tar.gz](https://github.com/Vito2492/WildGuns/releases/latest/download/WildGuns-Linux.tar.gz).
2. Descomprime y ejecuta (trae su propia copia de SDL2 en `lib/`):
   ```bash
   tar xzf WildGuns-Linux.tar.gz && cd WildGuns-Linux
   ./wildguns "ruta/a/Wild Guns (USA).sfc"
   ```

### Android
1. Descarga e instala [WildGuns-Android.apk](https://github.com/Vito2492/WildGuns/releases/latest/download/WildGuns-Android.apk) desde el celular (hay que permitir instalar apps de origen desconocido).
2. La primera vez te pide elegir tu ROM (`.sfc`, `.smc` o `.zip`); se valida y queda guardada dentro de la app.
3. Las versiones nuevas se instalan encima y conservan las partidas. Juega con la pantalla táctil o con un mando Bluetooth/USB.

> Ningún paquete incluye el juego: necesitas tu propia copia (versión USA).

---

## Controles por defecto

| Acción | Jugador 1 (Teclado) | Jugador 1 (Ratón) | Jugador 2 (Teclado) | Mando |
|---|---|---|---|---|
| Mover personaje | Flechas o `WASD` | — | `I J K L` | Stick izq. / Cruceta |
| Apuntar mira (Twin-Stick) | Flechas | Mover cursor | `I J K L` | Stick der. |
| Disparar (`Y`) | `Z` | Clic izquierdo | `N` | `X` / Cuadrado / `R2` |
| Saltar / Rodar (`B`) | `X` | Clic derecho | `M` | `A` / Cruz |
| Bomba (`X`) | `C` | Clic central | `H` | `Y` / Triángulo |
| Macro Lazo instantáneo | `V` | Botón lateral 1 (`X1`) | `U` | `R1` |
| Macro Esquive (Roll) | `Espacio` | Botón lateral 2 (`X2`) | — | `B` / Círculo |
| Bullet Time (50% slow-mo) | `Shift` (mantener) | — | — | `L2` (mantener) |
| Rebobinado (4s Rewind) | `Backspace` (mantener) | — | — | `L1 + Select` (mantener) |
| Saltar al Jefe | `F8` | — | — | Menú Pausa -> Saltar al Jefe |
| Menú del Port | `ESC` / `F1` | — | — | `Guide` / `L1+R1+Start` |
| Save / Load / Slot | `F5` / `F7` / `F6` | — | — | Desde el menú de pausa |
| Pantalla completa | `F11` | — | — | Desde `OPCIONES` |

---

## Compilación desde código fuente

- **Linux:** `sudo apt install libsdl2-dev && make` (pon `snes9x_libretro.so` junto al programa).
- **Windows (MinGW, igual que el CI):**
  ```bash
  x86_64-w64-mingw32-gcc -O2 -Isrc -I$SDL/include/SDL2 -I$SDL/include src/wildguns.c -o WildGuns.exe \
    -L$SDL/lib -lmingw32 -lSDL2main -lSDL2 -lcomdlg32 -luser32 -lshell32 -lm -mwindows
  ```
  (o `build_win.bat` con MSVC 2022). Junto al `.exe` van `SDL2.dll` y `snes9x_libretro.dll`.
- **Android:** `tools/android-prepare.sh SDL2-2.30.9 snes9x/libretro/libs` y luego `cd android && gradle assembleRelease` (el workflow muestra los pasos completos).
- **GitHub Actions:** [`.github/workflows/build.yml`](.github/workflows/build.yml) compila Windows, Linux y Android (arm64-v8a, armeabi-v7a, x86_64) en cada push a `main`, con SDL2 2.30.9 y snes9x compilado desde su código fuente, y publica o actualiza el release `v` + [`VERSION`](VERSION).
- **Firma del APK:** con los secretos `ANDROID_KEYSTORE_B64`, `ANDROID_KEYSTORE_PASSWORD`, `ANDROID_KEY_ALIAS` y `ANDROID_KEY_PASSWORD` el APK sale firmado siempre con la misma clave, así cada versión se instala encima de la anterior. Sin ellos se compila igual, pero el APK no se publica.


---

## Aviso legal

Este proyecto contiene exclusivamente código fuente original del frontend y scripts de empaquetado. **No contiene ni distribuye ROMs, BIOS, gráficos ni código propietario de Nintendo o Natsume.** Necesitas proporcionar tu propia copia legal de *Wild Guns* (SNES) para ejecutarlo.
