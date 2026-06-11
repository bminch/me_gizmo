#include "parser.h"
#include "me_gizmo.h"

#define CMD_BUFFER_LENGTH   512

STATE_HANDLER_T parser_state, parser_last_state, parser_task;
char cmd_buffer[CMD_BUFFER_LENGTH], *cmd_buffer_pos;
uint16_t cmd_buffer_left;

void parser_normal(void);

uint32_t blink_time;
uint16_t blink_interval = 500;

void blink_task(void);

uint32_t adc_stream_time;
uint16_t adc_stream_interval = 200;

uint16_t adc_stream_scan_list, adc_stream_channel_count, adc_stream_index;
uint8_t adc_stream_scan_reg_val[3];
uint32_t adc_stream_buffer[16];

void adc_stream_task(void);

void ui_handler(char *args);
void adc_handler(char *args);
void dac_handler(char *args);
void eeprom_handler(char *args);
void idnQ_handler(char *args);

DISPATCH_ENTRY_T root_table[] = {{ "UI", ui_handler }, 
                                 { "ADC", adc_handler }, 
                                 { "DAC", dac_handler }, 
                                 { "EEPROM", eeprom_handler }, 
                                 { "*IDN?", idnQ_handler }};

#define ROOT_TABLE_ENTRIES      sizeof(root_table) / sizeof(DISPATCH_ENTRY_T)

void led_handler(char *args);
void ledQ_handler(char *args);
void blink_handler(char *args);
void bootselQ_handler(char *args);

DISPATCH_ENTRY_T ui_table[] = {{ "LED", led_handler }, 
                               { "LED?", ledQ_handler }, 
                               { "BLINK", blink_handler }, 
                               { "BOOTSEL?", bootselQ_handler }};

#define UI_TABLE_ENTRIES        sizeof(ui_table) / sizeof(DISPATCH_ENTRY_T)

void blink_interval_handler(char *args);
void blink_intervalQ_handler(char *args);
void blink_start_handler(char *args);
void blink_stop_handler(char *args);

DISPATCH_ENTRY_T blink_table[] = {{ "INTERVAL", blink_interval_handler }, 
                                  { "INTERVAL?", blink_intervalQ_handler }, 
                                  { "START", blink_start_handler }, 
                                  { "STOP", blink_stop_handler }};

#define BLINK_TABLE_ENTRIES     sizeof(blink_table) / sizeof(DISPATCH_ENTRY_T)

void adc_cmd_handler(char *args);
void adc_read_reg_handler(char *args);
void adc_write_reg_handler(char *args);
void adc_irqQ_handler(char *args);
void adc_azmux_handler(char *args);
void adc_azmuxQ_handler(char *args);
void adc_enoffcal_handler(char *args);
void adc_enoffcalQ_handler(char *args);
void adc_engaincal_handler(char *args);
void adc_engaincalQ_handler(char *args);
void adc_osr_handler(char *args);
void adc_osrQ_handler(char *args);
void adc_gain_handler(char *args);
void adc_gainQ_handler(char *args);
void adc_dataQ_handler(char *args);
void adc_scan_handler(char *args);
void adc_scanQ_handler(char *args);
void adc_stream_handler(char *args);

DISPATCH_ENTRY_T adc_table[] = {{ "CMD", adc_cmd_handler }, 
                                { "RD_REG", adc_read_reg_handler }, 
                                { "WR_REG", adc_write_reg_handler }, 
                                { "IRQ?", adc_irqQ_handler }, 
                                { "AZMUX", adc_azmux_handler }, 
                                { "AZMUX?", adc_azmuxQ_handler }, 
                                { "ENOFFCAL", adc_enoffcal_handler }, 
                                { "ENOFFCAL?", adc_enoffcalQ_handler }, 
                                { "ENGAINCAL", adc_engaincal_handler }, 
                                { "ENGAINCAL?", adc_engaincalQ_handler }, 
                                { "OSR", adc_osr_handler }, 
                                { "OSR?", adc_osrQ_handler }, 
                                { "GAIN", adc_gain_handler }, 
                                { "GAIN?", adc_gainQ_handler }, 
                                { "DATA?", adc_dataQ_handler }, 
                                { "SCAN", adc_scan_handler }, 
                                { "SCAN?", adc_scanQ_handler }, 
                                { "STREAM", adc_stream_handler }};

