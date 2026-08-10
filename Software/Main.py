"""
'scripts must be run in the bradlab environment which can be found at
https://github.com/bminch/tkplot/blob/main/bradlab.py#L264.
Run via 'exec(open('Main.py').read())' in the terminal.
"""

import math
from pynput import keyboard
import me_gizmo


def interrupt_thread(key, gizmo):
    """
    Non-blockoing keyboard interrupt function using "pynput"
    """
    if key == keyboard.Key.space:
        gizmo.is_detecting = False
    return


def detect_config(gizmo):
    """
    Guesses the desired operating mode of the device and updates
    the switch and DAC settings to reflect it.

    gizmo - An instance of the 'me_gizmo' class
    """
    listener = keyboard.Listener(on_release=lambda key: interrupt_thread(key, gizmo))
    listener.start()
    tol = 0.02  # Mode detection tolerance
    thermo_thresh = 0.05  # Thermocouple voltage threshold
    hbridge_divider = 0.7966  # 350 / (350 + 120 || 350))
    gizmo.sw_set_config()
    gizmo.sw_set_ref(350)

    gizmo.is_detecting = True
    while gizmo.is_detecting:
        # Check 3-wire connections
        gizmo.adc_scan_set_list(255)
        # Scan channels
        value = gizmo.adc_scan_once()
        value = [val - 4294967296 if val > 2147483647 else val for val in value]
        value = [
            gizmo.adc_slopes[gizmo.adc_gain] * (val + gizmo.adc_offsets[gizmo.adc_gain])
            for val in value
        ]
        wire_1 = [False, False, False, False]
        wire_2 = [False, False, False, False]
        for i in [6, 4, 2, 0]:
            if math.isclose(value[i], gizmo.VREF, abs_tol=tol) or math.isclose(
                value[i], 0, abs_tol=tol
            ):
                wire_1[i // 2] = True
            if math.isclose(value[i + 1], gizmo.VREF, abs_tol=tol) or math.isclose(
                value[i], 0, abs_tol=tol
            ):
                wire_2[i // 2] = True
        print(wire_1)
        print(wire_2)

        gizmo.adc_scan_set_list(255)
        reference_resistance = 0
        for i in [0, 2, 4, 6]:
            # Enable and measure along two channels at a time
            if i % 4 == 0:
                gizmo.sw_set_config(i // 2 + 1, 0)
                gizmo.sw_set_config(i // 2 + 2, 0)
                gizmo.sw_set_config(4 - i // 2, None)
                gizmo.sw_set_config(3 - i // 2, None)

                # Scan channels
                value = []
                while len(value) < 8:
                    value = gizmo.adc_scan_once()
                value = [val - 4294967296 if val > 2147483647 else val for val in value]
                value = [
                    gizmo.adc_slopes[gizmo.adc_gain]
                    * (val + gizmo.adc_offsets[gizmo.adc_gain])
                    for val in value
                ]
                value = [
                    value[7],
                    value[6],
                    value[5],
                    value[4],
                    gizmo.VREF - value[2],
                    gizmo.VREF - value[3],
                    gizmo.VREF - value[0],
                    gizmo.VREF - value[1],
                ]
                # Prototype:
                # value = [
                #     value[7],
                #     value[6],
                #     value[5],
                #     value[4],
                #     value[3],
                #     value[2],
                #     value[1],
                #     value[0],
                # ]

            # Detect if channel is being used
            if math.isclose(value[i], gizmo.VREF, abs_tol=tol) or math.isclose(
                value[i], 0, abs_tol=tol
            ):
                print(f"CH{i // 2 + 1} UNUSED")
                gizmo.operating_mode[i // 2] = None
                continue

            # Detect reference resistance
            previous_reference = reference_resistance
            if not (
                math.isclose(value[i], gizmo.VREF / 2, abs_tol=tol)
                or math.isclose(value[i], gizmo.VREF * 2 / 3, abs_tol=tol)
            ):
                reference_resistance = 120
            else:
                reference_resistance = 350
            if previous_reference not in (reference_resistance, 0):
                print("Error: Reference resistance mismatch")

            # Thermocouple
            if value[i] < thermo_thresh:
                gizmo.operating_mode[i // 2] = 7
                print("THERMOCOUPLE")

            # Full bridge config (mode 6)
            elif value[i] > gizmo.VREF * (tol + 1) and not math.isclose(
                value[i], gizmo.VREF * 350 / (120 + 350), abs_tol=tol
            ):
                gizmo.operating_mode[i // 2] = 6
                print("FULL BRIDGE")

            # Half-bridge opposite branches (mode 4&5)
            elif math.isclose(
                value[i + 1], gizmo.VREF * (1 - hbridge_divider), abs_tol=tol
            ):
                if wire_1[i // 2] and wire_2[i // 2] is True:
                    gizmo.operating_mode[i // 2] = 5
                    print("HALF 3-WIRE")
                else:
                    print("HALF 2-WIRE")
                    gizmo.operating_mode[i // 2] = 4

            # Half-bridge adjacent branches (mode 3)
            elif math.isclose(value[i + 1], gizmo.VREF * hbridge_divider, abs_tol=tol):
                print("HALF ADCJACENT")
                gizmo.operating_mode[i // 2] = 3

            # Quarter bridge config (modes 1&2)
            elif math.isclose(value[i + 1], gizmo.VREF / 2, abs_tol=tol):
                if wire_1[i // 2] is True:
                    print("QUARTER 3-WIRE")
                    gizmo.operating_mode[i // 2] = 2
                else:
                    print("QUARTER 2-WIRE")
                    gizmo.operating_mode[i // 2] = 1

            else:
                gizmo.operating_mode[i // 2] = 0
                print("Error: Unable to detect configuration")
        gizmo.SWMODE = gizmo.operating_mode[0:]

        # Pull-up ADC nodes
        for i in [3, 4]:
            gizmo.sw_set_config(i, None)

    # Update reference resistance and mode configuration for all channels
    gizmo.sw_set_ref(reference_resistance)


def stream_channels(pts, fs, mode, RREF):
    """
    Plots the differential voltages of ADC channels 0/1, 2/3, 4/5, and 6/7 with a
    given number of points and frequency. Data is converted using the
    ADC "SCAN" mode, repeatedly cycling through the four channels with
    a delay between each cycle.

    Args:
        pts - Integer indicating the number of data points to collect.
        fs - Float indicating the sampling frequency in Hz.
        mode - list containing four integers corresponding to the modes of each channel
        RREF - Reference resistance (accepts either 120 or 350)
    """
    gizmo = me_gizmo.me_gizmo()
    x = linspace(0, (pts - 1) // fs, pts)
    ch1 = zeros(pts)
    ch2 = zeros(pts)
    ch3 = zeros(pts)
    ch4 = zeros(pts)
    gizmo.sw_set_ref(RREF)
    for i in range(3):
        gizmo.sw_set_config(i + 1, mode[i])
    gizmo.adc_scan_set_list(255)
    gizmo.adc_stream_set_interval(1000 / fs)
    gizmo.adc_stream_start()
    for i in range(pts):
        line = gizmo.read()
        value = [int(val, 16) for val in line.strip().split(",")]
        value = [val - 4294967296 if val > 2147483647 else val for val in value]
        value = [
            gizmo.adc_slopes[gizmo.adc_gain] * (val + gizmo.adc_offsets[gizmo.adc_gain])
            for val in value
        ]
        ch1[i] = value[2] - value[3]
        ch2[i] = value[0] - value[1]
        ch3[i] = value[6] - value[7]
        ch4[i] = value[4] - value[5]
        # print([ch1, ch2, ch3, ch4])
        plot(x[:i], [ch1[:i], ch2[:i], ch3[:i], ch4[:i]], ["b.", "r.", "bo", "ro"])
        xlabel("Time (s)")
        ylabel("Voltage (V)")
        ylabel("", yaxis="right")
        draw_now()
    gizmo.adc_stream_stop()


g = me_gizmo.me_gizmo()
g.start_ui_thread()
detect_config(g)
# stream_channels(
#     int(input("Number of points: ")),
#     int(input("Sampling frequency: ")),
#     [1, 3, 4, 6],
#     350,
# )
