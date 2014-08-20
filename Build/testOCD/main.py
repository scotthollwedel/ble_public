from pyOCD.board import MbedBoard

import logging
logging.basicConfig(level=logging.INFO)

board = MbedBoard.chooseBoard()

target = board.target
flash = board.flash
target.resume()
target.halt()

print "pc: 0x%X" % target.readCoreRegister("pc")
    ##pc: 0xA64

target.step()
print "pc: 0x%X" % target.readCoreRegister("pc")
    ##pc: 0xA30

target.step()
print "pc: 0x%X" % target.readCoreRegister("pc")
    ##pc: 0xA32

flash.flashBinary("binaries/l1_lpc1768.bin")
print "pc: 0x%X" % target.readCoreRegister("pc")
    ##pc: 0x10000000

target.reset()
target.halt()
print "pc: 0x%X" % target.readCoreRegister("pc")
    ##pc: 0xAAC

board.uninit()
