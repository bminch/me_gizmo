#include "me_gizmo.h"
#include <SPI.h>

SPISettings ADC_SPISettings(2000000, MSBFIRST, SPI_MODE0);

uint16_t dacA_val = 0x800, dacB_val = 0x800, dacC_val = 0x800, dacD_val = 0x800;
uint8_t dacA_ena = 0, dacB_ena = 0, dacC_ena = 0, dacD_ena = 0;

uint16_t adc_scan_lists[4] = { 0, 0, 0, 0 };

void init_me_gizmo(void) {
    pinMode(DEBUG_TIMING_PIN, OUTPUT);
    digitalWrite(DEBUG_TIMING_PIN, LOW);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    init_dac_pre_init_adc();
    init_adc();
    init_dac_post_init_adc();

    EEPROM.begin(EEPROM_LENGTH);
}

// Analog-to-digital converter (ADC) functinons
void init_adc(void) {
    uint8_t buffer[5];

    pinMode(ADC_IRQ_, INPUT);

    pinMode(ADC_CSN, OUTPUT);
    digitalWrite(ADC_CSN, HIGH);

    SPI.setSCK(ADC_SCK);
    SPI.setMISO(ADC_MISO);
    SPI.setMOSI(ADC_MOSI);

    SPI.begin();

    // Initialize ADC configuration and IRQ registers (see pp. 91-95 of the MCP3561/2/4R datasheet)
    buffer[0] = 0x42;   // CONFIG0: VREF_SEL = 0 (external reference), 
                        //          CLK_SEL[1:0] = 00 (external clock), 
                        //          CS_SEL[1:0] = 00 (no input current sources), 
                        //          ADC_MODE[1:0] = 10 (standby mode)
    buffer[1] = 0x0C;   // CONFIG1: PRE[1:0] = 00 (AMCLK prescaler value = 1), 
                        //          OSR[3:0] = 0011 (OSR = 256)
    buffer[2] = 0x89;   // CONFIG2: BOOST[1:0] = 10 (normal ADC bias current level), 
                        //          GAIN[2:0] = 001 (gain = 1), 
                        //          AZ_MUX = 0 (ADC auto-zeroing algorithm is disabled), 
                        //          AZ_REF = 0 (internal voltage reference auto-zeroing is disabled)
    buffer[3] = 0x80;   // CONFIG3: CONV_MODE[1:0] = 10 (one-shot conversion, ADC_MODE[1:0] set to 10 (standby) at end of conversion cycle), 
                        //          DATA_FORMAT[1:0] = 00 (24-bit ADC data), 
                        //          CRC_FORMAT = 0 (16-bit CRC), 
                        //          EN_CRCCOM = 0 (CRC on communications disabled), 
                        //          EN_OFFCAL = 0 (digital offset calibration disabled), 
                        //          EN_GAINCAL = 0 (digital gain calibration disabled)
    buffer[4] = 0x06;   // IRQ:     IRQ_MODE[1:0] = 01 (IRQ output is selected, inactive state is logic high), 
                        //          EN_FASTCMD = 1 (fast commands are enabled), 
                        //          EN_STP = 0 (conversion start interrupt output disabled)
    adc_write_reg_incremental(1, ADC_CONFIG0, buffer, 5);
}

uint8_t adc_fast_cmd(uint8_t address, uint8_t command) {
    uint8_t status;

    SPI.beginTransaction(ADC_SPISettings);

    digitalWrite(ADC_CSN, LOW);

    status = SPI.transfer(((address & 0x3) << 6) | ((command & 0xF) << 2));

    digitalWrite(ADC_CSN, HIGH);

    SPI.endTransaction();

    return status;
}

uint8_t adc_read_reg_static(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t num_bytes) {
    uint8_t status, i;

    SPI.beginTransaction(ADC_SPISettings);

    digitalWrite(ADC_CSN, LOW);

    status = SPI.transfer(((address & 0x3) << 6) | ((reg & 0xF) << 2) | 0x01);

    for (i = 0; i < num_bytes; i++) {
        buffer[i] = SPI.transfer(0);
    }

    digitalWrite(ADC_CSN, HIGH);

    SPI.endTransaction();

    return status;
}

