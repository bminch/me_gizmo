import copy
import serial
import serial.tools.list_ports as list_ports
import string, array


class me_gizmo:

    MCLK_FREQ = 19.6608e6

    VREF = 2.5

    GAINS = (1.0 / 3.0, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0)
    nominal_adc_slopes = [
        VREF / GAINS[0] / 2**23,
        VREF / GAINS[1] / 2**23,
        VREF / GAINS[2] / 2**23,
        VREF / GAINS[3] / 2**23,
        VREF / GAINS[4] / 2**23,
        VREF / GAINS[5] / 2**23,
        VREF / GAINS[6] / 2**23,
        VREF / GAINS[7] / 2**23,
    ]
    nominal_adc_offsets = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]

    nominal_dacA_slope = 2**12 / VREF
    nominal_dacA_offset = 0.0

    nominal_dacB_slope = 2**12 / VREF
    nominal_dacB_offset = 0.0

    nominal_dacC_slope = 2**12 / VREF
    nominal_dacC_offset = 0.0

    nominal_dacD_slope = 2**12 / VREF
    nominal_dacD_offset = 0.0

    nominal_diffAB_slope = 2**12 / VREF
    nominal_diffAB_offset = 0.0

    nominal_diffCD_slope = 2**12 / VREF
    nominal_diffCD_offset = 0.0

    def __init__(self, port=""):
        self.nominal_calib_values()

        # MCP3564R ADC register address definitions (see Table 8-1 on p. 89 of the MCP3561/2/4R datasheet)
        self.ADCDATA = 0  # Latest ADC conversion data output value (3 or 4 bytes depending on DATA_FORMAT[1:0])
        self.CONFIG0 = 1  # ADC operating mode, Master clock mode and input bias current source mode (1 byte)
        self.CONFIG1 = 2  # Prescale and OSR settings (1 byte)
        self.CONFIG2 = 3  # ADC boost and gain settings, auto-zeroing settings for analog mux, voltage regerence and ADC (1 byte)
        self.CONFIG3 = 4  # Conversion mode, data and CRC format settings, enable for CRC on communications, enable for digital offset and gain calibrations (1 byte)
        self.IRQ = 5  # IRQ status bits and mode settings, enable for fast commands and for conversion start pulse (1 byte)
        self.MUX = 6  # Analog mux input selection (1 byte)
        self.SCAN = 7  # SCAN mode settings (3 bytes)
        self.TIMER = 8  # Delay value for TIMER between SCAN cycles (3 bytes)
        self.OFFSETCAL = 9  # ADC digital offset calibration value (3 bytes)
        self.GAINCAL = 10  # ADC digital gain calibration value (3 bytes)
        self.RESERVED1 = 11  # Reserved (3 bytes)
        self.RESERVED2 = 12  # Reserved (1 byte)
        self.LOCK = 13  # Password value for SPI write mode locking (1 byte)
        self.RESERVED3 = 14  # Reserved (2 bytes)
        self.CRCCFG = 15  # CRC checksum for device configuration (2 bytes)

        # MCP3564R ADC fast command definitions (see Table 6-2 on p. 68 of the MCP3561/2/4R datasheet)
        self.START_CONV = 10
        self.STANDBY = 11
        self.SHUTDOWN = 12
        self.FULL_SHUTDOWN = 13
        self.FULL_RESET = 14

        # MCP3564R ADC input multiplexer register values (see Secion 8.7 on p. 96 of the MCP3561/2/4R datasheet)
        self.MUX_VREF = 0xB8  # VIN+ = REFIN+/OUT, VIN- = AGND
        self.MUX_NEG_VREF = 0x8B  # VIN+ = AGND, VIN- = REFIN+/OUT
        self.MUX_DIFFAB = 0x13  # VIN+ = CH1/DACA, VIN- = CH3/DACB
        self.MUX_DIFFCD = 0x57  # VIN+ = CH5/DACC, VIN- = CH7/DACD
        self.MUX_CH0_SE = 0x08  # VIN+ = CH0, VIN- = AGND
        self.MUX_CH1_SE = 0x18  # VIN+ = CH1/DACA, VIN- = AGND
        self.MUX_CH2_SE = 0x28  # VIN+ = CH2, VIN- = AGND
        self.MUX_CH3_SE = 0x38  # VIN+ = CH3/DACB, VIN- = AGND
        self.MUX_CH4_SE = 0x48  # VIN+ = CH4, VIN- = AGND
        self.MUX_CH5_SE = 0x58  # VIN+ = CH5/DACC, VIN- = AGND
        self.MUX_CH6_SE = 0x68  # VIN+ = CH6, VIN- = AGND
        self.MUX_CH7_SE = 0x78  # VIN+ = CH7/DACD, VIN- = AGND
        self.MUX_CHA_DIFF = 0x01  # VIN+ = CH0, VIN- = CH1/DACA
        self.MUX_CHB_DIFF = 0x23  # VIN+ = CH2, VIN- = CH3/DACB
        self.MUX_CHC_DIFF = 0x45  # VIN+ = CH4, VIN- = CH5/DACC
        self.MUX_CHD_DIFF = 0x67  # VIN+ = CH6, VIN- = CH7/DACD

        self.adc_gain = 1

        if port == "":
            self.dev = None
            self.connected = False
            devices = list_ports.comports()
            for device in devices:
                if device.vid == 0x2E8A and (
                    device.pid == 0xF00A or device.pid == 0xF00F
                ):
                    try:
                        self.dev = serial.Serial(device.device, 115200)
                        self.connected = True
                        print(f"Connected to {device.device}...")
                    except:
                        pass
                if self.connected:
                    break
        else:
            try:
                self.dev = serial.Serial(port)
                self.connected = True
            except:
                self.dev = None
                self.connected = False

        if self.connected:
            self.write("")
            self.adc_gain = self.adc_get_gain()
            self.read_calib_values()

    def write(self, command):
        if not self.connected:
            return
        self.dev.write(f"{command}\r".encode())

    def read(self):
        if not self.connected:
            return
        return self.dev.readline().decode()

    #
    # UI methods
    #

    def toggle_led(self):
        if not self.connected:
            return
        self.write("UI:LED TOGGLE")

    def set_led(self, val):
        if not self.connected:
            return
        self.write(f"UI:LED {int(val) & 0xFFFF:X}")

    def get_led(self):
        if not self.connected:
            return
        self.write("UI:LED?")
        return int(self.read(), 16)

    def blink_get_interval(self):
        if not self.connected:
            return
        self.write("UI:BLINK:INTERVAL?")
        return int(self.read(), 16)

    def blink_set_interval(self, val):
        if not self.connected:
            return
        self.write(f"UI:BLINK:INTERVAL {int(val) & 0xFFFF:X}")

    def blink_start(self):
        if not self.connected:
            return
        self.write("UI:BLINK:START")

    def blink_stop(self):
        if not self.connected:
            return
        self.write("UI:BLINK:STOP")

    def read_bootsel(self):
        if not self.connected:
            return
        self.write("UI:BOOTSEL?")
        return int(self.read(), 16)

    #
    # ADC Methods
    #

    def adc_cmd(self, command, address=1):
        if not self.connected:
            return
        self.write(f"ADC:CMD {int(address) & 0xFF:X},{int(command) & 0xFF:X}")

    def adc_read_reg(self, reg, num_bytes=1, address=1):
        if not self.connected:
            return
        self.write(
            f"ADC:RD_REG {int(address) & 0xFF:X},{int(reg) & 0xFF:X},{int(num_bytes) & 0xFF:X}"
        )
        vals = [int(val, 16) for val in self.read().split(",")]
        if len(vals) == 1:
            return vals[0]
        else:
            return vals

    def adc_write_reg(self, reg, vals, address=1):
        if not self.connected:
            return
        self.write(
            f"ADC:WR_REG {int(address) & 0xFF:X},{int(reg) & 0xFF:X},"
            + ",".join([f"{int(val) & 0xFF:X}" for val in vals])
        )

    def adc_get_irq(self):
        if not self.connected:
            return
        self.write("ADC:IRQ?")
        return int(self.read(), 16)

    def adc_get_azmux(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:AZMUX? {int(address) & 0xFF:X}")
        return int(self.read(), 16)

    def adc_set_azmux(self, val=1, address=1):
        if not self.connected:
            return
        self.write(f"ADC:AZMUX {int(address) & 0xFF:X},{int(val) & 0xFF:X}")

    def adc_get_enoffcal(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:ENOFFCAL? {int(address) & 0xFF:X}")
        return int(self.read(), 16)

    def adc_set_enoffcal(self, val=0, address=1):
        if not self.connected:
            return
        self.write(f"ADC:ENOFFCAL {int(address) & 0xFF:X},{int(val) & 0xFF:X}")

    def adc_get_engaincal(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:ENGAINCAL? {int(address) & 0xFF:X}")
        return int(self.read(), 16)

    def adc_set_engaincal(self, val=0, address=1):
        if not self.connected:
            return
        self.write(f"ADC:ENGAINCAL {int(address) & 0xFF:X},{int(val) & 0xFF:X}")

    def adc_get_osr(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:OSR? {int(address) & 0xFF:X}")
        return int(self.read(), 16)

    def adc_set_osr(self, val=3, address=1):
        if not self.connected:
            return
        self.write(f"ADC:OSR {int(address) & 0xFF:X},{int(val) & 0xFF:X}")

    def adc_get_gain(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:GAIN? {int(address) & 0xFF:X}")
        return int(self.read(), 16)

    def adc_set_gain(self, val=1, address=1):
        if not self.connected:
            return
        self.write(f"ADC:GAIN {int(address) & 0xFF:X},{int(val) & 0xFF:X}")
        self.adc_gain = self.adc_get_gain()

    def adc_get_data(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:DATA? {int(address) & 0xFF:X}")
        val = int(self.read(), 16)
        if val > 2147483647:
            val = val - 4294967296
        return val

    def adc_scan_get_list(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:SCAN:LIST? {int(address) & 0xFF:X}")
        return int(self.read(), 16)

    def adc_scan_set_list(self, val=0, address=1):
        if not self.connected:
            return
        self.write(f"ADC:SCAN:LIST {int(address) & 0xFF:X},{int(val) & 0xFFFF:X}")

    def adc_scan_get_delay(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:SCAN:DELAY? {int(address) & 0xFF:X}")
        return int(self.read(), 16)

    def adc_scan_set_delay(self, val=0, address=1):
        if not self.connected:
            return
        self.write(f"ADC:SCAN:DELAY {int(address) & 0xFF:X},{int(val) & 0xFF:X}")

    def adc_scan_once(self, address=1):
        if not self.connected:
            return
        self.write(f"ADC:SCAN? {int(address) & 0xFF:X}")
        vals = [int(val, 16) for val in self.read().split(",")]
        vals = [val - 4294967296 if val > 2147483647 else val for val in vals]
        if len(vals) == 1:
            return vals[0]
        else:
            return vals

    def adc_stream_get_interval(self):
        if not self.connected:
            return
        self.write("ADC:STREAM:INTERVAL?")
        return int(self.read(), 16)

    def adc_stream_set_interval(self, val):
        if not self.connected:
            return
        self.write(f"ADC:STREAM:INTERVAL {int(val) & 0xFFFF:X}")

    def adc_stream_start(self):
        if not self.connected:
            return
        self.write("ADC:STREAM:START")

    def adc_stream_stop(self):
        if not self.connected:
            return
        self.write("ADC:STREAM:STOP")

    def adc_get_voltage(self, address=1):
        if not self.connected:
            return
        value = self.adc_get_data(address)
        return self.adc_slopes[self.adc_gain] * (
            value + self.adc_offsets[self.adc_gain]
        )

    def adc_get_data_avg(self, num_avg=10, address=1):
        if not self.connected:
            return
        total = 0
        for i in range(num_avg):
            total += self.adc_get_data(address)
        return total / num_avg

    def adc_get_voltage_avg(self, num_avg=10, address=1):
        if not self.connected:
            return
        total = 0
        for i in range(num_avg):
            total += self.adc_get_data(address)
        value = total / num_avg
        return self.adc_slopes[self.adc_gain] * (
            value + self.adc_offsets[self.adc_gain]
        )

    #
    # DAC methods
    #

    def dacA_set_value(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DACA:VALUE {int(value) & 0x0FFF:X}")

    def dacA_get_value(self):
        if not self.connected:
            return
        self.write("DAC:DACA:VALUE?")
        return int(self.read(), 16)

    def dacA_set_ena(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DACA:ENA {int(value) & 0xFF:X}")

    def dacA_get_ena(self):
        if not self.connected:
            return
        self.write("DAC:DACA:ENA?")
        return int(self.read(), 16)

    def dacB_set_value(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DACB:VALUE {int(value) & 0x0FFF:X}")

    def dacB_get_value(self):
        if not self.connected:
            return
        self.write("DAC:DACB:VALUE?")
        return int(self.read(), 16)

    def dacB_set_ena(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DACB:ENA {int(value) & 0xFF:X}")

    def dacB_get_ena(self):
        if not self.connected:
            return
        self.write("DAC:DACB:ENA?")
        return int(self.read(), 16)

    def dacC_set_value(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DACC:VALUE {int(value) & 0x0FFF:X}")

    def dacC_get_value(self):
        if not self.connected:
            return
        self.write("DAC:DACC:VALUE?")
        return int(self.read(), 16)

    def dacC_set_ena(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DACC:ENA {int(value) & 0xFF:X}")

    def dacC_get_ena(self):
        if not self.connected:
            return
        self.write("DAC:DACC:ENA?")
        return int(self.read(), 16)

    def dacD_set_value(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DACD:VALUE {int(value) & 0x0FFF:X}")

    def dacD_get_value(self):
        if not self.connected:
            return
        self.write("DAC:DACD:VALUE?")
        return int(self.read(), 16)

    def dacD_set_ena(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DACD:ENA {int(value) & 0xFF:X}")

    def dacD_get_ena(self):
        if not self.connected:
            return
        self.write("DAC:DACD:ENA?")
        return int(self.read(), 16)

    def dac_diffAB_set_value(self, value):
        if not self.connected:
            return
        if not (-4095 <= value <= 4095):
            return
        value = value if value >= 0 else value + 65536
        self.write(f"DAC:DIFFAB:VALUE {int(value):X}")

    def dac_diffAB_get_value(self):
        if not self.connected:
            return
        self.write("DAC:DIFFAB:VALUE?")
        value = int(self.read(), 16)
        return value if value < 32768 else value - 65536

    def dac_diffAB_set_ena(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DIFFAB:ENA {int(value) & 0xFF:X}")

    def dac_diffAB_get_ena(self):
        if not self.connected:
            return
        self.write("DAC:DIFFAB:ENA?")
        return int(self.read(), 16)

    def dac_diffCD_set_value(self, value):
        if not self.connected:
            return
        if not (-4095 <= value <= 4095):
            return
        value = value if value >= 0 else value + 65536
        self.write(f"DAC:DIFFCD:VALUE {int(value):X}")

    def dac_diffCD_get_value(self):
        if not self.connected:
            return
        self.write("DAC:DIFFCD:VALUE?")
        value = int(self.read(), 16)
        return value if value < 32768 else value - 65536

    def dac_diffCD_set_ena(self, value):
        if not self.connected:
            return
        self.write(f"DAC:DIFFCD:ENA {int(value) & 0xFF:X}")

    def dac_diffCD_get_ena(self):
        if not self.connected:
            return
        self.write("DAC:DIFFCD:ENA?")
        return int(self.read(), 16)

    def dacA_set_voltage(self, voltage):
        if not self.connected:
            return
        value = int(round(voltage * self.dacA_slope + self.dacA_offset))
        value = value if value >= 0 else 0
        value = value if value <= 4095 else 4095
        self.dacA_set_value(value)

    def dacA_get_voltage(self):
        if not self.connected:
            return
        value = self.dacA_get_value()
        return (value - self.dacA_offset) / self.dacA_slope

    def dacB_set_voltage(self, voltage):
        if not self.connected:
            return
        value = int(round(voltage * self.dacB_slope + self.dacB_offset))
        value = value if value >= 0 else 0
        value = value if value <= 4095 else 4095
        self.dacB_set_value(value)

    def dacB_get_voltage(self):
        if not self.connected:
            return
        value = self.dacB_get_value()
        return (value - self.dacB_offset) / self.dacB_slope

    def dacC_set_voltage(self, voltage):
        if not self.connected:
            return
        value = int(round(voltage * self.dacC_slope + self.dacC_offset))
        value = value if value >= 0 else 0
        value = value if value <= 4095 else 4095
        self.dacC_set_value(value)

    def dacC_get_voltage(self):
        if not self.connected:
            return
        value = self.dacC_get_value()
        return (value - self.dacC_offset) / self.dacC_slope

    def dacD_set_voltage(self, voltage):
        if not self.connected:
            return
        value = int(round(voltage * self.dacD_slope + self.dacD_offset))
        value = value if value >= 0 else 0
        value = value if value <= 4095 else 4095
        self.dacD_set_value(value)

    def dacD_get_voltage(self):
        if not self.connected:
            return
        value = self.dacD_get_value()
        return (value - self.dacD_offset) / self.dacD_slope

    def dac_diffAB_set_voltage(self, voltage):
        if not self.connected:
            return
        value = int(round(voltage * self.diffAB_slope + self.diffAB_offset))
        value = value if value >= -4095 else -4095
        value = value if value <= 4095 else 4095
        self.dac_diffAB_set_value(value)

    def dac_diffAB_get_voltage(self):
        if not self.connected:
            return
        value = self.dac_diffAB_get_value()
        return (value - self.diffAB_offset) / self.diffAB_slope

    def dac_diffCD_set_voltage(self, voltage):
        if not self.connected:
            return
        value = int(round(voltage * self.diffCD_slope + self.diffCD_offset))
        value = value if value >= -4095 else -4095
        value = value if value <= 4095 else 4095
        self.dac_diffCD_set_value(value)

    def dac_diffCD_get_voltage(self):
        if not self.connected:
            return
        value = self.dac_diffCD_get_value()
        return (value - self.diffCD_offset) / self.diffCD_slope

    #
    # Switch methods
    #
    def sw_set_state(self, update):
        """
        Enter the id of x switches to be engaged in
        any number of daisy chained switch arrays.

        Arg:
            update - A list of integers, each corresponding to the number of switches
                    before the desired switch to be engaged in the daisy chain (counting up).

                    e.g. The 4th and 8th switch in the second 8-switch array would be [11, 15].
        """
        if not self.connected:
            return

        state = [0, 0, 0, 0, 0]
        for i in update:
            state[i // 8] += 2 ** (i % 8)
        self.dev.write(
            ("SW:STATE " + "".join([f"{i:X}," for i in state]) + "\r").encode()
        )

    def sw_get_state(self):
        """
        Returns a list of positive integers corresponding to each switch that is currently on.
        A given switch corresponds to the unique integer representing the number of switches before
        it in the daisy chain (counted low to high within a switch array).
        """
        if not self.connected:
            return

        self.write("SW:STATE?")
        read_state = (self.read()).split(",")
        print(read_state)

        sw_per_array = 8
        state = []
        array_id = 0
        for sw in read_state:
            sw_id = 0
            for i in bin(int(sw, 16))[-1:1:-1]:
                if int(i):
                    state.append(sw_id + array_id)
                sw_id += 1
            array_id += sw_per_array
        return state

    def sw_reset(self):
        if not self.connected:
            return
        self.write("SW:RESET")

    #
    # EEPROM methods
    #

    def eeprom_read(self, address, num_bytes):
        if not self.connected:
            return
        self.write(f"EEPROM:READ {int(address) & 0xFFFF:X},{int(num_bytes) & 0xFFFF:X}")
        values = [int(value, 16) for value in self.read().split(",")]
        if len(values) == 1:
            return values[0]
        else:
            return values

    def eeprom_write(self, address, values):
        if not self.connected:
            return
        self.write(
            f"EEPROM:WRITE {int(address) & 0xFFFF:X},"
            + ",".join([f"{int(value) & 0xFF:X}" for value in values])
        )

    def eeprom_commit(self):
        if not self.connected:
            return
        self.write("EEPROM:COMMIT")

    def eeprom_length(self):
        if not self.connected:
            return
        self.write("EEPROM:LENGTH?")
        return int(self.read(), 16)

    def eeprom_erase(self):
        if not self.connected:
            return
        self.write("EEPROM:ERASE")

    def write_serial_number(self, serial_number):
        if not self.connected:
            return

        length = len(serial_number)
        if length == 0 or length > 254:
            return

        vals = [length] + list(serial_number.encode()) + [0xFF] * (255 - length)
        self.eeprom_write(0, vals[:128])
        self.eeprom_write(128, vals[128:])
        self.eeprom_commit()

    def read_serial_number(self):
        if not self.connected:
            return
        self.write("*IDN?")
        return self.read().strip()

    def nominal_calib_values(self):
        self.adc_slopes = copy.deepcopy(me_gizmo.nominal_adc_slopes)
        self.adc_offsets = copy.deepcopy(me_gizmo.nominal_adc_offsets)

        self.dacA_slope = copy.deepcopy(me_gizmo.nominal_dacA_slope)
        self.dacA_offset = copy.deepcopy(me_gizmo.nominal_dacA_offset)

        self.dacB_slope = copy.deepcopy(me_gizmo.nominal_dacB_slope)
        self.dacB_offset = copy.deepcopy(me_gizmo.nominal_dacB_offset)

        self.dacC_slope = copy.deepcopy(me_gizmo.nominal_dacC_slope)
        self.dacC_offset = copy.deepcopy(me_gizmo.nominal_dacC_offset)

        self.dacD_slope = copy.deepcopy(me_gizmo.nominal_dacD_slope)
        self.dacD_offset = copy.deepcopy(me_gizmo.nominal_dacD_offset)

        self.diffAB_slope = copy.deepcopy(me_gizmo.nominal_diffAB_slope)
        self.diffAB_offset = copy.deepcopy(me_gizmo.nominal_diffAB_offset)

        self.diffCD_slope = copy.deepcopy(me_gizmo.nominal_diffCD_slope)
        self.diffCD_offset = copy.deepcopy(me_gizmo.nominal_diffCD_offset)

    def float_to_vals(self, x):
        if x < 0.0:
            x_sign = 1
            x = -x
        else:
            x_sign = 0
        x_int = int(x)
        x_frac = x - int(x)
        val1 = x_int & 0x7FFFFF
        val1 = val1 | 0x800000 if x_sign else val1
        val2 = int(x_frac * (2**24))
        return (val1, val2)

    def vals_to_float(self, val1, val2):
        x_int = val1 & 0x7FFFFF
        x_sign = -1.0 if val1 & 0x800000 else 1.0
        x_frac = val2 / 2**24
        x = x_int + x_frac
        return x_sign * x

    def write_calib_values(self):
        if not self.connected:
            return

        calib_values = []

        for i in range(8):
            slope_val = int(
                round(2**23 * self.adc_slopes[i] / me_gizmo.nominal_adc_slopes[i])
            )
            calib_values.extend(
                [slope_val & 0xFF, (slope_val >> 8) & 0xFF, slope_val >> 16]
            )

            offset_vals = self.float_to_vals(self.adc_offsets[i])
            calib_values.extend(
                [
                    offset_vals[0] & 0xFF,
                    (offset_vals[0] >> 8) & 0xFF,
                    offset_vals[0] >> 16,
                ]
            )
            calib_values.extend(
                [
                    offset_vals[1] & 0xFF,
                    (offset_vals[1] >> 8) & 0xFF,
                    offset_vals[1] >> 16,
                ]
            )

        slope_val = int(round(2**23 * self.dacA_slope / me_gizmo.nominal_dacA_slope))
        calib_values.extend(
            [slope_val & 0xFF, (slope_val >> 8) & 0xFF, slope_val >> 16]
        )

        offset_vals = self.float_to_vals(self.dacA_offset)
        calib_values.extend(
            [offset_vals[0] & 0xFF, (offset_vals[0] >> 8) & 0xFF, offset_vals[0] >> 16]
        )
        calib_values.extend(
            [offset_vals[1] & 0xFF, (offset_vals[1] >> 8) & 0xFF, offset_vals[1] >> 16]
        )

        slope_val = int(round(2**23 * self.dacB_slope / me_gizmo.nominal_dacB_slope))
        calib_values.extend(
            [slope_val & 0xFF, (slope_val >> 8) & 0xFF, slope_val >> 16]
        )

        offset_vals = self.float_to_vals(self.dacB_offset)
        calib_values.extend(
            [offset_vals[0] & 0xFF, (offset_vals[0] >> 8) & 0xFF, offset_vals[0] >> 16]
        )
        calib_values.extend(
            [offset_vals[1] & 0xFF, (offset_vals[1] >> 8) & 0xFF, offset_vals[1] >> 16]
        )

        slope_val = int(round(2**23 * self.dacC_slope / me_gizmo.nominal_dacC_slope))
        calib_values.extend(
            [slope_val & 0xFF, (slope_val >> 8) & 0xFF, slope_val >> 16]
        )

        offset_vals = self.float_to_vals(self.dacC_offset)
        calib_values.extend(
            [offset_vals[0] & 0xFF, (offset_vals[0] >> 8) & 0xFF, offset_vals[0] >> 16]
        )
        calib_values.extend(
            [offset_vals[1] & 0xFF, (offset_vals[1] >> 8) & 0xFF, offset_vals[1] >> 16]
        )

        slope_val = int(round(2**23 * self.dacD_slope / me_gizmo.nominal_dacD_slope))
        calib_values.extend(
            [slope_val & 0xFF, (slope_val >> 8) & 0xFF, slope_val >> 16]
        )

        offset_vals = self.float_to_vals(self.dacD_offset)
        calib_values.extend(
            [offset_vals[0] & 0xFF, (offset_vals[0] >> 8) & 0xFF, offset_vals[0] >> 16]
        )
        calib_values.extend(
            [offset_vals[1] & 0xFF, (offset_vals[1] >> 8) & 0xFF, offset_vals[1] >> 16]
        )

        slope_val = int(
            round(2**23 * self.diffAB_slope / me_gizmo.nominal_diffAB_slope)
        )
        calib_values.extend(
            [slope_val & 0xFF, (slope_val >> 8) & 0xFF, slope_val >> 16]
        )

        offset_vals = self.float_to_vals(self.diffAB_offset)
        calib_values.extend(
            [offset_vals[0] & 0xFF, (offset_vals[0] >> 8) & 0xFF, offset_vals[0] >> 16]
        )
        calib_values.extend(
            [offset_vals[1] & 0xFF, (offset_vals[1] >> 8) & 0xFF, offset_vals[1] >> 16]
        )

        slope_val = int(
            round(2**23 * self.diffCD_slope / me_gizmo.nominal_diffCD_slope)
        )
        calib_values.extend(
            [slope_val & 0xFF, (slope_val >> 8) & 0xFF, slope_val >> 16]
        )

        offset_vals = self.float_to_vals(self.diffCD_offset)
        calib_values.extend(
            [offset_vals[0] & 0xFF, (offset_vals[0] >> 8) & 0xFF, offset_vals[0] >> 16]
        )
        calib_values.extend(
            [offset_vals[1] & 0xFF, (offset_vals[1] >> 8) & 0xFF, offset_vals[1] >> 16]
        )

        self.eeprom_write(256, calib_values)
        self.eeprom_commit()

    def read_calib_values(self):
        if not self.connected:
            return

        calib_values = self.eeprom_read(256, 126)

        if len(calib_values) != 126:
            return

        if all([val == 0xFF for val in calib_values]):
            return

        for i in range(8):
            slope_val = (
                calib_values[9 * i]
                | (calib_values[9 * i + 1] << 8)
                | (calib_values[9 * i + 2] << 16)
            )
            slope_val = slope_val / 2**23
            self.adc_slopes[i] = slope_val * me_gizmo.nominal_adc_slopes[i]

            offset_val1 = (
                calib_values[9 * i + 3]
                | (calib_values[9 * i + 4] << 8)
                | (calib_values[9 * i + 5] << 16)
            )
            offset_val2 = (
                calib_values[9 * i + 6]
                | (calib_values[9 * i + 7] << 8)
                | (calib_values[9 * i + 8] << 16)
            )
            self.adc_offsets[i] = self.vals_to_float(offset_val1, offset_val2)

        i = 8
        slope_val = (
            calib_values[9 * i]
            | (calib_values[9 * i + 1] << 8)
            | (calib_values[9 * i + 2] << 16)
        )
        slope_val = slope_val / 2**23
        self.dacA_slope = slope_val * me_gizmo.nominal_dacA_slope

        offset_val1 = (
            calib_values[9 * i + 3]
            | (calib_values[9 * i + 4] << 8)
            | (calib_values[9 * i + 5] << 16)
        )
        offset_val2 = (
            calib_values[9 * i + 6]
            | (calib_values[9 * i + 7] << 8)
            | (calib_values[9 * i + 8] << 16)
        )
        self.dacA_offset = self.vals_to_float(offset_val1, offset_val2)

        i = 9
        slope_val = (
            calib_values[9 * i]
            | (calib_values[9 * i + 1] << 8)
            | (calib_values[9 * i + 2] << 16)
        )
        slope_val = slope_val / 2**23
        self.dacB_slope = slope_val * me_gizmo.nominal_dacB_slope

        offset_val1 = (
            calib_values[9 * i + 3]
            | (calib_values[9 * i + 4] << 8)
            | (calib_values[9 * i + 5] << 16)
        )
        offset_val2 = (
            calib_values[9 * i + 6]
            | (calib_values[9 * i + 7] << 8)
            | (calib_values[9 * i + 8] << 16)
        )
        self.dacB_offset = self.vals_to_float(offset_val1, offset_val2)

        i = 10
        slope_val = (
            calib_values[9 * i]
            | (calib_values[9 * i + 1] << 8)
            | (calib_values[9 * i + 2] << 16)
        )
        slope_val = slope_val / 2**23
        self.dacC_slope = slope_val * me_gizmo.nominal_dacC_slope

        offset_val1 = (
            calib_values[9 * i + 3]
            | (calib_values[9 * i + 4] << 8)
            | (calib_values[9 * i + 5] << 16)
        )
        offset_val2 = (
            calib_values[9 * i + 6]
            | (calib_values[9 * i + 7] << 8)
            | (calib_values[9 * i + 8] << 16)
        )
        self.dacC_offset = self.vals_to_float(offset_val1, offset_val2)

        i = 11
        slope_val = (
            calib_values[9 * i]
            | (calib_values[9 * i + 1] << 8)
            | (calib_values[9 * i + 2] << 16)
        )
        slope_val = slope_val / 2**23
        self.dacD_slope = slope_val * me_gizmo.nominal_dacD_slope

        offset_val1 = (
            calib_values[9 * i + 3]
            | (calib_values[9 * i + 4] << 8)
            | (calib_values[9 * i + 5] << 16)
        )
        offset_val2 = (
            calib_values[9 * i + 6]
            | (calib_values[9 * i + 7] << 8)
            | (calib_values[9 * i + 8] << 16)
        )
        self.dacD_offset = self.vals_to_float(offset_val1, offset_val2)

        i = 12
        slope_val = (
            calib_values[9 * i]
            | (calib_values[9 * i + 1] << 8)
            | (calib_values[9 * i + 2] << 16)
        )
        slope_val = slope_val / 2**23
        self.diffAB_slope = slope_val * me_gizmo.nominal_diffAB_slope

        offset_val1 = (
            calib_values[9 * i + 3]
            | (calib_values[9 * i + 4] << 8)
            | (calib_values[9 * i + 5] << 16)
        )
        offset_val2 = (
            calib_values[9 * i + 6]
            | (calib_values[9 * i + 7] << 8)
            | (calib_values[9 * i + 8] << 16)
        )
        self.diffAB_offset = self.vals_to_float(offset_val1, offset_val2)

        i = 13
        slope_val = (
            calib_values[9 * i]
            | (calib_values[9 * i + 1] << 8)
            | (calib_values[9 * i + 2] << 16)
        )
        slope_val = slope_val / 2**23
        self.diffCD_slope = slope_val * me_gizmo.nominal_diffCD_slope

        offset_val1 = (
            calib_values[9 * i + 3]
            | (calib_values[9 * i + 4] << 8)
            | (calib_values[9 * i + 5] << 16)
        )
        offset_val2 = (
            calib_values[9 * i + 6]
            | (calib_values[9 * i + 7] << 8)
            | (calib_values[9 * i + 8] << 16)
        )
        self.diffCD_offset = self.vals_to_float(offset_val1, offset_val2)

    def save_calib_values(self, filename):
        file = open(filename, "w")

        for i in range(8):
            slope_val = int(
                round(2**23 * self.adc_slopes[i] / me_gizmo.nominal_adc_slopes[i])
            )
            file.write(f"{slope_val:06X}\n")

            offset_vals = self.float_to_vals(self.adc_offsets[i])
            file.write(f"{offset_vals[0]:06X}\n")
            file.write(f"{offset_vals[1]:06X}\n")

        slope_val = int(round(2**23 * self.dacA_slope / me_gizmo.nominal_dacA_slope))
        file.write(f"{slope_val:06X}\n")

        offset_vals = self.float_to_vals(self.dacA_offset)
        file.write(f"{offset_vals[0]:06X}\n")
        file.write(f"{offset_vals[1]:06X}\n")

        slope_val = int(round(2**23 * self.dacB_slope / me_gizmo.nominal_dacB_slope))
        file.write(f"{slope_val:06X}\n")

        offset_vals = self.float_to_vals(self.dacB_offset)
        file.write(f"{offset_vals[0]:06X}\n")
        file.write(f"{offset_vals[1]:06X}\n")

        slope_val = int(round(2**23 * self.dacC_slope / me_gizmo.nominal_dacC_slope))
        file.write(f"{slope_val:06X}\n")

        offset_vals = self.float_to_vals(self.dacC_offset)
        file.write(f"{offset_vals[0]:06X}\n")
        file.write(f"{offset_vals[1]:06X}\n")

        slope_val = int(round(2**23 * self.dacD_slope / me_gizmo.nominal_dacD_slope))
        file.write(f"{slope_val:06X}\n")

        offset_vals = self.float_to_vals(self.dacD_offset)
        file.write(f"{offset_vals[0]:06X}\n")
        file.write(f"{offset_vals[1]:06X}\n")

        slope_val = int(
            round(2**23 * self.diffAB_slope / me_gizmo.nominal_diffAB_slope)
        )
        file.write(f"{slope_val:06X}\n")

        offset_vals = self.float_to_vals(self.diffAB_offset)
        file.write(f"{offset_vals[0]:06X}\n")
        file.write(f"{offset_vals[1]:06X}\n")

        slope_val = int(
            round(2**23 * self.diffCD_slope / me_gizmo.nominal_diffCD_slope)
        )
        file.write(f"{slope_val:06X}\n")

        offset_vals = self.float_to_vals(self.diffCD_offset)
        file.write(f"{offset_vals[0]:06X}\n")
        file.write(f"{offset_vals[1]:06X}\n")

        file.close()

    def load_calib_values(self, filename):
        try:
            file = open(filename, "r")
        except FileNotFoundError:
            return

        for i in range(8):
            slope_val = int(file.readline().strip(), 16)
            slope_val = slope_val / 2**23
            self.adc_slopes[i] = slope_val * me_gizmo.nominal_adc_slopes[i]

            offset_val1 = int(file.readline().strip(), 16)
            offset_val2 = int(file.readline().strip(), 16)
            self.adc_offsets[i] = self.vals_to_float(offset_val1, offset_val2)

        slope_val = int(file.readline().strip(), 16)
        slope_val = slope_val / 2**23
        self.dacA_slope = slope_val * me_gizmo.nominal_dacA_slope

        offset_val1 = int(file.readline().strip(), 16)
        offset_val2 = int(file.readline().strip(), 16)
        self.dacA_offset = self.vals_to_float(offset_val1, offset_val2)

        slope_val = int(file.readline().strip(), 16)
        slope_val = slope_val / 2**23
        self.dacB_slope = slope_val * me_gizmo.nominal_dacB_slope

        offset_val1 = int(file.readline().strip(), 16)
        offset_val2 = int(file.readline().strip(), 16)
        self.dacB_offset = self.vals_to_float(offset_val1, offset_val2)

        slope_val = int(file.readline().strip(), 16)
        slope_val = slope_val / 2**23
        self.dacC_slope = slope_val * me_gizmo.nominal_dacC_slope

        offset_val1 = int(file.readline().strip(), 16)
        offset_val2 = int(file.readline().strip(), 16)
        self.dacC_offset = self.vals_to_float(offset_val1, offset_val2)

        slope_val = int(file.readline().strip(), 16)
        slope_val = slope_val / 2**23
        self.dacD_slope = slope_val * me_gizmo.nominal_dacD_slope

        offset_val1 = int(file.readline().strip(), 16)
        offset_val2 = int(file.readline().strip(), 16)
        self.dacD_offset = self.vals_to_float(offset_val1, offset_val2)

        slope_val = int(file.readline().strip(), 16)
        slope_val = slope_val / 2**23
        self.diffAB_slope = slope_val * me_gizmo.nominal_diffAB_slope

        offset_val1 = int(file.readline().strip(), 16)
        offset_val2 = int(file.readline().strip(), 16)
        self.diffAB_offset = self.vals_to_float(offset_val1, offset_val2)

        slope_val = int(file.readline().strip(), 16)
        slope_val = slope_val / 2**23
        self.diffCD_slope = slope_val * me_gizmo.nominal_diffCD_slope

        offset_val1 = int(file.readline().strip(), 16)
        offset_val2 = int(file.readline().strip(), 16)
        self.diffCD_offset = self.vals_to_float(offset_val1, offset_val2)

        file.close()
