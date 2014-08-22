CC          := arm-none-eabi-gcc
OBJCOPY     := arm-none-eabi-objcopy
PROJ        := /home/scott/Projects/BLE/ARM
INCLUDE     := /home/scott/Projects/BLE/ARM/Include -I.
#BOARD       := BOARD_NRF6310
BOARD       := BOARD_PCA10001
CFLAGS      := -mcpu=cortex-m0 -mthumb -Wall -D$(BOARD) -DNRF51
LOCAL_CFLAGS := $(CFLAGS) -std=c99
LFLAGS      := -L"/usr/arm-none-eabi/lib/armv6-m" -Xlinker -Map=main.map -mcpu=cortex-m0 -T$(PROJ)/gcc_nrf51_blank.ld -mthumb -mabi=aapcs
VPATH       := WifiDriver JSON
OBJ_FILES   := main.o gpio.o serial.o spi.o cc3000_common.o evnt_handler.o hci.o netapp.o nvmem.o socket.o wlan.o security.o

default: main.bin main.hex 

%.o : %.c
	$(CC) -g $(LOCAL_CFLAGS) -I$(INCLUDE) $< -c $<

system_nrf51.o: $(PROJ)/Libs/system_nrf51.c 
	$(CC) -g $(CFLAGS) -I$(INCLUDE) -c $< 

nrf_delay.o: $(PROJ)/Libs/nrf_delay.c
	$(CC) -g $(CFLAGS) -I$(INCLUDE) -c $< 

startup_nrf51.o: $(PROJ)/startup_nrf51.s 
	$(CC) -g $(CFLAGS) -I$(INCLUDE) -c $< 

main.out: system_nrf51.o nrf_delay.o startup_nrf51.o $(OBJ_FILES)
	$(CC) -g $(LFLAGS) $(OBJ_FILES) system_nrf51.o nrf_delay.o startup_nrf51.o -o main.out

main.bin: main.out
	$(OBJCOPY) -O binary main.out main.bin

main.hex: main.out
	$(OBJCOPY) -O ihex main.out main.hex

clean:
	rm -f *.o *.out *.hex *.seg *.map *.bin JLink.log 
