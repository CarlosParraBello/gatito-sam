<div align="center">

# 🐱 Gatito Sam

**Una mascota virtual animada para el ESP32-S3 con pantalla táctil de 2.8"**

Un gatito que parpadea, mueve la cola y maúlla cuando lo tocas.

![Platform](https://img.shields.io/badge/platform-ESP32--S3-E7352C?logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino_IDE-2.x-00979D?logo=arduino&logoColor=white)
![Core](https://img.shields.io/badge/esp32_core-3.x-blue)
![Library](https://img.shields.io/badge/graphics-LovyanGFX-ff69b4)
![License](https://img.shields.io/badge/license-MIT-green)

<!-- Reemplaza con tu foto o GIF real -->
<img src="docs/demo.gif" alt="Demo del gatito" width="280">

</div>

---

## ✨ Características

| | |
|---|---|
| 🎨 **Animación fluida** | ~30 FPS con framebuffer completo en PSRAM, sin parpadeo |
| 👀 **Ojos vivos** | Miran de lado a lado y parpadean de forma natural |
| 🐈 **Cola ondulante** | Movimiento senoidal continuo |
| 🌈 **Nombre arcoíris** | "Sam" rebota y cambia de color en tonos pastel |
| ✨ **Cielo estrellado** | Estrellas que titilan de fondo |
| 👆 **Interacción táctil** | Al tocar: ojos felices `^ ^`, lluvia de corazones 💕 |
| 💡 **LED RGB** | Destello rosa que se desvanece suavemente |
| 🔊 **Miau sintetizado** | Generado por código en tiempo real, sin archivos de audio |
| 😻 **Ronroneo** | Mantén el dedo sobre él: ronronea y el LED "respira" |
| 😴 **Modo dormido** | Sin toques por 30 s: cierra los ojos, salen "zzz" y la pantalla se atenúa |
| 🕐 **Reloj Wi-Fi** | Hora sincronizada por NTP debajo del nombre |

Todo el gatito está dibujado con primitivas gráficas (círculos, elipses, triángulos): no usa imágenes ni bitmaps.

## 🛠️ Hardware

**Placa:** ESP32-S3 2.8" IPS con táctil capacitivo (vendida como *Hosyond*, modelo **ES3C28P**)

| Componente | Detalle |
|---|---|
| MCU | ESP32-S3 dual-core @ 240 MHz, 16 MB Flash, 8 MB PSRAM |
| Pantalla | IPS 2.8" 240×320, controlador **ILI9341V** (SPI) |
| Táctil | Capacitivo **FT6336G** (I2C, dir. `0x38`) |
| Audio | Codec **ES8311** + amplificador **FM8002E** |
| LED | WS2812 RGB direccionable |

Solo necesitas además un **altavoz pequeño** (4–8 Ω) conectado al conector `SPK`.

### 📌 Pinout

<details>
<summary><b>Ver tabla completa de pines</b></summary>

| Función | GPIO | Notas |
|---|:---:|---|
| LCD SCLK | 12 | SPI |
| LCD MOSI | 11 | SPI |
| LCD MISO | 13 | SPI |
| LCD DC | 46 | |
| LCD CS | 10 | |
| LCD RST | — | Compartido con EN |
| LCD Backlight | 45 | PWM |
| Touch SDA | 16 | I2C compartido con ES8311 |
| Touch SCL | 15 | I2C compartido con ES8311 |
| Touch INT | 17 | |
| Touch RST | 18 | |
| I2S MCLK | 4 | |
| I2S BCLK | 5 | |
| I2S LRCK (WS) | 7 | |
| I2S DOUT | 8 | Hacia el codec (altavoz) |
| I2S DIN | 6 | Desde el codec (micrófono) |
| Amp enable | 1 | ⚠️ **Activo en BAJO** |
| LED RGB | 42 | WS2812 |

</details>

## 🚀 Instalación

### 1. Preparar Arduino IDE

1. Instala el paquete **esp32 by Espressif** versión **3.x** desde *Boards Manager*.
2. Instala la librería **LovyanGFX** (by lovyan03) desde *Library Manager*.

### 2. Configurar la placa

En **Tools**:

| Opción | Valor |
|---|---|
| Board | **ESP32S3 Dev Module** |
| Flash Size | 16MB |
| PSRAM | OPI PSRAM |
| Port | El COM de tu placa |

> ⚠️ **No uses "ESP32 Family Device"** (la opción que aparece al autodetectar). Compila para el chip equivocado y falla.

### 3. Configurar Wi-Fi

Copia `gatito_sam/secrets.example.h` como `gatito_sam/secrets.h` y escribe tu red:

```cpp
#define WIFI_SSID "TU_RED_WIFI"
#define WIFI_PASS "TU_CONTRASEÑA"
```

> 🔒 `secrets.h` está en `.gitignore`, así que tu contraseña nunca se sube al repo.
> La zona horaria se cambia en `TZ_INFO` (por defecto `CST6`, Ciudad de México).

### 4. Subir

```bash
git clone https://github.com/CarlosParraBello/gatito-sam.git
```

Abre `gatito_sam/gatito_sam.ino`, conecta la placa y presiona **Upload**.
En el Serial Monitor (115200 baud) deberías ver `Audio OK`.

## 🎛️ Personalización

| Qué cambiar | Dónde |
|---|---|
| Nombre mostrado | `drawName()` → `"Sam"` |
| Volumen | `es8311Write(0x32, 0xBF)` → más alto = más fuerte |
| Tono del miau | `audioTaskFn()` → frecuencias `550 / 900 / 450` Hz |
| Duración del miau | `audioTaskFn()` → `0.65f` segundos |
| Colores del gatito | `setup()` → `C_FUR`, `C_EYE`, `C_PINK`… |
| Color del LED | `updateLed()` → proporciones R/G/B |
| Tiempo para dormirse | `SLEEP_MS` (ms) |
| Tiempo para ronronear | `PURR_HOLD_MS` (ms) |
| Zona horaria | `TZ_INFO` (formato POSIX) |

## 🩺 Solución de problemas

<details>
<summary><b>Compila con error / "Unable to find executable file .elf"</b></summary>

El `.elf` no se genera porque la compilación falló antes. Revisa que la placa sea **ESP32S3 Dev Module** y busca la primera línea `error:` en la consola.
</details>

<details>
<summary><b>"secrets.h: No such file or directory"</b></summary>

Falta crear `secrets.h`: copia `secrets.example.h` con ese nombre (ver Instalación, paso 3).
</details>

<details>
<summary><b>No aparece la hora</b></summary>

La hora solo se dibuja cuando el ESP32 se conecta y sincroniza por NTP. Revisa SSID y contraseña, y que la red sea de **2.4 GHz** (el ESP32 no ve redes de 5 GHz).
</details>

<details>
<summary><b>Colores invertidos o rojo/azul intercambiados</b></summary>

En la clase `LGFX`, cambia `cfg.invert` o `cfg.rgb_order` a `true`.
</details>

<details>
<summary><b>"Audio OK" pero no suena nada</b></summary>

El amplificador FM8002E se habilita con **GPIO 1 en LOW**. Verifica también que el altavoz esté en el conector `SPK`.
</details>

<details>
<summary><b>"ES8311: no responde en I2C"</b></summary>

Tu revisión de la placa puede no tener el codec o usar otros pines. Revisa el esquemático de tu modelo.
</details>

## 🧠 Cómo funciona

- **Render:** todo se dibuja en un `LGFX_Sprite` de 240×320 (16 bits, en PSRAM) y se envía a la pantalla por SPI con DMA en un solo `pushSprite()`.
- **Audio:** una tarea de FreeRTOS en el *core 0* espera una notificación del toque y sintetiza el miau muestra por muestra: un barrido de tono (sube en "mi", baja en "au"), armónicos que simulan la vocal, vibrato y una envolvente de amplitud. Se envía al ES8311 por I2S a 16 kHz.
- **Ronroneo:** la misma tarea de audio genera un zumbido grave (110 Hz + ruido filtrado) pulsado a ~26 Hz, como un ronroneo real, con un ciclo de respiración de 1.8 s. Suena en bloques de 16 ms mientras el dedo siga sobre la pantalla.
- **Reloj:** Wi-Fi y NTP arrancan sin bloquear; `time()` + `localtime_r()` consultan la hora sin esperar, así la animación nunca se congela.
- **I2C compartido:** el táctil y el codec viven en el mismo bus. El codec se configura con el driver I2C de LovyanGFX para evitar conflictos con `Wire`.

## 🗺️ Roadmap

- [x] Modo dormido (ojos cerrados y "zzz") tras inactividad
- [x] Reloj por Wi-Fi (NTP)
- [x] Ronroneo al mantener presionado
- [ ] Migración a LVGL

## 🙏 Créditos

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) por lovyan03
- Pinout de audio verificado con la documentación de [arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools/wiki/LCD-Boards-with-Audio)

## 📄 Licencia

[MIT](LICENSE) © 2026 Carlos Enrique Parra Bello

<div align="center">
<sub>Hecho con 🧡 y un ESP32-S3</sub>
</div>
