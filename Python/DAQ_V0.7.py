# quick and dirty example to import analog-input data from Arduino
#
# Connect Arduino through USB and check in computermanagement which COM-port is in use
# 

import serial
import time
import matplotlib.pyplot as plt

com_port = 'COM8:'      # check in pc's computer management which com: port is in use
baudrate = 230400       # Communication speed: Arduino and Spider should have the same value

#default values AD conversion
category = 'ADC'        # default category
ports = '2'             # default number of analog input ports in char
ports_int = int(ports)  # the integer variant
n_samples =   '1000'    # default number of samples per analog port
samples_s =  '10000'    # default sample rate per port
trigger = '0'           # '0'trigger off, '1' trigger on (port 27 arduino)
trigger_edge = '3'      #0 = LOW (trigger whenever pin level is low)
                    	#1 = HIGH (trigger whenever pin level is high)
                        #2 = CHANGE edge ( trigger whenever the pin changes value)
                        #3 = RISING edge (trigger when the pin goes from low to high)
                        #4 = FALLING edge (trigger when the pin goes from high to low)
debounce = '0'          # Time in microseconds a level must be stable to prevent false start because of jitter or noise 
                        # (only visible when trigger_edge = 0 or 1)

# Min/max values AD parameters
port_min = 1            # minimum and maximum number of analog ports
port_max = 8
n_samples_min = 1       # minimum and maximum number of samples/second
n_samples_max = 50000
samples_s_min = 1       # minimum and maximum number of samples/port
samples_s_max = 100000  # for test purpose, the actual maximum is (a total of all ports) about 86.000 samples/sec 
trigger_min = 0         # wait foran trigger event: 1 or start ADC immediately: 0
trigger_max = 1
event_min = 0           # minimum and maximum number for the events ('see trigger_edge') 
event_max = 4
debounce_min = 0        # minimum and maximum value for debounce timing in microsec
debounce_max = 10000

##############################################

# check the user input is within the limits
def check_val(val, _min, _max):
    if (int(val) < _min):
        print('The value ' + val + ' is too low, replaced with: ' + str(_min) )
        val = str(_min)
    if (int(val) > _max):
        print('The value ' + val + ' is too large, replaced with: ' + str(_max) )
        val = str(_max)
    return(val)

##############################################
    
# user interface input
# All those conversions from str-> int and vice versa! Communication with Arduino is in string format... 
#   the number of analog ports, number of samples, samples per second, wait for trigger, event and debounce time
def user_input(c, p, i, s, t, te, h):
    # ask for input, enter is default value
    p = input('Number of Arduino analog input (1-' + str(port_max) + ') [' + p + '] : ') or p
    p = check_val(p, port_min, port_max)
    global ports                            # change the global variable
    global ports_int                        # used to demultiplex before plotting
    ports = p       
    ports_int = int(p)                      # helps decoding the multiplexed data from arduino
    
    global n_samples_max
    n_samples_max = int(n_samples_max/ports_int)
    i = input('Number of samples/analog input (' + str(n_samples_min) + '-' + str(n_samples_max) + ') [' + i + '] : ') or i
    i = check_val(i, n_samples_min, n_samples_max)

    global samples_s_max
    samples_s_max = int(samples_s_max/ports_int)
    s = input('Number of samples/second/port  (' + str(samples_s_min) + '-' + str(samples_s_max) + ') [' + s + '] : ') or s
    s = check_val(s, samples_s_min, samples_s_max)
    global samples_s                        # change the global variable
    samples_s = s
    
    t = input('Trigger on port 27 (1 = wait for event, 0 = trigger disabled) [' + t + '] : ')  or t
    t = check_val(t, trigger_min, trigger_max)
    
    # ask only for trigger event when trigger is enabled
    if (t != "0"):
        te = input('Trigger on event: (0 = LOW-level, 1 = HIGH-Level 2 = CHANGE, 3 = RISING-edge, 4 = FALLING-edge) [' + te + '] : ') or te
        te = check_val(te, event_min, event_max)
        
        # Ask for debounce time when trigger is 'on level'
        if (int(te) < 2):
            h = input('Debounce in microseconds: (' + str(debounce_min) + '-' + str(debounce_max) + ') [' + h + '] : ') or h
            h = check_val(h, debounce_min, debounce_max)

    
    # The total number of samples is the number of analog input ports multiplied by the number of samples per channel
    i = str(ports_int * int(i))             # total number of samples

    # All input is available, prepare for transport to Arduino, every value separated with spaces    
    param = c + ' ' + p + ' ' + i + ' ' + s + ' ' + t + ' ' + te + ' ' + h

    print()
    print("Parameters send to DAQ: " + param)

    param = param.encode('utf-8')           # Unicode Transformation Format, 8bits
    cr = 13
    lf = 10
    param = param + cr.to_bytes(1,'big')    # end the string with CR Line Feed
    param = param + lf.to_bytes(1,'big')
    ser.write(param)                        # send parameters to Arduino

    return(i)                               # return the the total number of samples for all analog ports together

