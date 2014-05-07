CC          := arm-none-eabi-gcc
OBJCOPY     := arm-none-eabi-objcopy
SIZE        := arm-none-eabi-size
GDB         := arm-none-eabi-gdb
PROJ        := /home/scott/Projects/BLE/ARM
INCLUDE     := /home/scott/Projects/BLE/ARM/Include
BOARD       := BOARD_NRF6310
TEMPLATE_PATH := /usr/arm-none-eabi/lib/armv6-m
LINKER_SCRIPT := $(PROJ)/gcc_nrf51_blank.ld
CPUFLAGS    = -mthumb -mcpu=cortex-m0 -march=armv6-m
CFLAGS      += -std=gnu99 -c $(CPUFLAGS) -Wall -DNRF51  -DBOARD_NRF6310 -MD
LDFLAGS     += $(CPUFLAGS) -L $(TEMPLATE_PATH) -T $(LINKER_SCRIPT) -Wl,-Map=main.Map
LOCAL_CFLAGS := $(CFLAGS) -std=gnu99
OUTPUT_NAME := main
BIN         := $(OUTPUT_NAME).bin
HEX         := $(OUTPUT_NAME).hex
ELF         := $(OUTPUT_NAME).elf
FLASH_START_ADDRESS = "0"
JLINK_OPTIONS = -device nrf51822 -if swd -speed 1000
SEGGER_SERIAL := 518004755
JLINKEXE_OPTION := -SelectEmuBySn $(SEGGER_SERIAL)
JLINKGDBSERVER_OPTION := -select USB=$(SEGGER_SERIAL)
JLINK := JLinkExe $(JLINK_OPTIONS) $(JLINKEXE_OPTION)
JLINKGDBSERVER := JLinkGDBServer $(JLINK_OPTIONS) $(JLINKGDBSERVER_OPTION)

OBJS   := system_nrf51.o nrf_delay.o startup_nrf51.o ble.o gpio.o hub.o radio.o serial.o state_variables.o time.o main.o

all: $(OBJS) $(HEX)

.c.o:
	$(CC) $(LOCAL_CFLAGS) -I$(INCLUDE) $< -c $<

system_nrf51.o: $(PROJ)/Libs/system_nrf51.c 
	$(CC) $(CFLAGS)  -I$(INCLUDE) -c $< 

nrf_delay.o: $(PROJ)/Libs/nrf_delay.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c $< 

startup_nrf51.o: $(PROJ)/startup_nrf51.s 
	$(CC) $(CFLAGS) -I$(INCLUDE) -c $< 

$(HEX): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(ELF)
	$(OBJCOPY) -Oihex $(ELF) $(HEX)
	$(OBJCOPY) -Obinary $(ELF) $(BIN)
	$(SIZE) $(ELF)

size: $(ELF)
	$(SIZE) $(ELF)

clean:
	rm -f *.d *.o *.out *.hex *.seg *.elf *.Map *.bin JLink.log 

flash: $(HEX)
	$(JLINK) flash.jlink || true

startdebug:
	gnome-terminal -e "$(JLINKGDBSERVER) -port 5000"
	sleep 1
	gnome-terminal -e "$(GDB) $(ELF)"

.PHONY: flash
