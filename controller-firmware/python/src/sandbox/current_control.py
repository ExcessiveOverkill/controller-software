import numpy as np
import matplotlib.pyplot as plt

# Simulation parameters
R = .1       # Resistance in ohms
L = .001       # Inductance in henries
Vdc = 350.0    # DC supply voltage
f_pwm = 25e3  # PWM frequency in Hz

current_setpioint = 50.0  # Setpoint for current controller
bemf = 40.0              # Back EMF voltage

t_stop = 2e-3  # Simulate for 2 ms
dt = .1e-6      # Time step of .1 microsecond



MAX_DUTY_CYCLE = 0.95
MIN_DUTY_CYCLE = 0.00



# Time array
time = np.arange(0, t_stop, dt)

# Storage for current
current = np.zeros_like(time)

I = 0.0  # initial current
# Derived parameters
T_pwm = 1.0 / f_pwm

old_t_mod = 0

duty_cycle = 0
for i in range(1, len(time)):

    t = time[i]
    # Determine if we are in the "on" or "off" portion of the PWM cycle
    # Use modulo operation to find where we are within a PWM period
    t_mod = t % T_pwm

    if t_mod < old_t_mod:
        
        applied_voltage = Vdc - bemf

        a = (applied_voltage/R) - current_setpioint
        b = (applied_voltage/R) - I

        total_time = -(L/R) * np.log(a/b)

        duty_cycle = min(MAX_DUTY_CYCLE, max(MIN_DUTY_CYCLE, total_time/T_pwm))

        pass
    old_t_mod = t_mod

    on_time = duty_cycle * T_pwm
    off_time = T_pwm - on_time


    if t_mod < on_time:
        V = Vdc - bemf
    else:
        V = -bemf

    # Compute dI/dt
    dIdt = (V - R*I) / L
    dIdt *= 1
    # Integrate using Euler method
    I = I + dIdt * dt

    # Store current
    current[i] = I

# Plotting
plt.figure(figsize=(10, 5))
plt.plot(time*1e4, current, label='Current through inductor')
plt.title('PWM driven RL load')
plt.xlabel('Time (ms)')
plt.ylabel('Current (A)')
plt.grid(True)
plt.legend()
plt.show()
