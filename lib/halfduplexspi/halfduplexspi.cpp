#include "halfduplexspi.h"

void HalfDuplexSPI::setup() {
  // Output mode.
  pinMode(SPI_SCK, OUTPUT);
}

uint8_t HalfDuplexSPI::byte(uint8_t dataout) {
  uint8_t datain, bits = 8;

  pinMode(SPI_MOMI, OUTPUT);
  pinMode(SPI_MOMI, INPUT);

  do {
    datain <<= 1;
    if (digitalRead(SPI_MOMI)) {
      datain++;
    }

    // output mode
    pinMode(SPI_MOMI, OUTPUT);
    if (dataout & 0x80) {
      digitalWrite(SPI_MOMI, HIGH);
    }

    //uint8_t currentClock = digitalRead(SPI_SCK);
    digitalWrite(SPI_SCK, HIGH);

    digitalWrite(SPI_MOMI, LOW);

    // toggle SCK
    digitalWrite(SPI_SCK, LOW);

    // input mode
    pinMode(SPI_MOMI, INPUT);

    dataout <<= 1;

  } while (--bits);

  return datain;
}
