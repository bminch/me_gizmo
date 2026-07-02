#include "me_gizmo.h"
#include "parser.h"
#include "hardware/spi.h"

void setup() {
  init_me_gizmo();
  init_parser();
}

void loop() {
  parser_state();
}

// Simulated switch array
bool dataReceived = false;
uint8_t rxBuf[8];
uint8_t txBuf[8];
uint8_t rxCount = 0;

void setup1() {
  Serial1.begin(115200);
  while (!Serial);

  // Initialize SPI1 hardware directly via SDK

  spi_set_slave(spi1, true);        // Set slave mode

  // Set data format: 8 bits, CPOL=0, CPHA=1, MSB first
  spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

  // Assign GPIO pins to SPI1 function
  gpio_set_function(PICO_SCLK, GPIO_FUNC_SPI);
  gpio_set_function(PICO_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PICO_SYNC, GPIO_FUNC_SPI);

  Serial1.println("SPI Slave ready");
}


void loop1() {
  // Block until 16 bytes are exchanged with the master
  // txBuf is sent while rxBuf is filled simultaneously
  spi_write_read_blocking(spi1, txBuf, rxBuf, 8);

  for (uint8_t i = 0; i < 8; i++) {
      Serial.print((char)rxBuf[i]);
  }
}