#define ADC_TABLE_ENTRIES       sizeof(adc_table) / sizeof(DISPATCH_ENTRY_T)

void adc_scan_list_handler(char *args);
void adc_scan_listQ_handler(char *args);
void adc_scan_delay_handler(char *args);
void adc_scan_delayQ_handler(char *args);

DISPATCH_ENTRY_T adc_scan_table[] = {{ "LIST", adc_scan_list_handler }, 
                                     { "LIST?", adc_scan_listQ_handler }, 
                                     { "DELAY", adc_scan_delay_handler }, 
                                     { "DELAY?", adc_scan_delayQ_handler }};

#define ADC_SCAN_TABLE_ENTRIES  sizeof(adc_scan_table) / sizeof(DISPATCH_ENTRY_T)

void adc_stream_interval_handler(char *args);
void adc_stream_intervalQ_handler(char *args);
void adc_stream_start_handler(char *args);
void adc_stream_stop_handler(char *args);

DISPATCH_ENTRY_T adc_stream_table[] = {{ "INTERVAL", adc_stream_interval_handler }, 
                                       { "INTERVAL?", adc_stream_intervalQ_handler }, 
                                       { "START", adc_stream_start_handler }, 
                                       { "STOP", adc_stream_stop_handler }};

#define ADC_STREAM_TABLE_ENTRIES    sizeof(adc_stream_table) / sizeof(DISPATCH_ENTRY_T)

void dac_dacA_handler(char *args);
void dac_dacB_handler(char *args);
void dac_dacC_handler(char *args);
void dac_dacD_handler(char *args);
void dac_diffAB_handler(char *args);
void dac_diffCD_handler(char *args);

DISPATCH_ENTRY_T dac_table[] = {{ "DACA", dac_dacA_handler }, 
                                { "DACB", dac_dacB_handler }, 
                                { "DACC", dac_dacC_handler }, 
                                { "DACD", dac_dacD_handler }, 
                                { "DIFFAB", dac_diffAB_handler }, 
                                { "DIFFCD", dac_diffCD_handler }};

#define DAC_TABLE_ENTRIES       sizeof(dac_table) / sizeof(DISPATCH_ENTRY_T)

void dac_dacA_value_handler(char *args);
void dac_dacA_valueQ_handler(char *args);
void dac_dacA_ena_handler(char *args);
void dac_dacA_enaQ_handler(char *args);

DISPATCH_ENTRY_T dac_dacA_table[] = {{ "VALUE", dac_dacA_value_handler }, 
                                     { "VALUE?", dac_dacA_valueQ_handler }, 
                                     { "ENA", dac_dacA_ena_handler }, 
                                     { "ENA?", dac_dacA_enaQ_handler }};

#define DAC_DACA_TABLE_ENTRIES  sizeof(dac_dacA_table) / sizeof(DISPATCH_ENTRY_T)

void dac_dacB_value_handler(char *args);
void dac_dacB_valueQ_handler(char *args);
void dac_dacB_ena_handler(char *args);
void dac_dacB_enaQ_handler(char *args);

DISPATCH_ENTRY_T dac_dacB_table[] = {{ "VALUE", dac_dacB_value_handler }, 
                                     { "VALUE?", dac_dacB_valueQ_handler }, 
                                     { "ENA", dac_dacB_ena_handler }, 
                                     { "ENA?", dac_dacB_enaQ_handler }};

#define DAC_DACB_TABLE_ENTRIES  sizeof(dac_dacB_table) / sizeof(DISPATCH_ENTRY_T)

void dac_dacC_value_handler(char *args);
void dac_dacC_valueQ_handler(char *args);
void dac_dacC_ena_handler(char *args);
void dac_dacC_enaQ_handler(char *args);

DISPATCH_ENTRY_T dac_dacC_table[] = {{ "VALUE", dac_dacC_value_handler }, 
                                     { "VALUE?", dac_dacC_valueQ_handler }, 
                                     { "ENA", dac_dacC_ena_handler }, 
                                     { "ENA?", dac_dacC_enaQ_handler }};

#define DAC_DACC_TABLE_ENTRIES  sizeof(dac_dacC_table) / sizeof(DISPATCH_ENTRY_T)

