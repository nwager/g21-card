#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// #define DISPLAY_ADDRESS 0x3C

#define COLOR_OFF   0
#define COLOR_ON    1
#define COLOR_UNSET ((uint8_t)(-1))

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_CX (SCREEN_WIDTH / 2)
#define SCREEN_CY (SCREEN_HEIGHT / 2)

// full frame is 1024 bytes
// circuit draws about 17mA
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);

// gain of ~235 is max screen height
// gain of ~60 is half screen height
// takes about 50ms
void draw_heart(float gain, float cx, float cy, uint8_t dot_size);
void draw_heart_seq(float gain,
                    float cx,
                    float cy,
                    uint8_t dot_size,
                    unsigned long dot_delay_ms);
inline uint8_t heart_top_fn(uint8_t x, float a, float h, float k);
inline uint8_t heart_bot_fn(uint8_t x, float a, float h, float k);

void draw_dot_color(uint8_t x, uint8_t y, uint8_t size, uint8_t color_index);
inline void clear_dot(uint8_t x, uint8_t y, uint8_t size);
inline void draw_dot(uint8_t x, uint8_t y, uint8_t size);

void draw_tl_str();
void draw_tr_str();
void draw_bl_str();
void draw_br_str();

void setup() {
  display.begin();
}

void loop() {
  static bool heart_seq_on = true;
  static uint8_t prev_str_num = 0;

  display.clearBuffer();
  display.setDrawColor(COLOR_ON);

  uint8_t curr_str_num = (
    (prev_str_num + (uint8_t)floor(3 * (float)rand() / RAND_MAX) + 1) % 4
  );

  switch (curr_str_num) {
    case 0:
      draw_tl_str();
      break;
    case 1:
      draw_tr_str();
      break;
    case 2:
      draw_bl_str();
      break;
    case 3:
      draw_br_str();
      break;
  }

  prev_str_num = curr_str_num;

  display.setFont(u8g2_font_lubBI14_tr);
  uint8_t char_width = 14;
  display.drawStr(SCREEN_CX - char_width, SCREEN_CY, "21");
  
  if (!heart_seq_on) {
    // redraw heart so it can be removed by draw_heart_seq
    draw_heart(180, SCREEN_CX, 17, 2);
  }
  display.setDrawColor(heart_seq_on ? COLOR_ON : COLOR_OFF);
  draw_heart_seq(180, SCREEN_CX, 17, 2, 2);
  heart_seq_on = !heart_seq_on;
}

void draw_heart(float gain, float cx, float cy, uint8_t dot_size) {
  uint8_t x_min = (uint8_t)ceil((-2 * sqrtf(gain)) + cx);
  uint8_t x_max = (uint8_t)floor((2 * sqrtf(gain)) + cx);

  for (int x = x_min; x <= x_max; x++) {
    // center point is too far away, looks discontinuous
    if (x == (uint8_t)cx) continue;

    uint8_t top_y = heart_top_fn(x, gain, cx, cy);
    uint8_t bot_y = heart_bot_fn(x, gain, cx, cy);

    if (x == x_min || x == x_max) {
      for (int i = 0; i < dot_size; i++) {
        display.drawLine(x + i, bot_y, x + i, top_y);
      }
    }
  
    draw_dot(x, top_y, dot_size);
    draw_dot(x, bot_y, dot_size);
  }
}

void draw_heart_seq_helper(uint8_t x0,
                           uint8_t y0,
                           uint8_t x1,
                           uint8_t y1,
                           uint8_t dot_size,
                           unsigned long dot_delay_ms) {

  draw_dot_color(x0, y0, dot_size, -1);
  draw_dot_color(x1, y1, dot_size, -1);
  display.sendBuffer();
  delay(dot_delay_ms);
}

