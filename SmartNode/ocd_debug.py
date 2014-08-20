#!/usr/bin/python
import time
from pyOCD.gdbserver import GDBServer
from pyOCD.board import MbedBoard

import logging
logging.basicConfig(level=logging.INFO)

board = MbedBoard.chooseBoard()

# start gdbserver
gdb = GDBServer(board, 3333)
while 1:
    time.sleep(1);