void dac_dacD_value_handler(char *args);
void dac_dacD_valueQ_handler(char *args);
void dac_dacD_ena_handler(char *args);
void dac_dacD_enaQ_handler(char *args);

DISPATCH_ENTRY_T dac_dacD_table[] = {{ "VALUE", dac_dacD_value_handler }, 
                                     { "VALUE?", dac_dacD_valueQ_handler }, 
                                     { "ENA", dac_dacD_ena_handler }, 
                                     { "ENA?", dac_dacD_enaQ_handler }};

#define DAC_DACD_TABLE_ENTRIES  sizeof(dac_dacD_table) / sizeof(DISPATCH_ENTRY_T)

void dac_diffAB_value_handler(char *args);
void dac_diffAB_valueQ_handler(char *args);
void dac_diffAB_ena_handler(char *args);
void dac_diffAB_enaQ_handler(char *args);

DISPATCH_ENTRY_T dac_diffAB_table[] = {{ "VALUE", dac_diffAB_value_handler }, 
                                       { "VALUE?", dac_diffAB_valueQ_handler }, 
                                       { "ENA", dac_diffAB_ena_handler }, 
                                       { "ENA?", dac_diffAB_enaQ_handler }};

#define DAC_DIFFAB_TABLE_ENTRIES    sizeof(dac_diffAB_table) / sizeof(DISPATCH_ENTRY_T)

void dac_diffCD_value_handler(char *args);
void dac_diffCD_valueQ_handler(char *args);
void dac_diffCD_ena_handler(char *args);
void dac_diffCD_enaQ_handler(char *args);

DISPATCH_ENTRY_T dac_diffCD_table[] = {{ "VALUE", dac_diffCD_value_handler }, 
                                       { "VALUE?", dac_diffCD_valueQ_handler }, 
                                       { "ENA", dac_diffCD_ena_handler }, 
                                       { "ENA?", dac_diffCD_enaQ_handler }};

#define DAC_DIFFCD_TABLE_ENTRIES    sizeof(dac_diffCD_table) / sizeof(DISPATCH_ENTRY_T)

void eeprom_read_handler(char *args);
void eeprom_write_handler(char *args);
void eeprom_commit_handler(char *args);
void eeprom_lengthQ_handler(char *args);
void eeprom_erase_handler(char *args);

DISPATCH_ENTRY_T eeprom_table[] = {{ "READ", eeprom_read_handler }, 
                                   { "WRITE", eeprom_write_handler }, 
                                   { "COMMIT", eeprom_commit_handler }, 
                                   { "LENGTH?", eeprom_lengthQ_handler }, 
                                   { "ERASE", eeprom_erase_handler }};

#define EEPROM_TABLE_ENTRIES    sizeof(eeprom_table) / sizeof(DISPATCH_ENTRY_T)

// Parser utility functions
bool it_is_time(uint32_t t, uint32_t t0, uint16_t dt) {
    return ((t >= t0) && (t - t0 >= dt)) ||         // The first disjunct handles the normal case
           ((t < t0) && (t + (~t0) + 1 >= dt));     //   while the second handles the overflow case
}

int16_t str2hex(char *str, uint16_t *num) {
    if (!str)
        return -1;

    while ((*str == ' ') || (*str == '\t'))
        str++;

    *num = 0;
    while (*str) {
        if ((*str >= '0') && (*str <= '9'))
            *num = (*num << 4) + (*str - '0');
        else if ((*str >= 'a') && (*str <= 'f'))
            *num = (*num << 4) + 10 + (*str - 'a');
        else if ((*str >= 'A') && (*str <= 'F'))
            *num = (*num << 4) + 10 + (*str - 'A');
        else
            return -1;
        str++;
    }
    return 0;
}

int16_t str2num(char *str, uint16_t *num) {
    if (!str)
        return -1;

    while ((*str == ' ') || (*str == '\t'))
        str++;

    *num = 0;
    while (*str) {
        if ((*str >= '0') && (*str <= '9'))
            *num = *num * 10 + (*str - '0');
        else
            return -1;
        str++;
    }
    return 0;
}

void hex2str(uint16_t num, char *str) {
    uint16_t digit, i;

    for (i = 0; i < 4; i++) {
        digit = num >> 12;
        if (digit < 10)
            *str = '0' + (uint8_t)digit;
        else
            *str = 'A' + (uint8_t)digit - 10;
        str++;
        num = (num & 0x0FFF) << 4;
    }
    *str = '\0';
}

