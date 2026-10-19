/*
 * gatito_sam.ino
 * Animación: gatito (dibujado con primitivas) + nombre "Sam"
 * Placa: Hosyond ESP32-S3 2.8" ILI9341V (SPI) + FT6336G (I2C)
 * Librería: LovyanGFX (Gestor de librerías -> "LovyanGFX" by lovyan03)
 *
 * Animaciones:
 *  - Cabeza que se balancea suavemente
 *  - Ojos que miran a los lados y parpadean
 *  - Cola ondulante
 *  - Estrellas titilando de fondo
 *  - "Sam" rebotando con color arcoíris
 *  - Toque en pantalla: el gatito sonríe (^ ^) y suelta corazones
 *
 * Se dibuja todo en un sprite (framebuffer) y se envía de golpe -> sin parpadeo.
 */

#include <LovyanGFX.hpp>
#include <ESP_I2S.h>   // incluido en el core ESP32 3.x

// ======================= PINOUT (verificar con tu placa) =======================
#define PIN_LCD_SCLK  12
#define PIN_LCD_MOSI  11
#define PIN_LCD_MISO  13
#define PIN_LCD_DC    46
#define PIN_LCD_CS    10
#define PIN_LCD_RST   -1   // reset compartido con EN
#define PIN_LCD_BL    45

#define PIN_TP_SDA    16
#define PIN_TP_SCL    15
#define PIN_TP_INT    17
#define PIN_TP_RST    18

#define PIN_RGB_LED   42   // LED RGB direccionable (WS2812)

// Audio: codec ES8311 (I2C compartido con el táctil) + amplificador
#define PIN_I2S_MCLK  4
#define PIN_I2S_BCLK  5
#define PIN_I2S_LRCK  7
#define PIN_I2S_DOUT  8
#define PIN_I2S_DIN   6
#define PIN_PA_EN     1    // habilita el amplificador FM8002E (ACTIVO EN BAJO)
#define ES8311_ADDR   0x18

// ======================= CONFIGURACIÓN LovyanGFX =======================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;
  lgfx::Touch_FT5x06  _touch;   // compatible con FT6336G
