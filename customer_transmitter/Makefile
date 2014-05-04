CC          := arm-none-eabi-gcc
OBJCOPY     := arm-none-eabi-objcopy
PROJ        := /home/scott/Projects/BLE/ARM
INCLUDE     := /home/scott/Projects/BLE/ARM/Include
BOARD       := BOARD_NRF6310
CFLAGS      := -mcpu=cortex-m0 -mthumb -Wall -DBOARD_NRF6310 -DNRF51
LOCAL_CFLAGS := $(CFLAGS) -std=c99
LFLAGS      := -L"/usr/arm-none-eabi/lib/armv6-m" -Xlinker -Map=main.map -mcpu=cortex-m0 -T$(PROJ)/gcc_nrf51_blank.ld -mthumb -mabi=aapcs

OBJ_FILES   := ble.o gpio.o hub.o radio.o serial.o state_variables.o time.o main.o

default: main.bin main.hex

.c.o:
	$(CC) $(LOCAL_CFLAGS) -I$(INCLUDE) $< -c $<

system_nrf51.o: $(PROJ)/system_nrf51.c 
	$(CC) $(CFLAGS)  -I$(INCLUDE) -c $< 

nrf_delay.o: $(PROJ)/nrf_delay.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c $< 

startup_nrf51.o: $(PROJ)/startup_nrf51.s 
	$(CC) $(CFLAGS) -I$(INCLUDE) -c $< 

main.out: system_nrf51.o nrf_delay.o startup_nrf51.o $(OBJ_FILES)
	$(CC) $(LFLAGS) $(OBJ_FILES) system_nrf51.o nrf_delay.o startup_nrf51.o -o main.out

main.bin: main.out
	$(OBJCOPY) -O binary main.out main.bin

main.hex: main.out
	$(OBJCOPY) -O ihex main.out main.hex
clean:
	rm -f *.o *.out *.hex *.seg *.map *.bin JLink.log 