void hex2str_alt(uint16_t num, char *str) {
    uint16_t digit, i, hit_nonzero_digit = FALSE;

    for (i = 0; i < 4; i++) {
        digit = num >> 12;
        if (digit)
            hit_nonzero_digit = TRUE;
        if ((hit_nonzero_digit) || (i == 3)) {
            if (digit < 10)
                *str = '0' + (uint8_t)digit;
            else
                *str = 'A' + (uint8_t)digit - 10;
            str++;
        }
        num = (num & 0x0FFF) << 4;
    }
    *str = '\0';
}

int16_t str2hex32(char *str, uint32_t *num) {
    if (!str)
        return -1;

    while ((*str == ' ') || (*str == '\t'))
        str++;

    *num = 0;
    while (*str) {
        if ((*str >= '0') && (*str <= '9'))
            *num = (*num << 4) + (*str - '0');
        else if ((*str >= 'a') && (*str <= 'f'))
            *num = (*num << 4) + 10 + (*str - 'a');
        else if ((*str >= 'A') && (*str <= 'F'))
            *num = (*num << 4) + 10 + (*str - 'A');
        else
            return -1;
        str++;
    }
    return 0;
}

int16_t str2num32(char *str, uint32_t *num) {
    if (!str)
        return -1;

    while ((*str == ' ') || (*str == '\t'))
        str++;

    *num = 0;
    while (*str) {
        if ((*str >= '0') && (*str <= '9'))
            *num = *num * 10 + (*str - '0');
        else
            return -1;
        str++;
    }
    return 0;
}

void hex2str32(uint32_t num, char *str) {
    uint32_t digit, i;

    for (i = 0; i < 8; i++) {
        digit = num >> 28;
        if (digit < 10)
            *str = '0' + (uint8_t)digit;
        else
            *str = 'A' + (uint8_t)digit - 10;
        str++;
        num = (num & 0x0FFFFFFF) << 4;
    }
    *str = '\0';
}

void hex2str32_alt(uint32_t num, char *str) {
    uint32_t digit, i, hit_nonzero_digit = FALSE;

    for (i = 0; i < 8; i++) {
        digit = num >> 28;
        if (digit)
            hit_nonzero_digit = TRUE;
        if ((hit_nonzero_digit) || (i == 7)) {
            if (digit < 10)
                *str = '0' + (uint8_t)digit;
            else
                *str = 'A' + (uint8_t)digit - 10;
            str++;
        }
        num = (num & 0x0FFFFFFF) << 4;
    }
    *str = '\0';
}

int16_t str_cmp(char *str1, char *str2) {
    while ((*str1) && (*str1 == *str2)) {
        str1++;
        str2++;
    }

    if (*str1 == *str2)
        return 0;
    else if (*str1 < *str2)
        return -1;
    else
        return 1;
}

int16_t str_ncmp(char *str1, char *str2, uint16_t n) {
    if (n == 0)
        return 0;

    while ((*str1) && (*str2) && (*str1 == *str2) && (--n)) {
        str1++;
        str2++;
    }

    if (*str1 == *str2)
        return 0;
    else if (*str1 < *str2)
        return -1;
    else
        return 1;
}

char *str_tok_r(char *str, char *delim, char **save_str) {
    char *spos, *dpos, *token_start;

    if (!(str) && !(*save_str)) 
        return (char *)NULL;

    // Find the first non-delimiter character in the string
    for (spos = (str) ? str : *save_str; *spos; spos++) {
        for (dpos = delim; *dpos; dpos++) {
            if (*spos == *dpos)
                break;
        }
        if (*dpos == '\0')
            break;
    }
    if (*spos)
        token_start = spos;
    else {
        *save_str = (char *)NULL;
        return (char *)NULL;
    }

    // Find the first delimiter character in the string
    for (; *spos; spos++) {
        for (dpos = delim; *dpos; dpos++) {
            if (*spos == *dpos)
                break;
        }
        if (*spos == *dpos)
            break;
    }
    if (*spos) {
        *spos = '\0';
        *save_str = spos + 1;
    } else {
        *save_str = (char *)NULL;
    }

    return token_start;
}

