# Firmware

## Wiring
* GND -> GND
* GPIO0 -> GND
* GPIO15 -> GND
* VCC -> VCC
* CH_PD -> VCC

## Flash with esptool
```bash
esptool.py --port /dev/ttyUSB0 write_flash -fm qio -fs 32m \
           0x00000 ../boot_v1.6.bin \
           0x01000 ../user1.2048.new.5.bin \
           0x3fc000 ../esp_init_data_default.bin \
           0x3fe000 ../blank.bin 
```