uint8_t adc_read_reg_incremental(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t num_bytes) {
    uint8_t status, i;

    SPI.beginTransaction(ADC_SPISettings);

    digitalWrite(ADC_CSN, LOW);

    status = SPI.transfer(((address & 0x3) << 6) | ((reg & 0xF) << 2) | 0x03);

    for (i = 0; i < num_bytes; i++) {
        buffer[i] = SPI.transfer(0);
    }

    digitalWrite(ADC_CSN, HIGH);

    SPI.endTransaction();

    return status;
}

uint8_t adc_write_reg_incremental(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t num_bytes) {
    uint8_t status, i;

    SPI.beginTransaction(ADC_SPISettings);

    digitalWrite(ADC_CSN, LOW);

    status = SPI.transfer(((address & 0x3) << 6) | ((reg & 0xF) << 2) | 0x02);

    for (i = 0; i < num_bytes; i++) {
        SPI.transfer(buffer[i]);
    }

    digitalWrite(ADC_CSN, HIGH);

    SPI.endTransaction();

    return status;
}

uint8_t adc_get_irq(void) {
    return (uint8_t)digitalRead(ADC_IRQ_);
}

uint8_t adc_get_azmux(uint8_t address) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG2, &val, 1);
    return (val >> 2) & 0x01;
}

void adc_set_azmux(uint8_t address, uint8_t azmux) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG2, &val, 1);
    val = (val & 0xFB) | ((azmux ? 0x01 : 0x00) << 2);
    adc_write_reg_incremental(address, ADC_CONFIG2, &val, 1);
}

uint8_t adc_get_enoffcal(uint8_t address) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG3, &val, 1);
    return (val >> 1) & 0x01;
}

void adc_set_enoffcal(uint8_t address, uint8_t enoffcal) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG3, &val, 1);
    val = (val & 0xFD) | ((enoffcal ? 0x01 : 0x00) << 1);
    adc_write_reg_incremental(address, ADC_CONFIG3, &val, 1);
}

uint8_t adc_get_engaincal(uint8_t address) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG3, &val, 1);
    return val & 0x01;
}

void adc_set_engaincal(uint8_t address, uint8_t engaincal) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG3, &val, 1);
    val = (val & 0xFE) | (engaincal ? 0x01 : 0x00);
    adc_write_reg_incremental(address, ADC_CONFIG3, &val, 1);
}

uint8_t adc_get_osr(uint8_t address) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG1, &val, 1);
    return (val >> 2) & 0x0F;
}

void adc_set_osr(uint8_t address, uint8_t osr) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG1, &val, 1);
    val = (val & 0xC3) | ((osr & 0x0F) << 2);
    adc_write_reg_incremental(address, ADC_CONFIG1, &val, 1);
}

uint8_t adc_get_gain(uint8_t address) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG2, &val, 1);
    return (val >> 3) & 0x07;
}

void adc_set_gain(uint8_t address, uint8_t gain) {
    uint8_t val;

    adc_read_reg_incremental(address, ADC_CONFIG2, &val, 1);
    val = (val & 0xC7) | ((gain & 0x07) << 3);
    adc_write_reg_incremental(address, ADC_CONFIG2, &val, 1);
}

uint32_t adc_read_data(uint8_t address) {
    uint8_t buffer[3];

    adc_read_reg_incremental(address, ADC_ADCDATA, buffer, 3);
    return ((buffer[0] & 0x80) ? 0xFF000000 : 0) | ((uint32_t)buffer[0] << 16) | ((uint32_t)buffer[1] << 8) | (uint32_t)buffer[2];
}

uint32_t adc_get_data(uint8_t address) {
    adc_fast_cmd(address, ADC_START_CONV);

    while (adc_get_irq() == 1) {}

    return adc_read_data(address);
}