// UI commands
void ui_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < UI_TABLE_ENTRIES; i++) {
            if (str_cmp(command, ui_table[i].command) == 0) {
                ui_table[i].handler(remainder);
                break;
            }
        }
    }
}

void led_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            digitalWrite(LED_BUILTIN, HIGH);
        } else if (str_cmp(token, "OFF") == 0) {
            digitalWrite(LED_BUILTIN, LOW);
        } else if (str_cmp(token, "TOGGLE") == 0) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        } else if (str2hex(token, &val) == 0) {
            digitalWrite(LED_BUILTIN, val ? HIGH : LOW);
        }
    }
}

void ledQ_handler(char *args) {
    char str[5];

    hex2str_alt(digitalRead(LED_BUILTIN), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void blink_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < BLINK_TABLE_ENTRIES; i++) {
            if (str_cmp(command, blink_table[i].command) == 0) {
                blink_table[i].handler(remainder);
                break;
            }
        }
    }
}

void bootselQ_handler(char *args) {
    char str[5];

    hex2str_alt(BOOTSEL ? 1 : 0, str);
    Serial.print(str);
    Serial.print("\r\n");
}

// UI:BLINK commands
void blink_interval_handler(char *args) {
    char *arg, *remainder;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &blink_interval) != 0)
        return;
}

void blink_intervalQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)blink_interval, str);
    Serial.print(str);
    Serial.print("\r\n");
}

void blink_start_handler(char *args) {
    if (parser_task)
        return;

    digitalWrite(LED_BUILTIN, HIGH);
    blink_time = millis();
    parser_task = blink_task;
}

void blink_stop_handler(char *args) {
    if (parser_task != blink_task)
        return;

    digitalWrite(LED_BUILTIN, LOW);
    parser_task = (STATE_HANDLER_T)NULL;
}

void blink_task(void) {
    uint32_t t;

    t = millis();
    if (it_is_time(t, blink_time, blink_interval)) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        blink_time = t;
    }
}

// ADC commands
void adc_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < ADC_TABLE_ENTRIES; i++) {
            if (str_cmp(command, adc_table[i].command) == 0) {
                adc_table[i].handler(remainder);
                break;
            }
        }
    }
}

void adc_cmd_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, command;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &command) != 0) {
        return;
    }

    adc_fast_cmd((uint8_t)address, (uint8_t)command);
}

void adc_read_reg_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, reg, num_bytes;
    uint8_t buffer[16];
    uint16_t i;
    char str[5];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &reg) != 0) {
        Serial.print("\r\n");
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &num_bytes) != 0) {
        Serial.print("\r\n");
        return;
    }
    if (num_bytes > 16) {
        num_bytes = 16;
    }

    adc_read_reg_incremental((uint8_t)address, (uint8_t)reg, buffer, (uint8_t)num_bytes);

    for (i = 0; i < num_bytes; i++) {
        hex2str_alt((uint16_t)buffer[i], str);
        Serial.print(str);
        Serial.print((i == num_bytes - 1) ? "\r\n" : ",");
    }
}

void adc_write_reg_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, reg, val;
    uint8_t num_bytes;
    uint8_t buffer[16];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &reg) != 0) {
        return;
    }

    num_bytes = 0;
    while (num_bytes < 16) {
        arg = str_tok_r((char *)NULL, ", ", &remainder);
        if (str2hex(arg, &val) != 0) {
            break;
        }
        buffer[num_bytes++] = (uint8_t)val;
    }
    if (num_bytes == 0) {
        return;
    }

    adc_write_reg_incremental((uint8_t)address, (uint8_t)reg, buffer, num_bytes);
}

void adc_irqQ_handler(char *args) {
    Serial.print(adc_get_irq() ? "1\r\n" : "0\r\n");
}

void adc_azmux_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &val) != 0) {
        return;
    }

    adc_set_azmux((uint8_t)address, (uint8_t)val);
}

void adc_azmuxQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    Serial.print(adc_get_azmux((uint8_t)address) ? "1\r\n" : "0\r\n");
}

void adc_enoffcal_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &val) != 0) {
        return;
    }

    adc_set_enoffcal((uint8_t)address, (uint8_t)val);
}

void adc_enoffcalQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    Serial.print(adc_get_enoffcal((uint8_t)address) ? "1\r\n" : "0\r\n");
}

void adc_engaincal_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &val) != 0) {
        return;
    }

    adc_set_engaincal((uint8_t)address, (uint8_t)val);
}

