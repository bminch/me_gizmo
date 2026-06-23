#
# Current assumptions:
#
#  1. The calibration script will be run inside the bradlab environment.
#  2. The me_gizmo class has been imported via 'import me_gizmo'.
#  3. An me_gizmo object called 'gizmo' has been created via 'gizmo = me_gizmo.me_gizmo()' 
#     with the device plugged into a USB port.
#  4. A variable calle num_avg has been created indicating the number of readings to average for each measurment.
#  5. The calibration values will be saved to the device manually (if desired) via 'gizmo.write_calib_values()'.
#
# Run the script via 'exec(open('calibrate.py').read())'.
#
print(f'Running calibration, OSR = {gizmo.adc_get_osr()}, AZ_MUX = {gizmo.adc_get_azmux()}, and num_avg = {num_avg}...')

print('Calibrating ADC gain = 0 (Gain is x1/3)...')
gizmo.adc_set_gain(0)
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_VREF])
vrefp = gizmo.adc_get_data_avg(num_avg)
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_NEG_VREF])
vrefm = gizmo.adc_get_data_avg(num_avg)
gizmo.adc_slopes[0] = 5. / (vrefp - vrefm)
gizmo.adc_offsets[0] = -0.5 * (vrefp + vrefm)

print('Calibrating DACA...')
gizmo.dacA_set_ena(1)
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_CH1_SE])
x = linspace(0, 4000, 101)
y = zeros(101)
for i in range(101):
    gizmo.dacA_set_value(x[i])
    y[i] = gizmo.adc_get_voltage_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DACA code')
        ylabel('ADC voltage (V)')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, x, 1)
gizmo.dacA_slope = p[0]
gizmo.dacA_offset = p[1]

plot(y, [x, polyval(p, y)], ['b.', 'k-'])
xlabel('Voltage (V)')
ylabel('DACA code')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating DACB...')
gizmo.dacB_set_ena(1)
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_CH3_SE])
x = linspace(0, 4000, 101)
y = zeros(101)
for i in range(101):
    gizmo.dacB_set_value(x[i])
    y[i] = gizmo.adc_get_voltage_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DACB code')
        ylabel('ADC voltage (V)')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, x, 1)
gizmo.dacB_slope = p[0]
gizmo.dacB_offset = p[1]

plot(y, [x, polyval(p, y)], ['b.', 'k-'])
xlabel('Voltage (V)')
ylabel('DACB code')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating DACC...')
gizmo.dacC_set_ena(1)
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_CH5_SE])
x = linspace(0, 4000, 101)
y = zeros(101)
for i in range(101):
    gizmo.dacC_set_value(x[i])
    y[i] = gizmo.adc_get_voltage_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DACC code')
        ylabel('ADC voltage (V)')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, x, 1)
gizmo.dacC_slope = p[0]
gizmo.dacC_offset = p[1]

plot(y, [x, polyval(p, y)], ['b.', 'k-'])
xlabel('Voltage (V)')
ylabel('DACC code')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating DACD...')
gizmo.dacD_set_ena(1)
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_CH7_SE])
x = linspace(0, 4000, 101)
y = zeros(101)
for i in range(101):
    gizmo.dacD_set_value(x[i])
    y[i] = gizmo.adc_get_voltage_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DACD code')
        ylabel('ADC voltage (V)')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, x, 1)
gizmo.dacD_slope = p[0]
gizmo.dacD_offset = p[1]

plot(y, [x, polyval(p, y)], ['b.', 'k-'])
xlabel('Voltage (V)')
ylabel('DACD code')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating DAC DIFFAB (i.e., DACA - DACB)...')
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_DIFFAB])
x = linspace(-4000, 4000, 201)
y = zeros(201)
for i in range(201):
    gizmo.dac_diffAB_set_value(x[i])
    y[i] = gizmo.adc_get_voltage_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFAB code')
        ylabel('ADC voltage (V)')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, x, 1)
gizmo.diffAB_slope = p[0]
gizmo.diffAB_offset = p[1]

plot(y, [x, polyval(p, y)], ['b.', 'k-'])
xlabel('Voltage (V)')
ylabel('DIFFAB code')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating DAC DIFFCD (i.e., DACC - DACD)...')
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_DIFFCD])
x = linspace(-4000, 4000, 201)
y = zeros(201)
for i in range(201):
    gizmo.dac_diffCD_set_value(x[i])
    y[i] = gizmo.adc_get_voltage_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFCD code')
        ylabel('ADC voltage (V)')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, x, 1)
gizmo.diffCD_slope = p[0]
gizmo.diffCD_offset = p[1]

plot(y, [x, polyval(p, y)], ['b.', 'k-'])
xlabel('Voltage (V)')
ylabel('DIFFCD code')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating ADC gain = 1 (Gain is x1)...')
gizmo.adc_write_reg(gizmo.MUX, [gizmo.MUX_DIFFAB])
gizmo.adc_set_gain(1)
x = linspace(-2000, 2000, 201)
y = zeros(201)
for i in range(201):
    gizmo.dac_diffAB_set_value(x[i])
    y[i] = gizmo.adc_get_data_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFAB code')
        ylabel('ADC code')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, (x - gizmo.diffAB_offset) / gizmo.diffAB_slope, 1)
