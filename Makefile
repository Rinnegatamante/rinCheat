TARGET  = rinCheat
OBJS    = main.o blit.o font.o memory.o threads.o savedata.o cheats.o screenshot.o

LIBS    = -lSceAppUtil_stub -lSceAppMgr_stub -lSceCtrl_stub -lSceDisplay_stub -lSceFios2_stub -lSceKernel_stub -lScePower_stub -lSceLibC_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3 -nostartfiles
ASFLAGS = $(CFLAGS)

all: $(TARGET).suprx

%.suprx: %.velf
	vita-make-fself $< $@

%.velf: %.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).suprx $(TARGET).velf $(TARGET).elf $(OBJS)

send: $(TARGET).suprx
	curl -T $(TARGET).suprx ftp://$(PSVITAIP):1337/ux0:/plugins/$(TARGET).suprx
	@echo "Sent."