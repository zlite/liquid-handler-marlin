#include <Arduino.h>
#include <SPI.h>
#include <M5Stack.h>
#include <stdarg.h>
#include <M5_Max3421E_Usb.h>

SPIClass &sharedSPI = M5.Lcd.getSPIinstance();
M5_USBH_Host usb(&sharedSPI, 18, 23, 19, 5, 35);
Adafruit_USBH_CDC ender;
bool attached = false;
bool hostReady = false;
uint32_t mountedAt = 0;
uint32_t lastReport = 0;
unsigned sent = 0;
char screenLines[9][27] = {};
unsigned column = 0;
bool screenDirty = true;
uint32_t lastDraw = 0, received = 0;

void screenText(const char *text, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    char c = text[i];
    if (c == '\r') continue;
    if (c == '\n' || column == 26) {
      memmove(screenLines[0], screenLines[1], 8 * sizeof(screenLines[0]));
      memset(screenLines[8], 0, sizeof(screenLines[8]));
      column = 0;
      if (c == '\n') continue;
    }
    screenLines[8][column++] = (c >= 32 && c <= 126) ? c : '.';
  }
  screenDirty = true;
}
void logf(const char *format, ...) {
  char text[192];
  va_list args;
  va_start(args, format);
  vsnprintf(text, sizeof(text), format, args);
  va_end(args);
  Serial.print(text);
  screenText(text, strlen(text));
}
void drawScreen() {
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(hostReady ? GREEN : YELLOW, BLACK);
  M5.Lcd.fillRect(0, 26, 320, 18, BLACK);
  M5.Lcd.setCursor(4, 26);
  M5.Lcd.print(!hostReady ? "USB HOST INIT FAILED" : attached ? "ENDER SERIAL CONNECTED" : "Waiting for USB device");
  if (screenDirty) {
    M5.Lcd.fillRect(0, 52, 320, 148, BLACK);
    M5.Lcd.setTextColor(WHITE, BLACK);
    for (unsigned i = 0; i < 9; ++i) {
      M5.Lcd.setCursor(4, 52 + i * 16);
      M5.Lcd.print(screenLines[i]);
    }
  }
  screenDirty = false;
  M5.Lcd.fillRect(0, 205, 320, 35, BLACK);
  M5.Lcd.setTextColor(CYAN, BLACK);
  M5.Lcd.setCursor(4, 205);
  M5.Lcd.printf("Up %lus  RX %lu", (unsigned long)(millis()/1000), (unsigned long)received);
  M5.Lcd.setCursor(4, 223);
  M5.Lcd.print("A: restart probe");
}
const char *queries[] = {"M115\n", "M114\n", "M119\n"};

void tuh_mount_cb(uint8_t address) {
  uint16_t vid, pid;
  tuh_vid_pid_get(address, &vid, &pid);
  logf("USB attached: address=%u VID:PID=%04x:%04x\n", address, vid, pid);
}
void tuh_umount_cb(uint8_t address) {
  logf("USB removed: address=%u\n", address);
}
void tuh_cdc_mount_cb(uint8_t idx) {
  attached = ender.mount(idx);
  mountedAt = millis();
  sent = 0;
  logf("Serial mount: success=%d baud=%lu\n", attached, (unsigned long)ender.baud());
}
void tuh_cdc_umount_cb(uint8_t idx) {
  attached = false;
  ender.umount(idx);
  logf("Serial disconnected\n");
}
void tuh_hid_report_received_cb(uint8_t, uint8_t, uint8_t const*, uint16_t) {}
void tuh_hid_report_sent_cb(uint8_t, uint8_t, uint8_t const*, uint16_t) {}

void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);
  M5.begin(true, false, false, false);
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(CYAN, BLACK);
  M5.Lcd.setCursor(4, 4);
  M5.Lcd.print("Core Basic: Ender USB");
  M5.Lcd.setCursor(4, 26);
  M5.Lcd.print("Checking USB module...");
  // Keep SD inactive. LCD and USB use the same SPI object/transaction lock.
  pinMode(14, OUTPUT); digitalWrite(14, HIGH);
  pinMode(4, OUTPUT); digitalWrite(4, HIGH);
  sharedSPI.begin(18, 19, 23, 5);
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);
  for (uint32_t hz : {1000000UL, 4000000UL, 8000000UL, 26000000UL}) {
    sharedSPI.beginTransaction(SPISettings(hz, MSBFIRST, SPI_MODE0));
    digitalWrite(5, LOW); sharedSPI.transfer(0x8a); sharedSPI.transfer(0x10); digitalWrite(5, HIGH);
    digitalWrite(5, LOW); sharedSPI.transfer(0x90); uint8_t rev = sharedSPI.transfer(0); digitalWrite(5, HIGH);
    sharedSPI.endTransaction();
    logf("SPI %lu MHz: rev 0x%02x\n", (unsigned long)(hz/1000000), rev);
  }
  ender.begin(250000);
  logf("Read-only probe: 250000 bps\n");
  hostReady = usb.begin(1);
  logf("MAX3421E init: %s\n", hostReady ? "OK" : "FAILED");

}
void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) ESP.restart();
  if (millis() - lastDraw >= 1000) {
    lastDraw = millis();
    drawScreen();
  }
  if (!hostReady) { delay(10); return; }

  usb.task(0);
  if (attached && ender.available()) {
    uint8_t buf[64];
    size_t n = ender.read(buf, sizeof(buf));
    Serial.write(buf, n);
    received += n;
    screenText(reinterpret_cast<const char *>(buf), n);
  }
  if (attached && sent < 3 && millis() - mountedAt > 3000 + sent * 3000) {
    logf("TX: %s", queries[sent]);
    ender.print(queries[sent++]);
    ender.flush();
  }
  if (millis() - lastReport > 10000) {
    lastReport = millis();
    logf("Probe: serial=%s baud=%lu queries=%u\n", attached ? "mounted" : "waiting", (unsigned long)ender.baud(), sent);
  }
  delay(1);
}
