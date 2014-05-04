import serial
import threading
import time
import sys

class SerialMonitor(threading.Thread):
    def run(self):
        while 1:
            getReceivedBeacon()


def handleResponse():
    ser.timeout = None
    data = ser.read(3)
    rxBuffer = bytearray(data)
    header = rxBuffer[0]
    size = (rxBuffer[1] << 8) + rxBuffer[2]
    print header, rxBuffer[1], rxBuffer[2]
    ser.timeout = 10
    payload = ser.read(size)
    return payload

def getReceivedBeacon():
    data = handleResponse()
    data = data[1:].encode("hex")
    print "Received"
    #print data
    

def setMode(value):
    data = bytearray([0,0,1,value])
    ser.write(data)
    return handleResponse()

alive = True
ser = serial.Serial(port=7,baudrate=19200, rtscts=True)
print "Set to receive mode"
setMode(1)
print "Waiting for data"
serialMonitor = SerialMonitor()
serialMonitor.daemon = True
serialMonitor.start()
raw_input()
ser.close()
print "Exiting"
