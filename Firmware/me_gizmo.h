#ifndef _ME_GIZMO_H_
#define _ME_GIZMO_H_

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE                1
#endif

#ifndef FALSE
#define FALSE               0
#endif

#define EEPROM_LENGTH       1024

#define DEBUG_TIMING_PIN    26

// MCP3562R analog-to-digital converter (ADC) pin definitions
#define ADC_IRQ_            20
#define ADC_CSN             17
#define ADC_SCK             18
#define ADC_MISO            16
#define ADC_MOSI            19

// MCP3562R register address definitions (see Table 8-1 on p. 89 of the MCP3561/2/4R datasheet)
#define ADC_ADCDATA         0   // Latest ADC conversion data output value (3 or 4 bytes depending on DATA_FORMAT[1:0])
#define ADC_CONFIG0         1   // ADC operating mode, Master clock mode and input bias current source mode (1 byte)
#define ADC_CONFIG1         2   // Prescale and OSR settings (1 byte)
#define ADC_CONFIG2         3   // ADC boost and gain settings, auto-zeroing settings for analog mux, voltage regerence and ADC (1 byte)
#define ADC_CONFIG3         4   // Conversion mode, data and CRC format settings, enable for CRC on communications, enable for digital offset and gain calibrations (1 byte)
#define ADC_IRQ             5   // IRQ status bits and mode settings, enable for fast commands and for conversion start pulse (1 byte)
#define ADC_MUX             6   // Analog mux input selection (1 byte)
#define ADC_SCAN            7   // SCAN mode settings (3 bytes)
#define ADC_TIMER           8   // Delay value for TIMER between SCAN cycles (3 bytes)
#define ADC_OFFSETCAL       9   // ADC digital offset calibration value (3 bytes)
#define ADC_GAINCAL         10  // ADC digital gain calibration value (3 bytes)
#define ADC_RESERVED1       11  // Reserved (3 bytes)
#define ADC_RESERVED2       12  // Reserved (1 byte)
#define ADC_LOCK            13  // Password value for SPI write mode locking (1 byte)
#define ADC_RESERVED3       14  // Reserved (2 bytes)
#define ADC_CRCCFG          15  // CRC checksum for device configuration (2 bytes)

// MCP3562R fast command definitions (see Table 6-2 on p. 68 of the MCP3561/2/4R datasheet)
#define ADC_START_CONV      10
#define ADC_STANDBY         11
#define ADC_SHUTDOWN        12
#define ADC_FULL_SHUTDOWN   13
#define ADC_FULL_RESET      14

// MCP3562R input multiplexer register values (see Secion 8.7 on p. 96 of the MCP3561/2/4R datasheet)
#define ADC_MUX_VREF        0xB8    // VIN+ = REFIN+/OUT, VIN- = AGND
#define ADC_MUX_NEG_VREF    0x8B    // VIN+ = AGND, VIN- = REFIN+/OUT
#define ADC_MUX_CALIB       0x13    // VIN+ = CH1/DACA, VIN- = CH3/DACB
#define ADC_MUX_CH0_SE      0x08    // VIN+ = CH0, VIN- = AGND
#define ADC_MUX_CH1_SE      0x18    // VIN+ = CH1/DACA, VIN- = AGND
#define ADC_MUX_CH2_SE      0x28    // VIN+ = CH2, VIN- = AGND
#define ADC_MUX_CH3_SE      0x38    // VIN+ = CH3/DACB, VIN- = AGND
#define ADC_MUX_CHA_DIFF    0x01    // VIN+ = CH0, VIN- = CH1/DACA
#define ADC_MUX_CHB_DIFF    0x23    // VIN+ = CH2, VIN- = CH3/DACB

// MCP4922 digital-to-analog converter (DAC) pin definitions
// Note: DAC shares SCK and MOSI lines with ADC
#define DAC_CSN             21
#define DAC_LDAC            22

void init_me_gizmo(void);

void init_adc(void);
uint8_t adc_fast_cmd(uint8_t address, uint8_t command);
uint8_t adc_read_reg_static(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t num_bytes);
uint8_t adc_read_reg_incremental(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t num_bytes);
uint8_t adc_write_reg_incremental(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t num_bytes);
uint8_t adc_get_irq(void);
uint8_t adc_get_azmux(uint8_t address);
void adc_set_azmux(uint8_t address, uint8_t azmux);
uint8_t adc_get_enoffcal(uint8_t address);
void adc_set_enoffcal(uint8_t address, uint8_t enoffcal);
uint8_t adc_get_engaincal(uint8_t address);
void adc_set_engaincal(uint8_t address, uint8_t engaincal);
uint8_t adc_get_osr(uint8_t address);
void adc_set_osr(uint8_t address, uint8_t osr);
uint8_t adc_get_gain(uint8_t address);
void adc_set_gain(uint8_t address, uint8_t gain);
uint32_t adc_read_data(uint8_t address);
uint32_t adc_get_data(uint8_t address);
uint16_t adc_get_scan_list(uint8_t address);
void adc_set_scan_list(uint8_t address, uint16_t value);
uint8_t adc_get_scan_delay(uint8_t address);
void adc_set_scan_delay(uint8_t address, uint8_t value);
uint16_t adc_scan_once(uint8_t address, uint32_t *buffer);

void init_dac_pre_init_adc(void);
void init_dac_post_init_adc(void);
void dac_update_dacA(void);
void dac_update_dacB(void);
void dac_load(void);
void dac_dacA_set_val(uint16_t value);
uint16_t dac_dacA_get_val(void);
void dac_dacA_set_ena(uint8_t ena);
uint8_t dac_dacA_get_ena(void);
void dac_dacB_set_val(uint16_t value);
uint16_t dac_dacB_get_val(void);
void dac_dacB_set_ena(uint8_t ena);
uint8_t dac_dacB_get_ena(void);
void dac_diff_set_val(int16_t value);
int16_t dac_diff_get_val(void);
void dac_diff_set_ena(uint8_t ena);
uint8_t dac_diff_get_ena(void);

#endif