public:
  LGFX() {
    { // Bus SPI
      auto cfg = _bus.config();
      cfg.spi_host   = SPI2_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.pin_sclk   = PIN_LCD_SCLK;
      cfg.pin_mosi   = PIN_LCD_MOSI;
      cfg.pin_miso   = PIN_LCD_MISO;
      cfg.pin_dc     = PIN_LCD_DC;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Panel
      auto cfg = _panel.config();
      cfg.pin_cs        = PIN_LCD_CS;
      cfg.pin_rst       = PIN_LCD_RST;
      cfg.pin_busy      = -1;
      cfg.panel_width   = 240;
      cfg.panel_height  = 320;
      cfg.offset_rotation = 0;
      cfg.readable      = true;
      cfg.invert        = false;  // si los colores salen "negativos", cambia a true
      cfg.rgb_order     = false;  // si rojo y azul están intercambiados, cambia a true
      cfg.bus_shared    = true;   // el bus SPI puede compartirse con la SD
      _panel.config(cfg);
    }
    { // Retroiluminación
      auto cfg = _light.config();
      cfg.pin_bl      = PIN_LCD_BL;
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    { // Táctil capacitivo
      auto cfg = _touch.config();
      cfg.x_min = 0;   cfg.x_max = 239;
      cfg.y_min = 0;   cfg.y_max = 319;
      cfg.pin_int  = PIN_TP_INT;
      cfg.pin_rst  = PIN_TP_RST;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = 0;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda  = PIN_TP_SDA;
      cfg.pin_scl  = PIN_TP_SCL;
      cfg.freq     = 400000;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};

LGFX tft;
LGFX_Sprite canvas(&tft);

// ======================= COLORES =======================
uint16_t C_BG, C_FUR, C_FUR_DARK, C_CREAM, C_PINK, C_BLUSH, C_EYE, C_PUPIL,
         C_WHITE, C_LINE, C_CUSHION, C_SHADOW;

// ======================= ESTRELLAS Y CORAZONES =======================
struct Star  { int16_t x, y; float phase; };
struct Heart { float x, y, vy, vx; bool active; };

const int NUM_STARS  = 35;
const int NUM_HEARTS = 12;
Star  stars[NUM_STARS];
Heart hearts[NUM_HEARTS];

uint32_t happyUntil = 0;
bool wasTouched = false;

// HSV -> RGB565 (tono pastel) para el arcoíris del nombre
uint16_t pastelHue(float h) {
  h = fmodf(h, 360.0f);
  float x = 1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f);
  float r = 0, g = 0, b = 0;
  if      (h < 60)  { r = 1; g = x; }
  else if (h < 120) { r = x; g = 1; }
  else if (h < 180) { g = 1; b = x; }
  else if (h < 240) { g = x; b = 1; }
  else if (h < 300) { r = x; b = 1; }
  else              { r = 1; b = x; }
  // aclarar hacia blanco para tono pastel
  r = 0.45f + 0.55f * r; g = 0.45f + 0.55f * g; b = 0.45f + 0.55f * b;
  return canvas.color565(r * 255, g * 255, b * 255);
}

void drawHeart(int x, int y, int r, uint16_t c) {
  canvas.fillCircle(x - r / 2, y, r / 2 + 1, c);
  canvas.fillCircle(x + r / 2, y, r / 2 + 1, c);
  canvas.fillTriangle(x - r - 1, y + 1, x + r + 1, y + 1, x, y + r + r / 2, c);
}

void spawnHearts() {
  for (int i = 0; i < NUM_HEARTS; i++) {
    hearts[i].x  = 120 + random(-40, 41);
    hearts[i].y  = 200 + random(-20, 21);
    hearts[i].vy = -(1.5f + random(0, 100) / 50.0f);
    hearts[i].vx = (random(0, 100) - 50) / 60.0f;
    hearts[i].active = true;
  }
}

// ======================= LED RGB =======================
uint32_t ledStart = 0;
const uint32_t LED_MS = 1200;   // duración del destello
uint8_t lastLed = 255;

void setLed(uint8_t r, uint8_t g, uint8_t b) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  rgbLedWrite(PIN_RGB_LED, r, g, b);
#else
  neopixelWrite(PIN_RGB_LED, r, g, b);
#endif
}

// Rosa que se desvanece; solo escribe al LED cuando cambia el nivel
void updateLed(uint32_t now) {
  uint8_t level = 0;
  uint32_t el = now - ledStart;
  if (ledStart && el < LED_MS) level = 255 - (el * 255 / LED_MS);
  if (level == lastLed) return;
  lastLed = level;
  setLed(level * 64 / 255, level * 8 / 255, level * 32 / 255);  // ~25% brillo máx
}

// ======================= AUDIO (ES8311 + I2S) =======================
I2SClass i2s;
TaskHandle_t audioTask = nullptr;
bool audioOk = false;
const uint32_t SAMPLE_RATE = 16000;

// Escritura de registros por el mismo bus I2C que usa LovyanGFX para el táctil
bool es8311Write(uint8_t reg, uint8_t val) {
  return lgfx::i2c::writeRegister8(0, ES8311_ADDR, reg, val, 0, 100000).has_value();
}

