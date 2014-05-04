import serial
import serial.tools.list_ports
#import Threading

def handleResponse():
    ser.timeout = 0
    rxBuffer = bytearray(ser.read(3))
    header = rxBuffer[0]
    size = (rxBuffer[1] << 8) + rxBuffer[2]
    #print header, size
    ser.timeout = 10
    payload = ser.read(size)
    return payload
    
def getFirmwareVersion():
    data = bytearray([1,0,0])
    ser.write(data)
    return handleResponse()

def getHardwareVersion():
    data = bytearray([2,0,0])
    ser.write(data)
    return handleResponse()

def getSerialNumber():
    data = bytearray([3,0,0])
    ser.write(data)
    return handleResponse();

def setMode(value):
    data = bytearray([0,0,1,value])
    ser.write(data)
    return handleResponse()

def getTransmitterPeriod(value):
    data = bytearray([5,0,0])
    ser.write(data)
    return handleResponse();

def setTransmitterPeriod(value):
    data = bytearray([5,0,3,0,0,value])
    ser.write(data)
    return handleResponse();

def setTransmitterOutputPower(value):
    data = bytearray([5,0,3,0,1,value])
    ser.write(data)
    return handleResponse();

def setPayload(value):
    data = bytearray([5,0,2 + len(value),0,2])
    data = data + value
    ser.write(data)
    return handleResponse();
        
#print "Start"
#comPortList = serial.tools.list_ports.comports()
#for x in comPortList:
    #print x[0]
ser = serial.Serial(port=7,baudrate=38400, rtscts=True)

    

##setTransmitterPeriod(10)
##setTransmitterOutputPower(0)
##iBeaconHeader= bytearray([0x02,0x01,0x1A,0x1A,0xFF,0x4C,0x00,0x02,0x15])
##mfgInfo = bytearray([0x3A, 0xB0, 0x2B, 0xB7, 0x6A, 0xEC, 0x47, 0xBA,
##                     0xAF, 0x56, 0xAB, 0x0B, 0x2A, 0x9E, 0x0A, 0xA0])
##major = bytearray([0x00, 0x00])
##minor = bytearray([0x00, 0x01])
##power = bytearray([0xc5])
##payload = iBeaconHeader + mfgInfo + major + minor + power;
##setPayload(payload)

##setMode(1)
print getFirmwareVersion()
ser.close()
