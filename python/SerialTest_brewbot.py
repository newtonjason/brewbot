
import serial
import time
#Can be Downloaded from this Link
#https://pypi.python.org/pypi/pyserial

#Global Variables
ser = serial.Serial()


#Function to Initialize the Serial Port
def init_serial():
    
    ser.baudrate = 9600
    ser.port = 5   #COM Port Name Start from 0
    
    #ser.port = '/dev/ttyUSB0' #If Using Linux

    #Specify the TimeOut in seconds, so that SerialPort
    #Doesn't hangs
    ser.timeout = 10
    ser.open()          #Opens SerialPort

    # print port open or closed
    if ser.isOpen():
        print 'Open: ' + ser.portstr
#Function Ends Here
        

#Call the Serial Initilization Function, Main Program Starts from here
init_serial()

while True:
    temp = raw_input(': ')
    ser.write(temp)         #Writes to the SerialPort
    time.sleep (50.0 / 1000.0);
    while ser.inWaiting() > 0:    
        bytes = ser.readline()  #Read from Serial Port
        print bytes      #Print What is Read from Port

#Ctrl+C to Close Python Window