// Config mínima: modo esclavo, MCLK = 256*fs, I2S 16 bits, solo DAC (salida)
bool es8311Init() {
  if (!es8311Write(0x00, 0x1F)) return false;   // reset (si falla, no hay codec)
  delay(20);
  es8311Write(0x00, 0x00);
  es8311Write(0x00, 0x80);   // power on, esclavo
  es8311Write(0x01, 0x3F);   // relojes on, MCLK desde pin
  es8311Write(0x02, 0x00);   // pre_div=1, mult=1
  es8311Write(0x03, 0x10);   // ADC OSR
  es8311Write(0x04, 0x10);   // DAC OSR
  es8311Write(0x05, 0x00);   // divisores ADC/DAC = 1
  es8311Write(0x06, 0x03);
  es8311Write(0x07, 0x00);
  es8311Write(0x08, 0xFF);
  es8311Write(0x09, 0x0C);   // entrada serie: I2S 16 bits
  es8311Write(0x0A, 0x0C);   // salida serie: I2S 16 bits
  es8311Write(0x0B, 0x00);
  es8311Write(0x0C, 0x00);
  es8311Write(0x10, 0x1F);
  es8311Write(0x11, 0x7F);
  es8311Write(0x13, 0x10);
  es8311Write(0x0D, 0x01);   // encender bloque analógico
  es8311Write(0x0E, 0x02);
  es8311Write(0x12, 0x00);   // encender DAC
  es8311Write(0x14, 0x1A);
  es8311Write(0x15, 0x40);
  es8311Write(0x37, 0x08);   // bypass EQ del DAC
  es8311Write(0x45, 0x00);
  es8311Write(0x31, 0x00);   // DAC sin mute
  es8311Write(0x32, 0xBF);   // volumen DAC (0x00..0xFF, 0xBF = 0 dB)
  return true;
}

// Tarea en el core 0: sintetiza el "miau" sin congelar la animación
void audioTaskFn(void*) {
  const int CHUNK = 256;
  static int16_t buf[CHUNK * 2];   // estéreo
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // espera un toque

    const int total = SAMPLE_RATE * 0.65f;     // 650 ms
    float phase = 0;
    for (int n = 0; n < total; ) {
      int cnt = min(CHUNK, total - n);
      for (int i = 0; i < cnt; i++, n++) {
        float p = (float)n / total;   // progreso 0..1
        // Tono: sube en "mi" y baja en "au"
        float f0 = (p < 0.3f) ? 550 + (p / 0.3f) * 350
                              : 900 - ((p - 0.3f) / 0.7f) * 450;
        f0 *= 1.0f + 0.02f * sinf(2 * PI * 6 * n / SAMPLE_RATE);   // vibrato
        // "Vocal": más armónicos a la mitad (a), menos al final (u)
        float open = sinf(PI * p);
        phase += 2 * PI * f0 / SAMPLE_RATE;
        if (phase > 2 * PI) phase -= 2 * PI;
        float s = sinf(phase) + 0.6f * open * sinf(2 * phase)
                + 0.35f * open * sinf(3 * phase) + 0.15f * open * sinf(4 * phase);
        // Envolvente: ataque 40 ms + caída suave
        float env = min(1.0f, n / (0.04f * SAMPLE_RATE)) * powf(1.0f - p, 0.6f);
        int16_t v = (int16_t)(s * env * 7000);
        buf[2 * i] = v;
        buf[2 * i + 1] = v;
      }
      i2s.write((uint8_t*)buf, cnt * 4);
    }
    // Silencio para vaciar el DMA y evitar zumbido residual
    memset(buf, 0, sizeof(buf));
    for (int k = 0; k < 4; k++) i2s.write((uint8_t*)buf, sizeof(buf));
  }
}

void audioInit() {
  pinMode(PIN_PA_EN, OUTPUT);
  digitalWrite(PIN_PA_EN, HIGH);   // amplificador apagado durante la config

  i2s.setPins(PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT, PIN_I2S_DIN, PIN_I2S_MCLK);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("I2S: fallo al iniciar");
    return;
  }
  // El MCLK ya corre; ahora sí se puede configurar el codec
  if (!es8311Init()) {
    Serial.println("ES8311: no responde en I2C (revisar pines/dirección)");
    return;
  }
  digitalWrite(PIN_PA_EN, LOW);    // LOW = amplificador encendido
  audioOk = true;
  xTaskCreatePinnedToCore(audioTaskFn, "audio", 4096, nullptr, 2, &audioTask, 0);
  Serial.println("Audio OK");
}

void triggerMeow() {
  if (audioOk && audioTask) xTaskNotifyGive(audioTask);
}

// ======================= DIBUJO DEL GATITO =======================
void drawTail(float t) {
  const float bx = 162, by = 262;
  for (int i = 0; i < 17; i++) {
    float px = bx + i * 3.2f;
    float py = by - i * 4.5f + sinf(t * 3.0f + i * 0.35f) * (i * 0.9f);
    int   r  = 8 - (int)(i * 0.3f);
    canvas.fillCircle(px, py, r, (i > 13) ? C_FUR_DARK : C_FUR);
  }
}

