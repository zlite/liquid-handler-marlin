#include <M5Stack.h>

const uint16_t colors[] = {RED, GREEN, BLUE, WHITE};
const char *names[] = {"RED", "GREEN", "BLUE", "WHITE"};
uint32_t lastFrame = 0;
unsigned frame = 0;

void drawFrame() {
  unsigned index = frame++ % 4;
  M5.Lcd.fillScreen(colors[index]);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(BLACK, colors[index]);
  M5.Lcd.setCursor(12, 35);
  M5.Lcd.println("CORE BASIC");
  M5.Lcd.setCursor(12, 75);
  M5.Lcd.println("DISPLAY TEST");
  M5.Lcd.setCursor(12, 115);
  M5.Lcd.println(names[index]);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(12, 165);
  M5.Lcd.printf("Uptime: %lus", (unsigned long)(millis()/1000));
  Serial.printf("Display frame %u: %s; uptime=%lu; backlight GPIO32=%d\n",
                frame, names[index], (unsigned long)(millis()/1000), digitalRead(32));
}

void setup() {
  Serial.begin(115200);
  Serial.println("Standalone Core Basic display test; USB host disabled");
  // Deselect external USB and SD devices before initializing the LCD.
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);
  pinMode(4, OUTPUT); digitalWrite(4, HIGH);
  M5.begin(true, false, false, false);
  M5.Lcd.setRotation(1);
  // Bypass PWM to test the Basic's backlight directly at full brightness.
  ledcDetachPin(32);
  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);
  Serial.println("LCD initialized; backlight forced HIGH; colors change every 3s");
  drawFrame();
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) Serial.println("Button A pressed");
  if (M5.BtnB.wasPressed()) Serial.println("Button B pressed");
  if (M5.BtnC.wasPressed()) Serial.println("Button C pressed");
  if (millis() - lastFrame >= 3000) {
    lastFrame = millis();
    drawFrame();
  }
  delay(10);
}