##############################################



# start MAIN program

ser = serial.Serial(com_port, baudrate)     # initiate the serial port
time.sleep(1.5)                             # 

# first reading after power on Arduino can give garbage, so flush
#ser.write(b"34 1 20000 \r\n")
#temp = ser.readline()
ser.flushInput()                            # zou in versie 3 zijn vervangen door 'reset_input_buffer()'

print("\nDemo Python/Arduino (ESP32) analog input with transmit buffer")

# send parameters to Arduino: the number of analog ports, number of samples, samples per second, wait for trigger, event and debounce time
n_samples = user_input(category, ports, n_samples, samples_s, trigger, trigger_edge, debounce) # return the total number of samples 
n_samples_int = int(n_samples)

print("\nWaiting for AD conversion and serial sample data transport (" + n_samples + " samples in total) .....\n")

# when Arduino is ready converting it sends the collected sample data to Python per serial protocol
# the data is send as a pair of bytes, first MSB then LSB.
# read this serial data until all bytes (n_samples_int x 2) are received

# Store the multiplexed sample data, Arduinono sends a stream alternately MSB and LSB
y_temp = []                                 # store the analog values in this list
y_temp = ser.read(size = n_samples_int * 2) # 2 bytes unsigned integer per sample, so read twice the number of samples
ser.close()                                 # serial data received, close serial port

print("\nData received (" + str(len(y_temp)) + " bytes), preparing plot....")

# each sample consists of two bytes = 16 bits. 
# -     The 12 LSB contain the sample values
# -     The 4 MSB  contain the de-multiplex code for the port number
# |        MSB        ||      LSB         |
# | 7 | 6 5 4|3 2 1 0 ||  7 6 5 4 3 2 1 0 |
# | X |PORTNR|      SAMPLE VALUES         |  
#
# demultiplex the samples to sample values/analog port, assign every value to the corresponding port in 13,14 and 15th bit
t = 0                                       # pointer to the byte list y_temp[]
val_array = [[],[],[],[],[],[],[],[]]       # multi dimensional list: analog portnumber 0-7, store the right value in the richt cel
while (t < (n_samples_int * 2)):            # for every value in y_temp[]
    temp = (y_temp[t] * 256) + y_temp[t+1]  # reconstruct the two byte unsigned integer from the MSB and LSB value 
    # demultiplex.... kan eleganter met een 4xright shift van MSB
    port_num = int((temp & 61440)/4096)     # filter the 4 bits 13-16, the 3 bits 13,14,15 is the port number 
    port_val = int(temp & 4095)             # 12 bits for the value, filter all except the sample data
    val_array[port_num].extend([port_val])  # extend the multidimensional array with the sample data (bron: https://www.geeksforgeeks.org/multi-dimensional-lists-in-python/)
    t= t + 2                                # point to the next MSB byte

# prepare plotting data, define x-values for the time base
x=[]                                        # array to store the timebase 
x = list(range(len(val_array[0])))          # divisions of the x-axis depends on nr of samples 1st port

# define figure size
plt.figure(figsize=(14, 6))

# plot in one graph, every port its own line in the default line color
n = 0
while (n < ports_int):
    plt.plot(x, val_array[n], linewidth=2.0, )
    n = n + 1

samples_channel = n_samples_int / ports_int
total_samples_time = samples_channel/int(samples_s)

plt.xlabel('Samples/channel (total time = ' + str(total_samples_time) +' seconds)')
plt.ylabel('Arduino analog-in (0 - 4095)')
plt.title('Demo Arduino Analog-in Graph')
plt.grid(True)
plt.show()


# close serial port