void drawBody() {
  canvas.fillEllipse(120, 290, 85, 13, C_CUSHION);      // cojín
  canvas.fillEllipse(120, 245, 52, 42, C_FUR);          // cuerpo
  canvas.fillEllipse(120, 254, 30, 28, C_CREAM);        // pancita
  canvas.fillEllipse(100, 284, 13, 8, C_CREAM);         // patitas
  canvas.fillEllipse(140, 284, 13, 8, C_CREAM);
}

void drawEye(int ex, int ey, float look, bool blink, bool happy) {
  if (happy) {               // ojos felices ^ ^
    canvas.drawArc(ex, ey + 5, 10, 7, 200, 340, C_LINE);
    return;
  }
  if (blink) {               // parpadeo
    canvas.fillRect(ex - 10, ey - 1, 20, 3, C_LINE);
    return;
  }
  canvas.fillEllipse(ex, ey, 11, 13, C_EYE);
  int px = ex + (int)look;
  canvas.fillCircle(px, ey + 1, 7, C_PUPIL);
  canvas.fillCircle(px - 3, ey - 3, 3, C_WHITE);       // brillo
  canvas.fillCircle(px + 2, ey + 3, 1, C_WHITE);
}

void drawHead(float t, uint32_t now) {
  int cx = 120 + (int)(sinf(t * 1.6f) * 3);
  int cy = 165 + (int)(sinf(t * 2.0f) * 3);

  // Orejas
  canvas.fillTriangle(cx - 44, cy - 20, cx - 34, cy - 64, cx - 6,  cy - 40, C_FUR);
  canvas.fillTriangle(cx + 44, cy - 20, cx + 34, cy - 64, cx + 6,  cy - 40, C_FUR);
  canvas.fillTriangle(cx - 37, cy - 26, cx - 32, cy - 52, cx - 15, cy - 38, C_PINK);
  canvas.fillTriangle(cx + 37, cy - 26, cx + 32, cy - 52, cx + 15, cy - 38, C_PINK);

  // Cabeza
  canvas.fillEllipse(cx, cy, 50, 44, C_FUR);

  // Rayitas en la frente
  canvas.fillTriangle(cx - 4, cy - 44, cx + 4, cy - 44, cx, cy - 30, C_FUR_DARK);
  canvas.fillTriangle(cx - 16, cy - 42, cx - 10, cy - 43, cx - 12, cy - 32, C_FUR_DARK);
  canvas.fillTriangle(cx + 16, cy - 42, cx + 10, cy - 43, cx + 12, cy - 32, C_FUR_DARK);

  // Hocico claro
  canvas.fillEllipse(cx, cy + 20, 20, 13, C_CREAM);

  // Mejillas
  canvas.fillEllipse(cx - 32, cy + 14, 8, 5, C_BLUSH);
  canvas.fillEllipse(cx + 32, cy + 14, 8, 5, C_BLUSH);

  // Ojos
  bool happy = now < happyUntil;
  bool blink = (now % 3500) < 140;
  float look = sinf(t * 0.8f) * 4.0f;
  drawEye(cx - 18, cy - 2, look, blink, happy);
  drawEye(cx + 18, cy - 2, look, blink, happy);

  // Nariz y boca
  canvas.fillTriangle(cx - 6, cy + 13, cx + 6, cy + 13, cx, cy + 19, C_PINK);
  canvas.drawArc(cx - 6, cy + 20, 6, 5, 0, 180, C_LINE);
  canvas.drawArc(cx + 6, cy + 20, 6, 5, 0, 180, C_LINE);
  if (happy) canvas.fillEllipse(cx, cy + 29, 5, 4, C_PINK);   // boquita abierta

  // Bigotes
  canvas.drawLine(cx - 22, cy + 16, cx - 58, cy + 8,  C_LINE);
  canvas.drawLine(cx - 22, cy + 20, cx - 58, cy + 22, C_LINE);
  canvas.drawLine(cx + 22, cy + 16, cx + 58, cy + 8,  C_LINE);
  canvas.drawLine(cx + 22, cy + 20, cx + 58, cy + 22, C_LINE);
}

