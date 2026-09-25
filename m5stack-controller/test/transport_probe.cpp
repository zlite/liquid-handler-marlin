#include <M5Stack.h>
#include "ch340_host.h"
USB usb;
Ch340Host ender(usb);
uint32_t attachedAt=0;
unsigned sent=0;
bool attached=false;
const char *commands[]={"M115\n","M114\n","M119\n","M114\n"};
void setup(){
  Serial.setTxBufferSize(4096); Serial.begin(115200);
  pinMode(5,OUTPUT);digitalWrite(5,HIGH);
  pinMode(4,OUTPUT);digitalWrite(4,HIGH);
  M5.begin(true,false,false,false); M5.Lcd.setBrightness(200);
  M5.Lcd.setTextSize(2);M5.Lcd.println("USB transport test");
  Serial.printf("UHS init: %d\n",usb.Init());
}
void loop(){
  usb.Task();
  if(ender.connected()!=attached){
    attached=ender.connected();attachedAt=millis();sent=0;
    Serial.printf("CH340 connected=%d\n",attached);
    M5.Lcd.println(attached?"CH340 connected":"Disconnected");
  }
  if(attached){
    uint8_t data[64];uint16_t n=sizeof(data);
    uint8_t rc=ender.read(data,n);
    if(!rc && n)Serial.write(data,n);
    else if(rc!=hrNAK && rc)Serial.printf("RX error %u\n",rc);
    if(sent<4 && millis()-attachedAt>3000+sent*3000){
      Serial.printf("TX %s",commands[sent]);
      uint8_t rc=ender.write(commands[sent++]);Serial.printf("USB TX rc=%u\n",rc);
    }
  }
}
