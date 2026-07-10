"""
'collect.py' script must be run in the bradlab environment which can be found at
https://github.com/bminch/tkplot/blob/main/bradlab.py#L264.
"""

import me_gizmo

gizmo = me_gizmo.me_gizmo()


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


gizmo.dacA_set_voltage(1.25)
gizmo.dacA_set_ena(1)
pts = int(input("Number of points: "))
fs = int(input("Sampling frequency: "))
mode = [1, 3, 4, 6]
RREF = 350
stream_channels(pts, fs, mode, RREF)
