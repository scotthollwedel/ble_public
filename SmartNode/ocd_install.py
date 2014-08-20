#! /usr/bin/python

import argparse
import pyOCD
from pyOCD.board import MbedBoard
import logging

logging.basicConfig(level=logging.DEBUG)

parser = argparse.ArgumentParser(description='A CMSIS-DAP python debugger')
parser.add_argument('-f', help='binary file', dest = "file", required=True)
args = parser.parse_args()

try:

    board = MbedBoard.chooseBoard()
    target_type = board.getTargetType()
    binary_file = args.file
    print "Unique ID: %s" % board.getUniqueID()
    print "binary file: %s" % binary_file
    target = board.target
    flash = board.flash
    flash.flashBinary(binary_file)
    target.reset()
except BaseException as e:
    print e