void adc_engaincalQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    Serial.print(adc_get_engaincal((uint8_t)address) ? "1\r\n" : "0\r\n");
}

void adc_osr_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &val) != 0) {
        return;
    }

    adc_set_osr((uint8_t)address, (uint8_t)val);
}

void adc_osrQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address;
    char str[5];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    hex2str_alt((uint16_t)adc_get_osr((uint8_t)address), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void adc_gain_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &val) != 0) {
        return;
    }

    adc_set_gain((uint8_t)address, (uint8_t)val);
}

void adc_gainQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address;
    char str[5];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    hex2str_alt((uint16_t)adc_get_gain((uint8_t)address), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void adc_dataQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address;
    char str[9];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    hex2str32_alt(adc_get_data((uint8_t)address), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void adc_scan_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < ADC_SCAN_TABLE_ENTRIES; i++) {
            if (str_cmp(command, adc_scan_table[i].command) == 0) {
                adc_scan_table[i].handler(remainder);
                break;
            }
        }
    }
}

void adc_scanQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, channels, i;
    uint32_t buffer[16];
    char str[9];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    channels = adc_scan_once((uint8_t)address, buffer);
    if (channels == 0) {
        Serial.print("\r\n");
        return;
    }

    for (i = 0; i < channels; i++) {
        hex2str32_alt(buffer[i], str);
        Serial.print(str);
        Serial.print((i == channels - 1) ? "\r\n" : ",");
    }
}

void adc_stream_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < ADC_STREAM_TABLE_ENTRIES; i++) {
            if (str_cmp(command, adc_stream_table[i].command) == 0) {
                adc_stream_table[i].handler(remainder);
                break;
            }
        }
    }
}

// ADC:SCAN commands
void adc_scan_list_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, value;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &value) != 0) {
        return;
    }

    adc_set_scan_list((uint8_t)address, value);
}

void adc_scan_listQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address;
    char str[5];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    hex2str_alt(adc_get_scan_list((uint8_t)address), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void adc_scan_delay_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, value;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &value) != 0) {
        return;
    }

    adc_set_scan_delay((uint8_t)address, (uint8_t)value);
}

void adc_scan_delayQ_handler(char *args) {
    char *arg, *remainder;
    uint16_t address;
    char str[5];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    hex2str_alt((uint16_t)adc_get_scan_delay((uint8_t)address), str);
    Serial.print(str);
    Serial.print("\r\n");
}

// ADC:STREAM commands
void adc_stream_interval_handler(char *args) {
    char *arg, *remainder;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &adc_stream_interval) != 0)
        return;
}

void adc_stream_intervalQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)adc_stream_interval, str);
    Serial.print(str);
    Serial.print("\r\n");
}

void adc_stream_start_handler(char *args) {
    uint16_t i;

    if (parser_task)
        return;

    adc_stream_scan_list = adc_get_scan_list(1);
    if (adc_stream_scan_list != 0) {
        // Count the number of enabled channels in the scan list
        i = adc_stream_scan_list;
        adc_stream_channel_count = 0;
        while (i != 0) {
            i &= i - 1;     // Clear the least-significant set bit
            adc_stream_channel_count++;
        }

        // Write scan list to the ADC SCAN register
        adc_read_reg_incremental(1, ADC_SCAN, adc_stream_scan_reg_val, 3);
        adc_stream_scan_reg_val[1] = (uint8_t)(adc_stream_scan_list >> 8);
        adc_stream_scan_reg_val[2] = (uint8_t)(adc_stream_scan_list & 0xFF);
        adc_write_reg_incremental(1, ADC_SCAN, adc_stream_scan_reg_val, 3);

        adc_stream_index = 0;
    }
    adc_stream_time = millis();
    parser_task = adc_stream_task;
}

void adc_stream_stop_handler(char *args) {
    if (parser_task != adc_stream_task)
        return;

    parser_task = (STATE_HANDLER_T)NULL;

    if (adc_stream_scan_list != 0) {
        // Clear the scan list in the ADC SCAN register to return ADC to MUX mode
        adc_stream_scan_reg_val[1] = 0;
        adc_stream_scan_reg_val[2] = 0;
        adc_write_reg_incremental(1, ADC_SCAN, adc_stream_scan_reg_val, 3);
    }
}