void drawName(float t) {
  int y = 48 + (int)(sinf(t * 3.0f) * 7);
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  canvas.setTextSize(1.4f);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(C_SHADOW);
  canvas.drawString("Sam", 123, y + 3);
  canvas.setTextColor(pastelHue(t * 90.0f));
  canvas.drawString("Sam", 120, y);
}

void drawStars(float t) {
  for (int i = 0; i < NUM_STARS; i++) {
    float b = 0.35f + 0.65f * (0.5f + 0.5f * sinf(t * 2.5f + stars[i].phase));
    uint8_t v = b * 255;
    uint16_t c = canvas.color565(v, v, v * 0.85f + 38);
    if (b > 0.85f) {   // estrellas brillantes en forma de cruz
      canvas.drawFastHLine(stars[i].x - 2, stars[i].y, 5, c);
      canvas.drawFastVLine(stars[i].x, stars[i].y - 2, 5, c);
    } else {
      canvas.drawPixel(stars[i].x, stars[i].y, c);
    }
  }
}

void updateHearts() {
  for (int i = 0; i < NUM_HEARTS; i++) {
    if (!hearts[i].active) continue;
    hearts[i].y += hearts[i].vy;
    hearts[i].x += hearts[i].vx;
    if (hearts[i].y < -10) { hearts[i].active = false; continue; }
    drawHeart(hearts[i].x, hearts[i].y, 6, C_PINK);
  }
}

// ======================= SETUP / LOOP =======================
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(0);        // vertical 240x320
  tft.setBrightness(200);

  setLed(0, 0, 0);
  audioInit();   // después de tft.init(): reutiliza el bus I2C del táctil

  // Framebuffer completo: 16 bits en PSRAM si hay; si falla, 8 bits
  canvas.setPsram(true);
  canvas.setColorDepth(16);
  if (!canvas.createSprite(240, 320)) {
    Serial.println("Sprite 16-bit falló, usando 8-bit");
    canvas.setPsram(false);
    canvas.setColorDepth(8);
    canvas.createSprite(240, 320);
  }

  C_BG       = canvas.color565(24, 28, 58);
  C_FUR      = canvas.color565(245, 160, 70);
  C_FUR_DARK = canvas.color565(205, 110, 40);
  C_CREAM    = canvas.color565(255, 236, 210);
  C_PINK     = canvas.color565(255, 130, 160);
  C_BLUSH    = canvas.color565(255, 175, 160);
  C_EYE      = canvas.color565(120, 200, 140);
  C_PUPIL    = canvas.color565(20, 20, 30);
  C_WHITE    = TFT_WHITE;
  C_LINE     = canvas.color565(90, 50, 30);
  C_CUSHION  = canvas.color565(120, 90, 180);
  C_SHADOW   = canvas.color565(10, 10, 25);

  randomSeed(esp_random());
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i] = { (int16_t)random(4, 236), (int16_t)random(4, 316),
                 random(0, 628) / 100.0f };
  }
}

void loop() {
  uint32_t now = millis();
  float t = now / 1000.0f;

  // Táctil: un toque por pulsación
  uint16_t tx, ty;
  if (tft.getTouch(&tx, &ty)) {
    if (!wasTouched) {
      happyUntil = now + 1500;
      spawnHearts();
      ledStart = now;   // destello rosa
      triggerMeow();    // ¡miau!
    }
    wasTouched = true;
  } else {
    wasTouched = false;
  }

  canvas.fillScreen(C_BG);
  drawStars(t);
  drawName(t);
  drawTail(t);
  drawBody();
  drawHead(t, now);
  updateHearts();
  updateLed(now);

  canvas.pushSprite(0, 0);   // ~31 ms a 40 MHz -> ~30 FPS

  // Limitar a ~33 FPS
  uint32_t dt = millis() - now;
  if (dt < 30) delay(30 - dt);
}