uint16_t adc_get_scan_list(uint8_t address) {
    return adc_scan_lists[address & 0x03];
}

void adc_set_scan_list(uint8_t address, uint16_t value) {
    adc_scan_lists[address & 0x03] = value;
}

uint8_t adc_get_scan_delay(uint8_t address) {
    uint8_t buffer[3];

    adc_read_reg_incremental(address, ADC_SCAN, buffer, 3);
    return buffer[0] >> 5;
}

void adc_set_scan_delay(uint8_t address, uint8_t value) {
    uint8_t buffer[3];

    adc_read_reg_incremental(address, ADC_SCAN, buffer, 3);
    buffer[0] = (buffer[0] & 0x1F) | (value << 5);
    adc_write_reg_incremental(address, ADC_SCAN, buffer, 3);
}

uint16_t adc_scan_once(uint8_t address, uint32_t *buffer) {
    uint16_t scan_list, channel_count, i;
    uint8_t scan_reg_val[3];

    scan_list = adc_scan_lists[address];

    // Count the number of enabled channels in the scan list
    i = scan_list;
    channel_count = 0;
    while (i != 0) {
        i &= i - 1;     // Clear the least-significant set bit
        channel_count++;
    }
    if (channel_count == 0) {
        return channel_count;
    }

    // Write scan list to the ADC SCAN register
    adc_read_reg_incremental(address, ADC_SCAN, scan_reg_val, 3);
    scan_reg_val[1] = (uint8_t)(scan_list >> 8);
    scan_reg_val[2] = (uint8_t)(scan_list & 0xFF);
    adc_write_reg_incremental(address, ADC_SCAN, scan_reg_val, 3);

    // Initiate scan
    adc_fast_cmd(address, ADC_START_CONV);

    // Read the converted value on each enabled channel when it is ready
    for (i = 0; i < channel_count; i++) {
        while (adc_get_irq() == 1) {}
        buffer[i] = adc_read_data(address);
    }

    // Clear the scan list in the ADC SCAN register to return ADC to MUX mode
    scan_reg_val[1] = 0;
    scan_reg_val[2] = 0;
    adc_write_reg_incremental(address, ADC_SCAN, scan_reg_val, 3);

    return channel_count;
}

// Digital-to-analog converter (DAC) functions
void init_dac_pre_init_adc(void) {
    pinMode(DAC1_CSN, OUTPUT);
    digitalWrite(DAC1_CSN, HIGH);

    pinMode(DAC2_CSN, OUTPUT);
    digitalWrite(DAC2_CSN, HIGH);

    pinMode(DAC_LDAC, OUTPUT);
    digitalWrite(DAC_LDAC, HIGH);
}

void init_dac_post_init_adc(void) {
    dac_update_dacA();
    dac_update_dacB();
    dac_update_dacC();
    dac_update_dacD();
    dac_load();
}

void dac_update_dacA(void) {
    SPI.beginTransaction(ADC_SPISettings);

    digitalWrite(DAC1_CSN, LOW);

    SPI.transfer((dacA_ena ? 0x70 : 0x60) | (dacA_val >> 8));
    SPI.transfer(dacA_val & 0xFF);

    digitalWrite(DAC1_CSN, HIGH);

    SPI.endTransaction();
}

void dac_update_dacB(void) {
    SPI.beginTransaction(ADC_SPISettings);

    digitalWrite(DAC1_CSN, LOW);

    SPI.transfer((dacB_ena ? 0xF0 : 0xE0) | (dacB_val >> 8));
    SPI.transfer(dacB_val & 0xFF);

    digitalWrite(DAC1_CSN, HIGH);

    SPI.endTransaction();
}

void dac_update_dacC(void) {
    SPI.beginTransaction(ADC_SPISettings);

    digitalWrite(DAC2_CSN, LOW);

    SPI.transfer((dacC_ena ? 0x70 : 0x60) | (dacC_val >> 8));
    SPI.transfer(dacC_val & 0xFF);

    digitalWrite(DAC2_CSN, HIGH);

    SPI.endTransaction();
}