void adc_stream_task(void) {
    uint32_t t;
    uint16_t i;
    char str[9];

    t = millis();
    if (it_is_time(t, adc_stream_time, adc_stream_interval)) {
        adc_stream_time = t;
        if (adc_stream_scan_list != 0) {    // SCAN mode
            adc_fast_cmd(1, ADC_START_CONV);

            for (i = 0; i < adc_stream_channel_count; i++) {
                while (adc_get_irq() == 1) {}
                adc_stream_buffer[i] = adc_read_data(1);
            }

            for (i = 0; i < adc_stream_channel_count; i++) {
                hex2str32_alt(adc_stream_buffer[i], str);
                Serial.print(str);
                Serial.print((i == adc_stream_channel_count - 1) ? "\r\n" : ",");
            }
        } else {                            // MUX mode
            adc_fast_cmd(1, ADC_START_CONV);

            while (adc_get_irq() == 1) {}

            hex2str32_alt(adc_read_data(1), str);
            Serial.print(str);
            Serial.print("\r\n");
        }
    }
}

// DAC commands
void dac_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac_table[i].command) == 0) {
                dac_table[i].handler(remainder);
                break;
            }
        }
    }
}

void dac_dacA_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC_DACA_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac_dacA_table[i].command) == 0) {
                dac_dacA_table[i].handler(remainder);
                break;
            }
        }
    }
}

void dac_dacB_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC_DACB_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac_dacB_table[i].command) == 0) {
                dac_dacB_table[i].handler(remainder);
                break;
            }
        }
    }
}

void dac_dacC_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC_DACC_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac_dacC_table[i].command) == 0) {
                dac_dacC_table[i].handler(remainder);
                break;
            }
        }
    }
}

void dac_dacD_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC_DACD_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac_dacD_table[i].command) == 0) {
                dac_dacD_table[i].handler(remainder);
                break;
            }
        }
    }
}

void dac_diffAB_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC_DIFFAB_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac_diffAB_table[i].command) == 0) {
                dac_diffAB_table[i].handler(remainder);
                break;
            }
        }
    }
}

void dac_diffCD_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC_DIFFCD_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac_diffCD_table[i].command) == 0) {
                dac_diffCD_table[i].handler(remainder);
                break;
            }
        }
    }
}

// DAC:DACA commands
void dac_dacA_value_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_dacA_set_val(val);
}

void dac_dacA_valueQ_handler(char *args) {
    char str[5];

    hex2str_alt(dac_dacA_get_val(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void dac_dacA_ena_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_dacA_set_ena((uint8_t)val);
}

void dac_dacA_enaQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)dac_dacA_get_ena(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

// DAC:DACB commands
void dac_dacB_value_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_dacB_set_val(val);
}

void dac_dacB_valueQ_handler(char *args) {
    char str[5];

    hex2str_alt(dac_dacB_get_val(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void dac_dacB_ena_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_dacB_set_ena((uint8_t)val);
}

void dac_dacB_enaQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)dac_dacB_get_ena(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

// DAC:DACC commands
void dac_dacC_value_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_dacC_set_val(val);
}

void dac_dacC_valueQ_handler(char *args) {
    char str[5];

    hex2str_alt(dac_dacC_get_val(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void dac_dacC_ena_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_dacC_set_ena((uint8_t)val);
}

void dac_dacC_enaQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)dac_dacC_get_ena(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

// DAC:DACD commands
void dac_dacD_value_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_dacD_set_val(val);
}

void dac_dacD_valueQ_handler(char *args) {
    char str[5];

    hex2str_alt(dac_dacD_get_val(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void dac_dacD_ena_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_dacD_set_ena((uint8_t)val);
}

void dac_dacD_enaQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)dac_dacD_get_ena(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

// DAC:DIFFAB commands
void dac_diffAB_value_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_diffAB_set_val((int16_t)val);
}

void dac_diffAB_valueQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)dac_diffAB_get_val(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void dac_diffAB_ena_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_diffAB_set_ena((uint8_t)val);
}

void dac_diffAB_enaQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)dac_diffAB_get_ena(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

// DAC:DIFFCD commands
void dac_diffCD_value_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_diffCD_set_val((int16_t)val);
}

void dac_diffCD_valueQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)dac_diffCD_get_val(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void dac_diffCD_ena_handler(char *args) {
    char *arg, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val) != 0)
        return;

    dac_diffCD_set_ena((uint8_t)val);
}

