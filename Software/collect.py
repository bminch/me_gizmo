"""
'collect.py' script must be run in the bradlab environment which can be found at
https://github.com/bminch/tkplot/blob/main/bradlab.py#L264.
"""

import me_gizmo

gizmo = me_gizmo.me_gizmo()


def stream_channels(pts, fs):
    """
    Plots the differential voltages of ADC channels 0/1 and 2/3 with a
    given number of points and frequency. Data is converted using the
    ADC "SCAN" mode, repeatedly cycling through the four channels with
    a delay between each cycle.

    Args:
        pts - Integer indicating the number of data points to collect.
        fs - Float indicating the sampling frequency in Hz.
    """
    gizmo.adc_scan_set_list(15)
    gizmo.adc_stream_set_interval(1000 / fs)
    gizmo.adc_stream_start()
    x = linspace(0, (pts - 1) // fs, pts)
    ch1 = zeros(pts)
    ch2 = zeros(pts)
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
        plot(x[:i], [ch1[:i], ch2[:i]], ["b.", "r."])
        xlabel("Time (s)")
        ylabel("Voltage (V)")
        ylabel("", yaxis="right")
        draw_now()
    gizmo.adc_stream_stop()


gizmo.dac_diff_set_voltage(0)
gizmo.dac_diff_set_ena(1)
pts = int(input("Number of points: "))
fs = int(input("Sampling frequency: "))
stream_channels(pts, fs)