void dac_update_dacD(void) {
    SPI.beginTransaction(ADC_SPISettings);

    digitalWrite(DAC2_CSN, LOW);

    SPI.transfer((dacD_ena ? 0xF0 : 0xE0) | (dacD_val >> 8));
    SPI.transfer(dacD_val & 0xFF);

    digitalWrite(DAC2_CSN, HIGH);

    SPI.endTransaction();
}

void dac_load(void) {
    digitalWrite(DAC_LDAC, LOW);
    digitalWrite(DAC_LDAC, HIGH);
}

void dac_dacA_set_val(uint16_t value) {
    dacA_val = value & 0x0FFF;
    dac_update_dacA();
    dac_load();
}

uint16_t dac_dacA_get_val(void) {
    return dacA_val;
}

void dac_dacA_set_ena(uint8_t ena) {
    dacA_ena = ena ? 1 : 0;
    dac_update_dacA();
    dac_load();
}

uint8_t dac_dacA_get_ena(void) {
    return dacA_ena;
}

void dac_dacB_set_val(uint16_t value) {
    dacB_val = value & 0x0FFF;
    dac_update_dacB();
    dac_load();
}

uint16_t dac_dacB_get_val(void) {
    return dacB_val;
}

void dac_dacB_set_ena(uint8_t ena) {
    dacB_ena = ena ? 1 : 0;
    dac_update_dacB();
    dac_load();
}

uint8_t dac_dacB_get_ena(void) {
    return dacB_ena;
}

void dac_dacC_set_val(uint16_t value) {
    dacC_val = value & 0x0FFF;
    dac_update_dacC();
    dac_load();
}

uint16_t dac_dacC_get_val(void) {
    return dacC_val;
}

void dac_dacC_set_ena(uint8_t ena) {
    dacC_ena = ena ? 1 : 0;
    dac_update_dacC();
    dac_load();
}

uint8_t dac_dacC_get_ena(void) {
    return dacC_ena;
}

void dac_dacD_set_val(uint16_t value) {
    dacD_val = value & 0x0FFF;
    dac_update_dacD();
    dac_load();
}

uint16_t dac_dacD_get_val(void) {
    return dacD_val;
}

void dac_dacD_set_ena(uint8_t ena) {
    dacD_ena = ena ? 1 : 0;
    dac_update_dacD();
    dac_load();
}

uint8_t dac_dacD_get_ena(void) {
    return dacD_ena;
}

void dac_diffAB_set_val(int16_t value) {
    dacA_val = (uint16_t)(((0x1000 + value) >> 1) & 0xFFF);
    dacB_val = (uint16_t)(((0x1000 - value) >> 1) & 0xFFF);
    dac_update_dacA();
    dac_update_dacB();
    dac_load();
}

int16_t dac_diffAB_get_val(void) {
    return (int16_t)dacA_val - (int16_t)dacB_val;
}

void dac_diffAB_set_ena(uint8_t ena) {
    dacA_ena = ena ? 1 : 0;
    dacB_ena = ena ? 1 : 0;
    dac_update_dacA();
    dac_update_dacB();
    dac_load();
}

uint8_t dac_diffAB_get_ena(void) {
    return dacA_ena | dacB_ena;
}

void dac_diffCD_set_val(int16_t value) {
    dacC_val = (uint16_t)(((0x1000 + value) >> 1) & 0xFFF);
    dacD_val = (uint16_t)(((0x1000 - value) >> 1) & 0xFFF);
    dac_update_dacC();
    dac_update_dacD();
    dac_load();
}

int16_t dac_diffCD_get_val(void) {
    return (int16_t)dacC_val - (int16_t)dacD_val;
}

void dac_diffCD_set_ena(uint8_t ena) {
    dacC_ena = ena ? 1 : 0;
    dacD_ena = ena ? 1 : 0;
    dac_update_dacC();
    dac_update_dacD();
    dac_load();
}

uint8_t dac_diffCD_get_ena(void) {
    return dacC_ena | dacD_ena;
}