void dac_diffCD_enaQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)dac_diffCD_get_ena(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

// EEPROM commands
void eeprom_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < EEPROM_TABLE_ENTRIES; i++) {
            if (str_cmp(command, eeprom_table[i].command) == 0) {
                eeprom_table[i].handler(remainder);
                break;
            }
        }
    }
}

void eeprom_read_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, num_bytes, i;
    char str[5];

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        Serial.print("\r\n");
        return;
    }

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &num_bytes) != 0) {
        Serial.print("\r\n");
        return;
    }

    for (i = 0; (i < num_bytes) && (address + i < EEPROM_LENGTH); i++) {
        hex2str_alt((uint16_t)EEPROM.read(address + i), str);
        Serial.print(str);
        Serial.print((i < num_bytes - 1) && (address + i < EEPROM_LENGTH - 1) ? "," : "\r\n");
    }
}

void eeprom_write_handler(char *args) {
    char *arg, *remainder;
    uint16_t address, val;
    uint8_t num_bytes;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &address) != 0) {
        return;
    }

    if (address >= EEPROM_LENGTH) {
        return;
    }

    num_bytes = 0;
    while (address + num_bytes < EEPROM_LENGTH) {
        arg = str_tok_r((char *)NULL, ", ", &remainder);
        if (str2hex(arg, &val) != 0) {
            break;
        }
        EEPROM.write(address + num_bytes, (uint8_t)val);
        num_bytes++;
    }
}

void eeprom_commit_handler(char *args) {
    EEPROM.commit();
}

void eeprom_lengthQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)EEPROM.length(), str);
    Serial.print(str);
    Serial.print("\r\n");
}

void eeprom_erase_handler(char *args) {
    uint16_t i;

    for (i = 0; i < EEPROM_LENGTH; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
}

// *IDN? command
void idnQ_handler(char *args) {
    uint8_t i, len;
    char str[256];

    len = EEPROM.read(0);
    if (len == 0xFF) {
        Serial.print("\r\n");
        return;
    }

    for (i = 0; i < len; i++) {
        str[i] = (char)EEPROM.read(i + 1);
    }
    str[len] = '\0';
    Serial.print(str);
    Serial.print("\r\n");
}

// Parser public methods
void init_parser(void) {
    cmd_buffer_pos = cmd_buffer;
    cmd_buffer_left = CMD_BUFFER_LENGTH;

    parser_state = parser_normal;
    parser_last_state = (STATE_HANDLER_T)NULL;
    parser_task = (STATE_HANDLER_T)NULL;

    Serial.begin(115200);
}

void parser_normal(void) {
    uint8_t ch;
    uint16_t i;
    char *command, *remainder;

    if (parser_state != parser_last_state) {
        parser_last_state = parser_state;
        cmd_buffer_pos = cmd_buffer;
        cmd_buffer_left = CMD_BUFFER_LENGTH;
    }

    if (parser_task)
        parser_task();

    if (Serial.available()) {
        ch = (uint8_t)Serial.read();
        if (cmd_buffer_left == 1) {
            cmd_buffer_pos = cmd_buffer;
            cmd_buffer_left = CMD_BUFFER_LENGTH;

            *cmd_buffer_pos++ = ch;
            cmd_buffer_left--;
        } else if (ch == '\r') {
            *cmd_buffer_pos = '\0';

//            Serial.print('[');
//            Serial.print(cmd_buffer);
//            Serial.print("]\r\n");

            remainder = (char *)NULL;
            command = str_tok_r(cmd_buffer, ":, ", &remainder);
            if (command) {
                for (i = 0; i < ROOT_TABLE_ENTRIES; i++) {
                    if (str_cmp(command, root_table[i].command) == 0) {
                        root_table[i].handler(remainder);
                        break;
                    }
                }
            }

            cmd_buffer_pos = cmd_buffer;
            cmd_buffer_left = CMD_BUFFER_LENGTH;
        } else {
            *cmd_buffer_pos++ = ch;
            cmd_buffer_left--;
        }
    }

    if (parser_state != parser_last_state) {
        parser_task = (STATE_HANDLER_T)NULL;
    }
}