gizmo.adc_slopes[1] = p[0]
gizmo.adc_offsets[1] = p[1] / p[0]

plot(y, [(x - gizmo.diffAB_offset) / gizmo.diffAB_slope, polyval(p, y)], ['b.', 'k-'])
xlabel('ADC code')
ylabel('DIFFAB voltage (V)')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1] / p[0]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating ADC gain = 2 (Gain is x2)...')
gizmo.adc_set_gain(2)
x = linspace(-1000, 1000, 201)
y = zeros(201)
for i in range(201):
    gizmo.dac_diffAB_set_value(x[i])
    y[i] = gizmo.adc_get_data_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFAB code')
        ylabel('ADC code')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, (x - gizmo.diffAB_offset) / gizmo.diffAB_slope, 1)
gizmo.adc_slopes[2] = p[0]
gizmo.adc_offsets[2] = p[1] / p[0]

plot(y, [(x - gizmo.diffAB_offset) / gizmo.diffAB_slope, polyval(p, y)], ['b.', 'k-'])
xlabel('ADC code')
ylabel('DIFFAB voltage (V)')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1] / p[0]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating ADC gain = 3 (Gain is x4)...')
gizmo.adc_set_gain(3)
x = linspace(-500, 500, 201)
y = zeros(201)
for i in range(201):
    gizmo.dac_diffAB_set_value(x[i])
    y[i] = gizmo.adc_get_data_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFAB code')
        ylabel('ADC code')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, (x - gizmo.diffAB_offset) / gizmo.diffAB_slope, 1)
gizmo.adc_slopes[3] = p[0]
gizmo.adc_offsets[3] = p[1] / p[0]

plot(y, [(x - gizmo.diffAB_offset) / gizmo.diffAB_slope, polyval(p, y)], ['b.', 'k-'])
xlabel('ADC code')
ylabel('DIFFAB voltage (V)')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1] / p[0]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating ADC gain = 4 (Gain is x8)...')
gizmo.adc_set_gain(4)
x = linspace(-400, 400, 201)
y = zeros(201)
for i in range(201):
    gizmo.dac_diffAB_set_value(x[i])
    y[i] = gizmo.adc_get_data_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFAB code')
        ylabel('ADC code')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, (x - gizmo.diffAB_offset) / gizmo.diffAB_slope, 1)
gizmo.adc_slopes[4] = p[0]
gizmo.adc_offsets[4] = p[1] / p[0]

plot(y, [(x - gizmo.diffAB_offset) / gizmo.diffAB_slope, polyval(p, y)], ['b.', 'k-'])
xlabel('ADC code')
ylabel('DIFFAB voltage (V)')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1] / p[0]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating ADC gain = 5 (Gain is x16)...')
gizmo.adc_set_gain(5)
x = linspace(-160, 160, 161)
y = zeros(161)
for i in range(161):
    gizmo.dac_diffAB_set_value(x[i])
    y[i] = gizmo.adc_get_data_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFAB code')
        ylabel('ADC code')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, (x - gizmo.diffAB_offset) / gizmo.diffAB_slope, 1)
gizmo.adc_slopes[5] = p[0]
gizmo.adc_offsets[5] = p[1] / p[0]

plot(y, [(x - gizmo.diffAB_offset) / gizmo.diffAB_slope, polyval(p, y)], ['b.', 'k-'])
xlabel('ADC code')
ylabel('DIFFAB voltage (V)')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1] / p[0]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating ADC gain = 6 (Gain is x32)...')
gizmo.adc_set_gain(6)
x = linspace(-80, 80, 161)
y = zeros(161)
for i in range(161):
    gizmo.dac_diffAB_set_value(x[i])
    y[i] = gizmo.adc_get_data_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFAB code')
        ylabel('ADC code')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, (x - gizmo.diffAB_offset) / gizmo.diffAB_slope, 1)
gizmo.adc_slopes[6] = p[0]
gizmo.adc_offsets[6] = p[1] / p[0]

plot(y, [(x - gizmo.diffAB_offset) / gizmo.diffAB_slope, polyval(p, y)], ['b.', 'k-'])
xlabel('ADC code')
ylabel('DIFFAB voltage (V)')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1] / p[0]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

print('Calibrating ADC gain = 7 (Gain is x64)...')
gizmo.adc_set_gain(7)
x = linspace(-40, 40, 81)
y = zeros(81)
for i in range(81):
    gizmo.dac_diffAB_set_value(x[i])
    y[i] = gizmo.adc_get_data_avg(num_avg)
    if i > 0:
        plot(x[:i], y[:i])
        xlabel('DIFFAB code')
        ylabel('ADC code')
        ylabel('', yaxis = 'right')
        draw_now()

p = polyfit(y, (x - gizmo.diffAB_offset) / gizmo.diffAB_slope, 1)
gizmo.adc_slopes[7] = p[0]
gizmo.adc_offsets[7] = p[1] / p[0]

plot(y, [(x - gizmo.diffAB_offset) / gizmo.diffAB_slope, polyval(p, y)], ['b.', 'k-'])
xlabel('ADC code')
ylabel('DIFFAB voltage (V)')
ylabel(f'slope = {p[0]:.5e}, offset = {p[1] / p[0]:.5e}', yaxis = 'right')
draw_now()
input('Press enter to continue.')

