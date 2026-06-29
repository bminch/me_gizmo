const uint8_t RGB_R = 22;
const uint8_t RGB_G = 23;
const uint8_t RGB_B = 24;
const uint8_t LED = 25;

const uint16_t BLINK_INTERVAL = 500;
const uint16_t HUE_INTERVAL = 10;

uint32_t blink_time, hue_time;
uint8_t hue_value;

bool it_is_time(uint32_t t, uint32_t t0, uint16_t dt) {
  return ((t >= t0) && (t - t0 >= dt)) ||         // The first disjunct handles the normal case
            ((t < t0) && (t + (~t0) + 1 >= dt));  //   while the second handles the overflow case
}

void rgb_set_hue(uint8_t hue) {
    uint8_t phase, value;
    uint8_t red, green, blue;

    hue = (hue >> 2) + (hue >> 1);
    phase = hue >> 5;
    value = hue & 0x1F;
    switch (phase) {
        case 0:
            red = 0xFF;
            green = value << 3;
            blue = 0x00;
            break;
        case 1:
            red = (~(value << 3)) & 0xFF;
            green = 0xFF;
            blue = 0x00;
            break;
        case 2:
            red = 0x00;
            green = 0xFF;
            blue = value << 3;
            break;
        case 3:
            red = 0x00;
            green = (~(value << 3)) & 0xFF;
            blue = 0xFF;
            break;
        case 4:
            red = value << 3;
            green = 0x00;
            blue = 0xFF;
            break;
        case 5:
            red = 0xFF;
            green = 0x00;
            blue = (~(value << 3)) & 0xFF;
            break;
    }

    analogWrite(RGB_R, 255 - red);
    analogWrite(RGB_G, 255 - green);
    analogWrite(RGB_B, 255 - blue);
}

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  pinMode(RGB_R, OUTPUT);
  digitalWrite(RGB_R, HIGH);
  pinMode(RGB_G, OUTPUT);
  digitalWrite(RGB_G, HIGH);
  pinMode(RGB_B, OUTPUT);
  digitalWrite(RGB_B, HIGH);

  analogWriteFreq(250000);
  analogWriteRange(255);

  blink_time = millis();
  hue_time = blink_time;
  hue_value = 0;
  rgb_set_hue(hue_value);
}

void loop() {
  uint32_t t;

  t = millis();
  if (it_is_time(t, blink_time, BLINK_INTERVAL)) {
    digitalWrite(LED, !digitalRead(LED));
    blink_time = t;
  }
  if (it_is_time(t, hue_time, HUE_INTERVAL)) {
    rgb_set_hue(++hue_value);
    hue_time = t;
  }
}
