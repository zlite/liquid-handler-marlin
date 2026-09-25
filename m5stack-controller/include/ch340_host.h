#pragma once
#include <Usb.h>

// CH340 vendor requests follow the Linux ch341 driver and TinyUSB CH34x driver.
// Specifically supports the observed 1a86:7523 bridge, at 250000 baud, 8N1.
class Ch340Host : public USBDeviceConfig, public UsbConfigXtracter {
  USB &usb;
  EpInfo ep[3] = {};
  uint8_t address = 0, config = 0;
  bool ready = false;
  uint8_t control(uint8_t request, uint16_t value, uint16_t index) {
    return usb.ctrlReq(address, 0, 0x40, request, value & 255, value >> 8,
                       index, 0, 0, nullptr, nullptr);
  }
  void resetEndpoints() {
    memset(ep, 0, sizeof(ep));
    ep[0].maxPktSize = 8;
    ep[0].bmNakPower = USB_NAK_MAX_POWER;
    ep[1].bmNakPower = USB_NAK_NOWAIT;
    ep[2].bmNakPower = USB_NAK_MAX_POWER;
  }
public:
  explicit Ch340Host(USB &host) : usb(host) {
    resetEndpoints(); usb.RegisterDeviceClass(this);
  }
  bool connected() const { return ready; }
  uint8_t GetAddress() override { return address; }
  bool VIDPIDOK(uint16_t vid, uint16_t pid) override { return vid == 0x1a86 && pid == 0x7523; }
  uint8_t Release() override {
    if (address) usb.GetAddressPool().FreeAddress(address);
    address = config = 0; ready = false; resetEndpoints(); return 0;
  }
  void EndpointXtract(uint8_t conf, uint8_t, uint8_t, uint8_t,
                     const USB_ENDPOINT_DESCRIPTOR *d) override {
    if ((d->bmAttributes & 3) != 2) return;
    config = conf;
    unsigned slot = (d->bEndpointAddress & 0x80) ? 1 : 2;
    ep[slot].epAddr = d->bEndpointAddress & 15;
    ep[slot].maxPktSize = d->wMaxPacketSize;
  }
  uint8_t Init(uint8_t parent, uint8_t port, bool low) override {
    if (address) return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
    auto &pool = usb.GetAddressPool();
    auto *zero = pool.GetUsbDevicePtr(0);
    if (!zero || !zero->epinfo) return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    USB_DEVICE_DESCRIPTOR d;
    auto *old = zero->epinfo;
    zero->epinfo = ep; zero->lowspeed = low;
    uint8_t rc = usb.getDevDescr(0, 0, sizeof(d), (uint8_t*)&d);
    zero->epinfo = old;
    if (rc) return rc;
    if (!VIDPIDOK(d.idVendor, d.idProduct)) return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
    address = pool.AllocAddress(parent, false, port);
    if (!address) return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
    ep[0].maxPktSize = d.bMaxPacketSize0;
    rc = usb.setAddr(0, 0, address);
    zero->lowspeed = false;
    if (!rc) {
      delay(20);
      pool.GetUsbDevicePtr(address)->lowspeed = low;
      rc = usb.setEpInfoEntry(address, 1, ep);
    }
    for (uint8_t i=0; !rc && i<d.bNumConfigurations && !config; ++i) {
      ConfigDescParser<0xff,0,0,CP_MASK_COMPARE_CLASS> parser(this);
      rc = usb.getConfDescr(address, 0, i, &parser);
    }
    if (!rc && (!ep[1].epAddr || !ep[2].epAddr)) rc = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
    if (!rc) rc = usb.setEpInfoEntry(address, 3, ep);
    if (!rc) rc = usb.setConf(address, 0, config);
    uint8_t version[2];
    if (!rc) rc = usb.ctrlReq(address,0,0xc0,0x5f,0,0,0,2,2,version,nullptr);
    // divisor=256-(6000000/250000)=232, prescaler=3, immediate RX flag=0x80.
    if (!rc) rc = control(0xa1, 0, 0);
    if (!rc) rc = control(0x9a, 0x1312, 0xe883);
    if (!rc) rc = control(0x9a, 0x2518, 0x00c3);
    if (!rc) rc = control(0x9a, 0x0f2c, 0x0007);
    if (!rc) rc = control(0x9a, 0x2727, 0x0000);
    if (!rc) rc = control(0xa4, 0xff9f, 0); // Assert DTR and RTS.
    if (rc) { Release(); return rc; }
    ready = true;
    return 0;
  }
  uint8_t read(uint8_t *data, uint16_t &length) {
    // One endpoint packet per read. A multi-packet read can receive a full
    // packet then return NAK; UHS does not commit its toggle on that path.
    if(length > ep[1].maxPktSize) length = ep[1].maxPktSize;
    return usb.inTransfer(address, ep[1].epAddr, &length, data);
  }
  uint8_t write(const char *data) {
    return usb.outTransfer(address, ep[2].epAddr, strlen(data), (uint8_t*)data);
  }
};