void draw_heart_seq(float gain,
                    float cx,
                    float cy,
                    uint8_t dot_size,
                    unsigned long dot_delay_ms) {
  
  uint8_t x_min = (uint8_t)ceil((-2 * sqrtf(gain)) + cx);
  uint8_t x_max = (uint8_t)floor((2 * sqrtf(gain)) + cx);
  uint8_t x_mid = (x_max - x_min) / 2 + x_min;

  // draw the top from center out
  for (int x0 = x_mid-1, x1 = x_mid+1; x0 >= x_min; x0--, x1++) {
    uint8_t y0 = heart_top_fn(x0, gain, cx, cy);
    uint8_t y1 = heart_top_fn(x1, gain, cx, cy);
    draw_heart_seq_helper(x0, y0, x1, y1, dot_size, dot_delay_ms);
  }

  // draw side lines
  for (int i = 0; i < dot_size; i++) {
    uint8_t y0_top = heart_top_fn(x_min, gain, cx, cy);
    uint8_t y0_bot = heart_bot_fn(x_min, gain, cx, cy);
    display.drawLine(x_min+i, y0_top, x_min+i, y0_bot);
    
    uint8_t y1_top = heart_top_fn(x_max, gain, cx, cy);
    uint8_t y1_bot = heart_bot_fn(x_max, gain, cx, cy);
    display.drawLine(x_max+i, y1_top, x_max+i, y1_bot);
  }
  display.sendBuffer();
  delay(dot_delay_ms);

  // draw the bottom from out to center
  for (int x0 = x_min, x1 = x_max; x0 < x_mid; x0++, x1--) {
    uint8_t y0 = heart_bot_fn(x0, gain, cx, cy);
    uint8_t y1 = heart_bot_fn(x1, gain, cx, cy);
    draw_heart_seq_helper(x0, y0, x1, y1, dot_size, dot_delay_ms);
  }
}

inline uint8_t heart_top_fn(uint8_t x, float a, float h, float k) {
  return (uint8_t)(
    -sqrtf(a - sq(abs(x - h) - sqrtf(a))) + k
  );
}

inline uint8_t heart_bot_fn(uint8_t x, float a, float h, float k) {
  return (uint8_t)(
    -(sqrtf(a) * (acosf(1.0 - abs((float)(x - h) / sqrtf(a))) - PI)) + k
  );
}

void draw_dot_color(uint8_t x, uint8_t y, uint8_t size, uint8_t color_index) {
  if (color_index != COLOR_UNSET) {
    display.setDrawColor(color_index);
  }
  for (int dy = y; dy < y + size; dy++) {
    for (int dx = x; dx < x + size; dx++) {
      display.drawPixel(dx, dy);
    }
  }
}

inline void draw_dot(uint8_t x, uint8_t y, uint8_t size) {
  draw_dot_color(x, y, size, 1);
}

inline void clear_dot(uint8_t x, uint8_t y, uint8_t size) {
  draw_dot_color(x, y, size, 0);
}

void draw_tl_str() {
  display.setFont(u8g2_font_streamline_entertainment_events_hobbies_t);
  // uint8_t char0_width = 21;
  uint8_t char0_height = 21;
  uint8_t str0_y = 2 + char0_height;
  display.drawStr(2, str0_y, "\x3d"); // frazzled ghost

  display.setFont(u8g2_font_profont10_tr);
  // uint8_t char1_width = 5;
  uint8_t char1_height = 10;
  display.drawStr(1, str0_y + 1 + char1_height, "ooOoO");
}

void draw_tr_str() {
  display.setFont(u8g2_font_streamline_pet_animals_t);
  uint8_t char0_width = 21;
  uint8_t char0_height = 21;
  uint8_t str0_x = SCREEN_WIDTH - char0_width - 2;
  uint8_t str0_y = char0_height + 2;
  display.drawStr(str0_x, str0_y, "\x35"); // frog

  display.setFont(u8g2_font_profont10_tr);
  // uint8_t char1_width = 5;
  uint8_t char1_height = 10;
  display.drawStr(str0_x, str0_y + char1_height + 1, "BONK");
}

void draw_bl_str() {
  display.setFont(u8g2_font_t0_11_tr);
  // uint8_t char_width = 6;
  uint8_t char_height = 11;
  display.drawStr(2, SCREEN_HEIGHT - 2 - char_height - 2, "party");
  display.drawStr(2, SCREEN_HEIGHT - 2, "time!");
}

void draw_br_str() {
  display.setFont(u8g2_font_t0_11_tr);
  uint8_t char_width = 6;
  uint8_t char_height = 11;
  String str = "wowza?!";
  display.drawStr(
    SCREEN_WIDTH - (str.length()*char_width) - 2,
    SCREEN_HEIGHT - (char_height / 2) - 2,
    str.c_str()
  );
}
